#pragma once

#include "datatypes/game_state.h"

void MainMenu_Init(GameContext* ctx);

/*
* returns the state to switch to, STATE_NONE to stay, or STATE_QUIT
*/
Game_State MainMenu_Logic(const FrameInput* in, GameContext* ctx);

void MainMenu_DrawTop(C3D_RenderTarget* target, float eyeOffset);

void MainMenu_DrawBottom(C3D_RenderTarget* target);

void MainMenu_End(void);
