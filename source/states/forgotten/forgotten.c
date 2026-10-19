#include "states/forgotten/forgotten.h"
#include "states/forgotten/forgotten_scene3d.h"
#include "states/forgotten/forgotten_config.h"
#include "game_logic/player.h" //init player, set speeed, move player, player rect, free player
#include "utils/graphics.h"
#include "utils/debug.h"
#include "utils/popup.h"
#include "utils/loader.h"
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
        .rect = {.x = -WALL_T, .y = 0.0f, .width = WALL_T, .height = WORLD_BOT_TOP_Y},
        .type = WALL },
    [SCENE_RECT_BOUND_RIGHT_TOP] = {
        .rect = { .x = WORLD_PX_W, .y = 0.0f, .width = WALL_T, .height = WORLD_BOT_TOP_Y},
        .type = WALL},
    //the bottom screen bounding walls
    [SCENE_RECT_BOUND_LEFT_BOT] = {
        .rect = { .x = -WALL_T, .y = WORLD_BOT_TOP_Y, .width = BOT_INSET + WALL_T, .height = WORLD_PX_H - WORLD_BOT_TOP_Y},
        .type = WALL},
    [SCENE_RECT_BOUND_RIGHT_BOT] = {
        .rect = { .x = WORLD_PX_W - BOT_INSET, .y = WORLD_BOT_TOP_Y, .width = BOT_INSET + WALL_T, .height = WORLD_PX_H - WORLD_BOT_TOP_Y},
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
    C2D_TextBuf textBoxBuf;
    C2D_Text textBoxText[HIS_LINES_MAX_PAGES];
    Game_Rect* SURROUNDING[9];
    float swayAngle; 
    float fadeAlpha; 
    bool fading; //if the popup is done and the screen is ramping out.
} ForgottenState;


static ForgottenState* fgstate;


//popup end callback, starts the fade out
static void StartFade(void){
    fgstate->fading = true;
}


//checks if the player touched the trigger tile
static bool TouchedTrigger(const colisionInfo* hit){
    for (int i = 0; i < 9; i++){
        if (hit->colidedRect[i] == &SCENE_RECTS[SCENE_RECT_TRIGGER]){
            return true;
        }
    }
    return false;
}

//draws the fade to the screen
static void DrawFade(float w, float h){
    Rect fade = {.x = 0, .y = 0, .z = LAYER_POPUP, .width = w, .height = h, .Color = C2D_Color32(255, 255, 255, (u8)(fgstate->fadeAlpha * 255.0f))};
    DrawRect(&fade);
}


//chacks if the player is on the top screen or not
static inline bool PlayerOnTopScreen(){
    Rect p = playerRect();
    return p.y + p.height > 0.0f && p.y < WORLD_SEAM_Y - SCREEN_OVERLAP_DEPTH;
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

    //init the popup text
    fgstate->textBoxBuf = C2D_TextBufNew(HIS_LINES_BOX_GLYPHS);
    const int pages = Egg_Load(HIS_LINES_PATH, fgstate->textBoxText, HIS_LINES_MAX_PAGES, fgstate->textBoxBuf);

    //init the popup rect
    const Rect textBox = {.x = BOX_X, .y = BOX_Y, .width = BOX_W, .height = BOX_H, .Color = Colors[CLR_GRAY]};

    //init the popup
    popup_init(textBox, pages, false, false, BOX_WRAP_W, StartFade, NULL);
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

    //if the popup is up, stop the logic here
    if (popup_status()){
        popup_logic(in);
        return STATE_NONE;
    }
    
    //do the logic for fading the screen after the last line is read
    if (fgstate->fading){
        fgstate->fadeAlpha += FADE_SPEED;
        if (fgstate->fadeAlpha >= 1.0f){
            return STATE_MAIN_MENU;
        }
        return STATE_NONE;
    }

    //set the player's speed based on the circle pad
    setSpeed(in, false);

    //move the player, and get back info on what they hit.
    colisionInfo hit;
    move_player(fgstate->SURROUNDING, &hit);

    //check if the player leaves the scene
    if (hit.screenLeaveDirection != WALL_NONE){
        printConsole("forgotten: player left the play area, direction %d", hit.screenLeaveDirection);
    }

    //check if the player touched the trigger 
    if (TouchedTrigger(&hit)){
        popup_start(fgstate->textBoxText);
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

    //hand the gpu back to citro2d so the fade and bottom screen can render
    C2D_Prepare();

    //draw the fade
    SetDepthLayer(eyeOffset, DEPTH_POPUP);
    popup_draw(target);

    if (fgstate->fading){
        C2D_ViewReset();
        DrawFade(TOP_SCREEN_WIDTH, TOP_SCREEN_HIGHT);
    }
    C2D_Flush();

}

void forgotten_DrawBottom(C3D_RenderTarget* target){
    C2D_TargetClear(target, Colors[CLR_BLACK]);
    C2D_SceneBegin(target);
    if(fgstate == NULL){
        return;
    }

    //draw the player, but only if any of the player is lower than the bottom of the top screen
    Rect p = playerRect();
    if (p.y + p.height > WORLD_BOT_TOP_Y){
        //shift the player position from the world space to the screen coordinates
        p.x -= BOT_INSET;
        p.y -= WORLD_BOT_TOP_Y;
        p.z = LAYER_BASE;
        p.Color = Colors[CLR_LT_GRAY];
        DrawRect(&p);
    }

    //draw the fade on top of the screen
    if (fgstate->fading){
        DrawFade(BOTTOM_SCREEN_WIDTH, BOTTOM_SCREEN_HIGHT);
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
    popup_free();
    C2D_TextBufDelete(fgstate->textBoxBuf);

    //do not free surrounding, it is a list of borrowed pointers
    free(fgstate);
    fgstate = NULL;
}

