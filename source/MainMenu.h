#include <stdlib.h>
#include "state_utils.h"
#include "level_file.h"

typedef enum {
    Highlight_Maze,
    Highlight_Maker,
    Highlight_Lvl,
    Highlight_Quit
} HighlightPositions;

#define Highlight_COUNT (Highlight_Quit + 1)

bool MainMenu_Logic(u32 kDown, touchPosition* touch, Raw_Level* rawLvl, Built_Level* builtLvl, bool* stateSwitch, bool* rebuildLvl);

void MainMenu_Init(Raw_Level* rawLvl, Built_Level* builtLvl, bool* rebuildLvl);

void MainMenu_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom);

void MainMenu_End(Game_State* state);