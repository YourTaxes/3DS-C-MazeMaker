#pragma once

#include <stddef.h>
#include <3ds/types.h>

//enough for FormatTime's longest sensible output, plus room to spare
#define TIME_STR_LEN 16

/*
* writes `milliseconds` into buf as mm:ss:mmm. minutes are not clamped to two digits
* buf should be TIME_STR_LEN bytes.
*/
void FormatTime(u32 milliseconds, char* buf, size_t bufSize);
