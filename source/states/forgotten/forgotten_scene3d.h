#pragma once

#include <citro2d.h>
#include "datatypes/graphics_types.h"

//this inits the 3d componets of the scene on the top screen. 
//this includes the shader, the vertex buffer, toon lighting, and the camera
//returns false if anything failed, in which case nothing is drawn, but this should not happen.

bool scene3d_init();

//this draws the entire scene for one eye,
//and is called inside a frame, after C2D_SceneBegin
//and ends with C2D_Prepare to get things back to how they were for drawing 2d.
//eyeOffset works exactly as main does to draw top.
//sway is the leaf drift phase in radians, advanced by the state, and is the position on a circle that the leaf should be
//player is the player's 2d rect, which will be used to position the 3d cube.
void scene3d_render(float eyeOffset, float sway, const Rect* player);

//the free function for the 3d scene, frees everything
void scene3d_free(void);