#pragma once

#include "game_state.h"

void MainMenu_Init(GameContext* ctx);

/*
* returns the state to switch to, STATE_NONE to stay, or STATE_QUIT
*/
Game_State MainMenu_Logic(const FrameInput* in, GameContext* ctx);

void MainMenu_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom);

void MainMenu_End(void);
