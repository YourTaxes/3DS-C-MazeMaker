#include "states/LevelSelect/LevelSelect.h"
#include "utils/graphics.h"
#include "utils/debug.h"
#include "datatypes/save_file.h"
#include "utils/format.h" //FormatTime, for the best time lines
#include "utils/popup.h"
#include "game_logic/menu_nav.h"
#include <stdlib.h>
#include <stdio.h> //snprintf, for the slot number labels
#include <string.h> //memset, to blank a slot on delete


//defines
#define SLOT_COUNT 4
#define BOTTOM_RECT_COUNT 9
_Static_assert(RECT_DELETE == BOTTOM_RECT_COUNT - 1,
               "the action rects must end RectIDs, and BOTTOM_RECT_COUNT must cover them");

//where the baked level image sits on the top screen.
#define LEVEL_IMG_X ((TOP_SCREEN_WIDTH - BAKED_LEVEL_IMG_WIDTH) / 2)
#define LEVEL_IMG_Y ((TOP_SCREEN_HIGHT - BAKED_LEVEL_IMG_HEIGHT) / 2)

//the hovered slot's level name, centered in the strip above the baked image.
#define NAME_TEXT_SCALE 0.7f
#define NAME_Y (LEVEL_IMG_Y / 2)
//"Slot 4: " 8 plus a levelName that a full length rename leaves unterminated, plus the NUL
#define NAME_LINE_MAX (8 + LEVEL_NAME_MAX_LEN + 1)

//the hovered slot's two best times, in the strip below the baked image. that strip is the
//same height as the one above it, so one line at this scale clears it.
#define TIME_TEXT_SCALE 0.6f
#define TIME_Y (LEVEL_IMG_Y + BAKED_LEVEL_IMG_HEIGHT + LEVEL_IMG_Y / 2)
#define TIME_STANDARD_X 100
#define TIME_HARD_X 300
#define TIME_LINE_MAX (10 + TIME_STR_LEN) //"Standard: " plus a formatted time

//save slot buttons, a 2x2 grid on the upper part of the bottom screen
#define SLOT_W 80
#define SLOT_H 80
#define SLOT_X0 70            //left edge of the left column
#define SLOT_Y0 10            //top edge of the top row
#define SLOT_COL_SPACING 100  //left edge of one column to the left edge of the next
#define SLOT_ROW_SPACING 100
#define SLOT_TEXT_SCALE 1.0f

//the action button row across the bottom
#define ACTION_W 50
#define ACTION_H 30
#define ACTION_Y 200
#define ACTION_TEXT_SCALE 0.45f

//how far the highlight sticks out past the selected button, same idea as MainMenu.
#define HIGHLIGHT_PAD 5

/*
* the static buffer, everything that is built once in _Init and never reparsed:
* "Main\nMenu" 9 + "Rename" 6 + "Copy" 4 + "Clear\nTimes" 11 + "Delete" 6 + four slot
* numbers 4 = 40 glyphs. rounded up to the next power of two.
*/
#define LEVEL_SELECT_MAX_GLYPHS 64

/*
* a C2D_TextBuf can only be cleared whole, never entry by entry, so anything that changes
* while the state is up needs a buffer holding nothing but itself. the two below are
* those, and both are a fraction of the static one.
*
* the names change on rename, copy and delete. the times change on clear times, copy
* and delete. each group is reparsed without disturbing the other.
*/
#define LEVEL_SELECT_NAME_GLYPHS (SLOT_COUNT * NAME_LINE_MAX)
#define LEVEL_SELECT_TIME_GLYPHS (SLOT_COUNT * 2 * TIME_LINE_MAX)

#define LEVEL_SELECT_AYS_GLYPHS 256 //holds all 3 stages at once.

#define AYS_WINDOW_COUNT 3

//forward definitions of functions
static MenuEvent on_slot(int index, void* user);
static MenuEvent on_action(int index, void* user);
static MenuEvent on_back(int index, void* user);
static void on_hover(int index, void* user);

//the layout of the buttons in the menu
static const MenuDef LS_MENU[BOTTOM_RECT_COUNT] = {
    //save slot buttons
    [RECT_LVL1] = MENU_BTN_FULL(RECT_LVL2, RECT_LVL2, RECT_RENAME, RECT_LVL3, on_slot, on_hover, 0),
    [RECT_LVL2] = MENU_BTN_FULL(RECT_LVL1, RECT_LVL1, RECT_CLEAR_TIMES, RECT_LVL4, on_slot, on_hover, 0),
    [RECT_LVL3] = MENU_BTN_FULL(RECT_LVL4, RECT_LVL4, RECT_LVL1, RECT_RENAME, on_slot, on_hover, 0),
    [RECT_LVL4] = MENU_BTN_FULL(RECT_LVL3, RECT_LVL3, RECT_LVL2, RECT_CLEAR_TIMES, on_slot, on_hover, 0),

    //action buttons
    [RECT_MAINMENU] = MENU_BTN(RECT_DELETE, RECT_RENAME, RECT_LVL3, RECT_LVL1, on_back),
    [RECT_RENAME] = MENU_BTN(RECT_MAINMENU, RECT_COPY, RECT_LVL3, RECT_LVL1, on_action),
    [RECT_COPY] = MENU_BTN(RECT_RENAME, RECT_CLEAR_TIMES, RECT_LVL3, RECT_LVL1, on_action),
    [RECT_CLEAR_TIMES] = MENU_BTN(RECT_COPY, RECT_DELETE, RECT_LVL4, RECT_LVL2, on_action),
    [RECT_DELETE] = MENU_BTN(RECT_CLEAR_TIMES, RECT_MAINMENU, RECT_LVL4, RECT_LVL2, on_action),

};



static const char* const AYS_HEADS[AYS_WINDOW_COUNT] = {
    "Are you sure\nyou want",
    "Are you Really sure\nyou want",
    "Are you ABSOLUTELY\nsure you want"
};

//structs

/*
* the action button row, in left to right order, which is also RECT_MAINMENU through
* RECT_DELETE. this is the only place one is defined: _Init, _Logic and _Draw all loop
* over it. the x values are not evenly spaced, so they are spelled out rather than
* derived the way the slot grid is.
*/
static const struct {
    const char* label; //drawn on the button, so it carries its own line break
    float x;
} ACTION_BUTTONS[] = {
    { "Main\nMenu", 10 },
    { "Rename", 70 },
    { "Copy", 135 },
    { "Clear\nTimes", 200 },
    { "Delete", 260 },
};
#define ACTION_BUTTON_COUNT (sizeof(ACTION_BUTTONS) / sizeof(ACTION_BUTTONS[0]))
_Static_assert(ACTION_BUTTON_COUNT == RECT_DELETE - RECT_MAINMENU + 1,
               "ACTION_BUTTONS must cover RECT_MAINMENU through RECT_DELETE");





//everything owned by the LevelSelect
typedef struct{
    C2D_TextBuf textBuf; //the static text, parsed once in _Init
    C2D_TextBuf nameBuf; //just the four level names, reparsed on a rename
    C2D_TextBuf timeBuf; //just the eight best times, reparsed when a time is cleared
    C2D_TextBuf AysWindowBuf; //the buffer for the text shown in the are you sure window, not the buttons.

    //one preview per save slot, each carrying whether its texture is ours to free, so a
    //borrowed gui image can sit in a slot without being deleted out from under graphics.c.
    LevelPreview fullLevelPreviews[SLOT_COUNT];
    C3D_RenderTarget *fullLevelTarget[SLOT_COUNT];

    

    //this holds all the rects for the ui for the bottom screen.

    Rect bottomRects[BOTTOM_RECT_COUNT];

    //one label per button, parallel to the rects above
    C2D_Text slotLabels[SLOT_COUNT];
    C2D_Text actionLabels[ACTION_BUTTON_COUNT];
    C2D_Text aysText[AYS_WINDOW_COUNT];

    //the name of the level in each slot, drawn above that slot's preview on the top
    //screen. these live in nameBuf, so they all go stale together whenever
    //RebuildNameLabels runs. same indexing as fullLevelPreviews.
    C2D_Text slotNameLabels[SLOT_COUNT];

    //each slot's two best times, below the preview. these live in timeBuf, so they all
    //go stale together whenever RebuildTimeLabels runs.
    C2D_Text slotStandardLabels[SLOT_COUNT];
    C2D_Text slotHardLabels[SLOT_COUNT];

    //the slot the game currently has loaded, copied off ctx in _Init because _Draw is
    //not handed a GameContext. 0 indexed, and its button is drawn green.
    u8 activeSlot;

    //the whole save file, read once in _Init and kept for the life of the state so
    //the slot list and the rename, copy and delete actions all work off one copy.
    //it is on the heap because a Save_File is far too big to sit in a stack frame.
    Save_File *savfle;

    //holds the pointer to the main game context, which is what holds the currently loaded save files.
    GameContext* ctx;

    //the pointer to the menu navigation handler
    MenuNav* menu;


    u8 curAction;    //Load_Action values, cast to u8 to save space
    u8 pendingSlot;  //the slot curAction lands on, captured when interacting with the slot.

    //this value holds the most recent hovered or tapped save slot,
    //and it is what is used to choose which level texture to display.
    //0 indexed
    u8 curSlotImage;

} LevelSelectState;

static LevelSelectState* lsstate;


/*
* reparse all four level names out of lsstate->savfle. a C2D_TextBuf cannot drop one
* entry, so every name is rebuilt even when only one changed, which is what nameBuf holds
* nothing else for. called after any action that edits a name: rename, copy, delete.
*/
static void RebuildNameLabels(void){
    C2D_TextBufClear(lsstate->nameBuf);

    for (int i = 0; i < SLOT_COUNT; i++){
        char line[NAME_LINE_MAX];
        //a slot with no save file behind it reads the same as an empty one, since
        //neither has a level to name
        if (lsstate->savfle == NULL || lsstate->savfle->Levels[i].empty){
            snprintf(line, sizeof line, "Slot %d: Empty", i + 1);
        } else {
            //levelName is a fixed char[32] that a full length rename leaves unterminated,
            //so the precision caps how far %s reads. an unbounded %s could run off the end.
            snprintf(line, sizeof line, "Slot %d: %.*s", i + 1,
                     LEVEL_NAME_MAX_LEN, lsstate->savfle->Levels[i].levelName);
        }
        MakeText(line, &lsstate->slotNameLabels[i], lsstate->nameBuf);
    }
}

/*
* reparse all eight best time lines out of lsstate->savfle. same logic as
* RebuildNameLabels: timeBuf holds nothing else, so clearing it costs only these eight.
* called after any action that edits a time: clear times, copy, delete.
*/
static void RebuildTimeLabels(void){
    C2D_TextBufClear(lsstate->timeBuf);

    for (int i = 0; i < SLOT_COUNT; i++){
        //a slot with no save file behind it, an empty one, and one that has never been
        //finished all read the same: there is no time to print
        const Raw_Level* lvl = (lsstate->savfle != NULL && !lsstate->savfle->Levels[i].empty)
                             ? &lsstate->savfle->Levels[i] : NULL;
        char line[TIME_LINE_MAX];
        char time[TIME_STR_LEN];

        if (lvl != NULL && lvl->standardTimeValid) FormatTime(lvl->bestTimeStandard, time, sizeof time);
        snprintf(line, sizeof line, "Standard: %s",
                 (lvl != NULL && lvl->standardTimeValid) ? time : "--");
        MakeText(line, &lsstate->slotStandardLabels[i], lsstate->timeBuf);

        if (lvl != NULL && lvl->hardTimeValid) FormatTime(lvl->bestTimeHard, time, sizeof time);
        snprintf(line, sizeof line, "Hard: %s",
                 (lvl != NULL && lvl->hardTimeValid) ? time : "--");
        MakeText(line, &lsstate->slotHardLabels[i], lsstate->timeBuf);
    }
}


//rebuilds the text buff for the are you sure window and builds all of the 3 stages
//both are 0 indexed on the way in and printed 1 indexed
static void BuildConfirmText(){
    C2D_TextBufClear(lsstate->AysWindowBuf);

    char action[40];
    switch (lsstate->curAction){
        case ACTION_COPY:
            snprintf(action, sizeof(action), "\nto copy slot %d\ninto slot %d?", lsstate->activeSlot + 1, lsstate->pendingSlot + 1);
            break;
        case ACTION_CLEAR_TIMES:
            snprintf(action, sizeof(action), "\nto clear slot %d's\nbest times?", lsstate->pendingSlot + 1);
            break;
        case ACTION_DELETE:
            snprintf(action, sizeof(action), "\nto delete slot %d?", lsstate->pendingSlot + 1);
            break;
        default:
            return;
    }
    char full[80];
    for (int i = 0; i < AYS_WINDOW_COUNT; i++) {
        snprintf(full, sizeof(full), "%s%s", AYS_HEADS[i], action);
        MakeText(full, &lsstate->aysText[i], lsstate->AysWindowBuf);
    }
}



//reset all of the action buttons to their standard color
static void ResetActionColors(){
    for (int i = RECT_RENAME; i <= RECT_DELETE; i++){
        lsstate->bottomRects[i].Color = Colors[CLR_DK_GRAY];
    }
}

/*
* the slot's baked picture is stale, so throw it away and bake it again.
*
* BakeLevelTexture zeroes *out and *target on entry WITHOUT releasing what they held, so
* whatever is already there has to go first or it is leaked: 512KB of VRAM plus the
* malloc'd C3D_Tex, and an orphaned render target. target before texture, the same order
* _End uses, because a target that outlives its texture leaves citro3d holding a pointer
* into freed VRAM.
*
* safe on a slot that holds nothing, so _Init bakes through here too and the free before
* bake order lives in exactly one place.
*
* this opens a frame of its own, so it may only be reached from _Logic.
*/
static void RebakeSlot(u8 slot){
    if (lsstate->fullLevelTarget[slot] != NULL){
        C3D_RenderTargetDelete(lsstate->fullLevelTarget[slot]);
        lsstate->fullLevelTarget[slot] = NULL;
    }
    FreeLevelImage(&lsstate->fullLevelPreviews[slot]);
    if (!BakeLevelTexture(&lsstate->fullLevelPreviews[slot], &lsstate->fullLevelTarget[slot],
                          &lsstate->savfle->Levels[slot], true)){
        printConsole("RebakeSlot: slot %d did not bake", slot + 1);
    }
}

//renames the current level slot, and saves it. 
//brings up the software keyboard for input.
//returns true if the player selects ok,
//false if the player selects cancel.
static bool RenameLevel(){
    if (lsstate->savfle->Levels[lsstate->pendingSlot].empty){
        printConsole("Cannot rename an empty level");
        return false;
    }
    char hint[LEVEL_NAME_MAX_LEN]; // 32
    snprintf(hint, sizeof(hint), "Enter level name for slot %d", lsstate->pendingSlot + 1);
    if (!GetKeyboard(lsstate->savfle->Levels[lsstate->pendingSlot].levelName, LEVEL_NAME_MAX_LEN, hint, SWKBD_TYPE_NORMAL)){
        return false;
    }
    printConsole("renamed slot %d to %s", lsstate->pendingSlot + 1, lsstate->savfle->Levels[lsstate->pendingSlot].levelName);
    return true;

}


//checks if the action needs to ask for confirmation before it happens
static bool ActionNeedsConfirm(u8 slot){
    if(lsstate->curAction <= ACTION_RENAME) return false;
    if (lsstate->curAction == ACTION_COPY && lsstate->savfle->Levels[slot].empty) return false;
    return true;
}


//execute curAction on the pending slot, then go back to load action and take down the window
//this is the only place an action lands. this is called from tapping the slot, or from the last AYS
//the tail runs once per completed action instead of once per case.
static void CommitAction(){
    Raw_Level* lvl = &lsstate->savfle->Levels[lsstate->pendingSlot];

    switch (lsstate->curAction){
        case ACTION_LOAD:
            lsstate->bottomRects[lsstate->activeSlot].Color = Colors[CLR_DK_GRAY];
            *lsstate->ctx->rawLvl = *lvl;
            lsstate->ctx->rebuildLevel = true;
            lsstate->ctx->curSlot = lsstate->pendingSlot;
            lsstate->activeSlot = lsstate->pendingSlot;
            lsstate->bottomRects[lsstate->activeSlot].Color = Colors[CLR_GREEN];
            printConsole("loaded slot %d", lsstate->pendingSlot + 1);
            //load isn't an action to step out of, and it edited nothing, so it returns
            //before the tail: there is no slot to flush, relabel or rebake.
            return;
        case ACTION_RENAME:
            if (!RenameLevel()) return; //cancelled, stay in rename
            break;
        case ACTION_COPY:
            //the whole level, so the name and the best times travel with it
            *lvl = lsstate->savfle->Levels[lsstate->activeSlot];
            printConsole("copied slot %d into slot %d", lsstate->activeSlot + 1, lsstate->pendingSlot + 1);
            break;
        case ACTION_CLEAR_TIMES:
            lvl->bestTimeStandard = 0.0;
            lvl->standardTimeValid = false;
            lvl->bestTimeHard = 0.0;
            lvl->hardTimeValid = false;
            printConsole("cleared slot %d's best times", lsstate->pendingSlot + 1);
            break;
        case ACTION_DELETE:
            //memset rather than a compound literal: a Raw_Level is about 2.2KB, and the
            //literal can be built as a stack temporary and then copied.
            memset(lvl, 0, sizeof *lvl);
            lvl->empty = true;
            printConsole("deleted slot %d", lsstate->pendingSlot + 1);
            break;
    }

    //everything that reaches here edited the slot, so it all needs flushing and relabelling
    SaveFile_WriteSlot(lsstate->pendingSlot, lvl);
    RebuildNameLabels();
    RebuildTimeLabels();
    //only copy and delete change what the level looks like. a rename or a cleared time does not
    if (lsstate->curAction == ACTION_COPY || lsstate->curAction == ACTION_DELETE)
        RebakeSlot(lsstate->pendingSlot);

    //if the slot that just changed is the one the game has loaded, the copy in ctx is now
    //stale: its name, times and tiles are all from the old contents. MainMenu reads that
    //copy rather than the save file, so without this a deleted level keeps its name there.
    if (lsstate->pendingSlot == lsstate->ctx->curSlot){
        *lsstate->ctx->rawLvl = *lvl;
        lsstate->ctx->rebuildLevel = true;
    }

    lsstate->curAction = ACTION_LOAD;
    ResetActionColors();
}




//the callback for the popup for when it is canceled
static void CanceledAction(){
    printConsole("action %d canceled", lsstate->curAction);
    lsstate->curAction = ACTION_LOAD;
    ResetActionColors();
}


//the callback funcitons for the buttons in menuNav
static MenuEvent on_slot(int index, void* user){
    //cast user to correct type
    LevelSelectState* state = (LevelSelectState*)user;

    state->pendingSlot = index; //RECT_LVL1 are in front, so the rect is in this slot
    if (state->curAction == ACTION_COPY && state->activeSlot == index){
        printConsole("attemted to copy self");
        return menu_handled();
    }
    if (state->curAction > ACTION_LOAD && state->curAction != ACTION_COPY && state->savfle->Levels[index].empty){
        printConsole("slot %d is empty, nothing to do", index + 1);
        return menu_handled();
    }
    if (!ActionNeedsConfirm(index)){
        CommitAction();
    } else {
        BuildConfirmText();
        popup_start(state->aysText);
    }
    return menu_handled();
}

static MenuEvent on_action(int index, void* user){
    //cast user to correct type
    LevelSelectState* state = (LevelSelectState*)user;

    //rect RENAME through DELETE run paralel to the action RENAME and DELETE
    u8 act = index - RECT_MAINMENU; //which of the 4 actions were actually chosen
    state->curAction = (state->curAction == act) ? ACTION_LOAD : act; // if the same action was chosen a second time, set it to load
    ResetActionColors();
    if (state->curAction != ACTION_LOAD){
        state->bottomRects[index].Color = Colors[CLR_ORANGE];
    }
    printConsole("action is now %d", state->curAction);
    return menu_handled();
}

static MenuEvent on_back(int index, void* user){
    (void) index;
    (void) user;
    printConsole("Player is going back to main menu from level select");
    return menu_state(STATE_MAIN_MENU);
}

static void on_hover(int index, void* user){
    if (index < SLOT_COUNT){
        ((LevelSelectState*)user)->curSlotImage = index;
    }
}



void LevelSelect_Init(GameContext* ctx){
    printConsole("init Level Select, %u bytes", (unsigned)sizeof(LevelSelectState));

    //calloc, not malloc: _Draw and _End both read the preview and target arrays, and
    //a slot that fails to bake leaves its entry at zero, which both of them treat as
    //"nothing here" rather than as a stale pointer. the zeroing is also what lets the
    //bake loop below run without freeing first, since BakeLevelTexture overwrites its
    //output rather than releasing what was already there.
    lsstate = calloc(1, sizeof(LevelSelectState));
    if (lsstate == NULL){
        printConsole("LevelSelect_Init: out of memory for the state");
        return;
    }

    //set the gamecontext so that commit action can use it.
    lsstate->ctx = ctx;


    lsstate->savfle = malloc(sizeof(Save_File));
    if (lsstate->savfle == NULL || !SaveFile_Read(lsstate->savfle)){
        //every action this state offers reads the save file, so there is nothing to stay for.
        //tearing lsstate down here makes this look like the out of memory case above, which
        //_Logic, _Draw and _End already handle by doing nothing. carrying on with a NULL
        //savfle instead would draw a screen that looks fine and crash on the first slot tap.
        printConsole("LevelSelect_Init: could not read the save file, not entering level select");
        free(lsstate->savfle);
        free(lsstate);
        lsstate = NULL;
        return;
    }

    //one preview per save slot. RebakeSlot frees before it bakes, which the calloc above
    //makes a no-op on this first pass.
    for (int i = 0; i < SLOT_COUNT; i++) RebakeSlot(i);

    lsstate->textBuf = C2D_TextBufNew(LEVEL_SELECT_MAX_GLYPHS);
    lsstate->nameBuf = C2D_TextBufNew(LEVEL_SELECT_NAME_GLYPHS);
    lsstate->timeBuf = C2D_TextBufNew(LEVEL_SELECT_TIME_GLYPHS);
    lsstate->AysWindowBuf = C2D_TextBufNew(LEVEL_SELECT_AYS_GLYPHS);

    //which slot the game is on right now. green on the bottom screen, and the only
    //thing this state reads off the context.
    //first check that the slot is within range, juuuuust in case
    const int initialSlot = (ctx->curSlot < 0 || ctx->curSlot >= SLOT_COUNT) ? 0 :ctx->curSlot;
    lsstate->activeSlot = initialSlot;

    //the save slot buttons, a 2x2 grid.
    char label[8];
    for (int i = 0; i < SLOT_COUNT; i++){
        lsstate->bottomRects[RECT_LVL1 + i] = (Rect){
            .x = SLOT_X0 + (i % 2) * SLOT_COL_SPACING,
            .y = SLOT_Y0 + (i / 2) * SLOT_ROW_SPACING,
            .width = SLOT_W,
            .height = SLOT_H,
            //the slot the game is currently on reads green, the rest are the normal gray
            .Color = Colors[i == lsstate->activeSlot ? CLR_GREEN : CLR_DK_GRAY],
        };
        snprintf(label, sizeof label, "%d", i + 1); //slots read 1-4, not 0-3
        MakeText(label, &lsstate->slotLabels[i], lsstate->textBuf);
    }

    //the names and the times each live in a buffer of their own, so they are built
    //through the same helpers that rename, copy, delete and clear times use
    RebuildNameLabels();
    RebuildTimeLabels();

    //the action button row along the bottom
    for (int i = 0; i < ACTION_BUTTON_COUNT; i++){
        lsstate->bottomRects[RECT_MAINMENU + i] = (Rect){
            .x = ACTION_BUTTONS[i].x,
            .y = ACTION_Y,
            .width = ACTION_W,
            .height = ACTION_H,
            .Color = Colors[CLR_DK_GRAY],
        };
        MakeText(ACTION_BUTTONS[i].label, &lsstate->actionLabels[i], lsstate->textBuf);
    }

    //init the are you sure popup
    const Rect aysBox = {.x = 50, .y = 45, .width = 220, .height = 130, .Color = Colors[CLR_LT_GRAY]};
    popup_init(aysBox, AYS_WINDOW_COUNT, true, true, 0.0f, CommitAction, CanceledAction);
    
    MenuConfig menu_nav_cfg;
    menu_nav_cfg.rects = lsstate->bottomRects;
    menu_nav_cfg.buttons = LS_MENU;
    menu_nav_cfg.count = BOTTOM_RECT_COUNT;
    menu_nav_cfg.touch = true;
    menu_nav_cfg.user = lsstate;
    menu_nav_cfg.activeKey = KEY_A;
    menu_nav_cfg.selected = initialSlot;
    menu_nav_cfg.highlightPad = 5.0f;
    menu_nav_cfg.highlightColor = Colors[CLR_YELLOW];

    lsstate->menu = menu_nav_init(&menu_nav_cfg);



    
    //the cursor needs no init, the calloc already puts it on row 0 column 0, slot 1
    // lsstate->curSlotImage = ctx->curSlot;
    // switch(ctx->curSlot){
    //     case RECT_LVL1:
    //         lsstate->cursorCol = 0;
    //         lsstate->cursorRow = 0;
    //         break;
    //     case RECT_LVL2:
    //         lsstate->cursorCol = 1;
    //         lsstate->cursorRow = 0;
    //         break;
    //     case RECT_LVL3:
    //         lsstate->cursorCol = 0;
    //         lsstate->cursorRow = 1;
    //         break;
    //     case RECT_LVL4:
    //         lsstate->cursorCol = 1;
    //         lsstate->cursorRow = 1;
    //         break;
    //     default:
    //         lsstate->cursorCol = 0;
    //         lsstate->cursorRow = 0;
    //         lsstate->curSlotImage = 0;
    //         lsstate->activeSlot = 0;
    //         break;
    // }


    return;
}



Game_State LevelSelect_Logic(const FrameInput* in, GameContext* ctx){
    if (lsstate == NULL) return STATE_MAIN_MENU; //_Init ran out of memory, do not stay here

    if (popup_status()){
        popup_logic(in);
        return STATE_NONE;
    }

    MenuEvent menu_event = menu_nav_logic(lsstate->menu, in);
    //this should only happen when the main menu button is pressed.
    if (menu_event.kind == MENU_EV_STATE){
        return menu_event.state;
    }

    

    //do state exit logic (only way to leave is to go back to main menu)
    //if player pressed b or touched Main Menu button or clicked on it, return STATE_MAINMENU
    if (in->kDown & KEY_B){
        if (lsstate->curAction == ACTION_LOAD) return STATE_MAIN_MENU;
        lsstate->curAction = ACTION_LOAD;
        ResetActionColors();
    }
    return STATE_NONE;
}

void LevelSelect_DrawTop(C3D_RenderTarget* target, float eyeOffset){
    if (lsstate == NULL) return;

    C2D_TargetClear(target, Colors[CLR_WHITE]);
    C2D_SceneBegin(target);

    //the preview sits furthest back, so the labels around it read as floating in front
    SetDepthLayer(eyeOffset, DEPTH_BASE);

    //a baked preview is exactly BAKED_LEVEL_IMG_WIDTH x _HEIGHT, so it lands at scale 1 in
    //the same place it always has. the borrowed gui image an empty slot gets is a different
    //size, so it is the one that actually gets shrunk and centered here.
    //DrawImageFitCentered handles a slot with no picture at all, so there is no check here.
    DrawImageFitCentered(lsstate->fullLevelPreviews[lsstate->curSlotImage].img,
                         LEVEL_IMG_X, LEVEL_IMG_Y,
                         BAKED_LEVEL_IMG_WIDTH, BAKED_LEVEL_IMG_HEIGHT);

    SetDepthLayer(eyeOffset, DEPTH_INSET);

    //the hovered slot's level name, in the strip above the preview
    DrawTextCentered(&lsstate->slotNameLabels[lsstate->curSlotImage],
                     TOP_SCREEN_WIDTH / 2.0f, NAME_Y,
                     LAYER_BASE, NAME_TEXT_SCALE, NAME_TEXT_SCALE, Colors[CLR_BLACK]);

    //its two best times, side by side in the strip below the preview
    DrawTextCentered(&lsstate->slotStandardLabels[lsstate->curSlotImage],
                     TIME_STANDARD_X, TIME_Y,
                     LAYER_BASE, TIME_TEXT_SCALE, TIME_TEXT_SCALE, Colors[CLR_BLACK]);
    DrawTextCentered(&lsstate->slotHardLabels[lsstate->curSlotImage],
                     TIME_HARD_X, TIME_Y,
                     LAYER_BASE, TIME_TEXT_SCALE, TIME_TEXT_SCALE, Colors[CLR_BLACK]);

    //draw rest of top screen
}




void LevelSelect_DrawBottom(C3D_RenderTarget* target){
    if (lsstate == NULL) return;

    C2D_TargetClear(target, Colors[CLR_WHITE]);
    C2D_SceneBegin(target);

    //highlight: the cursor's rect, grown by the pad, drawn first so it sits underneath
    menu_nav_draw(lsstate->menu);


    //every rect carries its own fill color from _Init, so the active slot comes out
    //green here without a special case
    for (int i = 0; i < SLOT_COUNT; i++){
        DrawRect(&lsstate->bottomRects[RECT_LVL1 + i]);
        DrawTextInRect(&lsstate->slotLabels[i], &lsstate->bottomRects[RECT_LVL1 + i],
                       SLOT_TEXT_SCALE, Colors[CLR_WHITE]);
    }
    for (int i = 0; i < ACTION_BUTTON_COUNT; i++){
        DrawRect(&lsstate->bottomRects[RECT_MAINMENU + i]);
        DrawTextInRect(&lsstate->actionLabels[i], &lsstate->bottomRects[RECT_MAINMENU + i],
                       ACTION_TEXT_SCALE, Colors[CLR_WHITE]);
    }

    popup_draw(target);    
}



void LevelSelect_End(void){
    if (lsstate == NULL) return;

    //all four, target before texture: a render target that outlives the texture it draws
    //into leaves citro3d holding a pointer into freed VRAM. FreeLevelImage handles the
    //rest, including a slot that never baked and a slot holding a borrowed gui image,
    //which has no target of its own.
    for (int i = 0; i < SLOT_COUNT; i++){
        if (lsstate->fullLevelTarget[i] != NULL){
            C3D_RenderTargetDelete(lsstate->fullLevelTarget[i]);
            lsstate->fullLevelTarget[i] = NULL;
        }
        FreeLevelImage(&lsstate->fullLevelPreviews[i]);
    }

    //free the popup
    popup_free();

    

    C2D_TextBufDelete(lsstate->textBuf);
    C2D_TextBufDelete(lsstate->nameBuf);
    C2D_TextBufDelete(lsstate->timeBuf);
    C2D_TextBufDelete(lsstate->AysWindowBuf);
    free(lsstate->savfle);
    free(lsstate);
    lsstate = NULL;
    return;
}

