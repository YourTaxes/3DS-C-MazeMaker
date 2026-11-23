#pragma once

#include "datatypes/game_state.h"

void MenuNavTest_Init(GameContext* ctx);

//returns the state to switch to. same as all the other logics
Game_State MenuNavTest_Logic(const FrameInput* in, GameContext* ctx);

void MenuNavTest_DrawTop(C3D_RenderTarget* target, float eyeOffset);

void MenuNavTest_DrawBottom(C3D_RenderTarget* target);

void MenuNavTest_End(void);