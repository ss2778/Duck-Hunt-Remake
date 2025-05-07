#include <Windows.h>
#include <gl/GLU.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>

#include "duck.h"
#include "globals.h"
#include "Object.h"
#include "random.h"
#include "SOIL.h"
#include "baseTypes.h"
#include "draw.h"

#define NUM_DUCKS 2
#define M_PI (acos(-1.0) / 2)
#define GRASS_BOUND 700.0f

// all of these values are based upon the layout of the PNG
static const char DUCK_SHEET[] = "asset/NES - Duck Hunt - Ducks.png";
static const int32_t SPRITE_COUNT = 12;
static const int32_t flyFrameLength = 56;


static uint8_t _layer = 0;
static uint8_t _roundNum;
static const int32_t _scores[3][3] = {
	{500, 800, 1000}, // black duck scores
	{1500, 2400, 3000}, // red duck scores
	{1000, 1500, 2000} // blue duck scores
}; // index [x][0] is for rounds 1-5, [x][1] is for rounds 6-10
   // and index [x][2] is for rounds 11 onward

typedef enum color_t
{
	color_min,
	black=color_min,
	red,
	blue,
	color_count
} Color;

typedef enum duck_state_t
{
	flying,
	leaving,
    shot,
	dead,
	inactive
} DuckState;

typedef struct duck_t
{
	Object obj;

	uint32_t quackTimerSet;
	uint32_t quackTimer;
	uint8_t layer;
	Bounds2D bounds;
	Color type;
	bool bottomCollide;
	uint8_t frame;
	uint32_t frameUpdate;
	DuckState state;
} Duck;

// the object vtable for all ducks
static void _duckUpdate(Object* obj, uint32_t milliseconds);
static void _duckDraw(Object* obj);
static ObjVtable _duckVtable = {
	_duckDraw,
	_duckUpdate
};

static const Coord2D size = {
	136.0f,
	132.0f
};
static GLuint _duckTexture = 0;


// other private methods
static void _duckDoCollisions(Duck* duck);
static void _duckCollideField(Duck* duck);
static Coord2D _duckGetVel();

// initialize callbacks
static duckSoundCB _soundCB = NULL;


/// @brief Sets the callback
/// @param soundcb
void duckSetCB(duckSoundCB soundcb)
{
	_soundCB = soundcb;
}

/// @brief Clears the callback
void duckClearCB()
{
	_soundCB = NULL;
}

// Load sprite sheet
void duckInitTexture()
{
    if (_duckTexture == 0)
    {
        _duckTexture = SOIL_load_OGL_texture(DUCK_SHEET, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
            SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB);
        assert(_duckTexture != 0);
    }
}

/// @brief Instantiate and initialize a duck object
/// @param bounds 
/// @return 
Duck* duckNew(Bounds2D bounds)
{

	Duck* duck = malloc(sizeof(Duck));
	if (duck != NULL)
	{
		Coord2D pos = boundsGetCenter(&bounds);
		Coord2D vel = { 0, 0 };
		objInit(&duck->obj, &_duckVtable, pos, vel);

		duck->bounds = bounds;
		duck->bounds.botRight.y = GRASS_BOUND;
		duck->type = 0;
		duck->state = inactive;
        duck->frame = 0;
		duck->frameUpdate = 0;
		duck->layer = _layer++;
		duck->bottomCollide = false;
	}
	return duck;
}

/// @brief Free up any resources pertaining to a duck object
/// @param duck 
void duckDelete(Duck* duck)
{
    objDeinit(&duck->obj);

    free(duck);
}

/// @brief set the state of all ducks to be the fly away state
/// @param ducks
void ducksFlyAway(Duck** ducks)
{
	uint8_t i;
	for (i = 0; i < NUM_DUCKS; ++i)
	{
		if (ducks[i]->state == flying)
		{
			ducks[i]->state = leaving;
			ducks[i]->obj.velocity.x = 0;
			ducks[i]->obj.velocity.y = -60.0f * ((-22.0f / ((_roundNum)+1.0f)) + 15.0f);
		}
	}
}

/// @brief Check if the mouse click hit a duck
/// @param mousePos
/// @param ducks
int32_t duckCheckForHit(Coord2D mousePos, Duck** ducks)
{
	int i;

	for (i = 0; i < NUM_DUCKS; ++i)
	{
		// check if the mouse overlaps with the duck
		if ((ducks[i])->state == dead || (ducks[i])->state == shot || (ducks[i])->state == inactive || ((ducks[i])->obj.position.x - (size.x / 2) > mousePos.x ||
			((ducks[i])->obj.position.x + (size.x / 2)) < mousePos.x) || (ducks[i])->obj.position.y - (size.y / 2) > mousePos.y ||
			((ducks[i])->obj.position.y + (size.y / 2)) < mousePos.y)
			continue;
		// if the mouse is on top of a duck, shoot that duck
		// update its sprite and make it stand still
		(ducks[i])->state = shot;
		(ducks[i])->frame = 0;
		(ducks[i])->frameUpdate = 750;
		(ducks[i])->obj.velocity.x = 0.0f;
		(ducks[i])->obj.velocity.y = 0.0f;
		// return proper score value based on the duck color and round number
		if (_roundNum < 11)
			if (_roundNum < 6)
				return _scores[ducks[i]->type][0];
			else
				return _scores[ducks[i]->type][1];
		else
			return _scores[ducks[i]->type][2];
		break;
	}
	
	return 0;
}

/// @brief spawn 2 ducks for the current wave
/// @param roundNum, ducks
void ducksSetActive(uint8_t roundNum, Duck** ducks)
{
	int i;
	Coord2D vel;
	// update the local roundNUm
	_roundNum = roundNum;
	// iterate over the ducks
	for (i = 0; i < NUM_DUCKS; ++i)
	{
		vel = _duckGetVel();
		ducks[i]->state = flying;
		ducks[i]->type = randGetInt(color_min, color_count);
		ducks[i]->frame = randGetInt(0, 2);
		ducks[i]->quackTimerSet = (uint32_t)(1250 * randGetFloat(0.8f, 1.2f));
		ducks[i]->quackTimer = ducks[i]->quackTimerSet;
		ducks[i]->obj.position.y = ducks[i]->bounds.botRight.y + 50;
		ducks[i]->obj.position.x = (float)randGetInt(0 + (int32_t)(size.x / 2), (int32_t)uiSize.x - (int32_t)(size.x / 2));
		ducks[i]->obj.velocity.x = vel.x;
		ducks[i]->obj.velocity.y = vel.y;
		ducks[i]->bottomCollide = false;
	}
}

/// @brief calculate the speed of a duck based on the current round
/// @return vel
static Coord2D _duckGetVel()
{
	Coord2D vel = { 0,0 };
	float speed = ((-22.0f / ((_roundNum) + 1.0f)) + 15.0f) * 60.0f;
	speed += speed * randGetFloat(-0.1f, 0.1f);
	float angle = randGetFloat(((float)M_PI * 7.0f / 6.0f), (float)M_PI * 11.0f / 6.0f);
	vel.x = (float)cos(angle) * speed;
	vel.y = (float)sin(angle) * speed;
	if (vel.y > 0)
		vel.y = -vel.y;
	return vel;
}

/// @brief return whether any ducks in the given list are active
/// @param ducks
/// @return true if an active duck, false if no active ducks
bool duckActiveStatus(Duck** ducks)
{
	uint8_t i;
	for (i = 0; i < NUM_DUCKS; ++i)
		if (ducks[i]->state != inactive)
			return true;
	return false;
}

/// @brief Move the duck and process collisions based on duck state
/// @param obj 
/// @param milliseconds 
static void _duckUpdate(Object* obj, uint32_t milliseconds)
{
	Duck* duck = (Duck*)obj;
    objDefaultUpdate(obj, milliseconds);

	// update the duck's animation and velocity depending on its state
	// also play any needed sfx
	switch (duck->state)
	{
		// flying and leaving are the same case
		case flying:
			if (duck->state == flying)
				_duckDoCollisions((Duck*)obj);
			if (!(duck->bottomCollide) && ((duck->obj.position.y + (size.y / 2)) <= duck->bounds.botRight.y))
				duck->bottomCollide = true;
			// intentional lack of break statement
		case leaving:

			if (milliseconds >= duck->frameUpdate)
			{
				duck->frameUpdate = flyFrameLength;
				if (++(duck->frame) > 2)
					duck->frame = 0;
				// play the sound on different frames so it sounds more natural
				if (duck->layer == duck->frame)
					_soundCB(flap);
			}
			else
			{
				duck->frameUpdate -= milliseconds;
			}
			// check for despawn condition
			if (duck->obj.position.y < -size.y)
			{
				duck->state = inactive;
			}
			// check for quack sound
			if (milliseconds >= duck->quackTimer)
			{
				_soundCB(quack);
				duck->quackTimer = duck->quackTimerSet;
			}
			else
			{
				duck->quackTimer -= milliseconds;
			}
			break;
		case shot:
			if (milliseconds >= duck->frameUpdate)
			{
				duck->state = dead;
				duck->frame = 1;
				obj->velocity.y = 240.0f;
				duck->bottomCollide = false;
				duck->frameUpdate = flyFrameLength;
				_soundCB(fall);
			}
			else
			{
				duck->frameUpdate -= milliseconds;
			}
			break;
		case dead:
			// once the duck falls enough, despawn it
			if (duck->obj.position.y + (size.y / 2) >= duck->bounds.botRight.y + 132)
			{
				duck->state = inactive;
				_soundCB(thud);
			}
			else if (milliseconds >= duck->frameUpdate)
			{
				duck->frameUpdate = flyFrameLength;
				duck->frame = (duck->frame == 1) ? 2 : 1;
			}
			else
			{
				duck->frameUpdate -= milliseconds;
			}
			break;
	}
}

static void _duckDoCollisions(Duck* duck)
{
	_duckCollideField(duck);
}

static void _duckCollideField(Duck* duck)
{
	float rightSide = duck->bounds.botRight.x;
	float leftSide = duck->bounds.topLeft.x;
	float topSide = duck->bounds.topLeft.y;
	float bottomSide = duck->bounds.botRight.y;


	if (duck->obj.position.x - (size.x / 2) <= leftSide)
	{
		duck->obj.velocity.x = -duck->obj.velocity.x;
		duck->obj.position.x = leftSide + (size.x / 2);
	}
	if (duck->obj.position.x + (size.x / 2) >= rightSide)
	{
		duck->obj.velocity.x = -duck->obj.velocity.x;
		duck->obj.position.x = rightSide - (size.x / 2);
	}
	if (duck->bottomCollide && duck->obj.position.y + (size.y / 2) >= bottomSide)
	{
		duck->obj.velocity.y = -duck->obj.velocity.y;
		duck->obj.position.y = bottomSide - (size.y / 2);
	}
	if (duck->state != leaving && duck->obj.position.y - (size.y / 2) <= topSide)
	{
		duck->obj.velocity.y = -duck->obj.velocity.y;
		duck->obj.position.y = topSide + (size.y / 2);
	}
}

static void _duckDraw(Object* obj)
{
    Duck* duck = (Duck*)obj;

	// do not draw inactive ducks
	if (duck->state == inactive)
		return;

    // calculate the bounding box
    GLfloat xPositionLeft = (obj->position.x - size.x / 2);
    GLfloat xPositionRight = (obj->position.x + size.x / 2);
    GLfloat yPositionTop = (obj->position.y - size.y / 2);
    GLfloat yPositionBottom = (obj->position.y + size.y / 2);

    // find the proper sprite frame from the sprite sheet
    float uPerFrame = 1.0f / (float)(SPRITE_COUNT);
    float vPerColor = 1.0f / (float)color_count;

    // calculate the starting uv... remember v of 0 is the bottom of the texture
    GLfloat xTextureCoord = (duck->frame * uPerFrame) + (duck->state * 3 * uPerFrame);
    GLfloat yTextureCoord = (color_count - duck->type) * vPerColor;


	// adjust sprite offset for certian cases

	// if the duck is shot, move to the shot sprite
	// if the duck is flying/leaving, set sprite to appropriate flying sprite

	if (duck->state == leaving || duck->state == shot || (duck->state == flying && (-(obj->velocity.y) < abs((int32_t)obj->velocity.x))))
	{
		xTextureCoord += uPerFrame * 3;
	}

    const float DUCK_DEPTH = -0.5f + (duck->layer * 0.01f);
	// check if the duck is facing left or right. If it is facing left,
	// mirror the sprite
	if (obj->velocity.x >= 0)
	{
		drawSprite(_duckTexture, xPositionLeft, xPositionRight, yPositionTop, yPositionBottom,
				   uPerFrame, vPerColor, xTextureCoord, yTextureCoord, DUCK_DEPTH);
	}
	else
	{
		drawSprite(_duckTexture, xPositionLeft, xPositionRight, yPositionTop, yPositionBottom,
				   -uPerFrame, vPerColor, (xTextureCoord + uPerFrame), yTextureCoord, DUCK_DEPTH);
	}
        
}