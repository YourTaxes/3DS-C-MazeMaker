#pragma once

#include <stddef.h>

//enough for FormatTime's longest sensible output, plus room to spare
#define TIME_STR_LEN 16

/*
* writes `seconds` into buf as mm:ss:mmm. minutes are not clamped to two digits,
* so an hour and a half prints as 90:00:000. negative values are treated as 0.
* buf should be TIME_STR_LEN bytes.
*/
void FormatTime(double seconds, char* buf, size_t bufSize);
