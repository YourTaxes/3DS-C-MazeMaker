#pragma once

#include <3ds.h>

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
