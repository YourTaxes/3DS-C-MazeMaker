#pragma once

#include <citro2d.h>
#include <3ds.h>


enum Color_Names{
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
};

extern u32 Colors[11];

void MakeColors();

extern C2D_Font font;

void MakeFont();

/*
* printf to the console at a given line and column (1-based).
* Everything after col is passed straight to printf.
*/
void printConsole(int line, int col, const char* fmt, ...);

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
void MakeText(char* str, C2D_Font *font, C2D_Text* result);

/*
* returns true if the user ended the session with the OK button
* returns false if not
* outputs the response through the buff pointer no matter what
*/
bool GetKeyboard(char* buff, int maxlen, const char* hint, SwkbdType type);


