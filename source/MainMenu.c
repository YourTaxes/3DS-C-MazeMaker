#include "MainMenu.h"


//global vars

static Game_State* nextState;

//rects
static Rect* startMazeButton;
static Rect* startMakerButton;
static Rect* lvlSelectButton;
static Rect* quitButton;

//text objects
static C2D_Text* mazeText;
static C2D_Text* makerText;
static C2D_Text* lvlSelectText;
static C2D_Text* quitText;

void MainMenu_Init(Raw_Level* rawLvl, Built_Level* builtLvl, bool rebuildLvl){
    printConsole("init Main Menu");
    nextState = malloc(sizeof(Game_State));
    
    startMazeButton = malloc(sizeof(Rect));
    startMazeButton->x = 40;
    startMazeButton->y = 15;
    startMazeButton->z = 0;
    startMazeButton->width = 240;
    startMazeButton->height = 30;
    startMazeButton->Color = Colors[CLR_DK_GRAY];

    startMakerButton = malloc(sizeof(Rect));
    startMakerButton->x = 40;
    startMakerButton->y = 75;
    startMakerButton->z = 0;
    startMakerButton->height = 30;
    startMakerButton->width = 240;
    startMakerButton->Color = Colors[CLR_DK_GRAY];
    

    lvlSelectButton = malloc(sizeof(Rect));
    lvlSelectButton->x = 40;
    lvlSelectButton->y = 135;
    lvlSelectButton->z = 0;
    lvlSelectButton->height = 30;
    lvlSelectButton->width = 240;
    lvlSelectButton->Color = Colors[CLR_DK_GRAY];

    quitButton = malloc(sizeof(Rect));
    quitButton->x = 40;
    quitButton->y = 195;
    quitButton->z = 0;
    quitButton->height = 30;
    quitButton->width = 240;
    quitButton->Color = Colors[CLR_DK_GRAY];

}


void MainMenu_Logic(u32 kDown, Raw_Level* rawLvl, Built_Level* builtLvl, bool* stateSwitch, bool rebuildLvl){
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
}


void MainMenu_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom){
    //draw the top screen


    //draw the bottom screen
    C2D_TargetClear(bottom, Colors[CLR_WHITE]);
    C2D_SceneBegin(bottom);

    DrawRect(startMazeButton);
    DrawRect(startMakerButton);
    DrawRect(lvlSelectButton);
    DrawRect(quitButton);

}


void MainMenu_End(Game_State* state){
    //safe to call even if MainMenu_Init never ran, and safe to call twice
    if (nextState) *state = *nextState;
    free(nextState);
    free(startMazeButton);
    free(startMakerButton);
    free(lvlSelectButton);
    free(quitButton);
    nextState = NULL;
    startMazeButton = startMakerButton = lvlSelectButton = quitButton = NULL;
}