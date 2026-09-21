#include "MainMenu.h"

//defines
#define BUTTON_X 40

#define HIGHLIGHT_POS_MAZE 10
#define HIGHLIGHT_POS_MAKER 70
#define HIGHLIGHT_POS_LEVEL 130
#define HIGHLIGHT_POS_QUIT 190

//global vars

//rects
static Rect* startMazeButton;
static Rect* startMakerButton;
static Rect* lvlSelectButton;
static Rect* quitButton;
static Rect* selectHighlight;

static HighlightPositions* curHighlightPos;

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

    curHighlightPos = malloc(sizeof(HighlightPositions));
    *curHighlightPos = Highlight_Maze;
    
    init_Rect(&selectHighlight, BUTTON_X - 5, HIGHLIGHT_POS_MAZE, 0, 250, 40, Colors[CLR_YELLOW]);

    init_Rect(&startMazeButton, BUTTON_X, 15, 0, 240, 30, Colors[CLR_DK_GRAY]);
    // startMazeButton = calloc(1, sizeof(Rect)); //calloc zeros wasTouched
    // startMazeButton->x = 40;
    // startMazeButton->y = 15;
    // startMazeButton->z = 0;
    // startMazeButton->width = 240;
    // startMazeButton->height = 30;
    // startMazeButton->Color = Colors[CLR_DK_GRAY];

    init_Rect(&startMakerButton, BUTTON_X, 75, 0, 240, 30, Colors[CLR_DK_GRAY]);
    // startMakerButton = calloc(1, sizeof(Rect));
    // startMakerButton->x = 40;
    // startMakerButton->y = 75;
    // startMakerButton->z = 0;
    // startMakerButton->height = 30;
    // startMakerButton->width = 240;
    // startMakerButton->Color = Colors[CLR_DK_GRAY];
    
    init_Rect(&lvlSelectButton, BUTTON_X, 135, 0, 240, 30, Colors[CLR_DK_GRAY]);
    // lvlSelectButton = calloc(1, sizeof(Rect));
    // lvlSelectButton->x = 40;
    // lvlSelectButton->y = 135;
    // lvlSelectButton->z = 0;
    // lvlSelectButton->height = 30;
    // lvlSelectButton->width = 240;
    // lvlSelectButton->Color = Colors[CLR_DK_GRAY];

    init_Rect(&quitButton, BUTTON_X, 195, 0, 240, 30, Colors[CLR_DK_GRAY]);
    // quitButton = calloc(1, sizeof(Rect));
    // quitButton->x = 40;
    // quitButton->y = 195;
    // quitButton->z = 0;
    // quitButton->height = 30;
    // quitButton->width = 240;
    // quitButton->Color = Colors[CLR_DK_GRAY];

    //init text
    //top screen
    titleText = malloc(sizeof(C2D_Text));
    nameText = malloc(sizeof(C2D_Text));

    //bottom screen
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


static Game_State MazeButtonPressed(void){
    printConsole("Maze button pressed");
    return STATE_NONE;
}

static Game_State MakerButtonPressed(void){
    printConsole("Maker button pressed");
    return STATE_NONE;
}

static Game_State LvlButtonPressed(void){
    printConsole("Lvl button pressed");
    return STATE_NONE;
}

void SetHightlightPos(){
    switch(*curHighlightPos){
        case Highlight_Maze:
            selectHighlight->y = HIGHLIGHT_POS_MAZE;
            break;
        case Highlight_Maker:
            selectHighlight->y = HIGHLIGHT_POS_MAKER;
            break;
        case Highlight_Lvl:
            selectHighlight->y = HIGHLIGHT_POS_LEVEL;
            break;
        case Highlight_Quit:
            selectHighlight->y = HIGHLIGHT_POS_QUIT;
            break;
    }
}





Game_State MainMenu_Logic(u32 kDown, touchPosition* touch, Raw_Level* rawLvl, Built_Level* builtLvl, bool* stateSwitch, bool* rebuildLvl){

    if (*stateSwitch) {
        MainMenu_Init(rawLvl, builtLvl, rebuildLvl);
        *stateSwitch = false;
    }
    if (kDown & KEY_L)
    {
        printConsole("L pressed on Main Menu state");
        return STATE_DEBUG; //THIS IS DEBUG AND WILL BE CHANGED LATER
    }

    Game_State next = STATE_NONE;

    //clicking A on button logic, is overrided if player taps on button in same frame
    if (kDown & KEY_UP || kDown & KEY_CPAD_UP)
    {
        printConsole("player pressed up or cpad up");
        *curHighlightPos = (*curHighlightPos + Highlight_COUNT - 1) % Highlight_COUNT;
        SetHightlightPos();
    }
    if (kDown & KEY_DOWN || kDown & KEY_CPAD_DOWN)
    {
        printConsole("player pressed down or cpad down");
        *curHighlightPos = (*curHighlightPos + 1) % Highlight_COUNT;
        SetHightlightPos();
    }

    if (kDown & KEY_A) {
        switch (*curHighlightPos){
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
    if (touchingRect(startMazeButton, touch)){
        printConsole("player touched start maze button");
        next = MazeButtonPressed();
    }
    if (touchingRect(startMakerButton, touch)){
        printConsole("player touched start maker button");
        next = MakerButtonPressed();
    }
    if (touchingRect(lvlSelectButton, touch)){
        printConsole("player touched level select button");
        next = LvlButtonPressed();
    }
    if (touchingRect(quitButton, touch)){
        printConsole("player touched quit button");
        return STATE_QUIT;
    }
    return next;
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

    DrawRect(selectHighlight);

    DrawRect(startMazeButton);
    DrawRect(startMakerButton);
    DrawRect(lvlSelectButton);
    DrawRect(quitButton);

    DrawTextCentered(mazeText, 160, 30, .6, .6, Colors[CLR_WHITE]);
    DrawTextCentered(makerText, 160, 90, .6, .6, Colors[CLR_WHITE]);
    DrawTextCentered(lvlSelectText, 160, 150, .6, .6, Colors[CLR_WHITE]);
    DrawTextCentered(quitText, 160, 210, .6, .6, Colors[CLR_WHITE]);

    

}


void MainMenu_End(void){
    //safe to call even if MainMenu_Init never ran, and safe to call twice
    free(startMazeButton);
    free(startMakerButton);
    free(lvlSelectButton);
    free(quitButton);
    free(selectHighlight);
    free(curHighlightPos);


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

    startMazeButton = startMakerButton = lvlSelectButton = quitButton = selectHighlight = NULL;
    titleText = nameText = mazeText = makerText = lvlSelectText = quitText = NULL;
}