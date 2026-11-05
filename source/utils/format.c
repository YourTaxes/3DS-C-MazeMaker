#include "utils/format.h"
#include <stdio.h>

void FormatTime(u32 ms, char* buf, size_t bufSize)
{
    //%02ld pads to a minimum of two digits, so minutes past 99 just keep counting
    snprintf(buf, bufSize, "%02ld:%02ld:%03ld", ms / 60000, (ms / 1000) % 60, ms % 1000);
}
