#include "utils/graphics.h"



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

//this const contains all of the information for the static texture.
static const Tex3DS_SubTexture BAKED_LEVEL_SUBTEX = {
    .width = BAKED_LEVEL_IMG_WIDTH,
    .height = BAKED_LEVEL_IMG_HEIGHT,
    .left = 0.0f,
    .top = 1.0f,
    .right = (float)BAKED_LEVEL_IMG_WIDTH / (float)BAKED_LEVEL_TEX_WIDTH,
    .bottom = 1.0f - ((float)BAKED_LEVEL_IMG_HEIGHT / (float)BAKED_LEVEL_IMG_WIDTH),
};


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

void DrawTextCentered(const C2D_Text* text, float centerX, float centerY, float scaleX, float scaleY, u32 color){
    float width, height;
    C2D_TextGetDimensions(text, scaleX, scaleY, &width, &height);
    (void)width; //C2D_AlignCenter takes the center itself, so only the height is needed here

    float drawY = centerY - (height / 2.0f);

    //C2D_AlignCenter centers every line on centerX, so a label with a '\n' in it comes out
    //as stacked centered lines rather than left aligned ones inside a centered block
    C2D_DrawText(text, C2D_WithColor | C2D_AlignCenter, centerX, drawY, 1.0f, scaleX, scaleY, color);
}

void DrawTextInRect(const C2D_Text* text, const Rect* rect, float scale, u32 color){
    DrawTextCentered(text, rect->x + rect->width / 2.0f, rect->y + rect->height / 2.0f, scale, scale, color);
}

bool BakeLevelTexture(C2D_Image *curImage, Raw_Level *cur_level, bool emptyShowNoneImage){
    if (emptyShowNoneImage && cur_level->empty){
        //put the no level guy image into *out

        return true;
    }
    curImage->tex = malloc(sizeof(C3D_Tex));
    if (curImage->tex == NULL) return false;

    //actually init the texture and render target.
    //512 x 256 RGBA8 is half a megabyte of VRAM and there is one of these per save
    //slot, so this really can run out. a failed texture has no backing store, and a
    //render target built over one draws into nowhere, so stop here instead.
    if (!C3D_TexInitVRAM(curImage->tex, BAKED_LEVEL_TEX_WIDTH, BAKED_LEVEL_TEX_HEIGHT, GPU_RGBA8)){
        free(curImage->tex);
        curImage->tex = NULL;
        printConsole("BakeLevelTexture: out of VRAM for a %d x %d texture",
                     BAKED_LEVEL_TEX_WIDTH, BAKED_LEVEL_TEX_HEIGHT);
        return false;
    }

    //use nearest to ensure smearing does not happen
    C3D_TexSetFilter(curImage->tex, GPU_NEAREST, GPU_NEAREST);

    C3D_RenderTarget *curRenderTarget;
    curRenderTarget = C3D_RenderTargetCreateFromTex(curImage->tex, GPU_TEXFACE_2D, 0, -1);
    if (curRenderTarget == NULL){
        printConsole("BakeLevelTexture: could not make a render target for the level texture");
        C3D_TexDelete(curImage->tex);
        free(curImage->tex);
        curImage->tex = NULL;
        return false;
    }


    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(curRenderTarget, Colors[CLR_WHITE]);
    C2D_SceneBegin(curRenderTarget);

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
                        case PORTAL:
                            curRect.Color = Colors[CLR_BLUE];
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

    curImage->subtex = &BAKED_LEVEL_SUBTEX;
    C3D_RenderTargetDelete(curRenderTarget);
    return true;
}

void FreeLevelImage(C2D_Image *img){
    if(img->tex == NULL) return;
    C3D_TexDelete(img->tex); //release the vram
    free(img->tex); //release the tex struct
    *img = (C2D_Image){0};
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