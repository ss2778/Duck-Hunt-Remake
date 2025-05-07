#include <Windows.h>
#include <stdlib.h>
#include <gl/GLU.h>
#include <assert.h>
#include <time.h>

#include "baseTypes.h"
#include "levelmgr.h"
#include "roundmgr.h"
#include "field.h"
#include "duck.h"
#include "bg.h"
#include "player.h"
#include "globals.h"
#include "objmgr.h"
#include "SOIL.h"
#include "sound.h"


typedef struct level_t
{
    const LevelDef* def;

    Field* field;
    levelState state;
    Player* player;
    Round* round;
    Bg* bg;
    Duck** ducks;
} Level;

static Level* level = NULL;

static int32_t _soundId[numSounds];

// sound strings
static const char _soundNames[numSounds][50] = {
    "asset/sfx/wingFlap.wav",
    "asset/sfx/quack.wav",
    "asset/sfx/duckFall.wav",
    "asset/sfx/duckThud.wav",
    "asset/sfx/menu.wav",
    "asset/sfx/gameStart.wav",
    "asset/sfx/threeBark.wav",
    "asset/sfx/dogHoldingDuck.wav",
    "asset/sfx/laugh.wav",
    "asset/sfx/roundClear.wav",
    "asset/sfx/fail.wav",
    "asset/sfx/gameOver.wav",
    "asset/sfx/perfect.wav",
    "asset/sfx/gun.wav"
};

// local function prototypes
static void _levelMgrActiveDucks(uint8_t roundNum);
static void _levelMgrPlaySound(soundIds id);
static void _levelMgrFlyAway();
static void _levelMgrFlyAwayOver();
static bool _levelMgrCheckDucks();
static void _levelMgrEndGame();
static void _levelMgrPerfect(uint32_t score);
static void _levelMgrReload();

/// @brief Initialize the level manager
void levelMgrInit()
{
    int32_t i;
    duckInitTexture();
    playerInitTextures();
    bgInitTexture();
    roundInitTextures();
    srand((int32_t)time(NULL));
    roundSetCBs(_levelMgrPlaySound, _levelMgrFlyAway, _levelMgrActiveDucks, _levelMgrEndGame,
                _levelMgrCheckDucks, _levelMgrFlyAwayOver, _levelMgrPerfect, _levelMgrReload);
    duckSetCB(_levelMgrPlaySound);

    // load sounds
    for (i = 0; i < numSounds; ++i)
        _soundId[i] = SOUND_NOSOUND;
    for (i = 0; i < numSounds; ++i)
        _soundId[i] = soundLoad(_soundNames[i]);
    for (i = 0; i < numSounds; ++i)
        assert(_soundId[i] != SOUND_NOSOUND);
}

/// @brief Shutdown the level manager
void levelMgrShutdown()
{
    int32_t i;

    for (i = 0; i < numSounds; ++i)
        soundUnload(_soundId[i]);
}

/// @brief Loads the level and all required objects/assets
/// @param levelDef 
/// @return level
Level* levelMgrLoad(const LevelDef* levelDef)
{
    level = malloc(sizeof(Level));
    if (level != NULL)
    {
        level->def = levelDef;

        // the field provides the boundaries of the scene & encloses the ducks
        level->field = fieldNew(levelDef->fieldBounds, levelDef->fieldColor);
        
        // initialize the UI
        level->bg = bgInit(levelDef->fieldBounds);

        // initialize the ducks
        level->ducks = malloc(levelDef->numDucks * sizeof(Duck*));
        if (level->ducks != NULL)
        {
            for (uint32_t i = 0; i < levelDef->numDucks; ++i)
            {
                level->ducks[i] = duckNew(levelDef->fieldBounds);
            }
        }
        // set initial game state
        level->state = menuScreen;
        // initialize the round manager
        level->round = roundInit(levelDef->fieldBounds);
        // initialize the player
        level->player = playerInit(levelDef->fieldBounds);
        // play startup sound
        _levelMgrPlaySound(menu);
    }
    return level;
}

/// @brief Processes a click based on the current location of the mouse
/// @param pos, level
void processClick(Coord2D pos)
{
    State roundState = roundGetState(level->round);
    // check if the plater is currently allowed to shoot
    if (roundState != (State)4 && roundState != (State)5)
        return;
    // check if the player has a bullet to shoot
    if (!playerShoot(level->player))
        return;
    _levelMgrPlaySound(gun);
    int32_t hitScore = duckCheckForHit(pos, level->ducks);
    if (hitScore != 0)
    {
        playerUpScore(level->player, hitScore);
        // tell roundmgr that a duck was hit
        roundDuckHit(level->round);
    }
    
}

/// @brief Getter for the level state
/// @param level
/// @return level->state
levelState levelMgrGetState()
{
    return level->state;
}

/// @brief Begin the main game
void levelMgrStartGame()
{
    // change the level state
    level->state = gameScreen;
    // set game background
    bgGameBg(level->bg);
    // enable the round manager
    roundSetActive(level->round);
    // enable the player score display
    playerSetActive(level->player);
}

/// @brief Unloads the level and frees up any assets associated
/// @param level 
void levelMgrUnload(Level* level)
{
    if (level != NULL) 
    {
        for (uint32_t i = 0; i < level->def->numDucks; ++i)
        {
            duckDelete(level->ducks[i]);
        }
        free(level->ducks);
        roundDeInit(level->round);
        playerDeInit(level->player);
        bgDeInit(level->bg);
        fieldDelete(level->field);
        roundClearCBs();
        duckClearCB();

    }
    free(level);
}

static void _levelMgrActiveDucks(uint8_t roundNum)
{
    ducksSetActive(roundNum, level->ducks);
}

static void _levelMgrPlaySound(soundIds id)
{
    soundPlay(_soundId[id]);
}

static void _levelMgrFlyAway()
{
    // set all the ducks to the fly away state
    ducksFlyAway(level->ducks);
    // change background color
    bgFlyAway(level->bg);
}

static void _levelMgrFlyAwayOver()
{
    bgGameBg(level->bg);
}

static bool _levelMgrCheckDucks()
{
    return duckActiveStatus(level->ducks);
}

static void _levelMgrEndGame()
{
    // change the level state
    level->state = menuScreen;
    // set menu background
    bgMenuBg(level->bg);
    // disable the round manager
    roundSetInctive(level->round);
    // reset the player
    playerGameOver(level->player);
}

static void _levelMgrPerfect(uint32_t score)
{
    playerUpScore(level->player, score);
}

static void _levelMgrReload()
{
    playerReload(level->player);
}