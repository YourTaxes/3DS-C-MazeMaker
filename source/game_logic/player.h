#pragma once

#include <3ds.h>
#include "datatypes/level_file.h"

//need to figure out good size of the player.
//#define PLAYER_SIZE
//#define PLAYER_BASE_SPEED
//#define PLAYER_HARD_SPEED

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
    Rect rect;
    float speedY;
    float speedX;
    float x;
    float y;
    bool standard_tile_grid;
} PlayerVals;

typedef struct {
    u8 tileX;
    u8 tileY;
} PlayerTile;


typedef struct {
    //a list of pointers to the different rects that were touched.
    //the pointers should be the same ones passed in,
    //and the calling state should interpret what needs to be done with
    //the information given by knowing these rects were collided with
    //indexed by TilePositions. 
    //the loop in movePlayer that uses this is also going to be indexed by TilePositions
    Game_Rect *colidedRect[9]; 
    //the list of keys that were touched. 
    //can be RED_KEY, GREEN_KEY, BLUE_KEY, PINK_KEY
    //u8 keys[9]; //NULL will indicate the end of list. when reach NULL, break out of reading loop, or stop continuing when reach 9th slot
    u8 screenLeaveDirection; //use the TouchedWall enunm for this. WALL_NONE means it did not leave the level.
    u8 touched_portal; // if the player touched a portal, return it's index in tiles, which should be PORTAL1 or PORTAL2, or EMPTY if none.
    bool touched_finish; //if the player touched the finish, means the level complete state should be activated. 
} colisionInfo;

//initalizes the player's data.
//if standard tile grid,
//then only check colision for tiles in the 9 slots around the player (including center)
//else, just 
void init_player(bool standard_tile_grid);

//sets the speed X and speed Y
void setSpeed(float x, float y);

void setPosition(float x, float y);

//returns the current tile that the player is inside of, used to calculate the 
PlayerTile get_player_tile();

//this returns if the player is colliding with the rect.
//immedietly returns false if the rect has tile type EMPTY
//static touchedWall collidingRect(gameRect* curRect);

/*
* moves the player based on their speed, and if they collide with any of the surrounding rects, 
*
* to check for colision, loop through the surrounding Rects, and run collidingRect on them.
* 
* to correct the player's position, (if standard_tile_grid) - 
*
*
* to correct the player's position, (if !standard_tile_grid) - 
* the surrounding rects will be all of the rects in the world.
* loop through each rect to figure out if it is touching any of them. 
* if it is, use the returned touchedWall to determine which axis to nullify.
* 
*
* if the player went off the screen, 
* colissionInfo should report the direction off the screen the player went,
* and and the calling state should react accordingly. 
*/
colisionInfo move_player(Game_Rect *surroundingRects[9]);

//get the rect out of the player, for drawing. 
Rect playerRect();


//inverse of init_player
void free_player();