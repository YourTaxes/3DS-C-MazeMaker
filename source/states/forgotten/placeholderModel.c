#include "placeholderModel.h"

//this is a definition of the verticies of a cube, given the opposite corners of the box
#define BOX(x0, y0, z0, x1, y1, z1) \
    /*bottom -Z */ \
    x0,y0,z0,  0,0,-1,   x0,y1,z0,  0,0,-1,   x1,y1,z0,  0,0,-1, \
    x0,y0,z0,  0,0,-1,   x1,y1,z0,  0,0,-1,   x1,y0,z0,  0,0,-1, \
    /*top +Z */ \
    x0,y0,z1,  0,0, 1,   x1,y0,z1,  0,0, 1,   x1,y1,z1,  0,0, 1, \
    x0,y0,z1,  0,0, 1,   x1,y1,z1,  0,0, 1,   x0,y1,z1,  0,0, 1, \
    /* front, -Y (the edge nearest the camera) */ \
    x0,y0,z0,  0,-1,0,   x1,y0,z0,  0,-1,0,   x1,y0,z1,  0,-1,0, \
    x0,y0,z0,  0,-1,0,   x1,y0,z1,  0,-1,0,   x0,y0,z1,  0,-1,0, \
    /* back, +Y */ \
    x1,y1,z0,  0, 1,0,   x0,y1,z0,  0, 1,0,   x0,y1,z1,  0, 1,0, \
    x1,y1,z0,  0, 1,0,   x0,y1,z1,  0, 1,0,   x1,y1,z1,  0, 1,0, \
    /* left, -X */ \
    x0,y0,z0, -1,0,0,    x0,y0,z1, -1,0,0,    x0,y1,z1, -1,0,0, \
    x0,y0,z0, -1,0,0,    x0,y1,z1, -1,0,0,    x0,y1,z0, -1,0,0, \
    /* right, +X */ \
    x1,y0,z0,  1,0,0,    x1,y1,z0,  1,0,0,    x1,y1,z1,  1,0,0, \
    x1,y0,z0,  1,0,0,    x1,y1,z1,  1,0,0,    x1,y0,z1,  1,0,0


const float SCENE_VERTS[SCENE_VERT_COUNT * 6] = {
    //ground slab, one unit thick, top is flush with the ground.
    BOX(-6.667f, -4.0f, -1.0f, 6.667f, 4.0f, 0.0f),

    //trunk part, will be replaced by a blender model
    BOX(-0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 3.0f),

    //leaves of the tree
    BOX(-2.0f, -2.0f, 0.0f, 2.0f, 2.0f, 2.5f),

    //player, this part may stay and not get remade in blender,
    //because the player will be a cube anyway
    BOX(-0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 1.0f),
};

_Static_assert(sizeof(SCENE_VERTS) == SCENE_VERT_COUNT * 6 * sizeof(float), "SCENE_VERT_COUNT in model.h does not match boxes in model.c");