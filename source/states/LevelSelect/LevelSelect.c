#include "states/LevelSelect/LevelSelect.h"
#include "utils/graphics.h"
#include "utils/debug.h"
#include "datatypes/save_file.h"
#include <stdlib.h>


//defines
#define SLOT_COUNT 4
#define BOTTOM_RECT_COUNT 12

//where the baked level image sits on the top screen.
#define LEVEL_IMG_X ((TOP_SCREEN_WIDTH - BAKED_LEVEL_IMG_WIDTH) / 2)
#define LEVEL_IMG_Y ((TOP_SCREEN_HIGHT - BAKED_LEVEL_IMG_HEIGHT) / 2)



//structs





//everything owned by the LevelSelect
typedef struct{
    C2D_TextBuf textBuf;

    C3D_Tex fullLevelTextures[SLOT_COUNT];
    C3D_RenderTarget *fullLevelTarget[SLOT_COUNT];
    Tex3DS_SubTexture fullLevelSubTex[SLOT_COUNT];
    C2D_Image fullLevelImages[SLOT_COUNT];

    

    //this holds all the rects for the ui for the bottom screen.
    
    Rect bottomRects[BOTTOM_RECT_COUNT];
    
    //the whole save file, read once in _Init and kept for the life of the state so
    //the slot list and the rename, copy and delete actions all work off one copy.
    //it is on the heap because a Save_File is far too big to sit in a stack frame.
    Save_File *savfle;


    //levelSelect specific details
    u8 curAction; //this will hold LoadAction values casted to u8 to save space
    
    
    //This reperesents the current place in the are you sure chain. 
    //when AYS status is currently above 1, then a window will show up that asks "Are you Sure?" (or something adjacent on higher levels) with a yes and no button
    //on any stage, if the player presses no, or presses somewhere where Rect_Tapped(the AYS window background) != true, then AYS is set to 0, canceling the current action.
    //if the player presses inside the AYS window, but not a button, nothing happens.
    //if the player presses inside the yes button, then AYS will advance to the next stage, unless the current stage is the last stage.
    //if the current stage is the last stage, then pressing yes will complete the action in which AYS was asking.
    //the action to complete is stored in the curAction value, as it is unchanged from when AYS starts.
    //the d and circle pads do not affect the standard level select buttons when the window is up, instead it affects the window's buttons.
    u8 AYS_Status;

    //this value holds the most recent hovered or tapped save slot,
    //and it is what is used to choose which level texture to display.
    //0 indexed
    u8 curSlotImage;

} LevelSelectState;

static LevelSelectState* lsstate;




void LevelSelect_Init(GameContext* ctx){
    (void)ctx; //the slots come off the disk here, not from the loaded level

    //calloc, not malloc: _Draw and _End both read the texture and target arrays, and
    //a slot that fails to bake leaves its entry at zero, which both of them treat as
    //"nothing here" rather than as a stale pointer.
    lsstate = calloc(1, sizeof(LevelSelectState));
    if (lsstate == NULL){
        printConsole("LevelSelect_Init: out of memory for the state");
        return;
    }

    lsstate->savfle = malloc(sizeof(Save_File));
    if (lsstate->savfle == NULL || !SaveFile_Read(lsstate->savfle)){
        printConsole("LevelSelect_Init: could not read the save file");
        free(lsstate->savfle);
        lsstate->savfle = NULL;
    }

    //create all 4 level textures here, one per save slot.
    if (lsstate->savfle != NULL){
        for (int i = 0; i < SLOT_COUNT; i++){
            if (!BakeLevelTexture(&lsstate->fullLevelTextures[i], &lsstate->fullLevelTarget[i], &lsstate->fullLevelSubTex[i], &lsstate->savfle->Levels[i], &lsstate->fullLevelImages[i])){
                printConsole("LevelSelect_Init: slot %d did not bake", i);
            }
        }
    }

    lsstate->curSlotImage = 0;

    return;
}



Game_State LevelSelect_Logic(const FrameInput* in, GameContext* ctx){
    //do real frame logic
    
    

    //do state exit logic (only way to leave is to go back to main menu)
    //if player pressed b or touched Main Menu button or clicked on it, return STATE_MAINMENU
    if (in->kDown & KEY_B){
        printConsole("player is going back to main menu from level select");
        return STATE_MAIN_MENU;
    }
    return STATE_NONE;
}




void LevelSelect_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom){
    if (lsstate == NULL) return;

    C2D_TargetClear(top, Colors[CLR_WHITE]);
    C2D_SceneBegin(top);

    //a slot whose bake failed has no texture, and drawing it would follow a null one
    C2D_Image slotImage = lsstate->fullLevelImages[lsstate->curSlotImage];
    if (slotImage.tex != NULL){
        C2D_DrawImageAt(slotImage, LEVEL_IMG_X, LEVEL_IMG_Y, 0, NULL, 1, 1);
    }

    //draw rest of top screen


    C2D_TargetClear(bottom, Colors[CLR_WHITE]);
    C2D_SceneBegin(bottom);
    //draw bottom screen
}



void LevelSelect_End(void){
    if (lsstate == NULL) return;

    //free all four, skipping any slot that never baked
    for (int i = 0; i < SLOT_COUNT; i++){
        if (lsstate->fullLevelTarget[i] == NULL) continue;
        C3D_RenderTargetDelete(lsstate->fullLevelTarget[i]);
        lsstate->fullLevelTarget[i] = NULL;
        C3D_TexDelete(&lsstate->fullLevelTextures[i]);
    }
    free(lsstate->savfle);
    free(lsstate);
    lsstate = NULL;
    return;
}

