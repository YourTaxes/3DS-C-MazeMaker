#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "util.h"

#ifdef TOP
#include "top_screen.h"
#endif

#ifdef BOTTOM
#include "bottom_screen.h"
#endif



int main(int argc, char **argv);

void handleStateSwitch(u32* kDown);