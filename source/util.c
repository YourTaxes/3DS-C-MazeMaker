#include "util.h"
#include <stdarg.h>
#include <stdio.h>

u32 Colors[11];
C2D_Font font;

/*
* printf to the console at a given line and column (1-based).
* Everything after col is passed straight to printf.
* The cursor-position prefix and the caller's format are joined into
* one string so the whole thing goes out in a single printf call.
*/
void printConsole(int line, int col, const char* fmt, ...)
{
    #ifndef BOTH
        char full[256];
        snprintf(full, sizeof(full), "\x1b[%d;%dH%s", line, col, fmt);

        va_list args;
        va_start(args, fmt);
        vprintf(full, args);
        va_end(args);
    #endif
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
    printConsole(25, 1, "Keyboard Starting");

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