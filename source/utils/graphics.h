#pragma once

#include <citro2d.h>
#include <3ds.h>
#include <stdlib.h>
#include "utils/input.h" //Rect_Tapped reads the frame's touch
#include "datatypes/graphics_types.h" //Rect, Colors
#include "datatypes/level_file.h"
#include "utils/debug.h"



#define TOP_SCREEN_WIDTH 400
#define TOP_SCREEN_HIGHT 240

#define BOTTOM_SCREEN_WIDTH 320
#define BOTTOM_SCREEN_HIGHT 240

//one room's footprint in the baked level texture, in pixels
#define BAKED_ROOM_WIDTH (TILES_HORIZ * BAKED_LEVEL_TILE_SIZE)
#define BAKED_ROOM_HIGHT (TILES_VERT * BAKED_LEVEL_TILE_SIZE)

/*
* loads one ImageSet's t3x out of romfs and hands back both halves of it.
*
* *outLevelSheet is the SHEET, which owns the texture and the subtexture table. *outImage is
* only two pointers into that sheet, so it is valid exactly as long as the sheet is, and it
* must never be C3D_TexDelete'd or freed.
*
* the caller owns *outLevelSheet and has to give it back with C2D_SpriteSheetFree once,
* after the last draw that touches *outImage. LoadImages is that caller for every image the
* game ships; this is exposed so a state that wants something big for its own lifetime only
* can load and free it itself.
*
* both outputs are zeroed at entry, so on false there is nothing to free.
*/
bool getImage(ImageSet curImage, C2D_SpriteSheet* outLevelSheet, C2D_Image* outImage);

/*
* loads every ImageSet, once, into graphics.c's globals. call it after romfsInit, since it
* reads romfs:/gfx, and before any state can draw.
*
* returns false if any image failed, but always tries all of them so one bad file does not
* hide the rest. a failed image is left zeroed, which reads downstream as a null texture and
* simply draws nothing.
*/
bool LoadImages(void);

/*
* frees every sheet LoadImages took. call it after the last state's _End, so nothing is
* still holding a borrowed C2D_Image, and before C2D_Fini. safe to call twice.
*/
void FreeImages(void);

/*
* the image for one ImageSet, borrowed from the sheet graphics.c holds for the whole run.
*
* returns by value because a C2D_Image is only two pointers, and copying one makes it plain
* that the caller is borrowing rather than taking: do not free what comes back. an out of
* range value, or one whose load failed, comes back zeroed and draws nothing.
*/
C2D_Image GetImage(ImageSet curImage);


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

/*
* Draws img centered in the box, shrunk to fit if it is bigger than the box. the scale is
* never taken above 1, so an image smaller than the box is placed rather than blown up.
*
* draws nothing for an image with a null texture, which is what an empty save slot with no
* gui image looks like, so callers do not need their own check.
*/
void DrawImageFitCentered(C2D_Image img, float boxX, float boxY, float boxW, float boxH);

/*
* bakes one level into a 512 x 256 VRAM texture of its own and hands it back in *out, as a
* 300 x 180 image sitting in the texture's top left corner. *target receives the render
* target that texture was drawn through; it stays alive so the editor can repaint single
* tiles with updateBakedTile instead of re-baking the whole level.
*
* this draws, so it opens and closes a frame of its own. that means it must NOT be called
* from inside another C3D_FrameBegin/C3D_FrameEnd pair.
*
* emptyShowNoneImage makes an empty level show a gui image instead of baking a blank one.
* that image is not owned by the preview, so its ownsTex comes back false.
*/
bool BakeLevelTexture(LevelPreview *out, C3D_RenderTarget **target,
                      const Raw_Level *cur_level, bool emptyShowNoneImage);

/*
* gives back whatever a LevelPreview holds and zeroes it, so the same struct is ready to be
* baked into again. safe on a zeroed preview, on one whose bake failed, and to call twice,
* so a caller can loop over every slot without special cases.
*
* only frees a texture this file baked. a preview with ownsTex false is forgotten rather
* than deleted, because something longer lived owns that texture.
*
* does NOT touch the render target that went with it. delete that first, with
* C3D_RenderTargetDelete, or it outlives the texture it draws into.
*/
void FreeLevelImage(LevelPreview *preview);


//update one tile in the baked texture, for the level editor: repaints a single tile in
//place rather than re-baking the whole level. no caller yet.
//*target is the one BakeLevelTexture handed back for that slot
//unlike BakeLevelTexture this opens no frame of its own, so the caller has to be inside a
//C3D_FrameBegin/C3D_FrameEnd pair when it calls this.
void updateBakedTile(int roomX, int roomY, int tileX, int tileY, u32 newColor, C3D_RenderTarget **target);