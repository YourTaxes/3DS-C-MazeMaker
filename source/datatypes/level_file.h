#pragma once

#include <3ds.h>
#include "datatypes/graphics_types.h" //Rect

#define TILE_SIZE 20 //pixels. 20 x 12 tiles fills the 400 x 240 top screen
#define TILES_VERT 12
#define TILES_HORIZ 20
#define TILES_PER_SCREEN (TILES_VERT * TILES_HORIZ) //240
//total tile count is 2160
//total level size in tiles is 60 wide by 36 tall

//the size of each tile when made into a texture and put into vram.
//an exact quarter of TILE_SIZE, so the baked level is a clean 1:4 minimap of the
//playfield: whole pixels per tile, and no fractional tile anywhere in the image.
#define BAKED_LEVEL_TILE_SIZE 5
//total level size in pixels is 300 * 180, which is exactly centered on the
//400 x 240 top screen when drawn at (50, 30).
#define BAKED_LEVEL_IMG_WIDTH  (SCREENS_HORIZ * TILES_HORIZ * BAKED_LEVEL_TILE_SIZE)
#define BAKED_LEVEL_IMG_HEIGHT (SCREENS_VERT * TILES_VERT * BAKED_LEVEL_TILE_SIZE)

//the texture the level is baked into. the GPU only takes power of two sizes, so
//this is the smallest pair that holds BAKED_LEVEL_IMG_WIDTH x _HEIGHT. the image
//sits in the top left corner and the rest of the texture goes unused.
#define BAKED_LEVEL_TEX_WIDTH  512
#define BAKED_LEVEL_TEX_HEIGHT 256

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
    PORTAL1,
    PORTAL2, 
    Grandfather
} Tile_Type;

typedef struct{
    //holds Tile_Type values. u8 because it's smaller.
    u8 tiles[TILES_VERT][TILES_HORIZ];
} Raw_Screen;

//int a = sizeof(Raw_Screen);
//int b = sizeof(double);

typedef struct{
    Raw_Screen screens [SCREENS_VERT][SCREENS_HORIZ];
    char levelName[LEVEL_NAME_MAX_LEN];
    //make these values adjacent to make write slot time only one write
    double bestTimeStandard;
    bool standardTimeValid;
    bool empty;
    double bestTimeHard;
    bool hardTimeValid;
    
} Raw_Level;

//int c = sizeof(Raw_Level);

//The structure for the save file.
//this is the the struct that is written to the save file.
//the header lets a later version of the game recognise a save it wrote in a previous version
//check magic first (is this even our file?), then version (is the
//layout what this build expects, or does it need migrating?). bump
//SAVE_VERSION whenever anything below the header changes shape.
#define SAVE_MAGIC   0x4B4D5A4DU //"MZMK" as little endian bytes
#define SAVE_VERSION 3

typedef struct {
    u32 magic;   //always SAVE_MAGIC
    u32 version; //SAVE_VERSION of the build that wrote the file
    Raw_Level Levels[LEVEL_SLOT_CNT];
    u8 lastSlot;
} Save_File;

//int d = sizeof(Save_File);

typedef struct{
    Rect rect;
    Tile_Type type;
} Game_Rect;


typedef struct {
    Game_Rect walls[TILES_VERT][TILES_HORIZ];
    //bool isStart;
    //bool isFinish;
    //bool isPortal;
} Room_Data;

typedef struct {
    u8 screenX;
    u8 screenY;
    u8 tileX;
    u8 tileY;
} UniqueTilePos;

//the structure for the level data.
typedef struct {
    Room_Data Rooms[SCREENS_VERT][SCREENS_HORIZ];
    UniqueTilePos start; //cordinate position of the start tile
    //u8 startScreen[2]; //coordinate position of the screen with the start 
    //u8 startTile[2];
    UniqueTilePos blue_portal1;
    UniqueTilePos blue_portal2; //NEEDS TO BE MALLOCED, AND NEEDS TO BE FREED WHEN OVERWRITTEN
    //is a pointer to the list of portals other than the first one, 
    //only applicable if the player placed more than 2 blue portals.
    //list length should be stored in blue_portal_count
    UniqueTilePos orange_portal1;
    UniqueTilePos orange_portal2; //NEEDS TO BE MALLOCED AT CREATION, AND NEEDS TO BE FREED WHEN OVERWRITTEN
    //is a pointer to the list of portals other than the first one, 
    //only applicable if the player placed more than 2 blue portals.
    //list length should be stored in blue_portal_count
} Built_Level;
