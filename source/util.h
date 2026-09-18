#pragma once

#include <citro2d.h>
#include <3ds.h>

#define CPAD_MAX_RADIUS 150.0f
#define CPAD_DEADZONE 15.0f

#define TOP_SCREEN_WIDTH 400
#define TOP_SCREEN_HIGHT 240

#define TILE_SIZE 16
#define TILES_PER_SCREEN 375

#define BOTTOM_SCREEN_WIDTH 320
#define BOTTOM_SCREEN_HIGHT 240

typedef enum{
    STATE_MAIN_MENU,
    STATE_MAZE_GAME,
    STATE_MAZE_MAKER,
    STATE_SAVE_SELECT,
    STATE_OOB, //out of bounds
    //STATE_there_is_a_man_here,
    //STATE_he_is_behind_the_tree,
    //STATE_he_offers_you_something,
    //STATE_you_reach_out_your_hand,
    STATE_you_recieved_the_egg,
    //STATE_the_man_smiles,
    //STATE_there_is_no_longer_a_man_behind_the_tree,
    STATE_DEBUG
} Game_State;

typedef enum{
    CLR_RED,
    CLR_ORANGE, 
    CLR_YELLOW,
    CLR_GREEN,
    CLR_CYAN,
    CLR_BLUE,
    CLR_LT_GRAY,
    CLR_GRAY,
    CLR_DK_GRAY,
    CLR_BLACK,
    CLR_WHITE
} Color_Names;

typedef struct{
    u32 Color;
    float x;
    float y;
    float z;
    float width;
    float height;
} Rect;

extern u32 Colors[11];

void MakeColors();

extern C2D_Font font;

void MakeFont();

/*
* printf to the console at a given line and column (1-based).
* Everything after col is passed straight to printf.
*/
//void printConsole(int line, int col, const char* fmt, ...);

/*
* normalies the circle pad's inputs for use with the player controller
* outputs through normX and normY
*/
void normalizeCirclePad(circlePosition* cpad, float* normX, float* normY);

/*
* prints the current state of the used inputs
*/
void printInputs(float normX, float normY, u32 kDown, u32 kHeld, u32 kUp, u32 kDownOld, u32 kHeldOld, u32 kUpOld);


/*
* Writes the 32-bit binary representation of value into buff (MSB first).
* buff must be at least 33 bytes. Returns buff so it can be used inline.
*/
char* ToBinary(u32 value, char* buff);

/*
* Creates a C2D Text object and puts it in the buffer provided.
* The string is the source of the text
* The font is the font used in this text string
* the 
*/
void MakeText(char* str, C2D_Text* result);

/*
* returns true if the user ended the session with the OK button
* returns false if not
* outputs the response through the buff pointer no matter what
*/
bool GetKeyboard(char* buff, int maxlen, const char* hint, SwkbdType type);


void DrawTextCentered(C2D_Text* text, float centerX, float centerY, float scaleX, float scaleY, u32 color);

void DrawRect(Rect* rect);