#include "util.h"
#include <stdarg.h>
#include <stdio.h>

u32 Colors[11];
C2D_Font font;

/*
* printf a debug line. Everything after col is passed straight to printf.
* There is no on-screen console; stderr is routed to the debugger
* (GDB / Azahar log) by consoleDebugInit(debugDevice_SVC), so line/col
* are ignored.
*/
void printConsole(int line, int col, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr); // flush the buffer
}

void normalizeCirclePad(circlePosition* cpad, float* normX, float* normY) {
    //compute magnatude
    float rawX = (float)cpad->dx;
    float rawY = (float)cpad->dy;
    float magnatude = sqrtf(rawX * rawX + rawY * rawY);

    //apply deadzone
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


void printInputs(float normX, float normY, u32 kDown, u32 kHeld, u32 kUp, u32 kDownOld, u32 kHeldOld, u32 kUpOld){
    printConsole(3, 1, "Circle pad position: %.2f %.2f", normX, normY);
		//print all of the button info
		char binBuff[33];
		printConsole(4, 1, "down is 	%s", ToBinary(kDown, binBuff));
		printConsole(5, 1, "held is 	%s", ToBinary(kHeld, binBuff));
		printConsole(6, 1, "up is 		%s", ToBinary(kUp, binBuff));
		printConsole(7, 1, "old down is %s", ToBinary(kDownOld, binBuff));
		printConsole(8, 1, "old held is %s", ToBinary(kHeldOld, binBuff));
		printConsole(9, 1, "old up is 	%s", ToBinary(kUpOld, binBuff));
}

/*
* Writes the 32-bit binary representation of value into buff (MSB first).
* buff must be at least 33 bytes. Returns buff so it can be used inline.
*/
char* ToBinary(u32 value, char* buff)
{
    for (int i = 0; i < 32; i++)
        buff[i] = (value & (1u << (31 - i))) ? '1' : '0';
    buff[32] = '\0';
    return buff;
}


/*
* creates the colors for the color array
*/
void MakeColors(){
    Colors[CLR_RED] = C2D_Color32(255, 0, 0, 255);
    Colors[CLR_ORANGE] = C2D_Color32(255, 200, 0, 255);
    Colors[CLR_YELLOW] = C2D_Color32(255, 255, 0, 255);
    Colors[CLR_GREEN] = C2D_Color32(0, 255, 0, 255);
    Colors[CLR_CYAN] = C2D_Color32(0, 255, 255, 255);
    Colors[CLR_BLUE] = C2D_Color32(0, 0, 255, 255);
    Colors[CLR_LT_GRAY] = C2D_Color32(192, 192, 192, 255);
    Colors[CLR_GRAY] = C2D_Color32(128, 128, 128, 255);
    Colors[CLR_DK_GRAY] = C2D_Color32(64, 64, 64, 255);
    Colors[CLR_BLACK] = C2D_Color32(0, 0, 0, 255);
    Colors[CLR_WHITE] = C2D_Color32(255, 255, 255, 255);
}




//needs complete reworking due to memory leakage
void MakeText(char* str, C2D_Font *font, C2D_Text* result)
{
    C2D_TextBuf buff = C2D_TextBufNew(strlen(str));
	C2D_TextBufClear(buff);
	const char* indicator = C2D_TextFontParse(result, *font, buff, str);
	C2D_TextOptimize(result);
	
	if(!*font)
	{
        printConsole(14, 1, "No Font");
	} else {
        printConsole(14, 1, "Font");
	}
    printConsole(14, 9, "Char Array = %s", str);
    if (indicator == NULL) {
        printConsole(15, 1, "Indicator is Null");
    } else if (*indicator == '\0') {
        printConsole(15, 1, "Indicator is null character");
    } else {
        printConsole(15, 1, "Indicator is %c", *indicator);
    }
	return;
	
}


/*
* returns true if the user ended the session with the OK button
* returns false if not
* outputs the response through the buff pointer no matter what
*/
bool GetKeyboard(char* buff, int maxlen, const char* hint, SwkbdType type) 
{
    printConsole(24, 1, "Keyboard Starting");

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

void MakeFont(){
    font = C2D_FontLoad("romfs:/cbf_std.bcfnt");
	if (!font) {
		printConsole(26, 1, "Font is NULL");
	}
}

void DrawTextCentered(C2D_Text* text, float centerX, float centerY, float scaleX, float scaleY, u32 color){
    float width, height;
    C2D_TextGetDimensions(text, scaleX, scaleY, &width, &height);

    float drawX = centerX - (width / 2.0f);
    float drawY = centerY - (height / 2.0f);

    C2D_DrawText(text, C2D_WithColor, drawX, drawY, 1.0f, scaleX, scaleY, color);
}

void DrawRect(Rect* rect){
    C2D_DrawRectSolid(rect->x, rect->y, rect->z, rect->width, rect->height, rect->Color);
}