#pragma once

#include <stdlib.h>
#include "state_utils.h"



/*
* returns the state to switch to, STATE_NONE to stay, or STATE_QUIT
*/
Game_State Debug_logic(u32 kDown, bool* stateSwitch);

void Debug_Init(bool* stateSwitch);

void Debug_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom);

void Debug_end(void);