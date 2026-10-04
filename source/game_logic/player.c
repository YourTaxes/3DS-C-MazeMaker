#include "player.h"
#include "utils/input.h" //FrameInput and the KEY_* bits, for setSpeed
#include <math.h>        //sqrtf for the dpad diagonal, fminf/fmaxf for the wall snap
#include <stdlib.h>

/*
* the two axes. these index PlayerVals.speed, TILE_OFF and AXIS_FACES, which is what lets one
* function resolve either axis instead of there being a near duplicate of it per axis.
*/
#define AXIS_X 0
#define AXIS_Y 1

//the walls are the only tiles that stop the player, and Tile_Type keeps them in one run,
//so solidity is a range check rather than a five way comparison. this keeps that true.
_Static_assert(PINK_WALL - WALL == 4, "the solid tile types must stay contiguous in Tile_Type");


//player value struct, pretty self explanitory
typedef struct {
    Rect rect;
    float speed[2]; //indexed by AXIS_*
    bool standard_tile_grid;
} PlayerVals;

static PlayerVals* pVals;

//one axis of a rect: where it starts along that axis, and how far it runs
typedef struct {
    float lo;
    float size;
} Span;

/*
* which tile each TilePositions slot sits in, as an offset in TILES from the player's own tile.
* this table is the whole of what "the slot index is the position" means in grid mode.
*/
static const int TILE_OFF[9][2] = {
    [TILE_CENTER]      = { 0,  0}, [TILE_ABOVE]       = { 0, -1},
    [TILE_ABOVE_RIGHT] = { 1, -1}, [TILE_RIGHT]       = { 1,  0},
    [TILE_BELOW_RIGHT] = { 1,  1}, [TILE_BELOW]       = { 0,  1},
    [TILE_BELOW_LEFT]  = {-1,  1}, [TILE_LEFT]        = {-1,  0},
    [TILE_ABOVE_LEFT]  = {-1, -1},
};

/*
* the two faces of a wall that lie on each axis: [axis][0] is the near face (the player is on the
* low side of the wall and gets pushed back below it), [axis][1] the far face. a face on the OTHER
* axis means the shortest way out of that rect is not along this one, so this pass leaves it be
* and the other pass deals with it.
*/
static const TouchedWall AXIS_FACES[2][2] = {
    [AXIS_X] = {WALL_LEFT, WALL_RIGHT},
    [AXIS_Y] = {WALL_TOP,  WALL_BOTTOM},
};


//init function
void init_player(bool standard_tile_grid){
    //calloc, not malloc: this is the only thing that zeroes the position and the speed, and
    //every other function in here reads them without asking whether they were ever set
    pVals = calloc(1, sizeof(PlayerVals));
    pVals->rect.height = PLAYER_SIZE;
    pVals->rect.width = PLAYER_SIZE;
    pVals->standard_tile_grid = standard_tile_grid;
}


void setSpeed(const FrameInput* in, bool hardMode){
    //a held dpad direction overrides the circle pad on that axis. cpadY is negated: the pad
    //reads up as positive, the screen counts y downward
    float x = (in->kHeld & KEY_DLEFT) ? -1.0f : (in->kHeld & KEY_DRIGHT) ? 1.0f :  in->cpadX;
    float y = (in->kHeld & KEY_DUP)   ? -1.0f : (in->kHeld & KEY_DDOWN)  ? 1.0f : -in->cpadY;

    //normalizeCirclePad already keeps the pad inside the unit circle, so this only ever trims a
    //dpad diagonal, which would otherwise walk at 1.41x the speed of a straight line
    float mag = sqrtf(x * x + y * y);
    if (mag > 1.0f){
        x /= mag;
        y /= mag;
    }
    x *= !pVals->standard_tile_grid ? PLAYER_ABNORMAL_SPEED : hardMode ? PLAYER_HARD_SPEED : PLAYER_BASE_SPEED;
    y *= !pVals->standard_tile_grid ? PLAYER_ABNORMAL_SPEED : hardMode ?  PLAYER_HARD_SPEED : PLAYER_BASE_SPEED;
    pVals->speed[AXIS_X] = x;
    pVals->speed[AXIS_Y] = y;
}


void setPosition(float x, float y){
    pVals->rect.x = x;
    pVals->rect.y = y;
}


PlayerTile get_player_tile(){
    if (!pVals->standard_tile_grid){
        return (PlayerTile){0, 0};
    }
    /*
    * the CENTRE of the player, not their corner. the player is exactly one tile across, so a
    * centre in tile t means they cover t-1..t, and after a step of under half a tile at most
    * t-1..t+1 - exactly the 3x3 the caller builds from this. the corner would instead leave a
    * reachable tile outside that list, which is a hole to walk through.
    */
    int tx = (int)((pVals->rect.x + pVals->rect.width  * 0.5f) / TILE_SIZE);
    int ty = (int)((pVals->rect.y + pVals->rect.height * 0.5f) / TILE_SIZE);
    //a centre past the room edge means a room change is pending, and these are u8 that the
    //caller indexes walls[][] with. clamp so they stay legal for the frame it takes the caller
    //to see screenLeaveDirection and move the player into the next room
    tx = tx < 0 ? 0 : tx > TILES_HORIZ - 1 ? TILES_HORIZ - 1 : tx;
    ty = ty < 0 ? 0 : ty > TILES_VERT  - 1 ? TILES_VERT  - 1 : ty;
    return (PlayerTile){(u8)tx, (u8)ty};
}


/*
* the rect the player actually collides with, for one slot of the surrounding list.
*
* grid mode: the slot index IS the position, so the rect is built from base plus that slot's
* offset and the level's Game_Rect only has to carry a type. a slot outside the room comes out at
* a negative or past the edge coordinate, which is correct in room local space - so a caller that
* wants to can hand over the neighbouring room's edge tiles and they land in the right place.
* otherwise: the rect is already in world space and the slot index means nothing.
*
* base is the player's tile from BEFORE this frame's step, which is the tile the caller built the
* list around. re-reading it mid move would slide the whole neighbourhood sideways the moment the
* player crossed a tile line.
*/
static Game_Rect slotRect(int slot, const Game_Rect* src, PlayerTile base){
    if (!pVals->standard_tile_grid){
        return *src;
    }
    return (Game_Rect){
        .rect = {.x = (base.tileX + TILE_OFF[slot][AXIS_X]) * TILE_SIZE,
                 .y = (base.tileY + TILE_OFF[slot][AXIS_Y]) * TILE_SIZE,
                 .width = TILE_SIZE, .height = TILE_SIZE},
        .type = src->type};
}


/*
* is the player overlapping this rect, and if so which of its faces did they come in through -
* the one with the least overlap, which is the shortest way back out. that face is the answer to
* "where can the player not move to": moveAxis pushes them back off it.
*
* the test is strict, so a player resting flush against a wall is NOT touching it. that is what
* lets them walk along a wall without it interfering with their other axis, and it is why
* snapping flush is a resting state rather than a push that repeats every frame.
*/
static TouchedWall collidingRect(Game_Rect* curRect){
    if (curRect->type == EMPTY){
        return WALL_NONE;
    }
    Rect* p = &pVals->rect;
    Rect* w = &curRect->rect;
    //how deep the player is past each face of the wall. all four positive means they overlap;
    //any one at or below zero means there is a gap, or they are exactly flush
    float left   = p->x + p->width  - w->x;
    float right  = w->x + w->width  - p->x;
    float top    = p->y + p->height - w->y;
    float bottom = w->y + w->height - p->y;
    if (left <= 0.0f || right <= 0.0f || top <= 0.0f || bottom <= 0.0f){
        return WALL_NONE;
    }
    float least = fminf(fminf(left, right), fminf(top, bottom));
    return least == left  ? WALL_LEFT
         : least == right ? WALL_RIGHT
         : least == top   ? WALL_TOP
         : WALL_BOTTOM;
}


static Span axisSpan(const Rect* r, int axis){
    return axis == AXIS_X ? (Span){r->x, r->width} : (Span){r->y, r->height};
}


/*
* one axis of the move: step, then put the player flush against everything solid they stepped
* into, and report everything they touched either way.
*
* doing the axes one at a time is what makes a wall slide instead of stick: a blocked x does not
* cost the player their y, and the y pass steps into a floor only after x is already settled.
* taking the furthest of all the snaps clears every rect in one pass, so a corner between two
* walls cannot squeeze the player through either of them.
*
* there is no check for a zero step. the face, not the direction of travel, says which way out is
* shortest, so a pass whose axis did not move can still eject a corner clip - and a player
* standing still inside a trigger is still reported.
*/
static void moveAxis(int axis, Game_Rect* rects[9], PlayerTile base, colisionInfo* colInfo){
    float* pos = axis == AXIS_X ? &pVals->rect.x : &pVals->rect.y;
    *pos += pVals->speed[axis];

    Span p = axisSpan(&pVals->rect, axis);
    float snapped = *pos;
    for (int i = 0; i < 9; i++){
        if (rects[i] == NULL){
            continue;
        }
        Game_Rect cur = slotRect(i, rects[i], base);
        TouchedWall face = collidingRect(&cur);
        if (face == WALL_NONE){
            continue;
        }
        //hand back the pointer that came IN, not the local copy, and at the same index, so the
        //caller can recognise its own rects and (in grid mode) read the index as a TilePositions
        colInfo->colidedRect[i] = rects[i];
        colInfo->touched_somehting = true;
        if (cur.type == PORTAL1 || cur.type == PORTAL2){
            colInfo->touched_portal = cur.type;
        }
        //keys, the finish, the portals and the grandfather get reported and walked into
        if (cur.type < WALL || cur.type > PINK_WALL){
            continue;
        }
        Span w = axisSpan(&cur.rect, axis);
        if (face == AXIS_FACES[axis][0]){
            snapped = fminf(snapped, w.lo - p.size); //back off the near face
        } else if (face == AXIS_FACES[axis][1]){
            snapped = fmaxf(snapped, w.lo + w.size); //back off the far face
        }
    }
    *pos = snapped;
}


/*
* This determines what direction the player left out of.
*/
static TouchedWall leaveDirection(){
    if (!pVals->standard_tile_grid){
        return WALL_NONE;
    }
    float cx = pVals->rect.x + pVals->rect.width  * 0.5f;
    float cy = pVals->rect.y + pVals->rect.height * 0.5f;
    if (cx < 0.0f)                     return WALL_LEFT;
    if (cx >= TILES_HORIZ * TILE_SIZE) return WALL_RIGHT;
    if (cy < 0.0f)                     return WALL_TOP;
    if (cy >= TILES_VERT * TILE_SIZE)  return WALL_BOTTOM;
    return WALL_NONE;
}

//the function called by the caller
void move_player(Game_Rect *surroundingRects[9], colisionInfo* colInfo){
    //every zero in here is already the documented "nothing": NULL rects, WALL_NONE for the
    //direction, EMPTY for the portal, false for the flag
    *colInfo = (colisionInfo){0};

    /*
    * the tile the caller built the list around. it has to be read before the step moves the
    * player off it, and it is the same value the caller got from get_player_tile this frame -
    * which is the contract: call get_player_tile, build the nine slots from it, then call this.
    */
    PlayerTile base = get_player_tile();

    moveAxis(AXIS_X, surroundingRects, base, colInfo);
    moveAxis(AXIS_Y, surroundingRects, base, colInfo);

    colInfo->screenLeaveDirection = leaveDirection();
}


Rect playerRect(){
    return pVals->rect;
}


void free_player(){
    free(pVals);
    pVals = NULL; //so a state that re-enters, or a second free, cannot use the old block
}
