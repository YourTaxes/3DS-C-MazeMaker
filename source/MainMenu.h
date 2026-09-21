#pragma once

#include <stdlib.h>
#include "state_utils.h"
#include "level_file.h"

typedef enum {
    Highlight_Maze,
    Highlight_Maker,
    Highlight_Lvl,
    Highlight_Quit
} HighlightPositions;

#define Highlight_COUNT (Highlight_Quit + 1)

/*
* returns the state to switch to, STATE_NONE to stay, or STATE_QUIT
*/
Game_State MainMenu_Logic(const FrameInput* in, GameContext* ctx);

void MainMenu_Init(GameContext* ctx);

void MainMenu_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom);

void MainMenu_End(void);