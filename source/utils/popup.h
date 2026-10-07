#pragma once

#include <citro2d.h>
#include "datatypes/game_state.h"
#include "datatypes/graphics_types.h" //rect for geometry
#include "utils/input.h"

//this util is used to display and hold the state of the any popup screen
//only one popup can be used at a time
//window - where the box is and how big it is, in the screen's cordinates.
//the caller owns the geometry, because the top and bottom screen have different geometry.
//window count - the amount of screens that the session goes through
//canQuit - if this is true, the there is an no button, and tapping outside dismisses without running end. false is continue only
//wrapWidth - greater than 0 wraps the body text to that many pixes. 0 turns wrapping off. 
//end - the function to run when the session ends. can be null if there isn't one.
//cancel - the function pointer for the function to be called on success. can be null if there isn't one
void popup_init(Rect window, int windowCount, bool canQuit, bool bottomScreen, float wrapWidth, void (*end)(void), void (*cancel)(void));

//raises the window on the first screen 
//text - an array of windowCount C2D_Text, one per screen, and it must stay alive and the text buf must not be cleared until the popup closes.
void popup_start(const C2D_Text* text);

//returns if the window is currently up.
//if it is, then the state should skip it's own logic and only call popup_logic.
bool popup_status();


//one frame of the window. only call when popup_status is true
void popup_logic(const FrameInput* in);

//draw the window, or nothing if none is up, so this needs no guard
//call it last, so it is drawn over everything else.
//has no eye offset of it's own, because on the top screen it is after a SetDepthLayer.
void popup_draw(C3D_RenderTarget* screen);

//inverse of popup_init. safe to call on a popup that was never initalized
void popup_free();