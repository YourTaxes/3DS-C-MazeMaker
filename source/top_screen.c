#include "util.h"
#include "top_screen.h"




static C3D_RenderTarget* top;
static C2D_Text testing_text_top;

void Init_Top()
{
    #ifndef BOTH
	//Initialize console on top screen. Using NULL as the second argument tells the console library to use the internal console structure as current one
	    consoleInit(GFX_BOTTOM, NULL);
    #endif

	//TODO - make second render target that is the right eye, 
	//and shift all of the sprites in the left screen right slightly, 
	//and the right screen left slightly.
	//USE THE THINGS IN gfx.h 
	top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);



    MakeText("This is the top screen", &font, &testing_text_top);

}

void Top_Tick(){
    //begin the frame
    C2D_TargetClear(top, Colors[CLR_WHITE]);
    C2D_SceneBegin(top);

    DrawTextCentered(&testing_text_top, TOP_SCREEN_WIDTH/2, TOP_SCREEN_HIGHT/2, 1.0f, 1.0f, Colors[CLR_BLACK]);
    //frame is ended once in main, after all screens have drawn
}