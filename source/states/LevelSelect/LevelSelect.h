#pragma once

#include "datatypes/game_state.h"

//these actions are somewhat like substates of the level select,
//as they change what happens when the player clicks on a level.
typedef enum{
    ACTION_LOAD,
    ACTION_RENAME,
    ACTION_COPY,
    ACTION_CLEAR_TIMES,
    ACTION_DELETE
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
    RECT_AYS_WINDOW,
    RECT_AYS_NO,
    RECT_AYS_YES
} RectIDs;


void LevelSelect_Init(GameContext* ctx);

Game_State LevelSelect_Logic(const FrameInput* in, GameContext* ctx);

void LevelSelect_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom);

void LevelSelect_End(void);