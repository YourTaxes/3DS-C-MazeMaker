#pragma once

#include "datatypes/game_state.h"

//these actions are somewhat like substates of the level select,
//as they change what happens when the player clicks on a level.
typedef enum{
    ACTION_LOAD, 
    ACTION_RENAME,
    ACTION_COPY, // to copy slot x into slot y? - 28
    ACTION_CLEAR_TIMES, // to clear slot x's best times? - 30
    ACTION_DELETE // to delete slot x? - 18
} Load_Action;

typedef enum{
    RECT_LVL1,
    RECT_LVL2,
    RECT_LVL3,
    RECT_LVL4,
    RECT_MAINMENU,
    RECT_RENAME,
    RECT_COPY,
    RECT_CLEAR_TIMES,
    RECT_DELETE,
} RectIDs;


void LevelSelect_Init(GameContext* ctx);

Game_State LevelSelect_Logic(const FrameInput* in, GameContext* ctx);

void LevelSelect_DrawTop(C3D_RenderTarget* target, float eyeOffset);

void LevelSelect_DrawBottom(C3D_RenderTarget* target);

void LevelSelect_End(void);