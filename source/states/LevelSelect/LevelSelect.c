#include "states/LevelSelect/LevelSelect.h"
#include "utils/graphics.h"
#include "utils/debug.h"
#include <stdlib.h>


//defines
#define SLOT_COUNT 4
#define BOTTOM_RECT_COUNT 12



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
    lsstate = malloc(sizeof(LevelSelectState));
    //create all 4 level textures here
    for (int i = 0; i < SLOT_COUNT; i++){
        BakeLevelTexture(&lsstate->fullLevelTextures[i], &lsstate->fullLevelTarget[i], &lsstate->fullLevelSubTex[i], ctx->rawLvl, &lsstate->fullLevelImages[i]);
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
    //full texture is drawn at 50, 30
    C2D_TargetClear(top, Colors[CLR_WHITE]);
    C2D_SceneBegin(top);

    C2D_DrawImageAt(lsstate->fullLevelImages[lsstate->curSlotImage], 50, 30, 0, NULL, 1, 1);


}



void LevelSelect_End(void){
    for (int i = 0; i < SLOT_COUNT; i++){
        C3D_RenderTargetDelete(lsstate->fullLevelTarget[i]);
        lsstate->fullLevelTarget[i] = NULL;
        C3D_TexDelete(&lsstate->fullLevelTextures[i]);
    }
    free(lsstate);
    //free all four 
    return;
}

