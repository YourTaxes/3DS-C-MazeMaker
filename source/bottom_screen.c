#include "util.h"
#include "bottom_screen.h"


static C3D_RenderTarget* bottom;
static C2D_Text testing_text_bottom;

void Init_Bottom()
{
	

    MakeText("This is the bottom screen", &font, &testing_text_bottom);
}

void Bottom_Tick(Game_State* state, C3D_RenderTarget* bottom){
    //begin the frame
    C2D_TargetClear(bottom, Colors[CLR_WHITE]);
    C2D_SceneBegin(bottom);

    switch (*state){
        case STATE_DEBUG:
            DrawTextCentered(&testing_text_bottom, BOTTOM_SCREEN_WIDTH/2, BOTTOM_SCREEN_WIDTH/2, 1.0f, 1.0f, Colors[CLR_BLACK]);
        case STATE_MAIN_MENU:
            //show main menu options
        case STATE_MAZE_MAKER:
            //do the actual maze maker logic, largest for this section
        case STATE_MAZE_GAME:
            //show maze gui
        case STATE_SAVE_SELECT:
            //show the selection options
        case STATE_OOB:
            //show the out of bounds warning
        case STATE_you_recieved_the_egg:
            //do the egg room stuff
    }
    //frame is ended once in main, after all screens have drawn
}