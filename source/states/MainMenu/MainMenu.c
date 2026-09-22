#include "states/MainMenu/MainMenu.h"
#include "utils/graphics.h"
#include "utils/debug.h"
#include <stdlib.h>

//bottom screen button layout. every button is the same size, stacked top to bottom.
#define BUTTON_X 40
#define BUTTON_Y0 15       //top edge of the first button
#define BUTTON_SPACING 60  //top edge of one button to the top edge of the next
#define BUTTON_W 240
#define BUTTON_H 30
#define HIGHLIGHT_PAD 5    //how far the highlight sticks out past the selected button
#define BUTTON_TEXT_SCALE 0.6f

#define MAIN_MENU_MAX_GLYPHS 128 //room for every string this state parses

/*
* the main menu buttons, in top to bottom order. this is the only place a button is
* defined: init, logic and draw all loop over it. compile time data, so it
* lives in the executable. It will be eventually loaded into memory, but in all honesty, this alone is fine.
* it's either here or beind dynamically allocated.
*/
static const struct {
    const char* label;
    Game_State target; //where the button takes you. STATE_QUIT leaves the game
} MENU_ITEMS[] = {
    { "Start Maze",   STATE_MAZE_GAME   },
    { "Start Maker",  STATE_MAZE_MAKER  },
    { "Save Menu", STATE_SAVE_SELECT },
    { "Quit Game",    STATE_QUIT        },
};
#define MENU_BUTTON_COUNT (sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]))

/*
* everything this state owns while it is active. one allocation in _Init,
* one free in _End, and a single 4 byte pointer while the state is inactive.
*/
typedef struct {
    C2D_TextBuf textBuf; //handle to the glyph storage, its own heap block

    //top screen
    C2D_Text titleText;
    C2D_Text nameText;

    //bottom screen
    C2D_Text menu_text[MENU_BUTTON_COUNT];
    Rect rects[MENU_BUTTON_COUNT];
    int highlight_index; //index of the highlighted button
} MainMenuState;

static MainMenuState* mmstate; //NULL whenever the state is not active


void MainMenu_Init(GameContext* ctx){
    printConsole("init Main Menu, %u bytes", (unsigned)sizeof(MainMenuState));
    mmstate = calloc(1, sizeof(MainMenuState));

    mmstate->textBuf = C2D_TextBufNew(MAIN_MENU_MAX_GLYPHS);

    //top screen
    MakeText("Maze Maker", &mmstate->titleText, mmstate->textBuf);
    MakeText("By Finnegan McDevitt", &mmstate->nameText, mmstate->textBuf);

    //bottom screen
    for (int i = 0; i < MENU_BUTTON_COUNT; i++) {
        mmstate->rects[i] = (Rect){
            .x = BUTTON_X,
            .y = BUTTON_Y0 + i * BUTTON_SPACING,
            .width = BUTTON_W,
            .height = BUTTON_H,
            .Color = Colors[CLR_DK_GRAY],
        };
        MakeText(MENU_ITEMS[i].label, &mmstate->menu_text[i], mmstate->textBuf);
    }
    mmstate->highlight_index = 0;

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

    //move the highlight, wrapping at both ends
    if (in->kDown & (KEY_UP | KEY_CPAD_UP))
    {
        mmstate->highlight_index = (mmstate->highlight_index + MENU_BUTTON_COUNT - 1) % MENU_BUTTON_COUNT;
    }
    if (in->kDown & (KEY_DOWN | KEY_CPAD_DOWN))
    {
        mmstate->highlight_index = (mmstate->highlight_index + 1) % MENU_BUTTON_COUNT;
    }

    //only allow one action per frame. if both happen, then screen touch takes priority
    for (int i = 0; i < MENU_BUTTON_COUNT; i++) {
        if (Rect_Tapped(&mmstate->rects[i], in)) {
            printConsole("player touched \"%s\"", MENU_ITEMS[i].label);
            return MENU_ITEMS[i].target;
        }
    }
    if (in->kDown & KEY_A) {
        printConsole("player pressed A on \"%s\"", MENU_ITEMS[mmstate->highlight_index].label);
        return MENU_ITEMS[mmstate->highlight_index].target;
    }

    return STATE_NONE;
}


void MainMenu_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom){
    //draw the top screen
    C2D_TargetClear(top, Colors[CLR_WHITE]);
    C2D_SceneBegin(top);

    DrawTextCentered(&mmstate->titleText, TOP_SCREEN_WIDTH / 2, TOP_SCREEN_HIGHT / 4, 2, 2, Colors[CLR_BLACK]);
    DrawTextCentered(&mmstate->nameText, TOP_SCREEN_WIDTH / 2, 19 * (TOP_SCREEN_HIGHT / 20), 1, 1, Colors[CLR_BLACK]);

    //draw the bottom screen
    C2D_TargetClear(bottom, Colors[CLR_WHITE]);
    C2D_SceneBegin(bottom);

    //highlight: the selected button's rect, grown by the pad, drawn underneath it
    Rect highlight = mmstate->rects[mmstate->highlight_index];
    highlight.x -= HIGHLIGHT_PAD;
    highlight.y -= HIGHLIGHT_PAD;
    highlight.width += 2 * HIGHLIGHT_PAD;
    highlight.height += 2 * HIGHLIGHT_PAD;
    highlight.Color = Colors[CLR_YELLOW];
    DrawRect(&highlight);

    for (int i = 0; i < MENU_BUTTON_COUNT; i++) {
        DrawRect(&mmstate->rects[i]);
        DrawTextInRect(&mmstate->menu_text[i], &mmstate->rects[i], BUTTON_TEXT_SCALE, Colors[CLR_WHITE]);
    }
}


void MainMenu_End(void){
    C2D_TextBufDelete(mmstate->textBuf);
    free(mmstate);
    mmstate = NULL;
}
