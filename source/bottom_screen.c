#include "util.h"
#include "bottom_screen.h"


static C3D_RenderTarget* bottom;
static C2D_Text testing_text_bottom;

void Init_Bottom()
{
    #ifndef BOTH
	//Initialize console on top screen. Using NULL as the second argument tells the console library to use the internal console structure as current one
	    consoleInit(GFX_TOP, NULL);
    #endif

	bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    MakeText("This is the bottom screen", &font, &testing_text_bottom);
}

void Bottom_Tick(){
    //begin the frame
    C2D_TargetClear(bottom, Colors[CLR_WHITE]);
    C2D_SceneBegin(bottom);

    DrawTextCentered(&testing_text_bottom, BOTTOM_SCREEN_WIDTH/2, BOTTOM_SCREEN_WIDTH/2, 1.0f, 1.0f, Colors[CLR_BLACK]);
    //frame is ended once in main, after all screens have drawn
}