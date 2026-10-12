#pragma once

#include <citro2d.h> //also pulls in citro3d and tex3ds
#include "datatypes/graphics_types.h" //for image set enum

//all functions in this file are used to load things from romfs into the main game
//this is done mainly to save memory when they are not in use in the case of the text and the model vertecies.
//the only condition for running these is that the filesystem needs to be open, but that is done before the first state.

//load one of ImageSet's images
//outsheet is the sheet which owns the texture and subtexture,
//outimage is the pointers into that sheet, and it valid as long as the sheet is.
//the caller owns the outsheet, but it is reset at the begining of the function.
bool Image_Load(ImageSet toLoad, C2D_SpriteSheet* outSheet, C2D_Image* outImage);


//load the .egg text file, and parse it for it's use in a popup. 
//the text is delimited by newline, and parsed into the given buffer, and returns the page count.
//a page count of 0 means that the page could not be read, 
//which the caller should treat as "there is nothing to say",
//rather than an error, because (if this is used for a popup) a 0 page length popup completes the sequence immedietly.
//out must hold at minimum maxpages entries, 
//and buf must have room for every printing character 
//(probably best idea to have it be the same glyph count as maxpages)
//the parsed text lives in buf, not in out, because for the purposes of a popup it only needs the textbuf.
//blank lines are skipped, and trailing \r is skipped.
int Egg_Load(const char* path, C2D_Text* out, int maxPages, C2D_TextBuf buf);




//definition of the model struct for use in forgotten
#define MODEL_MAX_PARTS 8 //the maximum amount of parts that can be in a 3d scene
#define MODEL_FLOATS_PER_VERT 8 //x, y, z, nx, ny, nz, u, v.

//holds one model that exists in a scene.
typedef struct {
    int first; //the number of the first vertex in the shared vbo
    int count; //the amount of vertexes
    C3D_Tex tex; //this part's uv map
    Tex3DS_Texture t3x; //the subtexture's metahandle, which is null if no texture is loaded.
} ModelPart;

//holds all models that exist in a scene.
//it is an array like this so that they are linearAlloced all at once.
//also holds all of the indexes of the parts for the C3D_DrawArrays to index.
//this struct must be held and kept alive and freed by the caller.
typedef struct {
    void* vbo; //linearAlloc this
    ModelPart part[MODEL_MAX_PARTS];
    int partCount;
} Model;


//TODO - actually implement this, this is currently the plan. will flesh this out when I finish the model and can test it.

//loads the models for a scene (forgotten).
//each part is it's own file, which has nothing but the verticies, 
//so loading is nothing but a check for the size and then read directly into the buffer.
//each part's uv map is an image with the same name as the part.
//returns true on success, and false on fail.
bool Model_Load(const char* dir, const char* names, int partCount, Model* out);

//the inverse of load, and is safe on a zeroed struct, so the load calls it on a partial failure.
//it must run after scene3d_free.
void Model_Free(Model* m);