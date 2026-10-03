#include "player.h"
#include "stdlib.h"
#include "utils/input.h"





//player value struct, pretty self explanitory
typedef struct {
    Rect rect;
    float speedY;
    float speedX;
    bool standard_tile_grid;
} PlayerVals;

static PlayerVals* pVals;


//init function
void init_player(bool standard_tile_grid){
    pVals = malloc(sizeof(PlayerVals));
    pVals->rect.height = PLAYER_SIZE;
    pVals->rect.width = PLAYER_SIZE;
}

//NEED TO NORMALIZE DPAD INPUT LATER
void setSpeed(const FrameInput* in, float speedMult){
    if (in->kHeld & KEY_DLEFT){
        pVals->speedX = -speedMult;
    } else if (in->kHeld & KEY_DRIGHT){
        pVals->speedX = speedMult;
    } else {
        pVals->speedX = in->cpadX * speedMult;
    }

    if (in->kHeld & KEY_DUP){
        pVals->speedY = -speedMult;
    } else if (in->kHeld & KEY_DDOWN){
        pVals->speedY = speedMult;
    } else {
        pVals->speedY = in->cpadY * -speedMult;
    }
}



void setPosition(float x, float y){
    pVals->rect.x = x;
    pVals->rect.y = y;
}


PlayerTile get_player_tile(){
    PlayerTile cur_tile;
    if (pVals->standard_tile_grid){
        cur_tile.tileX = (int)(pVals->rect.x / TILES_HORIZ);
        cur_tile.tileY = (int)(pVals->rect.y / TILES_VERT);
    } else {
        cur_tile.tileX = 0;
        cur_tile.tileY = 0;
    }
    return cur_tile;
}


static TouchedWall collidingRect(Game_Rect* curRect){
    if (curRect->type == EMPTY){
        return WALL_NONE;
    }
    //math here for if the player is touching this rect

    //and then to see what side they touched.


    return WALL_NONE;
}






void move_player(Game_Rect *surroundingRects[9], colisionInfo* colInfo){
    colInfo->touched_portal = 0;
    colInfo->screenLeaveDirection = 0;
    colInfo->touched_somehting = false;
    

    //EVERYTHING IN THIS FUNCTION IS TERRIBLE.
    //DO NOT USE ANY OF THIS
    //REWRITE THIS ASAP
    pVals->rect.x += pVals->speedX;
    pVals->rect.y += pVals->speedY;

    for (int i = 0; i < 9; i++){
        colInfo->colidedRect[i] = NULL;
        if (surroundingRects[i] == NULL){
            continue;
        }
        if (collidingRect(surroundingRects[i]) != WALL_NONE){
            colInfo->colidedRect[i] = surroundingRects[i];
            pVals->rect.x -= pVals->speedX;
            pVals->rect.y -= pVals->speedY;
        } 
    }

    return;
}








Rect playerRect(){
    return pVals->rect;
}




void free_player(){
    free(pVals);
}