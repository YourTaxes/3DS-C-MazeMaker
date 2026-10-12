#include "utils/loader.h"
#include "utils/graphics.h" //has maketext
#include "utils/debug.h" //printconsole
#include <stdio.h> //fopen, fread, fclose, snprintf
#include <stdlib.h> //malloc and free
#include <string.h> //strlen and strchr

//where each image set lives. 
static const char* IMAGE_PATHS[IMAGE_COUNT] = {
    [IMAGE_OHWELLGUY] = "romfs:/gfx/ohwell.t3x",
    [IMAGE_REDKEY] = "romfs:/gfx/RedKey.t3x",
    [IMAGE_GREENKEY] = "romfs:/gfx/GreenKey.t3x",
    [IMAGE_BLUEKEY] = "romfs:/gfx/BlueKey.t3x",
    [IMAGE_PINKKEY] = "romfs:/gfx/PinkKey.t3x",
    [IMAGE_MAKERLOCK] = "romfs:/gfx/makerLock.t3x",
    [IMAGE_SKLUMBO] = "romfs:/gfx/sklumbo.t3x",
};

_Static_assert(IMAGE_COUNT == 7, "every ImageSet value needs a row in image paths");

//the max amount of bytes one .egg file can be
//modifiable for the future
#define EGG_MAX_BYTES 2048

bool Image_Load(ImageSet toLoad, C2D_SpriteSheet* outSheet, C2D_Image* outImage){
    //zero the out sheet and image first
    *outSheet = NULL;
    *outImage = (C2D_Image){0};

    if (toLoad < 0 || toLoad >= IMAGE_COUNT){
        printConsole("Image_Load: %d is not an ImageSet", (int)toLoad);
        return false;
    }

    //load the sprite sheet from the image
    C2D_SpriteSheet sheet = C2D_SpriteSheetLoad(IMAGE_PATHS[toLoad]);
    //a sheet that is loaded but holds nothing would hand back a C2D_Image full of garbage.
    //so instead I return an error and free the sheet
    if (sheet != NULL && C2D_SpriteSheetCount(sheet) == 0){
        C2D_SpriteSheetFree(sheet);
        sheet = NULL;
    }
    if (sheet == NULL){
        printConsole("Image_Load: %s missing or empty", IMAGE_PATHS[toLoad]);
        return false;
    }

    *outSheet = sheet;
    *outImage = C2D_SpriteSheetGetImage(sheet, 0); //one image per t3x, so always 0
    return true;
}


int Egg_Load(const char* path, C2D_Text* out, int maxPages, C2D_TextBuf buf) {
    //open the file
    FILE* f = fopen(path, "rb");

    //if the file is null, then return 0
    if (f == NULL) {
        printConsole("Egg_Load: could not open %s", path);
        return 0;
    }

    //malloc the buffer for the string, 
    //because the .egg is going to be too big to be a stack variable

    char* raw = malloc(EGG_MAX_BYTES + 1);
    if (raw == NULL){
        printConsole("Egg_Load: out of memory for %s", path);
        fclose(f);
        return 0;
    }
    size_t len = fread(raw, 1, EGG_MAX_BYTES, f);
    fclose(f);
    raw[len] = '\0'; //to ensure that the file is always null terminated

    //cut at every newline in place
    int count = 0;
    for (char* line = raw; line < raw + len && count < maxPages;){
        //search for the next newline, and if it exists, set it to the null character
        char* nl = strchr(line, '\n');
        if (nl != NULL){
            *nl = '\0';
        }

        //a file saved on windows leaves a '\r' before each '\n', which would render in the box
        //this is not good.
        //take it out back
        size_t n = strlen(line);
        if (n > 0 && line[n - 1] == '\r'){
            line[n - 1] = '\0';
        }

        //actually make the text for the buffer
        MakeText(line, &out[count++], buf);

        //if this is the last line, then nothing follows it
        if (nl == NULL){
            break;
        }
        
        //advance the for loop
        line = nl + 1;
    }

    //because the text was already made in maketext, the raw buffer is no longer needed
    free(raw);
    
    printConsole("Egg_Load: %s, %d pages, %u bytes", path, count, (unsigned)len);
    return count;
}


//TODO - actually implement this once I make the model so I can test it
bool Model_Load(const char* dir, const char* names, int partCount, Model* out){
    //DELETE THIS LATER
    (void) dir;
    (void) names;
    (void) partCount;
    

    if (out != NULL){
        *out = (Model){0};
    }
    printConsole("Model_Load: not implemented");
    return false;
}


//I know what this will be anyway so I will just put it here
//it just frees everything in the model struct and then the model struct
void Model_Free(Model* m){
    if (m == NULL){
        return;
    }
    //citro3d holds the pointer, and C2D_Prepare does not clear it
    //clear the texture in index 0
    C3D_TexBind(0, NULL);

    for (int i = 0; i < MODEL_MAX_PARTS; i++){
        if (m->part[i].t3x != NULL){
            Tex3DS_TextureFree(m->part[i].t3x); //free the metadata
            C3D_TexDelete(&m->part[i].tex); //free the texture pixels themselves
        }
    }
    //free the vbo if it needs to be
    if (m->vbo != NULL){
        linearFree(m->vbo);
        *m = (Model){0};
    }
}