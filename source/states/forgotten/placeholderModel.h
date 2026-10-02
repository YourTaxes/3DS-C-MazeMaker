#pragma once

//this holds the data for a cube. 
//this is so that I can have something to see when testing the scene.
//THIS NEEDS TO BE REMOVED LATER

#define BOX_VERT_COUNT 36 //12 triangles
#define SCENE_PART_COUNT 4 //there are 4 things in the scene
#define SCENE_VERT_COUNT (BOX_VERT_COUNT * SCENE_PART_COUNT)

extern const float SCENE_VERTS[SCENE_VERT_COUNT * 6];