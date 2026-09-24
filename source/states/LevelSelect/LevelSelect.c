#include "states/LevelSelect/LevelSelect.h"
#include "utils/graphics.h"
#include "utils/debug.h"
#include "datatypes/save_file.h"
#include "utils/format.h" //FormatTime, for the best time lines
#include <stdlib.h>
#include <stdio.h> //snprintf, for the slot number labels


//defines
#define SLOT_COUNT 4
#define BOTTOM_RECT_COUNT 12

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



//structs

/*
* the action button row, in left to right order, which is also RECT_MAINMENU through
* RECT_DELETE. this is the only place one is defined: _Init, _Logic and _Draw all loop
* over it. the x values are not evenly spaced, so they are spelled out rather than
* derived the way the slot grid is.
*/
static const struct {
    const char* label; //drawn on the button, so it carries its own line break
    const char* name;  //the same button on one line, for the console
    float x;
} ACTION_BUTTONS[] = {
    { "Main\nMenu",    "Main Menu",    10 },
    { "Rename",        "Rename",       70 },
    { "Copy",          "Copy",        135 },
    { "Clear\nTimes",  "Clear Times", 200 },
    { "Delete",        "Delete",      260 },
};
#define ACTION_BUTTON_COUNT (sizeof(ACTION_BUTTONS) / sizeof(ACTION_BUTTONS[0]))
_Static_assert(ACTION_BUTTON_COUNT == RECT_DELETE - RECT_MAINMENU + 1,
               "ACTION_BUTTONS must cover RECT_MAINMENU through RECT_DELETE");


#define CURSOR_ROW_COUNT 3
#define CURSOR_MAX_COLS 5
#define ACTION_ROW 2 //special case, 5 buttons in this row

/*
* the d and circle pad navigation matrix. rows 0 and 1 are the save slots, row 2 is the
* action row. entries are RectIDs, and count is how far right that row goes, since the
* rows are not all the same length.
*/
static const struct {
    u8 count;
    u8 rects[CURSOR_MAX_COLS];
} CURSOR_GRID[CURSOR_ROW_COUNT] = {
    { 2, { RECT_LVL1, RECT_LVL2 } },
    { 2, { RECT_LVL3, RECT_LVL4 } },
    { 5, { RECT_MAINMENU, RECT_RENAME, RECT_COPY, RECT_CLEAR_TIMES, RECT_DELETE } },
};
_Static_assert(RECT_LVL1 == 0 && RECT_LVL4 == SLOT_COUNT - 1,
               "the four slot rects must lead RectIDs for the hover check to work");

/*
* the slot rows are two wide and the action row is five, so a vertical move across them
* translates the column instead of keeping it. the pairs are by what sits under what on
* screen: slots 1 and 3 are above Rename, slots 2 and 4 are above Clear Times. going the
* other way, Main Menu, Rename and Copy are all left of the gap between the slot columns,
* Clear Times and Delete are right of it.
*/
static const u8 SLOT_COL_TO_ACTION_COL[2] = { 1, 3 }; //-> RECT_RENAME, RECT_CLEAR_TIMES
static const u8 ACTION_COL_TO_SLOT_COL[CURSOR_MAX_COLS] = { 0, 0, 0, 1, 1 };



//everything owned by the LevelSelect
typedef struct{
    C2D_TextBuf textBuf; //the static text, parsed once in _Init
    C2D_TextBuf nameBuf; //just the four level names, reparsed on a rename
    C2D_TextBuf timeBuf; //just the eight best times, reparsed when a time is cleared

    C3D_Tex fullLevelTextures[SLOT_COUNT];
    C3D_RenderTarget *fullLevelTarget[SLOT_COUNT];
    Tex3DS_SubTexture fullLevelSubTex[SLOT_COUNT];
    C2D_Image fullLevelImages[SLOT_COUNT];

    

    //this holds all the rects for the ui for the bottom screen.

    Rect bottomRects[BOTTOM_RECT_COUNT];

    //one label per button, parallel to the rects above
    C2D_Text slotLabels[SLOT_COUNT];
    C2D_Text actionLabels[ACTION_BUTTON_COUNT];

    //the name of the level in each slot, drawn above that slot's preview on the top
    //screen. these live in nameBuf, so they all go stale together whenever
    //RebuildNameLabels runs. same indexing as fullLevelImages.
    C2D_Text slotNameLabels[SLOT_COUNT];

    //each slot's two best times, below the preview. these live in timeBuf, so they all
    //go stale together whenever RebuildTimeLabels runs.
    C2D_Text slotStandardLabels[SLOT_COUNT];
    C2D_Text slotHardLabels[SLOT_COUNT];

    //the d pad cursor's place in CURSOR_GRID. starts on slot 1, which the calloc gives free.
    u8 cursorRow;
    u8 cursorCol;

    //the slot the game currently has loaded, copied off ctx in _Init because _Draw is
    //not handed a GameContext. 0 indexed, and its button is drawn green.
    int activeSlot;

    //the whole save file, read once in _Init and kept for the life of the state so
    //the slot list and the rename, copy and delete actions all work off one copy.
    //it is on the heap because a Save_File is far too big to sit in a stack frame.
    Save_File *savfle;


    //levelSelect specific details
    u8 curAction; //this will hold LoadAction values casted to u8 to save space
    
    
    //This reperesents the current place in the are you sure chain. 
    //when AYS status is currently above 1, then a window will show up that asks "Are you Sure?" (or something adjacent on higher levels) with a yes and no button
    //on any stage, if the player presses no, or presses somewhere where Rect_Tapped(the AYS window background) != true, then AYS is set to 0, canceling the current action.
    //if the player presses inside the AYS window, but not a button, nothing happens.
    //if the player presses inside the yes button, then AYS will advance to the next stage, unless the current stage is the last stage.
    //if the current stage is the last stage, then pressing yes will complete the action in which AYS was asking.
    //the action to complete is stored in the curAction value, as it is unchanged from when AYS starts.
    //the d and circle pads do not affect the standard level select buttons when the window is up, instead it affects the window's buttons.
    u8 AYS_Status;

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


//put the cursor on the button the player just touched, so the highlight does not sit
//somewhere else after a tap. a rect that is not on the grid leaves the cursor alone.
static void CursorToRect(int rectID){
    for (int r = 0; r < CURSOR_ROW_COUNT; r++){
        for (int c = 0; c < CURSOR_GRID[r].count; c++){
            if (CURSOR_GRID[r].rects[c] == rectID){
                lsstate->cursorRow = (u8)r;
                lsstate->cursorCol = (u8)c;
                return;
            }
        }
    }
}


void LevelSelect_Init(GameContext* ctx){
    //calloc, not malloc: _Draw and _End both read the texture and target arrays, and
    //a slot that fails to bake leaves its entry at zero, which both of them treat as
    //"nothing here" rather than as a stale pointer.
    lsstate = calloc(1, sizeof(LevelSelectState));
    if (lsstate == NULL){
        printConsole("LevelSelect_Init: out of memory for the state");
        return;
    }

    lsstate->savfle = malloc(sizeof(Save_File));
    if (lsstate->savfle == NULL || !SaveFile_Read(lsstate->savfle)){
        printConsole("LevelSelect_Init: could not read the save file");
        free(lsstate->savfle);
        lsstate->savfle = NULL;
    }

    //create all 4 level textures here, one per save slot.
    if (lsstate->savfle != NULL){
        for (int i = 0; i < SLOT_COUNT; i++){
            if (!BakeLevelTexture(&lsstate->fullLevelTextures[i], &lsstate->fullLevelTarget[i], &lsstate->fullLevelSubTex[i], &lsstate->savfle->Levels[i], &lsstate->fullLevelImages[i])){
                printConsole("LevelSelect_Init: slot %d did not bake", i);
            }
        }
    }

    lsstate->textBuf = C2D_TextBufNew(LEVEL_SELECT_MAX_GLYPHS);
    lsstate->nameBuf = C2D_TextBufNew(LEVEL_SELECT_NAME_GLYPHS);
    lsstate->timeBuf = C2D_TextBufNew(LEVEL_SELECT_TIME_GLYPHS);

    //which slot the game is on right now. green on the bottom screen, and the only
    //thing this state reads off the context.
    lsstate->activeSlot = ctx->curSlot;

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

    
    //the cursor needs no init, the calloc already puts it on row 0 column 0, slot 1
    lsstate->curSlotImage = 0;

    return;
}



Game_State LevelSelect_Logic(const FrameInput* in, GameContext* ctx){
    (void)ctx; //nothing here writes back to the context yet
    if (lsstate == NULL) return STATE_MAIN_MENU; //_Init ran out of memory, do not stay here

    //left and right walk the row the player is on, wrapping at both ends
    const u8 rowLen = CURSOR_GRID[lsstate->cursorRow].count;
    if (in->kDown & (KEY_LEFT | KEY_CPAD_LEFT))
        lsstate->cursorCol = (lsstate->cursorCol + rowLen - 1) % rowLen;
    if (in->kDown & (KEY_RIGHT | KEY_CPAD_RIGHT))
        lsstate->cursorCol = (lsstate->cursorCol + 1) % rowLen;

    //up and down wrap too, but crossing into or out of the five wide action row has to
    //translate the column, or the cursor would not land under where it started
    u8 newRow = lsstate->cursorRow;
    if (in->kDown & (KEY_UP | KEY_CPAD_UP))
        newRow = (newRow + CURSOR_ROW_COUNT - 1) % CURSOR_ROW_COUNT;
    if (in->kDown & (KEY_DOWN | KEY_CPAD_DOWN))
        newRow = (newRow + 1) % CURSOR_ROW_COUNT;

    if (newRow != lsstate->cursorRow){
        if (newRow == ACTION_ROW)
            lsstate->cursorCol = SLOT_COL_TO_ACTION_COL[lsstate->cursorCol];
        else if (lsstate->cursorRow == ACTION_ROW)
            lsstate->cursorCol = ACTION_COL_TO_SLOT_COL[lsstate->cursorCol];
        //slot row to slot row keeps the column, both of those rows are the same width
        lsstate->cursorRow = newRow;
    }

    //only allow one action per frame. if both happen, then screen touch takes priority
    int pressed = -1;
    for (int i = RECT_LVL1; i <= RECT_DELETE; i++){
        if (Rect_Tapped(&lsstate->bottomRects[i], in)){
            pressed = i;
            break;
        }
    }
    if (pressed >= 0){
        CursorToRect(pressed); //the highlight follows the finger
    } else if (in->kDown & KEY_A){
        pressed = CURSOR_GRID[lsstate->cursorRow].rects[lsstate->cursorCol];
    }

    //the top screen preview follows the cursor, so moving onto a slot with the d pad shows
    //that slot's level. a tap lands here too, because CursorToRect just put the cursor on it.
    u8 hovered = CURSOR_GRID[lsstate->cursorRow].rects[lsstate->cursorCol];
    if (hovered < SLOT_COUNT) lsstate->curSlotImage = hovered; //RECT_LVL1..4 lead the enum

    switch (pressed){
        case RECT_LVL1: case RECT_LVL2: case RECT_LVL3: case RECT_LVL4:
            //selecting a slot does not load it yet, the preview is all that moves
            printConsole("player selected slot %d", pressed - RECT_LVL1 + 1);
            break;

        case RECT_MAINMENU:
            printConsole("player is going back to main menu from level select");
            return STATE_MAIN_MENU;

        case RECT_RENAME:      lsstate->curAction = ACTION_RENAME;      break;
        case RECT_COPY:        lsstate->curAction = ACTION_COPY;        break;
        case RECT_CLEAR_TIMES: lsstate->curAction = ACTION_CLEAR_TIMES; break;
        case RECT_DELETE:      lsstate->curAction = ACTION_DELETE;      break;

        default: break; //-1, nothing was pressed this frame
    }
    if (pressed >= RECT_RENAME && pressed <= RECT_DELETE){
        printConsole("action set to \"%s\"", ACTION_BUTTONS[pressed - RECT_MAINMENU].name);
    }

    //do state exit logic (only way to leave is to go back to main menu)
    //if player pressed b or touched Main Menu button or clicked on it, return STATE_MAINMENU
    if (in->kDown & KEY_B){
        printConsole("player is going back to main menu from level select");
        return STATE_MAIN_MENU;
    }
    return STATE_NONE;
}




void LevelSelect_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom){
    if (lsstate == NULL) return;

    C2D_TargetClear(top, Colors[CLR_WHITE]);
    C2D_SceneBegin(top);

    //a slot whose bake failed has no texture, and drawing it would follow a null one
    C2D_Image slotImage = lsstate->fullLevelImages[lsstate->curSlotImage];
    if (slotImage.tex != NULL){
        C2D_DrawImageAt(slotImage, LEVEL_IMG_X, LEVEL_IMG_Y, 0, NULL, 1, 1);
    }

    //the hovered slot's level name, in the strip above the preview
    DrawTextCentered(&lsstate->slotNameLabels[lsstate->curSlotImage],
                     TOP_SCREEN_WIDTH / 2.0f, NAME_Y,
                     NAME_TEXT_SCALE, NAME_TEXT_SCALE, Colors[CLR_BLACK]);

    //its two best times, side by side in the strip below the preview
    DrawTextCentered(&lsstate->slotStandardLabels[lsstate->curSlotImage],
                     TIME_STANDARD_X, TIME_Y,
                     TIME_TEXT_SCALE, TIME_TEXT_SCALE, Colors[CLR_BLACK]);
    DrawTextCentered(&lsstate->slotHardLabels[lsstate->curSlotImage],
                     TIME_HARD_X, TIME_Y,
                     TIME_TEXT_SCALE, TIME_TEXT_SCALE, Colors[CLR_BLACK]);

    //draw rest of top screen


    C2D_TargetClear(bottom, Colors[CLR_WHITE]);
    C2D_SceneBegin(bottom);

    //highlight: the cursor's rect, grown by the pad, drawn first so it sits underneath
    Rect highlight = lsstate->bottomRects[CURSOR_GRID[lsstate->cursorRow].rects[lsstate->cursorCol]];
    highlight.x -= HIGHLIGHT_PAD;
    highlight.y -= HIGHLIGHT_PAD;
    highlight.width += 2 * HIGHLIGHT_PAD;
    highlight.height += 2 * HIGHLIGHT_PAD;
    highlight.Color = Colors[CLR_YELLOW];
    DrawRect(&highlight);

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
}



void LevelSelect_End(void){
    if (lsstate == NULL) return;

    //free all four, skipping any slot that never baked
    for (int i = 0; i < SLOT_COUNT; i++){
        if (lsstate->fullLevelTarget[i] == NULL) continue;
        C3D_RenderTargetDelete(lsstate->fullLevelTarget[i]);
        lsstate->fullLevelTarget[i] = NULL;
        C3D_TexDelete(&lsstate->fullLevelTextures[i]);
    }
    C2D_TextBufDelete(lsstate->textBuf);
    C2D_TextBufDelete(lsstate->nameBuf);
    C2D_TextBufDelete(lsstate->timeBuf);
    free(lsstate->savfle);
    free(lsstate);
    lsstate = NULL;
    return;
}

