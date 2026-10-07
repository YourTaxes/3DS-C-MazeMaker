#include "utils/popup.h"
#include "utils/graphics.h" //Rect, colors, drawrect, DrawTextInRect, MakeText, RectTapped
#include "utils/debug.h"
#include <stdlib.h>


//everything for the layout that is not the window's own position and size, which the caller gave.
//buttons are placed relative to the window rather than the screen.
//button sizes are constant, may change in the future to make them relative to th width of the window.
#define POPUP_BTN_W 50.0f
#define POPUP_BTN_H 30.0f
#define POPUP_BTN_GAP 20.0f
#define POPUP_BTN_PAD 5.0f //window's bottom edge up to buttons
#define POPUP_TEXT_PAD 10.0f //window's top edge down to first line
#define POPUP_TEXT_SCALE 0.5f
#define POPUP_BTN_SCALE 0.45f
#define POPUP_GLYPHS 16 //"A: Yes" and "B: No" or "A: Next"

typedef enum {
    P_WINDOW,
    P_YES,
    P_NO,
    P_RECT_COUNT
} PopupRectId;

typedef struct{
    Rect rect[P_RECT_COUNT];
    C2D_TextBuf btnBuf; //just the button captions, the body text belongs to the caller
    C2D_Text btnText[2]; //[0] confirm, [1] cancel (only used when canQuit)
    const C2D_Text* text; //borrowed from caller, one entry per window, NULL means no window is up.
    void (*end)(void); //the function to call when the sequence finishes
    void (*cancel)(void); //the callback that happens when the popup is canceled. can be null if canQuit is false, but has to have something otherwise
    float wrapWidth; //0 means do not wrap
    u8 windowCount;
    u8 cur;
    bool canQuit;
    bool bottomScreen;
} PopupState;

static PopupState* pop; 


void popup_init(Rect window, int windowCount, bool canQuit, bool bottomScreen, float wrapWidth, void (*end)(void), void (*cancel)(void)){
    //I do this so that a second init cannot leak the first one's text buffer.
    //popup_free is safe to do on an already freed popup, so this is safe.
    popup_free(); 


    if (windowCount < 0 || windowCount > 255) {
        printConsole("popup_init: refusing a %d page popup", windowCount);
        return;
    }

    pop = calloc(1, sizeof(PopupState));
    if (pop == NULL){
        printConsole("popup_init: out of memory");
        return;
    }

    pop->btnBuf = C2D_TextBufNew(POPUP_GLYPHS);
    //null check because it's c
    if (pop->btnBuf == NULL){
        printConsole("popup_init: no text buffer for the button captions");
        free(pop);
        pop = NULL;
        return;
    }

    pop->windowCount = (u8)windowCount;
    pop->canQuit = canQuit;
    pop->bottomScreen = bottomScreen;
    pop->wrapWidth = wrapWidth;
    pop->end = end;
    pop->cancel = cancel;

    //the caller's geometry, but with the layer changed back to the base. 
    window.z = LAYER_POPUP;
    pop->rect[P_WINDOW] = window;

    //the buttons sit on the windows's bottom edge.
    const float cx = window.x + window.width / 2.0f;
    const float btnY = window.y + window.height - POPUP_BTN_H - POPUP_BTN_PAD;

    pop->rect[P_YES] = (Rect){
        .x = canQuit ? cx + POPUP_BTN_GAP/2.0f : cx - POPUP_BTN_W/2.0f,
        .y = btnY,
        .z = LAYER_POPUP,
        .width = POPUP_BTN_W,
        .height = POPUP_BTN_H,
        .Color = Colors[CLR_DK_GRAY]
    };
    
    pop->rect[P_NO] = (Rect) {
        .x = cx - POPUP_BTN_GAP/2.0f - POPUP_BTN_W,
        .y = btnY,
        .z = LAYER_POPUP,
        .width = POPUP_BTN_W,
        .height = POPUP_BTN_H,
        .Color = Colors[CLR_DK_GRAY]
    };

    //create the captions
    MakeText(canQuit ? "A: Yes" : "A: Next", &pop->btnText[0], pop->btnBuf);
    if (canQuit) {
        MakeText("B: No", &pop->btnText[1], pop->btnBuf);
    }
}


void popup_start(const C2D_Text* text){
    if (pop == NULL) return;

    //a 0 page sequence means that the sequence is already finished, so it completes instead of doing anything.
    //this also happens if the content that the pages come from fail to load, which can happen, so it prevents a softlock.
    if (pop->windowCount == 0) {
        void (*end)(void) = pop->end;
        printConsole("popup_start: no pages, complete immedietly");
        if (end) end();
        return;
    }

    pop->text = text;
    pop->cur = 0;
}

bool popup_status(){
    //weather or no the text exists is the flag of if the window is up.
    return pop != NULL && pop->text != NULL;
}

void popup_logic(const FrameInput* in)
{
    if (!popup_status()) return;

    //any touchscreen logic is not used when the window is on the top screen
    const bool touch = pop->bottomScreen;

    //increment then compare so the window closes on the press when the last window is already up.
    if ((in->kDown & KEY_A) || (touch && Rect_Tapped(&pop->rect[P_YES], in))){
        if (++pop->cur >= pop->windowCount){
            //close this instance before end(), so that the callback is able to start another popup.
            pop->text = NULL;
            void (*end)(void) = pop->end;
            if (end) end();
        }
        return;
    }

    //b, the no button, or a tap anywhere outside the window.
    if (pop->canQuit && ((in->kDown & KEY_B) || (touch && Rect_Tapped(&pop->rect[P_NO], in)) || (touch && ((in->kDown & KEY_TOUCH) && !Rect_Contains(&pop->rect[P_WINDOW], in->touch.px, in->touch.py))))){
        pop->text = NULL;
        void (*cancel)(void) = pop->cancel;
        if (cancel) cancel();
    }
}


//the allignment of the text is top alligned if the text is wrapped, and centered if the text is not wrapped.
static void DrawBody(const C2D_Text* text)
{
    const Rect* box = &pop->rect[P_WINDOW];

    if (pop->wrapWidth <= 0.0f){
        DrawTextInRect(text, box, POPUP_TEXT_SCALE, Colors[CLR_BLACK]);
        return;
    }
    C2D_DrawText(text, C2D_WithColor | C2D_AlignCenter | C2D_WordWrap, box->x + box->width / 2.0f, box->y + POPUP_TEXT_PAD, box->z, POPUP_TEXT_SCALE, POPUP_TEXT_SCALE, Colors[CLR_BLACK], pop->wrapWidth);
}


void popup_draw(C3D_RenderTarget* screen)
{
    if (!popup_status()) return;

    //this call to c2d scene begin is valid because allows this to be drawn on scenees where the gpu is already on 3d scenes.
    C2D_SceneBegin(screen);

    DrawRect(&pop->rect[P_WINDOW]);
    DrawBody(&pop->text[pop->cur]);

    //the confirm buton draws on both paths, but the no button does not
    DrawRect(&pop->rect[P_YES]);
    DrawTextInRect(&pop->btnText[0], &pop->rect[P_YES], POPUP_BTN_SCALE, Colors[CLR_WHITE]);

    if (pop->canQuit){
        DrawRect(&pop->rect[P_NO]);
        DrawTextInRect(&pop->btnText[1], &pop->rect[P_NO], POPUP_BTN_SCALE, Colors[CLR_WHITE]);
    }
}



void popup_free(){
    if (pop == NULL){
        return;
    }
    //only button buf belongs to this file, so that is the only one freed here
    C2D_TextBufDelete(pop->btnBuf);
    free(pop);
    pop = NULL;
}