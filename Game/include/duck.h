#pragma once
#include "baseTypes.h"

typedef enum color_t Color;

typedef enum duck_state_t DuckState;

typedef struct duck_t Duck;

typedef void (*duckSoundCB)(soundIds);
typedef void (*duckScoreCB)(int32_t);
void duckSetCB(duckSoundCB soundcb);
void duckClearCB();


void duckInitTexture();
Duck* duckNew(Bounds2D bounds);
void duckDelete(Duck* duck);
void ducksFlyAway(Duck** ducks);
int32_t duckCheckForHit(Coord2D mousePos, Duck** ducks);
void ducksSetActive(uint8_t roundNum, Duck** ducks);
bool duckActiveStatus(Duck** ducks);