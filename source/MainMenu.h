#include <stdlib.h>
#include "util.h"
#include "level_file.h"

void MainMenu_Logic(u32 kDown, Raw_Level* rawLvl, Built_Level* builtLvl, bool* stateSwitch, bool rebuildLvl);

void MainMenu_Init(Raw_Level* rawLvl, Built_Level* builtLvl, bool rebuildLvl);

void MainMenu_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom);

void MainMenu_End(Game_State* state);