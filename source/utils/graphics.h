#pragma once

#include <citro2d.h>
#include <3ds.h>
#include "utils/input.h" //Rect_Tapped reads the frame's touch
#include "datatypes/graphics_types.h" //Rect, Colors
#include "datatypes/level_file.h"

#define TOP_SCREEN_WIDTH 400
#define TOP_SCREEN_HIGHT 240

#define BOTTOM_SCREEN_WIDTH 320
#define BOTTOM_SCREEN_HIGHT 240

//one room's footprint in the baked level texture, in pixels
#define BAKED_ROOM_WIDTH (TILES_HORIZ * BAKED_LEVEL_TILE_SIZE)
#define BAKED_ROOM_HIGHT (TILES_VERT * BAKED_LEVEL_TILE_SIZE)

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

//bake an entire level into a single texture
//need to free these seperately at end of state
//this draws, so it opens and closes a frame of its own. that means it must NOT be
//called from inside another C3D_FrameBegin/C3D_FrameEnd pair, i.e. not from a
//state's _Draw. one frame per call also keeps each bake's rects under citro2d's
//per frame object limit.
//returns false if the texture or its render target could not be made, in which
//case *levelTarget is NULL and *out is zeroed, and there is nothing to free.
bool BakeLevelTexture(C3D_Tex *level_Texture, C3D_RenderTarget **levelTarget, Tex3DS_SubTexture *levelSubTex, Raw_Level *cur_level, C2D_Image *out);


void updateBakedTile(int roomX, int roomY, int tileX, int tileY, u32 newColor, C3D_RenderTarget **target);