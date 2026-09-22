#pragma once

#include <3ds.h>

/*
* printf a debug line to the debugger (stderr). Everything is passed
* straight to printf; a newline is appended.
* avoid %f: newlib's printf mallocs scratch buffers the first time it formats
* a float and never frees them, which shifts the heap numbers we watch on KEY_GPIO14.
*/
void printConsole(const char* fmt, ...);

/*
* Writes the 32-bit binary representation of value into buff (MSB first).
* buff must be at least 33 bytes. Returns buff so it can be used inline.
*/
char* ToBinary(u32 value, char* buff);
