#include "utils/input.h"
#include "utils/debug.h"
#include <math.h>

void Input_Read(FrameInput* in) {
    hidScanInput();

    in->kDown = hidKeysDown();
    in->kHeld = hidKeysHeld();
    in->kUp = hidKeysUp();

    hidTouchRead(&in->touch);

    circlePosition cpad;
    hidCircleRead(&cpad);
    normalizeCirclePad(&cpad, &in->cpadX, &in->cpadY);
}

void normalizeCirclePad(circlePosition* cpad, float* normX, float* normY) {
    //compute magnatude
    float rawX = (float)cpad->dx;
    float rawY = (float)cpad->dy;
    float magnatude = sqrtf(rawX * rawX + rawY * rawY);

    //inside the deadzone the pad reads as centred
    *normX = 0.0f;
    *normY = 0.0f;

    if (magnatude > CPAD_DEADZONE) {
        float dirX = rawX / magnatude;
        float dirY = rawY / magnatude;

        //clamp the magnatude to the radius and scale to 1.0 to 0.0
        if (magnatude > CPAD_MAX_RADIUS) {
            magnatude = CPAD_MAX_RADIUS;
        }

        float normalizedMagnatude = (magnatude - CPAD_DEADZONE) / (CPAD_MAX_RADIUS - CPAD_DEADZONE);

        //set the actual values 
        *normX = dirX * normalizedMagnatude;
        *normY = dirY * normalizedMagnatude;
    }
}

void printInputs(const FrameInput* in){
    //integers, not %f: see printConsole in debug.h for why
    printConsole("Circle pad position (x100): %d %d", (int)(in->cpadX * 100.0f), (int)(in->cpadY * 100.0f));
    printConsole("Touch: %d %d", in->touch.px, in->touch.py);
    //print all of the button info
    char binBuff[33];
    printConsole("down is %s", ToBinary(in->kDown, binBuff));
    printConsole("held is %s", ToBinary(in->kHeld, binBuff));
    printConsole("up is   %s", ToBinary(in->kUp, binBuff));
}

bool GetKeyboard(char* buff, int maxlen, const char* hint, SwkbdType type) 
{
    printConsole("Keyboard Starting");

	//init keyboard values
	static SwkbdState swkbd;
	//static SwkbdStatusData swkbdStatus;
	//static SwkbdLearningData swkbdLearning;
	SwkbdButton button = SWKBD_BUTTON_NONE;
	
	//init the keyboard info
	swkbdInit(&swkbd, type, 2, maxlen);
	swkbdSetHintText(&swkbd, hint);
	swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
	
	//create the keyboard
	button = swkbdInputText(&swkbd, buff, maxlen);

	//make sure user did not cancel, and then apply change
	if (button != SWKBD_BUTTON_NONE && button != SWKBD_BUTTON_LEFT) {
		return true;
	}
	return false;
}
