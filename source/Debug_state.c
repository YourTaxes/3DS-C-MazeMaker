#include "Debug_state.h"


//global variables

static C2D_Text* top_text;
static C2D_Text* bottom_text;



Game_State Debug_logic(u32 kDown){
    if (kDown & KEY_L)
    {
        printConsole("L pressed on debug state");
        return STATE_MAIN_MENU;
    }
    return STATE_NONE;
}

void Debug_Init(void){
    printConsole("init debug state");

    top_text = malloc (sizeof(C2D_Text));
    bottom_text = malloc(sizeof(C2D_Text));
    MakeText("This is the top screen", top_text);
    MakeText("This is the bottom screen", bottom_text);
}

void Debug_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom){
    C2D_TargetClear(top, Colors[CLR_WHITE]);
    C2D_SceneBegin(top);

    DrawTextCentered(top_text, TOP_SCREEN_WIDTH/2, TOP_SCREEN_HIGHT/2, 1.0f, 1.0f, Colors[CLR_BLACK]);

    C2D_TargetClear(bottom, Colors[CLR_WHITE]);
    C2D_SceneBegin(bottom);

    DrawTextCentered(bottom_text, BOTTOM_SCREEN_WIDTH/2, BOTTOM_SCREEN_HIGHT/2, 1.0f, 1.0f, Colors[CLR_BLACK]);
}


void Debug_end(void) {
    C2D_TextBufDelete(top_text->buf);
    C2D_TextBufDelete(bottom_text->buf);
    free(top_text);
    free(bottom_text);
    top_text = bottom_text = NULL;
}