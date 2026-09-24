#pragma once

#include "datatypes/game_state.h"

void state_Init(GameContext* ctx);

Game_State state_Logic(const FrameInput* in, GameContext* ctx);

void state_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom);

void state_End(void);