#include "MainMenu.h"


//global vars

static Game_State* nextState;

//rects
static Rect* startMazeButton;
static Rect* startMakerButton;
static Rect* lvlSelectButton;
static Rect* quitButton;

//text objects
//top Screen
static C2D_Text* titleText;
static C2D_Text* nameText;

static C2D_Text* mazeText;
static C2D_Text* makerText;
static C2D_Text* lvlSelectText;
static C2D_Text* quitText;


void MainMenu_Init(Raw_Level* rawLvl, Built_Level* builtLvl, bool* rebuildLvl){
    printConsole("init Main Menu");
    nextState = malloc(sizeof(Game_State));
    
    startMazeButton = calloc(1, sizeof(Rect)); //calloc zeros wasTouched
    startMazeButton->x = 40;
    startMazeButton->y = 15;
    startMazeButton->z = 0;
    startMazeButton->width = 240;
    startMazeButton->height = 30;
    startMazeButton->Color = Colors[CLR_DK_GRAY];

    startMakerButton = calloc(1, sizeof(Rect));
    startMakerButton->x = 40;
    startMakerButton->y = 75;
    startMakerButton->z = 0;
    startMakerButton->height = 30;
    startMakerButton->width = 240;
    startMakerButton->Color = Colors[CLR_DK_GRAY];
    

    lvlSelectButton = calloc(1, sizeof(Rect));
    lvlSelectButton->x = 40;
    lvlSelectButton->y = 135;
    lvlSelectButton->z = 0;
    lvlSelectButton->height = 30;
    lvlSelectButton->width = 240;
    lvlSelectButton->Color = Colors[CLR_DK_GRAY];

    quitButton = calloc(1, sizeof(Rect));
    quitButton->x = 40;
    quitButton->y = 195;
    quitButton->z = 0;
    quitButton->height = 30;
    quitButton->width = 240;
    quitButton->Color = Colors[CLR_DK_GRAY];

    //init text
    titleText = malloc(sizeof(C2D_Text));
    nameText = malloc(sizeof(C2D_Text));

    mazeText = malloc(sizeof(C2D_Text));
    makerText = malloc(sizeof(C2D_Text));
    lvlSelectText = malloc(sizeof(C2D_Text));
    quitText = malloc(sizeof(C2D_Text));

    MakeText("Maze Maker", titleText);
    MakeText("By Finnegan McDevitt", nameText);

    MakeText("Start Maze", mazeText);
    MakeText("Start Maker", makerText);
    MakeText("Level Select", lvlSelectText);
    MakeText("Quit Game", quitText);

    if (*rebuildLvl) {
        printConsole("Coalesce the level here");

        //make funcition to rebuild the level and call it here

        *rebuildLvl = false;
    }

}


void MainMenu_Logic(u32 kDown, touchPosition* touch, Raw_Level* rawLvl, Built_Level* builtLvl, bool* stateSwitch, bool* rebuildLvl){
    if (*stateSwitch) {
        MainMenu_Init(rawLvl, builtLvl, rebuildLvl);
        *stateSwitch = false;
    }
    if (kDown & KEY_A)
    {
        printConsole("A pressed on Main Menu state");
        *stateSwitch = true;
        *nextState = STATE_DEBUG; //THIS IS DEBUG AND WILL BE CHANGED LATER
    }
    if (touchingRect(startMazeButton, touch)){
        printConsole("player touched start maze button");
    }
    if (touchingRect(startMakerButton, touch)){
        printConsole("player touched start maker button");
    }
    if (touchingRect(lvlSelectButton, touch)){
        printConsole("player touched level select button");
    }
    if (touchingRect(quitButton, touch)){
        printConsole("player touched quit button");
    }
}


void MainMenu_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom){
    //draw the top screen
    C2D_TargetClear(top, Colors[CLR_WHITE]);
    C2D_SceneBegin(top);

    DrawTextCentered(titleText, 200, 60, 2, 2, Colors[CLR_BLACK]);
    DrawTextCentered(nameText, 200, 180, 1, 1, Colors[CLR_BLACK]);

    //draw the bottom screen
    C2D_TargetClear(bottom, Colors[CLR_WHITE]);
    C2D_SceneBegin(bottom);

    DrawRect(startMazeButton);
    DrawRect(startMakerButton);
    DrawRect(lvlSelectButton);
    DrawRect(quitButton);

    DrawTextCentered(mazeText, 160, 30, .6, .6, Colors[CLR_WHITE]);
    DrawTextCentered(makerText, 160, 90, .6, .6, Colors[CLR_WHITE]);
    DrawTextCentered(lvlSelectText, 160, 150, .6, .6, Colors[CLR_WHITE]);
    DrawTextCentered(quitText, 160, 210, .6, .6, Colors[CLR_WHITE]);

    

}


void MainMenu_End(Game_State* state){
    //safe to call even if MainMenu_Init never ran, and safe to call twice
    if (nextState) *state = *nextState;
    free(nextState);
    free(startMazeButton);
    free(startMakerButton);
    free(lvlSelectButton);
    free(quitButton);

    if (titleText) C2D_TextBufDelete(titleText->buf);
    if (nameText) C2D_TextBufDelete(nameText->buf);

    if (mazeText) C2D_TextBufDelete(mazeText->buf);
    if (makerText) C2D_TextBufDelete(makerText->buf);
    if (lvlSelectText) C2D_TextBufDelete(lvlSelectText->buf);
    if (quitText) C2D_TextBufDelete(quitText->buf);
    
    free(titleText);
    free(nameText);
    free(mazeText);
    free(makerText);
    free(lvlSelectText);
    free(quitText);

    nextState = NULL;
    startMazeButton = startMakerButton = lvlSelectButton = quitButton = NULL;
}