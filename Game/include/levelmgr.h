#pragma once
#include "baseTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum levelStates_t
{
        menuScreen,
        gameScreen
}levelState;

typedef struct leveldef_t {
    Bounds2D fieldBounds;
    uint32_t fieldColor;
    uint32_t numDucks;
} LevelDef;

typedef struct level_t Level;

void levelMgrInit();
void levelMgrShutdown();
Level *levelMgrLoad(const LevelDef* levelDef);
void processClick(Coord2D pos);
levelState levelMgrGetState();
void levelMgrStartGame();
void levelMgrUnload(Level* level);

#ifdef __cplusplus
}
#endif