#include "debug.h"
#include <stdarg.h>
#include <stdio.h>

/*
* printf a debug line. Everything is passed straight to printf.
* There is no on-screen console; stderr is routed to the debugger
* (GDB / Azahar log) by consoleDebugInit(debugDevice_SVC).
*/
void printConsole(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr); // flush the buffer
}

char* ToBinary(u32 value, char* buff)
{
    for (int i = 0; i < 32; i++)
        buff[i] = (value & (1u << (31 - i))) ? '1' : '0';
    buff[32] = '\0';
    return buff;
}
