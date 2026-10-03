#include "states/forgotten/forgotten.h"
#include "states/forgotten/forgotten_scene3d.h"
#include "states/forgotten/forgotten_config.h"
#include "game_logic/player.h" //init player, set speeed, move player, player rect, free player
#include "utils/graphics.h"
#include "utils/debug.h"
#include <stdlib.h>


//enum of every solid thing in the scene
typedef enum{
    //top screen bounds
    SCENE_RECT_BOUND_TOP, 
    SCENE_RECT_BOUND_LEFT_TOP,
    SCENE_RECT_BOUND_RIGHT_TOP,
    //bottom scree bounds
    SCENE_RECT_BOUND_LEFT_BOT, 
    SCENE_RECT_BOUND_RIGHT_BOT, 
    SCENE_RECT_BOUND_BOTTOM,
    //actual objects
    SCENE_RECT_TREE,
    SCENE_RECT_TRIGGER,
    SCENE_RECT_COUNT 
} SceneRectId;


_Static_assert(SCENE_RECT_COUNT <= 9, "move_player only takes nine surrounding rects");



static Game_Rect SCENE_RECTS[SCENE_RECT_COUNT] = {
    //the bounding walls on the top screen
    [SCENE_RECT_BOUND_TOP] = {
        .rect = {.x = -WALL_T, .y = -WALL_T, .width = WORLD_PX_W + 2 * WALL_T, .height = WALL_T},
        .type = WALL },
    [SCENE_RECT_BOUND_LEFT_TOP] = {
        .rect = {.x = -WALL_T, .y = 0.0f, .width = WALL_T, .height = WORLD_SEAM_Y},
        .type = WALL },
    [SCENE_RECT_BOUND_RIGHT_TOP] = {
        .rect = { .x = WORLD_PX_W, .y = 0.0f, .width = WALL_T, .height = WORLD_SEAM_Y},
        .type = WALL},
    //the bottom screen bounding walls
    [SCENE_RECT_BOUND_LEFT_BOT] = {
        .rect = { .x = -WALL_T, .y = WORLD_SEAM_Y, .width = BOT_INSET + WALL_T, .height = WORLD_SEAM_Y},
        .type = WALL},
    [SCENE_RECT_BOUND_RIGHT_BOT] = {
        .rect = { .x = WORLD_PX_W - BOT_INSET, .y = WORLD_SEAM_Y, .width = BOT_INSET + WALL_T, .height = WORLD_SEAM_Y},
        .type = WALL},
    [SCENE_RECT_BOUND_BOTTOM] = {
        .rect = { .x = -WALL_T, .y = WORLD_PX_H, .width = WORLD_PX_W + 2*WALL_T, .height = WALL_T}, 
        .type = WALL},
    //The actual tree's footprint, what the player collides with
    [SCENE_RECT_TREE] = {
        .rect = {.x = TREE_PX_X - TREE_WALL_W / 2.0f, .y = TREE_PX_Y - TREE_WALL_H / 2.0f, .width = TREE_WALL_W, .height = TREE_WALL_H},
        .type = WALL},
    [SCENE_RECT_TRIGGER] = {
        .rect = {.x = TREE_PX_X - TRIGGER_W / 2.0f, .y = TREE_PX_Y - TRIGGER_BACK - TRIGGER_H, .width = TRIGGER_W, .height = TRIGGER_H},
        .type = Grandfather},
    };

typedef struct {
    Game_Rect* SURROUNDING[9];
    float swayAngle;
} ForgottenState;


static ForgottenState* fgstate;


//Helper functions


static inline bool PlayerOnTopScreen(){
    Rect p = playerRect();
    return p.y + p.height > 0.0f && p.y < WORLD_SEAM_Y;
}


//init the 3d screen.
//start the music here when I make it
//
void forgotten_Init(GameContext* ctx){
    printConsole("init Forgotten, %u bytes", (unsigned)sizeof(ForgottenState));
    fgstate = calloc(1, sizeof(ForgottenState));
    if (fgstate == NULL){
        printConsole("forgotten_Init: out of memory for the state");
        return;
    }

    //failure to render 3d will still have 2d render
    if (!scene3d_init()){
        printConsole("forgotten_Init: the 3D scene did not load");
    }

    //initalize the player, with a non standard tile grid
    init_player(false);

    //set up the surrounding list
    for (int i = 0; i < SCENE_RECT_COUNT; i++){
        fgstate->SURROUNDING[i] = &SCENE_RECTS[i];
    }
    fgstate->SURROUNDING[SCENE_RECT_COUNT] = NULL;

    //set the player's starting position
    setPosition(PLAYER_START_X, PLAYER_START_Y);
    //set the player's speed
    setSpeed(0.0f, 0.0f);
}


Game_State forgotten_Logic(const FrameInput* in, GameContext* ctx){
    if (fgstate == NULL){
        return STATE_MAIN_MENU;
    }
    if (in->kDown & KEY_R){
        return STATE_MAIN_MENU;
    }

    //update the sway angle, once every frame
    fgstate->swayAngle += SWAY_SPEED;

    //set the player's speed based on the circle pad
    setSpeed(in->cpadX * PLAYER_BASE_SPEED, -in->cpadY * PLAYER_BASE_SPEED);

    //move the player, and get back info on what they hit.
    colisionInfo hit;
    move_player(fgstate->SURROUNDING, &hit);

    //check if the player leaves the scene
    if (hit.screenLeaveDirection != WALL_NONE){
        printConsole("forgotten: player left the play area, direction %d", hit.screenLeaveDirection);
    }



    return STATE_NONE;
}

//nothing is drawn yet, so there is no SetDepthLayer call and eyeOffset goes unspent.
//give each group of elements one, with a DEPTH_* factor, as they are added.
void forgotten_DrawTop(C3D_RenderTarget* target, float eyeOffset){
    if (fgstate == NULL) return;

    C2D_TargetClear(target, Colors[CLR_BLACK]);
    C2D_SceneBegin(target);

    //if the player is null, then the player is not rendered on the top screen
    Rect p = playerRect();
    scene3d_render(eyeOffset, fgstate->swayAngle, PlayerOnTopScreen() ? &p : NULL);

    //hand the gpu back to citro2d so the bottom screen can render
    C2D_Prepare();

}

void forgotten_DrawBottom(C3D_RenderTarget* target){
    C2D_TargetClear(target, Colors[CLR_BLACK]);
    C2D_SceneBegin(target);
    if(fgstate == NULL){
        return;
    }

    //draw the player, but only if any of the player is lower than the bottom of the top screen
    Rect p = playerRect();
    if (p.y + p.height > WORLD_SEAM_Y){
        //shift the player position from the world space to the screen coordinates
        p.x -= BOT_INSET;
        p.y -= WORLD_SEAM_Y;
        p.z = LAYER_BASE;
        p.Color = Colors[CLR_LT_GRAY];
        DrawRect(&p);
    }
}

//tear down the 3d scene
//stop the music.
void forgotten_End(void){
    if (fgstate == NULL){
        return;
    }
    free_player();
    scene3d_free();

    //do not free surrounding, it is a list of borrowed pointers
    free(fgstate);
    fgstate = NULL;
}

