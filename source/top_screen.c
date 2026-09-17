#include "util.h"
#include "top_screen.h"




static C3D_RenderTarget* top;
static C2D_Text testing_text_top;

void Init_Top()
{
	//TODO - make second render target that is the right eye, 
	//and shift all of the sprites in the left screen right slightly, 
	//and the right screen left slightly.
	//USE THE THINGS IN gfx.h 
	top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);



    MakeText("This is the top screen", &font, &testing_text_top);
}

void Top_Tick(Game_State* state){
    //begin the frame
    C2D_TargetClear(top, Colors[CLR_WHITE]);
    C2D_SceneBegin(top);

    switch (*state){
        case STATE_DEBUG:
            DrawTextCentered(&testing_text_top, TOP_SCREEN_WIDTH/2, TOP_SCREEN_HIGHT/2, 1.0f, 1.0f, Colors[CLR_BLACK]);
        case STATE_MAIN_MENU:
            //show the title screen
        case STATE_MAZE_GAME:
            //draw player and do maze logic
        case STATE_MAZE_MAKER:
            //show the controls / instructions for the maze maker.
        case STATE_SAVE_SELECT:
            //show the controls / instructions for the save select
        case STATE_OOB:
            //show the funny guy and play the music
        case STATE_you_recieved_the_egg:
            //well, there is a tree here
    }
    
    //frame is ended once in main, after all screens have drawn
}