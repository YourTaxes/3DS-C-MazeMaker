#include "player.h"
#include "stdlib.h"

#define PLAYER_BASE_SPEED 1
#define PLAYER_HARD_SPEED 1.5



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
}



void setSpeed(float x, float y){
    pVals->speedX = x;
    pVals->speedY = y;
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

    //EVERYTHING IN THIS FUNCTION IS TERRIBLE.
    //DO NOT USE ANY OF THIS
    //REWRITE THIS ASAP
    pVals->rect.x += pVals->speedX;
    pVals->rect.y += pVals->speedY;

    for (int i = 0; i < 9; i++){
        if (collidingRect(surroundingRects[i]) != WALL_NONE){
            colInfo->colidedRect[i] = surroundingRects[i];
            pVals->rect.x -= pVals->speedX;
            pVals->rect.y -= pVals->speedY;
        } else {
            colInfo->colidedRect[i] = NULL;
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