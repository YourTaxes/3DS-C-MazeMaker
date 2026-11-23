#include "MenuNavTest.h"
#include "utils/graphics.h"
#include "utils/debug.h"
#include "menu_navigation/menu_group.h"
#include <stdlib.h>




static MenuEvent on_menu1_button(int index, void* user);
static MenuEvent on_menu2_button(int index, void* user);

typedef enum{
    M1BTN1,
    M1BTN2,
    M1BTN3,
    M1BTN_COUNT
} Menu1Buttons;

typedef enum{
    M2BTN1,
    M2BTN2,
    M2BTN3,
    M2BTN_COUNT
} Menu2Buttons;

static const MenuDef TEST_MENU1[M1BTN_COUNT] = {
    [M1BTN1] = MENU_BTN(M1BTN3, M1BTN2, MENU_OUT_UP, MENU_OUT_DOWN, on_menu1_button),
    [M1BTN2] = MENU_BTN(M1BTN1, M1BTN3, MENU_OUT_UP, MENU_OUT_DOWN, on_menu1_button),
    [M1BTN3] = MENU_BTN(M1BTN2, M1BTN1, MENU_OUT_UP, MENU_OUT_DOWN, on_menu1_button),
};

static const MenuDef TEST_MENU2[M2BTN_COUNT] = {
    [M2BTN1] = MENU_BTN(M2BTN3, M2BTN2, MENU_OUT_UP, MENU_OUT_DOWN, on_menu2_button),
    [M2BTN2] = MENU_BTN(M2BTN1, M2BTN3, MENU_OUT_UP, MENU_OUT_DOWN, on_menu2_button),
    [M2BTN3] = MENU_BTN(M2BTN2, M2BTN1, MENU_OUT_UP, MENU_OUT_DOWN, on_menu2_button),
};


typedef struct {
    C2D_TextBuf textbuf; //holds the button titles

    C2D_Text menu1Text[M1BTN_COUNT];
    C2D_Text menu2Text[M2BTN_COUNT];

    Rect menu1Rects[M1BTN_COUNT];
    Rect menu2Rects[M2BTN_COUNT];

    MenuGroup* menu;
    MenuNav* menu1;
    MenuNav* menu2;
} MenuNavTestState;

static MenuNavTestState* mnsstate;


static MenuEvent on_menu1_button(int index, void* user){
    (void) user;
    printConsole("pressed button %d in menu 1", index + 1);
    return menu_handled();
}


static MenuEvent on_menu2_button(int index, void* user){
    (void) user;
    printConsole("pressed button %d in menu 2", index + 1);
    return menu_handled();
}

void MenuNavTest_Init(GameContext *ctx)
{
    mnsstate = malloc(sizeof(MenuNavTestState));
}

Game_State MenuNavTest_Logic(const FrameInput *in, GameContext *ctx)
{
    if (in->kDown & KEY_B || in->kDown & KEY_ZL){
        return STATE_MAIN_MENU;
    }

    return STATE_NONE;
}

void MenuNavTest_DrawTop(C3D_RenderTarget *target, float eyeOffset)
{
    C2D_TargetClear(target, Colors[CLR_WHITE]);
    C2D_SceneBegin(target);
}

void MenuNavTest_DrawBottom(C3D_RenderTarget *target)
{
    C2D_TargetClear(target, Colors[CLR_WHITE]);
    C2D_SceneBegin(target);
}

void MenuNavTest_End(void)
{
    free(mnsstate);
}
