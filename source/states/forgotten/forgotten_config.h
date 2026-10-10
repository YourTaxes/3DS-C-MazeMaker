#pragma once 

#include "utils/graphics.h" //used for top screen width

// quick explination for myself later, 
// This is where all of the defines go for the forgotten scene,
// I decided to do this so that all of them could be seen and edited by me in one place,
// because there are a lot of little things that I need to change and tweak to get this scene right,
// and I don't want to go digging every time.

/*
* defines for the player's world, which is done using the 2d player framework =============================
*/

#define WORLD_SEAM_Y ((float)TOP_SCREEN_HIGHT) //240, the y value where the screens meet
#define WORLD_PX_W ((float)TOP_SCREEN_WIDTH) //400
#define WORLD_PX_H (WORLD_SEAM_Y * 2.0f) //480, height of both screens
//the lower screen is 320 wide, and the top is 400, so there is a wall placed on either side of the bottom screen which is 40 px wide.
#define BOT_INSET (((float)TOP_SCREEN_WIDTH - (float)BOTTOM_SCREEN_WIDTH) / 2.0f)
#define WALL_T 20.0f //how thick the bounding walls are. 

//how many 3d world units each top screen pixel is worth. the 400x240 play area is set to be. 13.33*8 units by using this factor. 
#define WORLD_PER_PX (1.0f / 30.0f)


//where everything on the top screen will be, but in 2D pixels ===================================
//these are the values that will be used by the player movement to actually move the player.
#define TREE_PX_X 200.0f
#define TREE_PX_Y 100.0f

//the tree's base footprint, which blocks the player. this rect will be centered on the tree.
//going to change when I finish the model
#define TREE_WALL_W 30.0f
#define TREE_WALL_H 30.0f


//the information about the hitbox that starts the text sequence 

//height and width
#define TRIGGER_W 40.0f
#define TRIGGER_H 25.0f
#define TRIGGER_BACK 20.0f

//where the player spawns, should in the middle x, and 3/4 ths the way down the bottom scren
#define PLAYER_START_X (WORLD_PX_W / 2.0f - (PLAYER_SIZE / 2.0f))
#define PLAYER_START_Y ((7.0f * WORLD_PX_H) / 8.0f - (PLAYER_SIZE / 2.0f))





/*
* 3d camera defines ===============================================================
*/

//height above the ground the camera should be. this entirely depends on the final size of the model, but 8 is a good start
#define CAM_HEIGHT 6.93f //4*cos(30)/(1-sin(30))
//the camera's downward tilt from horizontal. probably should stay between 30 and 60
#define CAM_PITCH_DEG 30.0f

//field of view of the camera
#define CAM_FOV_DEG 60.0f //90 - pitch
#define SCENE_NEAR 1.0f
#define SCENE_FAR 60.0f

//this is the world distance that appears to sit exactly in line with the screen in 3d mode.
//I want the scene to be somewhat inset, so it will be about halfway to the tree, or less
#define CAM_FOCAL CAM_HEIGHT

//where the light is in the world, in world cordinates.
#define LIGHT_X -6.0f
#define LIGHT_Y -8.0f //negative means towards the camera
#define LIGHT_Z 12.0f //well above the floor

//how many bands the toon ramp quantizes to
#define NUM_DIFFUSE_TONES 3
#define NUM_SPECULAR_TONES 2


/*
* The 3D scene's constants ==============================================
*/
//the trunk's height, unknown until I finish the model
//this is how high the leaves are moved up. 
#define TRUNK_H 3.0f

//the leaf's drift constants, determines the speed and radius of the swaying,
//I need to tune this to get this to look like the animation.
#define SWAY_SPEED 0.01f //leaves move this amount of radians per frame. at 0.01f, it should take 10 seconds every rotation.
#define SWAY_RADIUS 0.15f // the size of the circle that the leaves move in.


//the fadeout that happens at the end of the scene, it's speed is determined by how many frames it goes for
#define FADE_FRAMES 150.0f
#define FADE_SPEED (1.0f / FADE_FRAMES)




/*
* The text box ====================================================
*/

//the file path to the dialouge
#define HIS_LINES_PATH "romfs:/his_lines.egg"

//the maximum amount of pages that can be in the dialouge
//current version has 28 lines, but 32 is the closest power of 2 so I am using that
#define HIS_LINES_MAX_PAGES 32

//the maximum file size for the file itself
//again, extended to the closest power of 2
#define HIS_LINES_MAX_BYTES 2048

//the amount of glyphs used in the lines, about the same as the amount of bytes
//char being a byte and all
#define HIS_LINES_BOX_GLYPHS 2048

//the text box geometry ------------------
#define BOX_X 20.0f
#define BOX_Y 130.0f
#define BOX_W 360.0f
#define BOX_H 100.0f
#define BOX_PAD 12.0f //the inset from the window edge to the text on both sides.