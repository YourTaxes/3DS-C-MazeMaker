#include "states/forgotten/forgotten.h"
#include "utils/graphics.h"
#include "game_logic/player.h"





//no state struct yet. will be added soon





//init the 3d screen.
//start the music here
void forgotten_Init(GameContext* ctx){

}


Game_State forgotten_Logic(const FrameInput* in, GameContext* ctx){
    if (in->kDown & KEY_R){
        return STATE_MAIN_MENU;
    }




    return STATE_NONE;
}

//nothing is drawn yet, so there is no SetDepthLayer call and eyeOffset goes unspent.
//give each group of elements one, with a DEPTH_* factor, as they are added.
void forgotten_DrawTop(C3D_RenderTarget* target, float eyeOffset){
    C2D_TargetClear(target, Colors[CLR_BLACK]);
    C2D_SceneBegin(target);
}

void forgotten_DrawBottom(C3D_RenderTarget* target){
    C2D_TargetClear(target, Colors[CLR_BLACK]);
    C2D_SceneBegin(target);
}

//tear down the 3d scene
//stop the music.
void forgotten_End(void){

}

