#include "MainMenu.h"

//defines
#define BUTTON_X 40

#define HIGHLIGHT_POS_MAZE 10
#define HIGHLIGHT_POS_MAKER 70
#define HIGHLIGHT_POS_LEVEL 130
#define HIGHLIGHT_POS_QUIT 190

#define MAIN_MENU_MAX_GLYPHS 128 //room for every string this state parses

/*
* everything this state owns while it is active. one allocation in _Init,
* one free in _End, and a single 4 byte pointer while the state is inactive.
*/
typedef struct {
    C2D_TextBuf textBuf; //handle to the glyph storage, its own heap block

    //rects
    Rect startMazeButton;
    Rect startMakerButton;
    Rect lvlSelectButton;
    Rect quitButton;
    Rect selectHighlight;

    HighlightPositions curHighlightPos;

    //text objects
    //top screen
    C2D_Text titleText;
    C2D_Text nameText;
    //bottom screen
    C2D_Text mazeText;
    C2D_Text makerText;
    C2D_Text lvlSelectText;
    C2D_Text quitText;
} MainMenuState;

static MainMenuState* s; //NULL whenever the state is not active


void MainMenu_Init(GameContext* ctx){
    printConsole("init Main Menu, %u bytes", (unsigned)sizeof(MainMenuState));
    s = calloc(1, sizeof(MainMenuState));

    s->curHighlightPos = Highlight_Maze;

    s->selectHighlight  = (Rect){ .x = BUTTON_X - 5, .y = HIGHLIGHT_POS_MAZE, .width = 250, .height = 40, .Color = Colors[CLR_YELLOW] };

    s->startMazeButton  = (Rect){ .x = BUTTON_X, .y = 15,  .width = 240, .height = 30, .Color = Colors[CLR_DK_GRAY] };
    s->startMakerButton = (Rect){ .x = BUTTON_X, .y = 75,  .width = 240, .height = 30, .Color = Colors[CLR_DK_GRAY] };
    s->lvlSelectButton  = (Rect){ .x = BUTTON_X, .y = 135, .width = 240, .height = 30, .Color = Colors[CLR_DK_GRAY] };
    s->quitButton       = (Rect){ .x = BUTTON_X, .y = 195, .width = 240, .height = 30, .Color = Colors[CLR_DK_GRAY] };

    //init text
    s->textBuf = C2D_TextBufNew(MAIN_MENU_MAX_GLYPHS);

    //top screen
    MakeText("Maze Maker", &s->titleText, s->textBuf);
    MakeText("By Finnegan McDevitt", &s->nameText, s->textBuf);

    //bottom screen
    MakeText("Start Maze", &s->mazeText, s->textBuf);
    MakeText("Start Maker", &s->makerText, s->textBuf);
    MakeText("Level Select", &s->lvlSelectText, s->textBuf);
    MakeText("Quit Game", &s->quitText, s->textBuf);

    if (ctx->rebuildLevel) {
        printConsole("Coalesce the level here");

        //make funcition to rebuild ctx->builtLvl from ctx->rawLvl and call it here

        ctx->rebuildLevel = false;
    }
}


static Game_State MazeButtonPressed(void){
    printConsole("Maze button pressed");
    return STATE_MAZE_GAME;
}

static Game_State MakerButtonPressed(void){
    printConsole("Maker button pressed");
    return STATE_MAZE_MAKER;
}

static Game_State LvlButtonPressed(void){
    printConsole("Lvl button pressed");
    return STATE_SAVE_SELECT;
}

static void SetHightlightPos(void){
    switch(s->curHighlightPos){
        case Highlight_Maze:
            s->selectHighlight.y = HIGHLIGHT_POS_MAZE;
            break;
        case Highlight_Maker:
            s->selectHighlight.y = HIGHLIGHT_POS_MAKER;
            break;
        case Highlight_Lvl:
            s->selectHighlight.y = HIGHLIGHT_POS_LEVEL;
            break;
        case Highlight_Quit:
            s->selectHighlight.y = HIGHLIGHT_POS_QUIT;
            break;
    }
}


Game_State MainMenu_Logic(const FrameInput* in, GameContext* ctx){

    if (in->kDown & KEY_L)
    {
        printConsole("L pressed on Main Menu state");
        return STATE_DEBUG; //THIS IS DEBUG AND WILL BE CHANGED LATER
    }

    Game_State next = STATE_NONE;

    //clicking A on button logic, is overrided if player taps on button in same frame
    if (in->kDown & (KEY_UP | KEY_CPAD_UP))
    {
        printConsole("player pressed up or cpad up");
        s->curHighlightPos = (s->curHighlightPos + Highlight_COUNT - 1) % Highlight_COUNT;
        SetHightlightPos();
    }
    if (in->kDown & (KEY_DOWN | KEY_CPAD_DOWN))
    {
        printConsole("player pressed down or cpad down");
        s->curHighlightPos = (s->curHighlightPos + 1) % Highlight_COUNT;
        SetHightlightPos();
    }

    if (in->kDown & KEY_A) {
        switch (s->curHighlightPos){
            case Highlight_Maze:
                next = MazeButtonPressed();
                break;
            case Highlight_Maker:
                next = MakerButtonPressed();
                break;
            case Highlight_Lvl:
                next = LvlButtonPressed();
                break;
            case Highlight_Quit:
                printConsole("player pressed A on quit button");
                return STATE_QUIT;
        };
    }

    // detecting player touches a button, takes priority over clicking A
    if (Rect_Tapped(&s->startMazeButton, in)){
        printConsole("player touched start maze button");
        next = MazeButtonPressed();
    }
    if (Rect_Tapped(&s->startMakerButton, in)){
        printConsole("player touched start maker button");
        next = MakerButtonPressed();
    }
    if (Rect_Tapped(&s->lvlSelectButton, in)){
        printConsole("player touched level select button");
        next = LvlButtonPressed();
    }
    if (Rect_Tapped(&s->quitButton, in)){
        printConsole("player touched quit button");
        return STATE_QUIT;
    }
    return next;
}


void MainMenu_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom){
    //draw the top screen
    C2D_TargetClear(top, Colors[CLR_WHITE]);
    C2D_SceneBegin(top);

    DrawTextCentered(&s->titleText, 200, 60, 2, 2, Colors[CLR_BLACK]);
    DrawTextCentered(&s->nameText, 200, 180, 1, 1, Colors[CLR_BLACK]);

    //draw the bottom screen
    C2D_TargetClear(bottom, Colors[CLR_WHITE]);
    C2D_SceneBegin(bottom);

    DrawRect(&s->selectHighlight);

    DrawRect(&s->startMazeButton);
    DrawRect(&s->startMakerButton);
    DrawRect(&s->lvlSelectButton);
    DrawRect(&s->quitButton);

    DrawTextCentered(&s->mazeText, 160, 30, .6, .6, Colors[CLR_WHITE]);
    DrawTextCentered(&s->makerText, 160, 90, .6, .6, Colors[CLR_WHITE]);
    DrawTextCentered(&s->lvlSelectText, 160, 150, .6, .6, Colors[CLR_WHITE]);
    DrawTextCentered(&s->quitText, 160, 210, .6, .6, Colors[CLR_WHITE]);
}


void MainMenu_End(void){
    C2D_TextBufDelete(s->textBuf);
    free(s);
    s = NULL;
}
