#pragma once

#include <citro2d.h>
#include <3ds.h>

#define CPAD_MAX_RADIUS 150.0f
#define CPAD_DEADZONE 15.0f

#define TOP_SCREEN_WIDTH 400
#define TOP_SCREEN_HIGHT 240

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
    STATE_DEBUG,

    //non switching status identifiers
    STATE_COUNT, //number of real states above
    STATE_NONE,  //stay in the current state
    STATE_QUIT   //leave the game
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

extern u32 Colors[11];

/*
* populates the colors array
*/
void MakeColors();

/*
* printf a debug line to the debugger (stderr). Everything is passed
* straight to printf; a newline is appended.
*/
void printConsole(const char* fmt, ...);

/*
* everything the player did this frame. filled once per frame by Input_Read
* in main and passed to the current state's _Logic.
*/
typedef struct {
    u32 kDown; //buttons pressed this frame
    u32 kHeld; //buttons currently down
    u32 kUp;   //buttons released this frame
    touchPosition touch; //px, py are 0,0 when the screen is not touched
    float cpadX; //circle pad, -1.0 to 1.0, 0 inside the deadzone
    float cpadY;
} FrameInput;

/*
* scans the hardware and fills in every field of the struct
*/
void Input_Read(FrameInput* in);

/*
* normalies the circle pad's inputs for use with the player controller
* outputs through normX and normY, each in -1.0 to 1.0; both are 0 inside the deadzone
*/
void normalizeCirclePad(circlePosition* cpad, float* normX, float* normY);

/*
* prints the current state of the used inputs
*/
void printInputs(const FrameInput* in);


/*
* Writes the 32-bit binary representation of value into buff (MSB first).
* buff must be at least 33 bytes. Returns buff so it can be used inline.
*/
char* ToBinary(u32 value, char* buff);



/*
* returns true if the user ended the session with the OK button
* returns false if not
* outputs the response through the buff pointer no matter what
*/
bool GetKeyboard(char* buff, int maxlen, const char* hint, SwkbdType type);






