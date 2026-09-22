#pragma once

#include "state_utils.h"

/*
* the buttons, in top to bottom order. this is the only place a button is
* defined: init, logic and draw all loop over it. compile time data, so it
* lives in the executable and costs no RAM.
*/
static const struct {
    const char* label;
    Game_State target; //where the button takes you. STATE_QUIT leaves the game
} 

MENU_ITEMS[] = {
    { "Start Maze",   STATE_MAZE_GAME   },
    { "Start Maker",  STATE_MAZE_MAKER  },
    { "Level Select", STATE_SAVE_SELECT },
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
    int cur; //index of the highlighted button
} MainMenuState;

void MainMenu_Init(GameContext* ctx);

/*
* returns the state to switch to, STATE_NONE to stay, or STATE_QUIT
*/
Game_State MainMenu_Logic(const FrameInput* in, GameContext* ctx);

void MainMenu_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom);

void MainMenu_End(void);
