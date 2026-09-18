#include <stdlib.h>
#include "util.h"



void Debug_logic(u32 kDown, bool* stateSwitch);

void Debug_Init(bool* stateSwitch);

void Debug_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom);

void Debug_end(Game_State* state);