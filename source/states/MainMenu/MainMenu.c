#include "states/MainMenu/MainMenu.h"
#include "utils/graphics.h"
#include "utils/debug.h"
#include "utils/format.h"
#include "game_logic/menu_nav.h"
#include <stdlib.h>
#include <stdio.h> //snprintf, for the slot info lines

//bottom screen button layout. every button is the same size, stacked top to bottom.
#define BUTTON_X 40
#define BUTTON_Y0 15       //top edge of the first button
#define BUTTON_SPACING 60  //top edge of one button to the top edge of the next
#define BUTTON_W 240
#define BUTTON_H 30
#define HIGHLIGHT_PAD 5    //how far the highlight sticks out past the selected button
#define BUTTON_TEXT_SCALE 0.6f

//the current slot's info, in the middle of the top screen. the first two lines are
//the slot and its name, then a gap, then the two best times.
#define INFO_Y0 110          //center of the first line
#define INFO_LINE_SPACING 20
#define INFO_GROUP_GAP 8     //extra space between the slot pair and the time pair
#define INFO_TEXT_SCALE 0.6f
#define INFO_LINE_MAX 64     //longest info line is "Name: " plus a 32 char name

/*
* room for every string this state parses, one slot per character (citro2d only
* treats '\n' specially, so spaces count too). the worst case is 179: the title 10,
* the byline 20, the four button labels 39, "Slot 4" 6, "Name: " plus a 32 char
* name 38, and the two time lines 35 and 31. rounded up to the next power of two.
*/
#define MAIN_MENU_MAX_GLYPHS 256

//forward declarations for function pointers
static MenuEvent on_menu_button(int index, void* user);

/*
* the main menu buttons, in top to bottom order. this is the only place a button is
* defined: init, logic and draw all loop over it. compile time data, so it
* lives in the executable. It will be eventually loaded into memory, but in all honesty, this alone is fine.
* it's either here or beind dynamically allocated.
*/
// static const struct {
//     const char* label;
//     Game_State target; //where the button takes you. STATE_QUIT leaves the game
char* MENU_ITEMS[] = {
    "Start Maze",
    "Start Maker",
    "Save Menu",
    "Quit Game"
};

//enum of button destinations
typedef enum{
    MAZE,
    MAKER,
    SAVE,
    QUIT,
    MENU_BUTTON_COUNT
} MainMenuButtons;

static const MenuDef MM_MENU[MENU_BUTTON_COUNT] = {
    [MAZE] = MENU_BTN(MENU_NONE, MENU_NONE, QUIT, MAKER, on_menu_button),
    [MAKER] = MENU_BTN(MENU_NONE, MENU_NONE, MAZE, SAVE, on_menu_button),
    [SAVE] = MENU_BTN(MENU_NONE, MENU_NONE, MAKER, QUIT, on_menu_button),
    [QUIT] = MENU_BTN(MENU_NONE, MENU_NONE, SAVE, MAZE, on_menu_button),    
};


/*
* everything this state owns while it is active. one allocation in _Init,
* one free in _End, and a single 4 byte pointer while the state is inactive.
*/
typedef struct {
    C2D_TextBuf textBuf; //handle to the glyph storage, its own heap block

    //top screen
    C2D_Text titleText;
    C2D_Text bylineText;

    //top screen, the current slot's info. built in _Init from ctx, which is why the
    //block refreshes on its own: main runs _Init again on every entry to this state.
    C2D_Text slotText;
    C2D_Text levelNameText;
    C2D_Text standardTimeText;
    C2D_Text hardTimeText;
    bool drawHardTime; //false on an empty slot, where the hard line is left blank

    //bottom screen
    C2D_Text menu_text[MENU_BUTTON_COUNT];
    Rect rects[MENU_BUTTON_COUNT];
    
    //the struct that is used by menu_nav to keep track of the menu's state
    MenuNav* menu;
} MainMenuState;

static MainMenuState* mmstate; //NULL whenever the state is not active


//the callback used when a button is pressed
static MenuEvent on_menu_button(int index, void* user){
    //cast the user value to the main menu state
    //I know I could also access it through the global variable that is in this file,
    //but I am doing it this way to show the features of the menu nav I made. 
    //MainMenuState* mainMenuState = (MainMenuState*)user;
    //because this is done, this function does not actually need to exist in this file,
    //and could be moved to a 'callbacks' file instead

    //however, dispite all my rambling above, it just isn't used anyway.
    //so nevermind.
    (void) user;
    switch(index){
        case MAZE:
            return menu_state(STATE_MAZE_GAME);
        case MAKER:
            return menu_state(STATE_MAZE_MAKER);
        case SAVE:
            return menu_state(STATE_SAVE_SELECT);
        case QUIT:
        return menu_state(STATE_QUIT);
    }
    return menu_handled();
}



void MainMenu_Init(GameContext* ctx){
    printConsole("init Main Menu, %u bytes", (unsigned)sizeof(MainMenuState));
    mmstate = calloc(1, sizeof(MainMenuState));

    mmstate->textBuf = C2D_TextBufNew(MAIN_MENU_MAX_GLYPHS);

    //top screen
    MakeText("Maze Maker", &mmstate->titleText, mmstate->textBuf);
    MakeText("By Finnegan McDevitt", &mmstate->bylineText, mmstate->textBuf);

    //the current slot's info. these buffers are locals on purpose: C2D_TextParse
    //copies the glyphs into textBuf, so nothing here has to outlive _Init.
    char line[INFO_LINE_MAX];

    snprintf(line, sizeof line, "Slot %d", ctx->curSlot + 1); //slots read 1-4, not 0-3
    MakeText(line, &mmstate->slotText, mmstate->textBuf);

    if (ctx->rawLvl->empty) {
        MakeText("Name: Empty", &mmstate->levelNameText, mmstate->textBuf);
        MakeText("Empty level, no best times", &mmstate->standardTimeText, mmstate->textBuf);
        mmstate->drawHardTime = false; //the hard line stays empty
    } else {
        //levelName is a fixed char[32] that a full length rename leaves unterminated,
        //so the precision caps how far %s reads. an unbounded %s could run off the end.
        snprintf(line, sizeof line, "Name: %.*s", LEVEL_NAME_MAX_LEN, ctx->rawLvl->levelName);
        MakeText(line, &mmstate->levelNameText, mmstate->textBuf);

        char time[TIME_STR_LEN];

        if (ctx->rawLvl->standardTimeValid) FormatTime(ctx->rawLvl->bestTimeStandard, time, sizeof time);
        snprintf(line, sizeof line, "Best Standard Time: %s",
                 ctx->rawLvl->standardTimeValid ? time : "No time yet");
        MakeText(line, &mmstate->standardTimeText, mmstate->textBuf);

        if (ctx->rawLvl->hardTimeValid) FormatTime(ctx->rawLvl->bestTimeHard, time, sizeof time);
        snprintf(line, sizeof line, "Best Hard Time: %s",
                 ctx->rawLvl->hardTimeValid ? time : "No time yet");
        MakeText(line, &mmstate->hardTimeText, mmstate->textBuf);

        mmstate->drawHardTime = true;
    }

    //bottom screen
    for (int i = 0; i < MENU_BUTTON_COUNT; i++) {
        mmstate->rects[i] = (Rect){
            .x = BUTTON_X,
            .y = BUTTON_Y0 + i * BUTTON_SPACING,
            .width = BUTTON_W,
            .height = BUTTON_H,
            .Color = Colors[CLR_DK_GRAY],
        };
        MakeText(MENU_ITEMS[i], &mmstate->menu_text[i], mmstate->textBuf);
    }

    //initalize the buttons
    mmstate->menu = menu_nav_init(&(MenuConfig){
        .buttons = MM_MENU, 
        .count = MENU_BUTTON_COUNT,
        .rects = mmstate->rects,
        .selected = MAZE,
        .user = mmstate, 
        .touch = true,
        .highlightPad = 5.0f,
        .highlightColor = Colors[CLR_YELLOW],
    });
    


    if (ctx->rebuildLevel) {
        printConsole("Coalesce the level here");

        //make funcition to rebuild ctx->builtLvl from ctx->rawLvl and call it here

        ctx->rebuildLevel = false;
    }
}


Game_State MainMenu_Logic(const FrameInput* in, GameContext* ctx){

    if (in->kDown & KEY_L)
    {
        printConsole("L pressed on Main Menu state");
        return STATE_DEBUG; //THIS IS DEBUG AND WILL BE CHANGED LATER
    }

    if (in->kDown & KEY_R)
    {
        printConsole("R pressed on Main Menu state");
        return STATE_you_recieved_the_egg; //will be changed later, so that this happens randomly when the player goes out of bounds.
    }

    //check the menu nav for if the user pressed any buttons
    //and react accordingly.
    MenuEvent menu_event = menu_nav_logic(mmstate->menu, in);
    if (menu_event.kind == MENU_EV_STATE){
        return menu_event.state;
    }
    return STATE_NONE;
}

void MainMenu_DrawTop(C3D_RenderTarget* target, float eyeOffset){
    C2D_TargetClear(target, Colors[CLR_WHITE]);
    C2D_SceneBegin(target);

    //the title pair sits nearer the glass than the slot info behind it
    SetDepthLayer(eyeOffset, DEPTH_INSET);
    DrawTextCentered(&mmstate->titleText, TOP_SCREEN_WIDTH / 2, TOP_SCREEN_HIGHT / 4, LAYER_BASE, 2, 2, Colors[CLR_BLACK]);
    DrawTextCentered(&mmstate->bylineText, TOP_SCREEN_WIDTH / 2, 19 * (TOP_SCREEN_HIGHT / 20), LAYER_BASE, 1, 1, Colors[CLR_BLACK]);

    //the slot info block: slot and name, a gap, then the two best times
    const C2D_Text* infoLines[] = {
        &mmstate->slotText,
        &mmstate->levelNameText,
        &mmstate->standardTimeText,
        &mmstate->hardTimeText,
    };
    int infoCount = mmstate->drawHardTime ? 4 : 3; //an empty slot has no hard time line

    SetDepthLayer(eyeOffset, DEPTH_BASE);
    for (int i = 0; i < infoCount; i++) {
        float y = INFO_Y0 + i * INFO_LINE_SPACING + (i >= 2 ? INFO_GROUP_GAP : 0);
        DrawTextCentered(infoLines[i], TOP_SCREEN_WIDTH / 2, y,
                         LAYER_BASE, INFO_TEXT_SCALE, INFO_TEXT_SCALE, Colors[CLR_BLACK]);
    }
}

void MainMenu_DrawBottom(C3D_RenderTarget* target){
    C2D_TargetClear(target, Colors[CLR_WHITE]);
    C2D_SceneBegin(target);

    menu_nav_draw(mmstate->menu);

    for (int i = 0; i < MENU_BUTTON_COUNT; i++) {
        DrawRect(&mmstate->rects[i]);
        DrawTextInRect(&mmstate->menu_text[i], &mmstate->rects[i], BUTTON_TEXT_SCALE, Colors[CLR_WHITE]);
    }
}


void MainMenu_End(void){
    free(mmstate->menu);
    C2D_TextBufDelete(mmstate->textBuf);
    free(mmstate);
    mmstate = NULL;
}
