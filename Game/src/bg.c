#include <Windows.h>
#include <gl/GLU.h>
#include <stdint.h>
#include <assert.h>

#include "bg.h"
#include "Object.h"
#include "SOIL.h"
#include "globals.h"
#include "baseTypes.h"
#include "draw.h"

typedef enum bgState_t
{
    game,
    flyAway,
    startMenu
} BgState;

typedef struct bg_t
{
	Object obj;

    BgState state;
	Bounds2D bounds;
} Bg;

// the object vtable for the UI object
static void _bgUpdate(Object* obj, uint32_t milliseconds);
static void _bgDraw(Object* obj);
static ObjVtable _bgVtable = {
    _bgDraw,
    _bgUpdate
};


static const char BG[] = "asset/NES - Duck Hunt - Backdrop.png";
static const int32_t BG_COUNT = 3;

static GLuint _bgTexture = 0;


// load the bg image
void bgInitTexture()
{
    if (_bgTexture == 0)
    {
        _bgTexture = SOIL_load_OGL_texture(BG, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
             SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB);
        assert(_bgTexture != 0);
    }
}

/// @brief Instantiate and initialize the BG
/// @param bounds 
/// @return 
Bg* bgInit(Bounds2D bounds)
{
    const float MAX_VEL = 2.0f;

    Bg* bg= malloc(sizeof(Bg));
    if (bg != NULL)
    {
        Coord2D pos = boundsGetCenter(&bounds);
        Coord2D vel = { 0.0f, 0.0f };
        objInit(&bg->obj, &_bgVtable, pos, vel);
        bg->state = startMenu;
    }
    return bg;
}

/// @brief set the background to the fly away color
/// @param bg
void bgFlyAway(Bg* bg)
{
    bg->state = flyAway;

}

/// @brief set the background to the standard game color
/// @param bg
void bgGameBg(Bg* bg)
{
    bg->state = game;
}

/// @brief set the background to the main menu
/// @param bg
void bgMenuBg(Bg* bg)
{
    bg->state = startMenu;
}

/// @brief Free up any resources pertaining to the BG object
/// @param bg 
void bgDeInit(Bg* bg)
{
    objDeinit(&bg->obj);

    free(bg);
}

static void _bgUpdate(Object* obj, uint32_t milliseconds)
{

}

static void _bgDraw(Object* obj)
{
    Bg* bg = (Bg*)obj;

    // calculate the bounding box
    GLfloat xPositionLeft = (obj->position.x - uiSize.x / 2);
    GLfloat xPositionRight = (obj->position.x + uiSize.x / 2);
    GLfloat yPositionTop = (obj->position.y - uiSize.y / 2);
    GLfloat yPositionBottom = (obj->position.y + uiSize.y / 2);

    // find the proper sprite frame from the sprite sheet
    float uPerBg = 1.0f / (float)(BG_COUNT);
    float vPerBg = 1.0f;

    // calculate the starting uv... remember v of 0 is the bottom of the texture
    GLfloat xTextureCoord = ((bg->state) * uPerBg);
    GLfloat yTextureCoord = 1;

    const float BG_DEPTH = -0.98f;

    drawSprite(_bgTexture, xPositionLeft, xPositionRight, yPositionTop, yPositionBottom,
               uPerBg, vPerBg, xTextureCoord, yTextureCoord, BG_DEPTH);
}