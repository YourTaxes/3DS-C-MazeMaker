#pragma once

#include <citro2d.h> //C3D_RenderTarget
#include <3ds.h>
#include "input.h"
#include "level_file.h"

typedef enum{
    STATE_MAIN_MENU,
    STATE_MAZE_GAME,
    STATE_MAZE_MAKER,
    STATE_SAVE_SELECT,
    STATE_OOB, //out of bounds
    //STATE_there_is_a_man_here,
    //STATE_he_is_behind_the_tree,
    //STATE_he_offers_you_something,
    //STATE_you_reach_out_your_hand,
    STATE_you_recieved_the_egg,
    //STATE_the_man_smiles,
    //STATE_there_is_no_longer_a_man_behind_the_tree,
    STATE_DEBUG,

    //non switching status identifiers
    STATE_COUNT, //number of real states above
    STATE_NONE,  //stay in the current state
    STATE_QUIT   //leave the game
} Game_State;

/*
* game-wide data that outlives any one state. owned by main, and every
* state's _Init and _Logic get a pointer to it.
*/
typedef struct {
    Raw_Level* rawLvl;     //the editable grid for the current save slot
    Built_Level* builtLvl; //the playable version compiled from rawLvl
    bool rebuildLevel;     //rawLvl changed, so builtLvl is stale and must be recompiled
} GameContext;

/*
* the four functions every state file provides. main keeps one of these per
* Game_State and calls through it, so it never needs to know which state it is on.
*   init  - allocate everything the state needs. runs once, right before its first logic call
*   logic - one frame of input handling. returns the state to switch to, STATE_NONE to stay, or STATE_QUIT
*   draw  - render both screens
*   end   - free everything init allocated. runs once, after the state's last draw
*/
typedef struct {
    void (*init)(GameContext* ctx);
    Game_State (*logic)(const FrameInput* in, GameContext* ctx);
    void (*draw)(C3D_RenderTarget* top, C3D_RenderTarget* bottom);
    void (*end)(void);
} StateFns;
