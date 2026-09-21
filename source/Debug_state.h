#pragma once

#include "state_utils.h"



/*
* returns the state to switch to, STATE_NONE to stay, or STATE_QUIT
*/
Game_State Debug_logic(const FrameInput* in, GameContext* ctx);

void Debug_Init(GameContext* ctx);

void Debug_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom);

void Debug_end(void);