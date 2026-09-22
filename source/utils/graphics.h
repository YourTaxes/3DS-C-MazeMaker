#pragma once

#include <citro2d.h>
#include <3ds.h>
#include "utils/input.h" //Rect_Tapped reads the frame's touch

#define TOP_SCREEN_WIDTH 400
#define TOP_SCREEN_HIGHT 240

#define BOTTOM_SCREEN_WIDTH 320
#define BOTTOM_SCREEN_HIGHT 240

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
    CLR_WHITE
} Color_Names;

extern u32 Colors[11];

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

void DrawRect(const Rect* rect);

/*
* is the point inside the rectangle
*/
bool Rect_Contains(const Rect* rect, int px, int py);

/*
* returns true only on the frame the stylus first lands inside the rectangle.
*/
bool Rect_Tapped(const Rect* rect, const FrameInput* in);


/*
* Parses str into result, storing its glyphs in buf. A state owns one buf
* (C2D_TextBufNew in _Init, C2D_TextBufDelete in _End) shared by all its text.
* Text is rendered with the shared system font (same for JPN/USA/EUR/AUS consoles)
*/
void MakeText(const char* str, C2D_Text* result, C2D_TextBuf buf);

/*
* Draws Text to the screen with it's center at the point given.
*/
void DrawTextCentered(const C2D_Text* text, float centerX, float centerY, float scaleX, float scaleY, u32 color);

/*
* Draws Text centered inside the rectangle (e.g. a button's label).
*/
void DrawTextInRect(const C2D_Text* text, const Rect* rect, float scale, u32 color);
