#pragma once

#include <stdlib.h>
#include <citro2d.h>
#include <3ds.h>
#include "util.h" //screen and tile size defines live there
#include "level_file.h"

/*
* game-wide data that outlives any one state. owned by main, and every
* state's _Init and _Logic get a pointer to it.
*/
typedef struct {
    Raw_Level* rawLvl;     //the editable grid for the current save slot
    Built_Level* builtLvl; //the playable version compiled from rawLvl
    bool rebuildLevel;     //rawLvl changed, so builtLvl is stale and must be recompiled
} GameContext;


typedef struct{
    u32 Color;
    float x;
    float y;
    float z;
    float width;
    float height;
    bool wasTouched; //was the stylus inside this rect last frame (used by touchingRect)
} Rect;

void init_Rect(Rect** rect, float x, float y, float z, float width, float height, u32 color);

void DrawRect(Rect* rect);

/*
* returns true only on the frame the stylus enters the rectangle (like kDown).
* dragging onto the rect counts as entering, dragging off and back on counts again.
* must be called every frame for the rect so rect->wasTouched stays current.
* rect must be zero initialised (calloc) so wasTouched starts false.
*/
bool touchingRect(Rect* rect, const touchPosition* touch);

/*
* Creates a C2D Text object and puts it in the buffer provided.
* The string is the source of the text
* Text is rendered with the shared system font (same for JPN/USA/EUR/AUS consoles)
*/
void MakeText(const char* str, C2D_Text* result);

/*
* Draws Text to the screen with it's center at the point given. 
*/
void DrawTextCentered(C2D_Text* text, float centerX, float centerY, float scaleX, float scaleY, u32 color);