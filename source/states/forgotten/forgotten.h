#pragma once

//includes
#include "utils/input.h"
#include "datatypes/game_state.h"

void forgotten_Init(GameContext* ctx);

Game_State forgotten_Logic(const FrameInput* in, GameContext* ctx);

void forgotten_DrawTop(C3D_RenderTarget* target, float eyeOffset);

void forgotten_DrawBottom(C3D_RenderTarget* target);

void forgotten_End(void);