#include "utils/format.h"
#include <stdio.h>

void FormatTime(double seconds, char* buf, size_t bufSize)
{
    //the double is turned into whole milliseconds here and never handed to printf.
    //newlib mallocs scratch buffers the first time it formats a float and never
    //frees them, which would shift the heap numbers the KEY_GPIO14 readout watches.
    long ms = (seconds > 0.0) ? (long)(seconds * 1000.0 + 0.5) : 0;

    //%02ld pads to a minimum of two digits, so minutes past 99 just keep counting
    snprintf(buf, bufSize, "%02ld:%02ld:%03ld", ms / 60000, (ms / 1000) % 60, ms % 1000);
}
