#include "MainMenu.h"

//bottom screen button layout. every button is the same size, stacked top to bottom.
#define BUTTON_X 40
#define BUTTON_Y0 15       //top edge of the first button
#define BUTTON_SPACING 60  //top edge of one button to the top edge of the next
#define BUTTON_W 240
#define BUTTON_H 30
#define HIGHLIGHT_PAD 5    //how far the highlight sticks out past the selected button
#define BUTTON_TEXT_SCALE 0.6f

#define MAIN_MENU_MAX_GLYPHS 128 //room for every string this state parses



static MainMenuState* s; //NULL whenever the state is not active


void MainMenu_Init(GameContext* ctx){
    printConsole("init Main Menu, %u bytes", (unsigned)sizeof(MainMenuState));
    s = calloc(1, sizeof(MainMenuState));

    s->textBuf = C2D_TextBufNew(MAIN_MENU_MAX_GLYPHS);

    //top screen
    MakeText("Maze Maker", &s->titleText, s->textBuf);
    MakeText("By Finnegan McDevitt", &s->nameText, s->textBuf);

    //bottom screen
    for (int i = 0; i < MENU_BUTTON_COUNT; i++) {
        s->rects[i] = (Rect){
            .x = BUTTON_X,
            .y = BUTTON_Y0 + i * BUTTON_SPACING,
            .width = BUTTON_W,
            .height = BUTTON_H,
            .Color = Colors[CLR_DK_GRAY],
        };
        MakeText(MENU_ITEMS[i].label, &s->menu_text[i], s->textBuf);
    }
    s->cur = 0;

    if (ctx->rebuildLevel) {
        printConsole("Coalesce the level here");

        //make funcition to rebuild ctx->builtLvl from ctx->rawLvl and call it here

        ctx->rebuildLevel = false;
    }
}


Game_State MainMenu_Logic(const FrameInput* in, GameContext* ctx){

    if (in->kDown & KEY_L)
    {
        printConsole("L pressed on Main Menu state");
        return STATE_DEBUG; //THIS IS DEBUG AND WILL BE CHANGED LATER
    }

    //move the highlight, wrapping at both ends
    if (in->kDown & (KEY_UP | KEY_CPAD_UP))
    {
        s->cur = (s->cur + MENU_BUTTON_COUNT - 1) % MENU_BUTTON_COUNT;
    }
    if (in->kDown & (KEY_DOWN | KEY_CPAD_DOWN))
    {
        s->cur = (s->cur + 1) % MENU_BUTTON_COUNT;
    }

    //only allow one action per frame. if both happen, then screen touch takes priority
    for (int i = 0; i < MENU_BUTTON_COUNT; i++) {
        if (Rect_Tapped(&s->rects[i], in)) {
            printConsole("player touched \"%s\"", MENU_ITEMS[i].label);
            return MENU_ITEMS[i].target;
        }
    }
    if (in->kDown & KEY_A) {
        printConsole("player pressed A on \"%s\"", MENU_ITEMS[s->cur].label);
        return MENU_ITEMS[s->cur].target;
    }

    return STATE_NONE;
}


void MainMenu_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom){
    //draw the top screen
    C2D_TargetClear(top, Colors[CLR_WHITE]);
    C2D_SceneBegin(top);

    DrawTextCentered(&s->titleText, TOP_SCREEN_WIDTH / 2, 60, 2, 2, Colors[CLR_BLACK]);
    DrawTextCentered(&s->nameText, TOP_SCREEN_WIDTH / 2, 180, 1, 1, Colors[CLR_BLACK]);

    //draw the bottom screen
    C2D_TargetClear(bottom, Colors[CLR_WHITE]);
    C2D_SceneBegin(bottom);

    //highlight: the selected button's rect, grown by the pad, drawn underneath it
    Rect highlight = s->rects[s->cur];
    highlight.x -= HIGHLIGHT_PAD;
    highlight.y -= HIGHLIGHT_PAD;
    highlight.width += 2 * HIGHLIGHT_PAD;
    highlight.height += 2 * HIGHLIGHT_PAD;
    highlight.Color = Colors[CLR_YELLOW];
    DrawRect(&highlight);

    for (int i = 0; i < MENU_BUTTON_COUNT; i++) {
        DrawRect(&s->rects[i]);
        DrawTextInRect(&s->menu_text[i], &s->rects[i], BUTTON_TEXT_SCALE, Colors[CLR_WHITE]);
    }
}


void MainMenu_End(void){
    C2D_TextBufDelete(s->textBuf);
    free(s);
    s = NULL;
}
