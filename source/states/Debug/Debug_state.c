#include "states/Debug/Debug_state.h"
#include "utils/graphics.h"
#include "utils/debug.h"
#include <stdlib.h>

#define TOP_SCREEN_MAX_GLYPHS 32
#define BOTTOM_SCREEN_MAX_GLYPHS 32
#define DEBUG_MAX_GLYPHS TOP_SCREEN_MAX_GLYPHS + BOTTOM_SCREEN_MAX_GLYPHS //room for every string this state parses

/*
* everything this state owns while it is active. one allocation in _Init,
* one free in _End, and a single 4 byte pointer while the state is inactive.
*/
typedef struct {
    C2D_TextBuf textBuf; //handle to the glyph storage, its own heap block
    C2D_Text topText;
    C2D_Text bottomText;
} DebugState;

static DebugState* s; //NULL whenever the state is not active


void Debug_Init(GameContext* ctx){
    printConsole("init debug state, %u bytes", (unsigned)sizeof(DebugState));
    s = calloc(1, sizeof(DebugState));

    s->textBuf = C2D_TextBufNew(DEBUG_MAX_GLYPHS);
    MakeText("This is the top screen", &s->topText, s->textBuf);
    MakeText("This is the bottom screen", &s->bottomText, s->textBuf);
}

Game_State Debug_logic(const FrameInput* in, GameContext* ctx){
    if (in->kDown & KEY_L)
    {
        printConsole("L pressed on debug state");
        return STATE_MAIN_MENU;
    }
    return STATE_NONE;
}

void Debug_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom){
    C2D_TargetClear(top, Colors[CLR_WHITE]);
    C2D_SceneBegin(top);

    DrawTextCentered(&s->topText, TOP_SCREEN_WIDTH/2, TOP_SCREEN_HIGHT/2, 1.0f, 1.0f, Colors[CLR_BLACK]);

    C2D_TargetClear(bottom, Colors[CLR_WHITE]);
    C2D_SceneBegin(bottom);

    DrawTextCentered(&s->bottomText, BOTTOM_SCREEN_WIDTH/2, BOTTOM_SCREEN_HIGHT/2, 1.0f, 1.0f, Colors[CLR_BLACK]);
}

void Debug_end(void) {
    C2D_TextBufDelete(s->textBuf);
    free(s);
    s = NULL;
}
