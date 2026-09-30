#pragma once

#include <3ds.h>

typedef struct {
    double currentTime;
    int playerSpeed;
    bool hardMode;
    bool restarting;

} PlayerVals;

void init_player();

void tick_player();