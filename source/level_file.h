#pragma once

#include <3ds.h>

#define TILE_SIZE 16 //pixels. 25 x 15 tiles fills the 400 x 240 top screen
#define TILES_VERT 15
#define TILES_HORIZ 25
#define TILES_PER_SCREEN (TILES_VERT * TILES_HORIZ)

#define LEVEL_SLOT_CNT 4
#define LEVEL_NAME_MAX_LEN 32

#define SCREENS_VERT 3
#define SCREENS_HORIZ 3
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
    //holds Tile_Type values. u8 because it's smaller.
    u8 tiles[TILES_VERT][TILES_HORIZ];
} Raw_Screen;

typedef struct{
    Raw_Screen screens [SCREENS_VERT][SCREENS_HORIZ];
    char levelName[LEVEL_NAME_MAX_LEN];
    double bestTime;
    bool empty;
} Raw_Level;

//The structure for the save file.
//this is the the struct that is written to the save file.
//the header lets a later version of the game recognise a save it wrote in a previous version
//check magic first (is this even our file?), then version (is the
//layout what this build expects, or does it need migrating?). bump
//SAVE_VERSION whenever anything below the header changes shape.
#define SAVE_MAGIC   0x4B4D5A4DU //"MZMK" as little endian bytes
#define SAVE_VERSION 1

typedef struct {
    u32 magic;   //always SAVE_MAGIC
    u32 version; //SAVE_VERSION of the build that wrote the file
    Raw_Level Levels[LEVEL_SLOT_CNT];
} Save_File;


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
    int startScreen[2]; //coordinate position of the screen with the start 
} Built_Level;

