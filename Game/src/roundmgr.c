#include <stdint.h>
#include <stdlib.h>
#include <Windows.h>
#include <gl/GLU.h>
#include <assert.h>

#include "Object.h"
#include "SOIL.h"
#include "globals.h"
#include "roundmgr.h"
#include "draw.h"

typedef enum roundState_t
{
	inactive,
	start,
	startSeq,
	waveStart,
	wave,
	flyAway,
	duckWait,
	waveEnd,
	waveEndSeq,
	roundEnd,
	lose,
	ending,
	win,
	perfect
} State;

typedef struct roundmgr_t
{
	Object obj;

	bool barkFlag;
	State roundState;
	uint8_t roundNum;
	uint8_t waveNum;
	uint8_t ducksHit;
	uint8_t waveDucksHit;
	uint8_t currentDuck;
	uint8_t requiredDucks;
	bool duckStatus[10];

	uint32_t updateTimeLeft;
	uint32_t dogLaughTimer;
	uint8_t dogLaughFrame;

	Bounds2D bounds;
} Round;

// the object vtable for the round manager object
static void _roundUpdate(Object* obj, uint32_t milliseconds);
static void _roundDraw(Object* obj);
static ObjVtable _roundVtable = {
	_roundDraw,
	_roundUpdate
};

// static constants for draws
static const Coord2D roundNumPos = { 150, 819 };
static const Coord2D roundPopupPos = { 380, 242 };
static const Coord2D flyAwayPopupPos = { 360, 242 };
static const Coord2D duckUIpos = { 360, 887 };
static const Coord2D _cellSize = { 30.0f, 35.0f };
static const uint32_t _perfectbonus[] = { 10000, 15000, 20000, 30000 };
static const Coord2D _dogSize = { 216.0f, 186.0f };
static const float _dogLowy = 752.0f;
static const float _dogTopy = 579.0f;
static const float _dogSpeed = 600.0f;
static const uint32_t _dogFrameLength = 77;

static const char NUMBERS[] = "asset/numbers.png";
static const char DUCK_UI[] = "asset/DuckUI.png";
static const char DOG[] = "asset/dogSprites.png";
static const char BOXES[] = "asset/NES - Duck Hunt - UI elements.png";
static const char UI[] = "asset/NES - Duck Hunt - Backgrounds.png";
static GLuint _numberTexture = 0;
static GLuint _textBoxesTexture = 0;
static GLuint _interfaceTexture = 0;
static GLuint _duckUITexture = 0;
static GLuint _dogTexture = 0;

// initialize callbacks
static roundSoundCB _soundCB = NULL;
static roundNoArgCB _flyAwayCB = NULL;
static roundNoArgCB _flyAwayOverCB = NULL;
static roundMakeCB _makeDucksCB = NULL;
static roundNoArgCB _loseCB = NULL;
static roundBoolCB _roundDuckCheckCB = NULL;
static roundPerfCB _roundPerfCB = NULL;
static roundNoArgCB _reloadCB = NULL;

// private methods
static uint32_t _roundGetFlyAwayTime(uint8_t roundNum);

// load the cursor image
void roundInitTextures()
{
	if (_numberTexture == 0)
	{
		_numberTexture = SOIL_load_OGL_texture(NUMBERS, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
			SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB);
		assert(_numberTexture != 0);
	}
	if (_textBoxesTexture == 0)
	{
		_textBoxesTexture = SOIL_load_OGL_texture(BOXES, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
			SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB);
		assert(_textBoxesTexture != 0);
	}
	if (_interfaceTexture == 0)
	{
		_interfaceTexture = SOIL_load_OGL_texture(UI, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
			SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB);
		assert(_interfaceTexture != 0);
	}
	if (_duckUITexture == 0)
	{
		_duckUITexture = SOIL_load_OGL_texture(DUCK_UI, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
			SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB);
		assert(_duckUITexture != 0);
	}
	if (_dogTexture == 0)
	{
		_dogTexture = SOIL_load_OGL_texture(DOG, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
			SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB);
		assert(_dogTexture != 0);
	}
}

/// @brief Sets the callbacks
/// @param soundcb, flyAwaycb, losecb
void roundSetCBs(roundSoundCB soundcb, roundNoArgCB flyAwaycb, roundMakeCB makeDuckscb, roundNoArgCB losecb,
				 roundBoolCB duckCheckcb, roundNoArgCB flyAwayOvercb, roundPerfCB perfcb, roundNoArgCB reloadcb)
{
	_soundCB = soundcb;
	_flyAwayCB = flyAwaycb;
	_flyAwayOverCB = flyAwayOvercb;
	_makeDucksCB = makeDuckscb;
	_loseCB = losecb;
	_roundDuckCheckCB = duckCheckcb;
	_roundPerfCB = perfcb;
	_reloadCB = reloadcb;
}

/// @brief Clears the callbacks
void roundClearCBs()
{
	_soundCB = NULL;
	_flyAwayCB = NULL;
	_flyAwayOverCB = NULL;
	_makeDucksCB = NULL;
	_loseCB = NULL;
	_roundDuckCheckCB = NULL;
	_roundPerfCB = NULL;
	_reloadCB = NULL;
}

/// @brief enables the round manager
/// @params round
void roundSetActive(Round* round)
{
	round->roundState = start;
	round->requiredDucks = 6;
	_soundCB(gameStart);
	round->obj.position.x = boundsGetCenter(&(round->bounds)).x;
	round->obj.position.y = _dogLowy;
}

/// @brief disables the round manager
/// @params round
void roundSetInctive(Round* round)
{
	round->roundNum = 1;
	round->roundState = inactive;
}

/// @brief Instantiate and initialize the roundManager
/// @param bounds 
/// @return 
Round* roundInit(Bounds2D bounds)
{
	Round* round = malloc(sizeof(Round));
	if (round != NULL)
	{
		Coord2D pos = boundsGetCenter(&bounds);
		Coord2D vel = { 0.0f, 0.0f };
		objInit(&round->obj, &_roundVtable, pos, vel);

		round->roundNum = 1;
		round->bounds = bounds;
		round->roundState = inactive;
		round->ducksHit = 0;
		round->waveNum = 0;
		round->requiredDucks = 6;
		// the position and velocity of the round object are used for the dog
		round->obj.position.y = _dogLowy;
	}
	return round;
}

/// @brief Free up any resources pertaining to the round manager object
/// @param round
void roundDeInit(Round* round)
{
	objDeinit(&round->obj);

	free(round);
}

/// @brief get the state of the round object
/// @param round
/// @return round->roundState
State roundGetState(Round* round)
{
	return round->roundState;
}

/// @brief increment the number of hit ducks
/// @param round
void roundDuckHit(Round* round)
{
	++(round->ducksHit);
	++(round->waveDucksHit);
	// mark the current duck as hit
	round->duckStatus[round->currentDuck] = true;
	++(round->currentDuck);
}

static void _roundUpdate(Object* obj, uint32_t milliseconds)
{
	int8_t i;
	Round* round = (Round*)obj;
	// update the round manager depending on the state of the game
	switch (round->roundState)
	{
		case start:
			// check if the duck threshold has to increase
			if (round->roundNum == 11 || round->roundNum == 13 || round->roundNum == 15 ||
				round->roundNum == 20)
			{
				++(round->requiredDucks);
			}
			round->currentDuck = 0;
			round->waveNum = 0;
			round->barkFlag = false;
			_reloadCB();
			round->ducksHit = 0;
			round->roundState = startSeq;
			if (round->roundNum == 1)
				round->updateTimeLeft = 7500;
			else
				round->updateTimeLeft = 2500;
			for (i = 0; i < 10; ++i)
				round->duckStatus[i] = false;
			break;
		case startSeq:
			if (milliseconds >= round->updateTimeLeft)
			{
				round->roundState = waveStart;
			}
			else
			{
				round->updateTimeLeft -= milliseconds;
				// play the bark sound towards the end of the start sequence
				if (!round->barkFlag && round->updateTimeLeft <= 750)
				{
					_soundCB(bark);
					round->barkFlag = true;
				}
			}
			break;
		case waveStart:
			// spawn 2 ducks and start a fly away timer
			round->updateTimeLeft = _roundGetFlyAwayTime(round->roundNum);
			_makeDucksCB(round->roundNum);
			round->waveDucksHit = 0;
			_reloadCB();
			// update the round state
			round->roundState = wave;
			break;
		case wave:
			// if the timer is complete before both ducks are shot, show the fly away state
			if (round->waveDucksHit == 2)
			{
				round->roundState = duckWait;
			}
			else if (milliseconds >= round->updateTimeLeft)
			{
				round->roundState = flyAway;
				_flyAwayCB();
			}
			else
			{
				round->updateTimeLeft -= milliseconds;
			}
			break;
		case duckWait:
			// no break statement intentionally
		case flyAway:
			if (!(_roundDuckCheckCB()))
			{
				if (round->roundState == flyAway)
					_flyAwayOverCB();
				round->roundState = waveEnd;
			}
			break;
		case waveEnd:
			// play the apropriate sound based on whether the user hit any ducks
			// set the dog to behave properly for the wave end sequence
			if (round->waveDucksHit == 0)
			{
				_soundCB(laugh);
				round->dogLaughFrame = 0;
				round->dogLaughTimer = _dogFrameLength;
			}
			else
			{
				_soundCB(dogPopup);
			}
			round->roundState = waveEndSeq;
			round->obj.velocity.y = -_dogSpeed;
			break;
		case waveEndSeq:
			// update the dog position
			objDefaultUpdate(obj, milliseconds);
			// update the dog animation as needed
			// check the state of the dog and whether/how it has to move
			if (round->obj.position.y <= _dogTopy && round->obj.velocity.y == -_dogSpeed)
			{
				round->obj.velocity.y = 0.0f;
				round->obj.position.y = _dogTopy;
				round->updateTimeLeft = 500;
			}
			else if (round->obj.position.y == _dogTopy && round->obj.velocity.y == 0.0f)
			{
				if (milliseconds >= round->updateTimeLeft)
				{
					round->obj.velocity.y = _dogSpeed;
				}
				else
				{
					round->updateTimeLeft -= milliseconds;
				}
			}
			else if (round->obj.position.y >= _dogLowy && round->obj.velocity.y == _dogSpeed)
			{
				round->obj.position.y = _dogLowy;
				round->obj.velocity.y = 0.0f;
				// if this was the last wave, move to the round end. otherwise, move to next wave
				if (++(round->waveNum) == 5)
				{
					round->roundState = roundEnd;
				}
				else
				{
					round->roundState = waveStart;
					round->currentDuck = round->waveNum * 2;
				}
			}
			// if the dog is laughing, update the frame
			if (round->waveDucksHit == 0)
			{
				if (milliseconds >= round->dogLaughTimer)
				{
					round->dogLaughFrame = (round->dogLaughFrame) ? (0) : (1);
					round->dogLaughTimer = _dogFrameLength;

				}
				else
				{
					round->dogLaughTimer -= milliseconds;
				}
			}
			break;
		case roundEnd:
			// check if the user hit enough ducks to continue play
			if (round->ducksHit < round->requiredDucks)
			{
				round->roundState = lose;
				_soundCB(fail);
				round->updateTimeLeft = 2000;
			}
			else
			{
				round->roundState = win;
				_soundCB(roundClear);
				round->updateTimeLeft = 4500;
			}
			break;
		case win:
			if (milliseconds >= round->updateTimeLeft)
			{
				// check if the user had a perfect round
				if (round->ducksHit == 10)
				{
					round->roundState = perfect;
					_soundCB(perfSound);
					// update the score by the proper perfect bonus
					if (round->roundNum < 21)
					{
						if (round->roundNum < 16)
						{
							if (round->roundNum < 11)
							{
								_roundPerfCB(_perfectbonus[0]);
							}
							else
							{
								_roundPerfCB(_perfectbonus[1]);
							}
						}
						else
						{
							_roundPerfCB(_perfectbonus[2]);
						}
					}
					else
					{
						_roundPerfCB(_perfectbonus[3]);
					}
					round->updateTimeLeft = 3000;
				}
				else
				{
					round->roundState = start;
					++(round->roundNum);
				}
			}
			else
			{
				round->updateTimeLeft -= milliseconds;
			}
			break;
		case perfect:
			if (milliseconds >= round->updateTimeLeft)
			{
				++(round->roundNum);
				round->roundState = start;
			}
			else
			{
				round->updateTimeLeft -= milliseconds;
			}
			break;
		case lose:
			if (milliseconds >= round->updateTimeLeft)
			{
				round->roundState = ending;
				_soundCB(gameOver);
				round->updateTimeLeft = 4500;
				round->obj.velocity.y = -_dogSpeed;
				round->dogLaughFrame = 0;
				round->dogLaughTimer = _dogFrameLength;
			}
			else
			{
				round->updateTimeLeft -= milliseconds;
			}
			break;
		case ending:
			if (milliseconds >= round->updateTimeLeft)
			{
				_soundCB(menu);
				_loseCB();
			}
			else
			{
				round->updateTimeLeft -= milliseconds;
				// animate the dog laughing

				// update the dog position
				objDefaultUpdate(obj, milliseconds);
				if (round->obj.position.y <= _dogTopy)
				{
					round->obj.velocity.y = 0.0f;
					round->obj.position.y = _dogTopy;
				}
				if (milliseconds >= round->dogLaughTimer)
				{
					round->dogLaughFrame = (round->dogLaughFrame) ? (0) : (1);
					round->dogLaughTimer = _dogFrameLength;
				}
				else
				{
					round->dogLaughTimer -= milliseconds;
				}
			}
			break;
	}
}

/// @brief returns the current time for a fly away to trigger
/// @param roundNum
/// @return milliseconds
static uint32_t _roundGetFlyAwayTime(uint8_t roundNum)
{
	return (uint32_t)(((7.5f / (roundNum)) + 4.79f) * 1000);
}

static void _roundDraw(Object* obj)
{
	Round* round = (Round*)obj;
	if (round->roundState == inactive)
		return;

	Coord2D center = boundsGetCenter(&(round->bounds));
	
	GLfloat xPositionLeft;
	GLfloat xPositionRight;
	GLfloat yPositionTop;
	GLfloat yPositionBottom;
	GLfloat xTextureCoord;
	GLfloat yTextureCoord;
	const float UI_DEPTH = 0.75f;
	const float NUM_DEPTH = 0.76f;

	const float uPerNum = 1.0f / 11.0f;
	const float vPerNum = 1.0f / 2.0f;

	// draw the dog at the end of each round based on the number of ducks the user caught
	// also draw if the user has lost
	if (round->roundState == waveEndSeq || round->roundState == ending)
	{
		// if the user got no ducks, draw the laughing dog
		// otherwise draw the dog with the correct number of ducks
		// draw the round number
		// calculate the bounding box
		xPositionLeft = round->obj.position.x - (_dogSize.x / 2);
		xPositionRight = round->obj.position.x + (_dogSize.x / 2);
		yPositionTop = round->obj.position.y - (_dogSize.y / 2);
		yPositionBottom = round->obj.position.y + (_dogSize.y / 2);

		// find the proper sprite frame from the sprite sheet
		float uPerDog = 1.0f / 2.0f;
		float vPerDog = 1.0f / 2.0f;
		
		// calculate the starting uv... remember v of 0 is the bottom of the texture
		if (round->waveDucksHit > 0 && round->roundState != ending)
		{
			xTextureCoord = uPerDog;
			yTextureCoord = 1.0f / round->waveDucksHit;
		}
		else
		{
			xTextureCoord = 0;
			yTextureCoord = 1.0f - (round->dogLaughFrame * vPerDog);
		}

		const float DOG_DEPTH = -0.1f;

		drawSprite(_dogTexture, xPositionLeft, xPositionRight, yPositionTop, yPositionBottom,
				   uPerDog, vPerDog, xTextureCoord, yTextureCoord, DOG_DEPTH);
	}

	
	// draw the Ui elements
	// calculate the bounding box
	xPositionLeft = (center.x - uiSize.x / 2);
	xPositionRight = (center.x + uiSize.x / 2);
	yPositionTop = (center.y - uiSize.y / 2);
	yPositionBottom = (center.y + uiSize.y / 2);

	// find the proper sprite frame from the sprite sheet
	float uPerFrame = 1.0f;
	float startV = 1.0f;

	// calculate the starting uv... remember v of 0 is the bottom of the texture
	xTextureCoord = 0.0f;
	yTextureCoord = startV;

	const float BG_DEPTH = 0.0f;

	drawSprite(_interfaceTexture, xPositionLeft, xPositionRight, yPositionTop, yPositionBottom,
				uPerFrame, startV, xTextureCoord, yTextureCoord, BG_DEPTH);


	// draw the round number
	// calculate the bounding box
	xPositionLeft = roundNumPos.x;
	xPositionRight = roundNumPos.x + 34;
	yPositionTop = roundNumPos.y;
	yPositionBottom = roundNumPos.y + 34;

	

	uint8_t roundDigit = (round->roundNum >= 10) ? (round->roundNum / 10) : (round->roundNum);
	// calculate the starting uv... remember v of 0 is the bottom of the texture
	xTextureCoord = uPerNum * (roundDigit % 5);
	yTextureCoord = (roundDigit >= 5) ? vPerNum : 1;

	

	drawSprite(_numberTexture, xPositionLeft, xPositionRight, yPositionTop, yPositionBottom,
			   uPerNum, vPerNum, xTextureCoord, yTextureCoord, NUM_DEPTH);


	// draw the second round number
	// calculate the bounding box
	xPositionLeft = roundNumPos.x + 34;
	xPositionRight = roundNumPos.x + 68;
	yPositionTop = roundNumPos.y;
	yPositionBottom = roundNumPos.y + 34;


	// calculate the starting uv... remember v of 0 is the bottom of the texture
	if (round->roundNum >= 10)
	{
		xTextureCoord = uPerNum * ((round->roundNum % 10) % 5);
		yTextureCoord = (round->roundNum % 10 >= 5) ? vPerNum : 1;
	}
	else
	{
		xTextureCoord = uPerNum * 5;
		yTextureCoord = vPerNum;
	}

	drawSprite(_numberTexture, xPositionLeft, xPositionRight, yPositionTop, yPositionBottom,
			   uPerNum, vPerNum, xTextureCoord, yTextureCoord, NUM_DEPTH);

	// if a round is just beginning, display the round number popup
	if (round->roundState == startSeq || round->roundState == start)
	{
		// display the round number popup box
		// calculate the bounding box
		xPositionLeft = roundPopupPos.x;
		xPositionRight = roundPopupPos.x + 200;
		yPositionTop = roundPopupPos.y;
		yPositionBottom = roundPopupPos.y + 132;
		// find the proper sprite frame from the sprite sheet
		float uWidth = 50.0f / 130.0f;
		float v = 1.0f;

		xTextureCoord = 0.0f;
		yTextureCoord = 1.0f;

		drawSprite(_textBoxesTexture, xPositionLeft, xPositionRight, yPositionTop, yPositionBottom,
				   uWidth, v, xTextureCoord, yTextureCoord, UI_DEPTH);


		// first round number
		// calculate the bounding box
		xPositionLeft = roundPopupPos.x + 56;
		xPositionRight = xPositionLeft + 32;
		yPositionTop = roundPopupPos.y + 72;
		yPositionBottom = yPositionTop + 32;


		// calculate the starting uv... remember v of 0 is the bottom of the texture
		if (round->roundNum >= 10)
		{
			xTextureCoord = uPerNum * (((round->roundNum / 10) % 5) + 6);
			yTextureCoord = (round->roundNum / 10 >= 5) ? vPerNum : 1;
		}
		else
		{
			xTextureCoord = uPerNum * 5;
			yTextureCoord = 1;
		}

			
		drawSprite(_numberTexture, xPositionLeft, xPositionRight, yPositionTop, yPositionBottom,
				   uPerNum, vPerNum, xTextureCoord, yTextureCoord, NUM_DEPTH);


		// second round number
		// calculate the bounding box
		GLfloat xPositionLeft = roundPopupPos.x + 88;
		GLfloat xPositionRight = xPositionLeft + 32;
		GLfloat yPositionTop = roundPopupPos.y + 72;
		GLfloat yPositionBottom = yPositionTop + 32;


		// calculate the starting uv... remember v of 0 is the bottom of the texture
		xTextureCoord = uPerNum * ((round->roundNum % 5) + 6);
		yTextureCoord = (round->roundNum % 10 >= 5) ? vPerNum : 1;


		drawSprite(_numberTexture, xPositionLeft, xPositionRight, yPositionTop, yPositionBottom,
			uPerNum, vPerNum, xTextureCoord, yTextureCoord, NUM_DEPTH);

	}
	
	// if the round is in the fly away state, display the flyAway text box
	if (round->roundState == flyAway)
	{
		// calculate the bounding box
		GLfloat xPositionLeft = flyAwayPopupPos.x;
		GLfloat xPositionRight = flyAwayPopupPos.x + 250;
		GLfloat yPositionTop = flyAwayPopupPos.y;
		GLfloat yPositionBottom = flyAwayPopupPos.y + 132;
		GLfloat xTextureCoord = 0, yTextureCoord = 0;
		// find the proper sprite frame from the sprite sheet
		float uWidth = 80.0f / 130.0f;
		float v = 1.0f;

		xTextureCoord = 1 - uWidth;
		yTextureCoord = 1.0f;

		drawSprite(_textBoxesTexture, xPositionLeft, xPositionRight, yPositionTop, yPositionBottom,
					uWidth, v, xTextureCoord, yTextureCoord, UI_DEPTH);
	}

	uint8_t i;
	float leftx = duckUIpos.x;
	// draw the current hit ducks
	for (i = 0; i < 10; ++i)
	{
		if (round->duckStatus[i])
		{

			// calculate the bounding box
			xPositionLeft = leftx;
			xPositionRight = xPositionLeft + _cellSize.x;
			yPositionTop = duckUIpos.y;
			yPositionBottom = duckUIpos.y + _cellSize.y;
			xTextureCoord = 0, yTextureCoord = 0;
			// find the proper sprite frame from the sprite sheet
			float uWidth = 1.0f / 10.0f;
			float v = 1.0f / 2.0f;

			xTextureCoord = uWidth * i;
			yTextureCoord = 1.0f;
;

			drawSprite(_duckUITexture, xPositionLeft, xPositionRight, yPositionTop, yPositionBottom,
					   uWidth, v, xTextureCoord, yTextureCoord, UI_DEPTH);
		}
		leftx += _cellSize.x;
	}
	// draw the required ducks to hit
	leftx = duckUIpos.x;
	float topy = duckUIpos.y + _cellSize.y;
	for (i = 0; i < 10; ++i)
	{
		// calculate the bounding box
		xPositionLeft = leftx;
		xPositionRight = xPositionLeft + _cellSize.x;
		yPositionTop = topy;
		yPositionBottom = topy + _cellSize.y;
		xTextureCoord = 0, yTextureCoord = 0;
		// find the proper sprite frame from the sprite sheet
		float uWidth = 1.0f / 10.0f;
		float v = 1.0f / 2.0f;
		if (i < round->requiredDucks)
		{
			xTextureCoord = uWidth * i;
			yTextureCoord = v;
		}
		else
		{
			uWidth = 0.01f;
			v = 0.01f;
			xTextureCoord = 0;
			yTextureCoord = 0.99f;
		}

		drawSprite(_duckUITexture, xPositionLeft, xPositionRight, yPositionTop, yPositionBottom,
			uWidth, v, xTextureCoord, yTextureCoord, UI_DEPTH);

		leftx += _cellSize.x;
	}
}

