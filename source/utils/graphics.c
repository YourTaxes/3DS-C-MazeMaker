#include "utils/graphics.h"
#include <math.h> //fminf, for fitting an image into a box



_Static_assert(BAKED_LEVEL_IMG_WIDTH  <= BAKED_LEVEL_TEX_WIDTH,  "baked level is wider than its texture");
_Static_assert(BAKED_LEVEL_IMG_HEIGHT <= BAKED_LEVEL_TEX_HEIGHT, "baked level is taller than its texture");

u32 Colors[12];

/*
* creates the colors for the color array
*/
void MakeColors(void){
    Colors[CLR_RED] = C2D_Color32(255, 0, 0, 255);
    Colors[CLR_ORANGE] = C2D_Color32(255, 200, 0, 255);
    Colors[CLR_YELLOW] = C2D_Color32(255, 255, 0, 255);
    Colors[CLR_GREEN] = C2D_Color32(0, 255, 0, 255);
    Colors[CLR_CYAN] = C2D_Color32(0, 255, 255, 255);
    Colors[CLR_BLUE] = C2D_Color32(0, 0, 255, 255);
    Colors[CLR_LT_GRAY] = C2D_Color32(192, 192, 192, 255);
    Colors[CLR_GRAY] = C2D_Color32(128, 128, 128, 255);
    Colors[CLR_DK_GRAY] = C2D_Color32(64, 64, 64, 255);
    Colors[CLR_BLACK] = C2D_Color32(0, 0, 0, 255);
    Colors[CLR_WHITE] = C2D_Color32(255, 255, 255, 255);
    Colors[CLR_PINK] = C2D_Color32(255, 175, 175, 255);
}

//where each ImageSet lives in romfs. designated initializers so the table cannot drift out
//of step with the enum's order
static const char* const IMAGE_PATHS[IMAGE_COUNT] = {
    [IMAGE_OHWELLGUY] = "romfs:/gfx/ohwell.t3x",
    [IMAGE_KEY]       = "romfs:/gfx/key.t3x",
    [IMAGE_MAKERKEY]  = "romfs:/gfx/makerKey.t3x",
    [IMAGE_MAKERLOCK] = "romfs:/gfx/makerLock.t3x",
    [IMAGE_SKLUMBO]   = "romfs:/gfx/sklumbo.t3x",
};
_Static_assert(IMAGE_COUNT == 5, "every ImageSet value needs a row in IMAGE_PATHS");

//the loaded art, filled once by LoadImages and good for the rest of the run. each SHEET
//owns its texture and its subtexture table; the matching C2D_Image is only two pointers
//into that sheet, so whoever borrows one must never C3D_TexDelete or free it.
static C2D_SpriteSheet imageSheets[IMAGE_COUNT];
static C2D_Image       loadedImages[IMAGE_COUNT];

bool getImage(ImageSet curImage, C2D_SpriteSheet* outLevelSheet, C2D_Image* outImage){
    //start from nothing, so a failure leaves behind nothing that looks freeable
    *outLevelSheet = NULL;
    *outImage = (C2D_Image){0};

    if (curImage < 0 || curImage >= IMAGE_COUNT){
        printConsole("getImage: %d is not an ImageSet", (int)curImage);
        return false;
    }

    C2D_SpriteSheet sheet = C2D_SpriteSheetLoad(IMAGE_PATHS[curImage]);
    if (sheet == NULL){
        printConsole("getImage: could not load %s", IMAGE_PATHS[curImage]);
        return false;
    }

    //a sheet with no images would hand back a C2D_Image full of garbage below
    if (C2D_SpriteSheetCount(sheet) == 0){
        printConsole("getImage: %s holds no images", IMAGE_PATHS[curImage]);
        C2D_SpriteSheetFree(sheet);
        return false;
    }

    *outLevelSheet = sheet;
    *outImage = C2D_SpriteSheetGetImage(sheet, 0); //one image per t3x, so always index 0
    return true;
}

bool LoadImages(void){
    bool allLoaded = true;
    for (int i = 0; i < IMAGE_COUNT; i++){
        //keep going past a failure: one missing file should not hide the others, and
        //getImage already zeroed the slot it could not fill
        if (!getImage((ImageSet)i, &imageSheets[i], &loadedImages[i])) allLoaded = false;
    }
    return allLoaded;
}

void FreeImages(void){
    for (int i = 0; i < IMAGE_COUNT; i++){
        if (imageSheets[i] != NULL) C2D_SpriteSheetFree(imageSheets[i]);
        //nulled either way, so a second call does nothing and no stale C2D_Image is left
        //pointing into a sheet that is gone
        imageSheets[i] = NULL;
        loadedImages[i] = (C2D_Image){0};
    }
}

C2D_Image GetImage(ImageSet curImage){
    if (curImage < 0 || curImage >= IMAGE_COUNT){
        printConsole("GetImage: %d is not an ImageSet", (int)curImage);
        return (C2D_Image){0}; //a null texture draws nothing rather than reading off the end
    }
    return loadedImages[curImage];
}

#define BAKED_LEVEL_U(px) ((float)(px) / (float)BAKED_LEVEL_TEX_WIDTH)
#define BAKED_LEVEL_V(px) (1.0f - ((float)(px) / (float)BAKED_LEVEL_TEX_HEIGHT))

//this const contains all of the information for the static texture.
static const Tex3DS_SubTexture BAKED_LEVEL_SUBTEX = {
    .width  = BAKED_LEVEL_IMG_WIDTH,
    .height = BAKED_LEVEL_IMG_HEIGHT,
    .left   = BAKED_LEVEL_U(0),
    .top    = BAKED_LEVEL_V(0),
    .right  = BAKED_LEVEL_U(BAKED_LEVEL_IMG_WIDTH),
    .bottom = BAKED_LEVEL_V(BAKED_LEVEL_IMG_HEIGHT),
};


void SetDepthLayer(float eyeOffset, float depthFactor){
    C2D_ViewReset(); //ViewTranslate multiplies in, so without this the layers would compound
    C2D_ViewTranslate(eyeOffset * depthFactor, 0.0f);
}

void DrawRect(const Rect* rect){
    C2D_DrawRectSolid(rect->x, rect->y, rect->z, rect->width, rect->height, rect->Color);
}

bool Rect_Contains(const Rect* rect, int px, int py){
    return px >= rect->x && px < rect->x + rect->width
        && py >= rect->y && py < rect->y + rect->height;
}

bool Rect_Tapped(const Rect* rect, const FrameInput* in){
    //libctru sets KEY_TOUCH in the key bitmask while the screen is touched,
    //so kDown & KEY_TOUCH is "the touch began this frame"
    return (in->kDown & KEY_TOUCH) && Rect_Contains(rect, in->touch.px, in->touch.py);
}


void MakeText(const char* str, C2D_Text* result, C2D_TextBuf buf)
{
    const char* indicator = C2D_TextParse(result, buf, str);
    C2D_TextOptimize(result);

    if (indicator == NULL) {
        printConsole("MakeText: parse failed for \"%s\"", str);
        *result = (C2D_Text){0}; //draws nothing
    } else if (*indicator != '\0') {
        printConsole("MakeText: buffer full, stopped at '%c' in \"%s\"", *indicator, str);
    }
}

void DrawTextCentered(const C2D_Text* text, float centerX, float centerY, float z, float scaleX, float scaleY, u32 color){
    float width, height;
    C2D_TextGetDimensions(text, scaleX, scaleY, &width, &height);
    (void)width; //C2D_AlignCenter takes the center itself, so only the height is needed here

    float drawY = centerY - (height / 2.0f);

    //C2D_AlignCenter centers every line on centerX, so a label with a '\n' in it comes out
    //as stacked centered lines rather than left aligned ones inside a centered block
    C2D_DrawText(text, C2D_WithColor | C2D_AlignCenter, centerX, drawY, z, scaleX, scaleY, color);
}

void DrawTextInRect(const C2D_Text* text, const Rect* rect, float scale, u32 color){
    //use the same z as the rect
    DrawTextCentered(text, rect->x + rect->width / 2.0f, rect->y + rect->height / 2.0f,
                     rect->z, scale, scale, color);
}

void DrawImageFitCentered(C2D_Image img, float boxX, float boxY, float boxW, float boxH){
    //a slot with no picture leaves a null texture here: an empty level whose gui image did
    //not load, or one whose bake failed. drawing it would follow a null pointer, and every
    //caller wants the rest of the screen to draw anyway.
    if (img.tex == NULL || img.subtex == NULL) return;

    float imgW = img.subtex->width;
    float imgH = img.subtex->height;
    if (imgW <= 0.0f || imgH <= 0.0f) return;

    //one scale for both axes so nothing is stretched, and never above 1: an image that
    //already fits is placed at its own size rather than blown up into a blurry mess.
    float scale = fminf(1.0f, fminf(boxW / imgW, boxH / imgH));

    //C2D_DrawImageAt takes the top left corner, so center by hand off the drawn size
    float drawX = boxX + (boxW - imgW * scale) / 2.0f;
    float drawY = boxY + (boxH - imgH * scale) / 2.0f;

    C2D_DrawImageAt(img, drawX, drawY, 0.0f, NULL, scale, scale);
}

bool BakeLevelTexture(LevelPreview *out, C3D_RenderTarget **target,
                      const Raw_Level *cur_level, bool emptyShowNoneImage){
    //start from nothing, so every path below leaves a preview and a target the caller can
    //read, and nothing a failure leaves behind looks like something to free. slots still need to be freed before this
    *out = (LevelPreview){0};
    *target = NULL;

    if (emptyShowNoneImage && cur_level->empty){
        //borrowed from the sheet this file holds for the whole run, so ownsTex stays false
        //(the zeroing above already set it) and FreeLevelImage forgets this preview instead
        //of deleting a texture that is not ours. *target stays NULL too: nothing was baked,
        //so there is no render target to go with it.
        out->img = GetImage(IMAGE_OHWELLGUY);
        return true;
    }
    out->img.tex = malloc(sizeof(C3D_Tex));
    if (out->img.tex == NULL) return false;

    //actually init the texture and render target.
    //512 x 256 RGBA8 is half a megabyte of VRAM and there is one of these per save
    //slot, so this really can run out. a failed texture has no backing store, and a
    //render target built over one draws into nowhere, so stop here instead.
    if (!C3D_TexInitVRAM(out->img.tex, BAKED_LEVEL_TEX_WIDTH, BAKED_LEVEL_TEX_HEIGHT, GPU_RGBA8)){
        free(out->img.tex);
        *out = (LevelPreview){0};
        printConsole("BakeLevelTexture: out of VRAM for a %d x %d texture",
                     BAKED_LEVEL_TEX_WIDTH, BAKED_LEVEL_TEX_HEIGHT);
        return false;
    }

    //use nearest to ensure smearing does not happen
    C3D_TexSetFilter(out->img.tex, GPU_NEAREST, GPU_NEAREST);

    *target = C3D_RenderTargetCreateFromTex(out->img.tex, GPU_TEXFACE_2D, 0, -1);
    if (*target == NULL){
        printConsole("BakeLevelTexture: could not make a render target for the level texture");
        C3D_TexDelete(out->img.tex);
        free(out->img.tex);
        *out = (LevelPreview){0};
        return false;
    }


    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(*target, Colors[CLR_WHITE]);
    C2D_SceneBegin(*target);

    for (int curScreenY = 0; curScreenY < SCREENS_VERT; curScreenY++){
        for (int curScreenX = 0; curScreenX < SCREENS_HORIZ; curScreenX++){
            //for each screen
            for (int curTileY = 0; curTileY < TILES_VERT; curTileY++){
                for (int curTileX = 0; curTileX < TILES_HORIZ; curTileX++){
                    //for each tile 
                    Rect curRect;
                    curRect.x = (curScreenX * BAKED_ROOM_WIDTH) + (curTileX * BAKED_LEVEL_TILE_SIZE);
                    curRect.y = (curScreenY * BAKED_ROOM_HIGHT) + (curTileY * BAKED_LEVEL_TILE_SIZE);
                    curRect.z = 0.0f;
                    curRect.height = BAKED_LEVEL_TILE_SIZE;
                    curRect.width = BAKED_LEVEL_TILE_SIZE;
                    switch(cur_level->screens[curScreenY][curScreenX].tiles[curTileY][curTileX]){
                        case EMPTY:
                            curRect.Color = Colors[CLR_WHITE];
                            break;
                        case FINISH:
                        case START:
                            curRect.Color = Colors[CLR_LT_GRAY];
                            break;
                        case WALL:
                            curRect.Color = Colors[CLR_YELLOW];
                            break;
                        case RED_WALL:
                        case RED_KEY:
                            curRect.Color = Colors[CLR_RED];
                            break;
                        case GREEN_KEY:
                        case GREEN_WALL:
                            curRect.Color = Colors[CLR_GREEN];
                            break;
                        case BLUE_KEY:
                        case BLUE_WALL:
                            curRect.Color = Colors[CLR_BLUE];
                            break;
                        case PINK_KEY:
                        case PINK_WALL:
                            curRect.Color = Colors[CLR_PINK];
                            break;
                        case PORTAL1:
                            curRect.Color = Colors[CLR_CYAN];
                            break;
                        case PORTAL2:
                            curRect.Color = Colors[CLR_ORANGE];
                            break;
                        default:
                            curRect.Color = Colors[CLR_BLACK];
                            break;
                    };
                    DrawRect(&curRect);
                }
            }
        }
    }
    C2D_Flush();
    C3D_FrameEnd(0);

    //the target stays alive so the editor can repaint single tiles through updateBakedTile.
    //the caller owns it from here and has to delete it before the texture.
    out->img.subtex = &BAKED_LEVEL_SUBTEX;
    out->ownsTex = true; //this call made the texture, so FreeLevelImage has to undo it
    return true;
}

void FreeLevelImage(LevelPreview *preview){
    //only a texture baked here is ours to take apart. a borrowed one belongs to whatever
    //supplied it, and deleting it would pull it out from under everything else using it.
    if (preview->ownsTex && preview->img.tex != NULL){
        C3D_TexDelete(preview->img.tex); //release the vram
        free(preview->img.tex); //release the tex struct
    }
    //zeroed either way, so the slot reads as empty whichever kind of image it held, and so
    //a second call, or a call on a preview that was never filled, does nothing.
    *preview = (LevelPreview){0};
}

void updateBakedTile(int roomX, int roomY, int tileX, int tileY, u32 newColor, C3D_RenderTarget **target){
    C2D_SceneBegin(*target);
    Rect curRect;
    curRect.Color = newColor;
    curRect.x = (roomX * BAKED_ROOM_WIDTH) + (tileX * BAKED_LEVEL_TILE_SIZE);
    curRect.y = (roomY * BAKED_ROOM_HIGHT) + (tileY * BAKED_LEVEL_TILE_SIZE);
    curRect.z = 0.0f;
    curRect.height = BAKED_LEVEL_TILE_SIZE;
    curRect.width = BAKED_LEVEL_TILE_SIZE;
    DrawRect(&curRect);
    C2D_Flush();
}