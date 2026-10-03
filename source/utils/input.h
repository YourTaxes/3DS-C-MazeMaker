#pragma once

#include <3ds.h>

#define CPAD_MAX_RADIUS 150.0f
#define CPAD_DEADZONE 15.0f

/*
* HID pad bits 12 and 13 have no names in libctru. the hardware calls them the
* debug button and GPIO14, and neither is wired up on a retail 3DS, so they are
* free to use as emulator-only debug keys: azahar lists both in its input config
* right under the real buttons. on hardware these never fire, which is the point.
*/
#define KEY_DEBUG  BIT(12)
#define KEY_GPIO14 BIT(13)

/*
* the 3D slider at full tilt, in pixels of parallax per eye. the DEPTH_* factors in
* graphics.h scale this per layer, and DEPTH_BASE spends all of it, so this is the widest
* shift any element gets. 6px each way is 12px of total disparity on a 400px screen.
*/
#define SCALE_3D 6.0f

//under half a pixel both eyes draw the same image, so main skips the second pass
#define MIN_PARALLAX 0.5f

/*
* everything the player did this frame. filled once per frame by Input_Read
* in main and passed to the current state's _Logic.
*/
typedef struct FrameInput{
    u32 kDown; //buttons pressed this frame
    u32 kHeld; //buttons currently down
    u32 kUp;   //buttons released this frame
    touchPosition touch; //px, py are 0,0 when the screen is not touched
    float cpadX; //circle pad, -1.0 to 1.0, 0 inside the deadzone
    float cpadY;
    float screenDepth; //3D slider, in pixels of parallax: 0 to SCALE_3D
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
* returns true if the user ended the session with the OK button
* returns false if not
* outputs the response through the buff pointer no matter what
*/
bool GetKeyboard(char* buff, int maxlen, const char* hint, SwkbdType type);
