#pragma once

#include "datatypes/level_file.h"

//need to figure out good size of the player.
#define PLAYER_SIZE 20

/*
* pixels per frame. both of these have to stay UNDER TILE_SIZE / 2 (10): move_player resolves a
* collision by pushing the player back off the least overlapped face of the wall, so a step that
* can bury them past the middle of a 20px wall would be pushed out the far side instead - ie
* straight through it. the same bound is what keeps every tile the player can reach inside the
* 3x3 the caller builds around get_player_tile. worth rechecking if the delta time multiply
* move_player's comment mentions ever lands, since that changes what these mean.
*/

/*
* pixels per second. cannot be more than 10
*/
#define PLAYER_BASE_SPEED 4
#define PLAYER_HARD_SPEED 6

typedef struct FrameInput FrameInput;

typedef enum{
    GAME_KEY_RED,
    GAME_KEY_GREEN,
    GAME_KEY_BLUE,
    GAME_KEY_PINK,
    GAME_KEY_COUNT
} GameKeyIndex;

//the different sides of the tiles a rect can run into. returned by the 
typedef enum{
    WALL_NONE,
    WALL_BOTTOM,
    WALL_LEFT, 
    WALL_TOP,
    WALL_RIGHT
} TouchedWall;

typedef enum{
    TILE_CENTER,
    TILE_ABOVE,
    TILE_ABOVE_RIGHT,
    TILE_RIGHT,
    TILE_BELOW_RIGHT,
    TILE_BELOW,
    TILE_BELOW_LEFT,
    TILE_LEFT,
    TILE_ABOVE_LEFT
} TilePositions;



typedef struct {
    u8 tileX;
    u8 tileY;
} PlayerTile;


typedef struct {
    //a list of pointers to the different rects that were touched.
    //the pointers are the same ones passed in, at the same index they came in at,
    //and the calling state should interpret what needs to be done with
    //the information given by knowing these rects were collided with.
    //indexed by TilePositions in grid mode, and in a scene with no tile grid
    //it is a plain slot number into the list that went in. untouched slots are NULL.
    Game_Rect *colidedRect[9];
    //the list of keys that were touched. 
    //can be RED_KEY, GREEN_KEY, BLUE_KEY, PINK_KEY
    //u8 keys[9]; //NULL will indicate the end of list. when reach NULL, break out of reading loop, or stop continuing when reach 9th slot
    u8 screenLeaveDirection; //use the TouchedWall enunm for this. WALL_NONE means it did not leave the level.
    u8 touched_portal; // if the player touched a portal, return it's index in tiles, which should be PORTAL1 or PORTAL2, or EMPTY if none.
    bool touched_somehting; // the caller should only loop through the collided rects if this is true.
} colisionInfo;

//initalizes the player's data. the position and the speed both start at zero,
//so setPosition is what actually puts the player somewhere.
//if standard tile grid,
//then only check colision for tiles in the 9 slots around the player (including center),
//and take each slot's position from its index - see move_player.
//else, the 9 slots are any rects the scene likes, carrying their own positions.
void init_player(bool standard_tile_grid);

//sets the speed X and speed Y from this frame's input, with the speed for the current dificulty applied.
//a held dpad direction wins over the circle pad on that axis, and the pair is normalized so a
//diagonal is not faster than a straight line
void setSpeed(const FrameInput* in, bool hardMode);

void setPosition(float x, float y);

//returns the tile the player's CENTRE is inside of, which is the tile the 3x3 of surrounding
//rects has to be built around. grid mode only: without a tile grid this is always {0, 0}.
//a centre that has left the room is clamped to the edge tile, so it is always a legal index
PlayerTile get_player_tile();

//this returns if the player is colliding with the rect, and which of the rect's faces they came
//in through - the least overlapped one, ie the shortest way back out.
//immedietly returns WALL_NONE if the rect has tile type EMPTY, or if they are exactly flush
//static touchedWall collidingRect(gameRect* curRect);

/*
* moves the player based on their speed, (may need to multiply by delta time to find this using osGetTime)
* and if they collide with any of the surrounding rects, puts them flush against them.
*
* the move is done one axis at a time: step x, clear x, then step y, clear y. that is what makes
* the player slide along a wall instead of sticking to it - a blocked x does not cost them their
* y. each pass runs collidingRect on all nine slots and pushes the player back off the face it
* returns, but ONLY when that face lies on the axis that pass owns; a face on the other axis
* means the shortest way out is not along this one, so the other pass handles it. taking the
* furthest of all of a pass's pushes clears every rect at once, so a corner between two walls
* cannot squeeze the player through either of them.
*
* only the walls have colision, so the they are the rects that are not walls are only reported, not given colision
*
* the rect each slot collides with, (if standard_tile_grid) -
* the slot index IS the position: the rect is rebuilt from get_player_tile plus that slot's
* offset in TilePositions order, so the level only has to carry each tile's type.
*
* the rect each slot collides with, (if !standard_tile_grid) -
* the surrounding rects will be all of the rects in the world, carrying their own world space
* position, and the slot index means nothing but which slot to report them back in.
*
* if the player went off the screen,
* colissionInfo reports the direction off the screen the player went,
* and and the calling state should react accordingly. that is grid mode only: a scene with no
* tile grid has no room to leave and fences itself with ordinary wall rects in the list.
*/
void move_player(Game_Rect *surroundingRects[9], colisionInfo* colInfo);

//get the rect out of the player, for drawing. 
Rect playerRect();


//inverse of init_player
void free_player();