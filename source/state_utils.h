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

/*
* the four functions every state file provides. main keeps one of these per
* Game_State and calls through it, so it never needs to know which state it is on.
*   init  - allocate everything the state needs. runs once, right before its first logic call
*   logic - one frame of input handling. returns the state to switch to, STATE_NONE to stay, or STATE_QUIT
*   draw  - render both screens
*   end   - free everything init allocated. runs once, after the state's last draw
*/
typedef struct {
    void (*init)(GameContext* ctx);
    Game_State (*logic)(const FrameInput* in, GameContext* ctx);
    void (*draw)(C3D_RenderTarget* top, C3D_RenderTarget* bottom);
    void (*end)(void);
} StateFns;


typedef struct{
    u32 Color;
    float x;
    float y;
    float z;
    float width;
    float height;
} Rect;

void init_Rect(Rect** rect, float x, float y, float z, float width, float height, u32 color);

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
* Creates a C2D Text object and puts it in the buffer provided.
* The string is the source of the text
* Text is rendered with the shared system font (same for JPN/USA/EUR/AUS consoles)
*/
void MakeText(const char* str, C2D_Text* result);

/*
* Draws Text to the screen with it's center at the point given. 
*/
void DrawTextCentered(C2D_Text* text, float centerX, float centerY, float scaleX, float scaleY, u32 color);