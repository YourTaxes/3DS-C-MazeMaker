#pragma once

#include <3ds.h>
#include <citro2d.h>

/*
* the plain drawing types: a rectangle and the color palette.
*/

typedef enum{
    CLR_RED,
    CLR_ORANGE,
    CLR_YELLOW,
    CLR_GREEN,
    CLR_CYAN,
    CLR_BLUE,
    CLR_LT_GRAY,
    CLR_GRAY,
    CLR_DK_GRAY,
    CLR_BLACK,
    CLR_WHITE,
    CLR_PINK
} Color_Names;

extern u32 Colors[12];

/*
* populates the colors array
*/
void MakeColors(void);

typedef struct{
    u32 Color;
    float x;
    float y;
    float z;
    float width;
    float height;
} Rect;

/*
* every picture the game loads out of romfs:/gfx at startup. graphics.c holds one sheet per
* value for the whole run, and any state can ask for one with GetImage.
*
* IMAGE_COUNT is the size of those arrays, so keep it last and never give a value an
* explicit number. adding a value here means adding its path to IMAGE_PATHS in graphics.c,
* which the _Static_assert next to that table will remind you about.
*/
typedef enum{
    IMAGE_OHWELLGUY,
    IMAGE_KEY,
    IMAGE_MAKERKEY,
    IMAGE_MAKERLOCK,
    IMAGE_SKLUMBO,
    IMAGE_COUNT
} ImageSet;

/*
* a baked level preview and who owns the texture behind it.
*
* ownsTex is true for an image BakeLevelTexture rendered itself: the C3D_Tex was malloc'd
* there and its pixels live in VRAM, so FreeLevelImage has to give both back. it is false
* for an image borrowed from something that outlives the preview, such as one of the gui
* images graphics.c holds. that texture is not the preview's to delete.
*/
typedef struct{
    C2D_Image img;
    bool ownsTex; //true: we baked it. false: borrowed, caller needs to free it.
} LevelPreview;
