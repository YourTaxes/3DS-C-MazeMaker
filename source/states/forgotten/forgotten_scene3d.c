#include "states/forgotten/forgotten_scene3d.h"
#include "states/forgotten/forgotten_config.h"
#include "states/forgotten/placeholderModel.h"
#include "game_logic/player.h" //player size
#include "utils/graphics.h"
#include "utils/input.h" //for the 3d scale
#include "utils/debug.h"

#include <citro3d.h>
#include <stdlib.h>
#include <string.h> //for memcpy, which fills the vbo
#include <math.h> //floorf, powf, sinf, cosf, tanf

#include "scene3d_shbin.h" //the generated shader header, made from the pica file


//the model parts

typedef enum {
    PART_FLOOR,
    PART_TRUNK,
    PART_LEAVES,
    PART_PLAYER,
    PART_COUNT
} ScenePartId;

_Static_assert(PART_COUNT == SCENE_PART_COUNT, "parts here and boxes in placeholderModel.c disagree"); 


//the structs that holds the parts of the scene
typedef struct {
    int first;
    int count;
    C3D_Material material;
} ScenePart;

//STARTING PLACEHOLDER VALUES, REPLACE WITH BLENDER MODELS
static const ScenePart PARTS[PART_COUNT] = {
    [PART_FLOOR] = { PART_FLOOR * BOX_VERT_COUNT, BOX_VERT_COUNT, 
        {{0.25f, 0.10f, 0.25f}, {0,0,0}, {0.05f,0.05f,0.05f}, {0.45f, 0.20f, 0.45f}, {0,0,0}}},
    [PART_TRUNK] = {PART_TRUNK * BOX_VERT_COUNT, BOX_VERT_COUNT,
        {{0.08f,0.08f,0.14f}, {0,0,0}, {0.05f, 0.05f, 0.05f}, {0.15f,0.15f,0.25f}, {0,0,0}}},
    [PART_LEAVES] = {PART_LEAVES * BOX_VERT_COUNT, BOX_VERT_COUNT, 
        {{0.35f, 0.05f, 0.12f}, {0,0,0}, {0.05f,0.05f,0.05f}, {0.70f,0.10f,0.20f}, {0,0,0}}},
    [PART_PLAYER] = {PART_PLAYER * BOX_VERT_COUNT, BOX_VERT_COUNT, 
        {{0.22f,0.22f,0.22f}, {0,0,0}, {0.05f, 0.05f, 0.05f}, {0.55f, 0.55f, 0.55f}, {0,0,0}}},
};


















bool scene3d_init(){
    return true;
}


void scene3d_render(float eyeOffset, float sway, const Rect* player){
    //currently do not render
    return;
}

void scene3d_free(){
    //currently do nothing
    return;
}