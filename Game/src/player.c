#include <Windows.h>
#include <gl/GLU.h>
#include <stdint.h>
#include <assert.h>

#include "player.h"
#include "Object.h"
#include "SOIL.h"
#include "baseTypes.h"
#include "input.h"
#include "draw.h"

static const char CURSOR[] = "asset/cursor.png";
static const char NUMBERS[] = "asset/numbers.png";

typedef struct player_t
{
    Object obj;

	bool inGame;
	uint32_t score;
	uint8_t bullets;
    Bounds2D bounds;
} Player;

static const Coord2D _numSize = {
	30.0f,
	35.0f
};
static const Coord2D _scorePos = {
	720.0f,
	887.0f
};
static const Coord2D size = {
	30.0f,
	30.0f
};
static const Coord2D _bullSize = {
	27.0f,
	35.0f
};
static const Coord2D _bullpos = {
	150.0f,
	887.0f
};

static GLuint _cursorTexture = 0;
static GLuint _numberTexture = 0;
// the object vtable for the player object
static void _playerUpdate(Object* obj, uint32_t milliseconds);
static void _playerDraw(Object* obj);
static ObjVtable _playerVtable = {
    _playerDraw,
    _playerUpdate
};

// load the cursor image
void playerInitTextures()
{
    if (_cursorTexture == 0)
    {
        _cursorTexture = SOIL_load_OGL_texture(CURSOR, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
            SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB);
        assert(_cursorTexture != 0);
    }
	if (_numberTexture == 0)
	{
		_numberTexture = SOIL_load_OGL_texture(NUMBERS, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
			SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB);
		assert(_numberTexture != 0);
	}

}

/// @brief Instantiate and initialize the player object
/// @param bounds 
/// @return player
Player* playerInit(Bounds2D bounds)
{
	Player* player = malloc(sizeof(Player));
	if (player != NULL)
	{
		Coord2D pos = boundsGetCenter(&bounds);
		Coord2D vel = { 0.0f, 0.0f };
		objInit(&player->obj, &_playerVtable, pos, vel);

		player->inGame = false;
		player->score = 0;
		player->bounds = bounds;
		player->bullets = 3;
	}
	return player;
}

/// @brief attempt to shoot if the player has a bullet 
/// @param player
/// @return true if shot successful, false otherwise
bool playerShoot(Player* player)
{
	if (player->bullets > 0)
	{
		--(player->bullets);
		return true;
	}
	return false;
}

/// @brief updates the player's score based on the given point value
/// @params player, duckScore
void playerUpScore(Player* player, uint32_t duckScore)
{
	player->score += duckScore;
}

/// @brief enable the player for the main game
/// @param player
void playerSetActive(Player* player)
{
	player->inGame = true;
	player->bullets = 3;
}

/// @brief reset the player after the game ends
/// @param player
void playerGameOver(Player* player)
{
	player->inGame = false;
	player->score = 0;
}

/// @brief reload the plaer to 3 bullets
/// @param player
void playerReload(Player* player)
{
	player->bullets = 3;
}

/// @brief Free up any resources pertaining to the player object
/// @param player
void playerDeInit(Player* player)
{
    objDeinit(&player->obj);

    free(player);
}

static void _playerUpdate(Object* obj, uint32_t milliseconds)
{
	obj->position = inputMousePosition();
}

static void _playerDraw(Object* obj)
{
	Player* player = (Player*)obj;
	int32_t i;

	GLfloat xPositionLeft;
	GLfloat xPositionRight;
	GLfloat yPositionTop;
	GLfloat yPositionBottom;

	GLfloat xTextureCoord;
	GLfloat yTextureCoord;
	// draw player UI elements
	if (player->inGame)
	{
		float leftx = _scorePos.x, topy = _scorePos.y;

		for (i = 100000; i > 0; i /= 10)
		{
			// calculate the bounding box
			xPositionLeft = leftx;
			xPositionRight = xPositionLeft + _numSize.x;
			yPositionTop = topy;
			yPositionBottom = yPositionTop + _numSize.y;

			// find the proper sprite frame from the sprite sheet
			float uPerNum = 1.0f / 11.0f;
			float vPerNum = 1.0f / 2.0f;

			uint32_t digit = (player->score / i) % 10;
			// calculate the starting uv... remember v of 0 is the bottom of the texture
			xTextureCoord = uPerNum * ((digit % 5) + 6);
			yTextureCoord = (digit >= 5) ? vPerNum : 1;

			const float UI_DEPTH = 0.76f;

			drawSprite(_numberTexture, xPositionLeft, xPositionRight, yPositionTop, yPositionBottom,
					   uPerNum, vPerNum, xTextureCoord, yTextureCoord, UI_DEPTH);
			leftx += _numSize.x;
		}
		// draw over bullets if needed
		leftx = _bullpos.x;
		topy = _bullpos.y;
		for (i = 0; i < (3 - player->bullets); ++i)
		{
			// calculate the bounding box
			xPositionLeft = leftx;
			xPositionRight = xPositionLeft + _bullSize.x;
			yPositionTop = topy;
			yPositionBottom = yPositionTop + _bullSize.y;

			// find the proper sprite frame from the sprite sheet
			float u = 0.01f;
			float v = 0.01f;

			// calculate the starting uv... remember v of 0 is the bottom of the texture
			xTextureCoord = 0.5f;
			yTextureCoord = 1.0f;

			const float UI_DEPTH = 0.76f;

			drawSprite(_numberTexture, xPositionLeft, xPositionRight, yPositionTop, yPositionBottom,
				u, v, xTextureCoord, yTextureCoord, UI_DEPTH);
			leftx -= _bullSize.x;
		}
	}

		// calculate the bounding box
		xPositionLeft = (obj->position.x - size.x / 2);
		xPositionRight = (obj->position.x + size.x / 2);
		yPositionTop = (obj->position.y - size.y / 2);
		yPositionBottom = (obj->position.y + size.y / 2);

		const float CURSOR_DEPTH = 1.0f;

		drawSprite(_cursorTexture, xPositionLeft, xPositionRight, yPositionTop, yPositionBottom,
				   1, 1, 0, 1, CURSOR_DEPTH);
}
