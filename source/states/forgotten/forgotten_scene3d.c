#include "states/forgotten/forgotten_scene3d.h"
#include "states/forgotten/forgotten_config.h"
#include "states/forgotten/placeholderModel.h"
#include "game_logic/player.h" //player size
#include "utils/graphics.h"
#include "utils/input.h" //for the 3d scale
#include "utils/debug.h"

#include <citro3d.h>
#include <stdlib.h>
#include <string.h> //for memcpy, which fills the vbo
#include <math.h> //floorf, powf, sinf, cosf, tanf

#include "scene3d_shbin.h" //the generated shader header, made from the pica file


//the model parts

typedef enum {
    PART_FLOOR,
    PART_TRUNK,
    PART_LEAVES,
    PART_PLAYER,
    PART_COUNT
} ScenePartId;

_Static_assert(PART_COUNT == SCENE_PART_COUNT, "parts here and boxes in placeholderModel.c disagree"); 


//the structs that holds the parts of the scene
typedef struct {
    int first;
    int count;
    C3D_Material material;
} ScenePart;

//STARTING PLACEHOLDER VALUES, REPLACE WITH BLENDER MODELS
static const ScenePart PARTS[PART_COUNT] = {
    [PART_FLOOR] = { PART_FLOOR * BOX_VERT_COUNT, BOX_VERT_COUNT, 
        {{0.25f, 0.10f, 0.25f}, {0,0,0}, {0.05f,0.05f,0.05f}, {0.45f, 0.20f, 0.45f}, {0,0,0}}},
    [PART_TRUNK] = {PART_TRUNK * BOX_VERT_COUNT, BOX_VERT_COUNT,
        {{0.08f,0.08f,0.14f}, {0,0,0}, {0.05f, 0.05f, 0.05f}, {0.15f,0.15f,0.25f}, {0,0,0}}},
    [PART_LEAVES] = {PART_LEAVES * BOX_VERT_COUNT, BOX_VERT_COUNT, 
        {{0.35f, 0.05f, 0.12f}, {0,0,0}, {0.05f,0.05f,0.05f}, {0.70f,0.10f,0.20f}, {0,0,0}}},
    [PART_PLAYER] = {PART_PLAYER * BOX_VERT_COUNT, BOX_VERT_COUNT, 
        {{0.22f,0.22f,0.22f}, {0,0,0}, {0.05f, 0.05f, 0.05f}, {0.55f, 0.55f, 0.55f}, {0,0,0}}},
};


//the struct for the 3d scene
typedef struct {
    DVLB_s* dvlb;
    shaderProgram_s program;
    int uLoc_projection;
    int uLoc_modelView;
    C3D_AttrInfo attrInfo;
    C3D_BufInfo bufInfo;
    void* vbo; //needs to be linear allocd
    //holds the pointers into light and luts
    C3D_LightEnv lightEnv;
    C3D_Light light;
    C3D_LightLut lutToon;
    C3D_LightLut lutSpec;

    C3D_Mtx view; //the camera never moves, so it is only built once.
    //the camera up vector in world space
    float upY;
    float upZ;
} Scene3D;

static Scene3D* scn; //NULL when the scene is not loaded.

//helper functions



//the toon ramps, both quantize smooth lighting term into a few flat bands.
static float toon_diffuse(float x, float arg){
    (void) arg;
    const float factor = NUM_DIFFUSE_TONES - 1;
    return floorf(0.5f + x * factor) / factor;
}

static float toon_specular(float x, float shininess){
    const float factor = NUM_SPECULAR_TONES - 1;
    return floorf(0.5f + powf(x, shininess) * factor) / factor;
}

//these functions translate the 2d player world pixels to 3d world pixels.
static inline float WorldX(float px){ return (px - TOP_SCREEN_WIDTH / 2.0f) * WORLD_PER_PX; }
static inline float WorldY(float py){ return -(py - TOP_SCREEN_HIGHT / 2.0f) * WORLD_PER_PX; }

//translate the eye offset to Iod
static inline float EyeOffsetToIod(float eyeOffset)
{
    return -eyeOffset / (SCALE_3D * 3.0f);
}


//sets up the gpu for 3d graphics.
static void Scene3D_Bind()
{
    C3D_BindProgram(&scn->program);
    C3D_SetAttrInfo(&scn->attrInfo);
    C3D_SetBufInfo(&scn->bufInfo);
    C3D_LightEnvBind(&scn->lightEnv);

    //set read and write depth, so that the player can go behind the tree
    C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);
    C3D_CullFace(GPU_CULL_BACK_CCW);

    //blend the primary color with the secondary color the light env produces
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_RGB, GPU_FRAGMENT_PRIMARY_COLOR, GPU_FRAGMENT_SECONDARY_COLOR, 0);
    C3D_TexEnvSrc(env, C3D_Alpha, GPU_PRIMARY_COLOR, 0, 0);
    C3D_TexEnvFunc(env, C3D_RGB, GPU_ADD);
    C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);

    for (int i = 1; i < 6; i++){
        C3D_TexEnvInit(C3D_GetTexEnv(i));
    }
}


//draw one part in the scene
static void DrawPart(ScenePartId id, const C3D_Mtx* model){
    const ScenePart* part = &PARTS[id];

    C3D_Mtx modelView;
    Mtx_Multiply(&modelView, &scn->view, model);

    C3D_LightEnvMaterial(&scn->lightEnv, &part->material);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, scn->uLoc_modelView, &modelView);
    C3D_DrawArrays(GPU_TRIANGLES, part->first, part->count);
}






//interface for the forgotten state

bool scene3d_init(){
    scn = calloc(1, sizeof(Scene3D));
    if (scn == NULL){
        printConsole("scene3d_init: out of memory");
        return false;
    }

    //init the compiled shader
    scn->dvlb = DVLB_ParseFile((u32*)scene3d_shbin, scene3d_shbin_size);
    shaderProgramInit(&scn->program);
    shaderProgramSetVsh(&scn->program, &scn->dvlb->DVLE[0]);

    scn->uLoc_projection = shaderInstanceGetUniformLocation(scn->program.vertexShader, "projection");
    scn->uLoc_modelView = shaderInstanceGetUniformLocation(scn->program.vertexShader, "modelView");

    AttrInfo_Init(&scn->attrInfo);
    AttrInfo_AddLoader(&scn->attrInfo, 0, GPU_FLOAT, 3); //v0 = position
    AttrInfo_AddLoader(&scn->attrInfo, 1, GPU_FLOAT, 3); //v1 = normal

    //use linear alloc so that the gpu can use it. 
    scn->vbo = linearAlloc(sizeof(SCENE_VERTS));
    if (scn->vbo == NULL) {
        printConsole("scene3d_init: out of linear memory for the scene VBO");
        scene3d_free();
        return false;
    }
    memcpy(scn->vbo, SCENE_VERTS, sizeof(SCENE_VERTS));

    //stride is 6 floats, 2 attributes, and the 0x10 nibble is the packed permutation mapping.
    BufInfo_Init(&scn->bufInfo);
    BufInfo_Add(&scn->bufInfo, scn->vbo, sizeof(float[6]), 2, 0x10);


    //the toon lighting, D0 takes dot (N, H) and bands the highlight; D1 takes dot(L, N)
    //and bands the main ramp
    C3D_LightEnvInit(&scn->lightEnv);
    C3D_LightEnvBind(&scn->lightEnv);
    LightLut_FromFunc(&scn->lutSpec, toon_specular, 30.0f, false);
    LightLut_FromFunc(&scn->lutToon, toon_diffuse, 0.0f, false);
    C3D_LightEnvLut(&scn->lightEnv, GPU_LUT_D0, GPU_LUTINPUT_NH, false, &scn->lutSpec);
    C3D_LightEnvLut(&scn->lightEnv, GPU_LUT_D1, GPU_LUTINPUT_LN, false, &scn->lutToon);
    C3D_LightInit(&scn->light, &scn->lightEnv);

    //Init the camera
    //a view matrix is the inverse of the camera's own transform, it brings the world to the camera rather than the camera to the world. 
    const float pitch = C3D_AngleFromDegrees(CAM_PITCH_DEG);

    //the camera is raised CAM_HEIGHT and tilded down by pitch,
    //and the horizontal setback is whatever puts the cenre of the play area under it's crosshair.
    //a ray leaving at that angle from that height meets the ground CAM_HEIGHT/tan(pitch) ahead. 
    //negative because the camera sits on the near side of the floor, looking along +Y
    const float camY = -CAM_HEIGHT / tanf(pitch);

    Mtx_Identity(&scn->view);

    //world axes to eye axes, eye space is X right, Y up, Z toward the viewer, 
    //so a level camera looking along +Y needs -90 degrees here. 
    //tiling the camera down by pitch adds pitch back. 
    Mtx_RotateX(&scn->view, pitch - C3D_AngleFromDegrees(90.0f), true);
    Mtx_Translate(&scn->view, 0.0f, -camY, -CAM_HEIGHT, true);


    //the camera's up vector, for the leaf drift. 
    scn->upY = sinf(pitch);
    scn->upZ = cosf(pitch);

    return true;
}


void scene3d_render(float eyeOffset, float sway, const Rect* player){
    if (scn == NULL){
        return;
    }

    Scene3D_Bind();

    //the projection is the only thing that differs between the eyes.
    C3D_Mtx projection;
    Mtx_PerspStereoTilt(&projection, C3D_AngleFromDegrees(CAM_FOV_DEG), C3D_AspectRatioTop,
        SCENE_NEAR, SCENE_FAR, EyeOffsetToIod(eyeOffset), CAM_FOCAL, false);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, scn->uLoc_projection, &projection);

    //C3D light position wants eye space, so push the world space light through the view
    C3D_FVec lightWorld = FVec4_New(LIGHT_X, LIGHT_Y, LIGHT_Z, 1.0f);
    C3D_FVec lightEye = Mtx_MultiplyFVec4(&scn->view, lightWorld);
    C3D_LightPosition(&scn->light, &lightEye);

    C3D_Mtx model;

    //the floor is in world position, so it needs no transformation. 
    Mtx_Identity(&model);
    DrawPart(PART_FLOOR, &model);

    Mtx_Identity(&model);
    Mtx_Translate(&model, WorldX(TREE_PX_X), WorldY(TREE_PX_Y), 0.0f, true);
    DrawPart(PART_TRUNK, &model);

    //the leaves are lifted to the top of the trunk and walked slowly in a circle.
    //the circle is traced in the screne plane. 
    const float driftH = cosf(sway) * SWAY_RADIUS;
    const float driftV = sinf(sway) * SWAY_RADIUS;
    Mtx_Identity(&model);
    Mtx_Translate(&model, WorldX(TREE_PX_X) + driftH, WorldY(TREE_PX_Y) + driftV * scn->upY, TRUNK_H + driftV * scn->upZ, true);
    DrawPart(PART_LEAVES, &model);

    //the player. NULL means they are fully on the bottom screen this frame, so there is nothing to draw here. 
    if (player != NULL){
        const float scale = PLAYER_SIZE * WORLD_PER_PX;

        Mtx_Identity(&model);
        Mtx_Translate(&model, WorldX(player->x + player->width / 2.0f), WorldY(player->y + player->height / 2.0f), 0.0f, true);
        Mtx_Scale(&model, scale, scale, scale);
        DrawPart(PART_PLAYER, &model);
    }

    return;
}

void scene3d_free(){
    if (scn == NULL){
        return;
    }
    //hand the shader program back to citro 2d
    C2D_Prepare();
    C3D_LightEnvBind(NULL);

    if (scn->vbo != NULL){
        linearFree(scn->vbo);
    }
    
    
    shaderProgramFree(&scn->program);
    if (scn->dvlb != NULL) {
        DVLB_Free(scn->dvlb);
    }
    free(scn);
    scn = NULL;
    return;
}