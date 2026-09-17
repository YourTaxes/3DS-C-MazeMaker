#pragma once

#include <3ds.h>

#define TILES_VERT 15
#define TILES_HORIZ 25

#define LEVEL_SLOT_CNT 4
#define LEVEL_NAME_MAX_LEN 32

#define ROOMS_PER_LEVEL 9

typedef enum {
    EMPTY,
    WALL,
    RED_WALL,
    GREEN_WALL,
    BLUE_WALL,
    PINK_WALL,
    RED_KEY,
    GREEN_KEY,
    BLUE_KEY,
    PINK_KEY,
    START,
    FINISH,
    PORTAL
} Tile_Type;

typedef struct{
    u32 Color;
    float x;
    float y;
    float width;
    float height;
    Tile_Type type;
    bool interactable;

} Wall_Rect;


typedef struct {
    Wall_Rect walls[TILES_VERT][TILES_HORIZ];
    bool isStart;
    bool isFinish;
    bool isPortal;

    
} Room_Data;

//the structure for the level data.
typedef struct {
    Room_Data Rooms[ROOMS_PER_LEVEL];
    char levelName[LEVEL_NAME_MAX_LEN];
    double bestTime;
} Level_Data;

//The structure for the save file.
//this is the the struct that is written to the save file
typedef struct {
    Level_Data Levels[LEVEL_SLOT_CNT];
    
} Save_File;