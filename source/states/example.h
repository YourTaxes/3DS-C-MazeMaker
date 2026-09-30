#pragma once

#include "datatypes/game_state.h"

void state_Init(GameContext* ctx);

Game_State state_Logic(const FrameInput* in, GameContext* ctx);

void state_DrawTop(C3D_RenderTarget* target, float eyeOffset);

void state_DrawBottom(C3D_RenderTarget* target);

void state_End(void);