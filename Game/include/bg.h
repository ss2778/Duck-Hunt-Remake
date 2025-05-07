#pragma once
#include "baseTypes.h"

typedef enum bgState_t BgState;

typedef struct bg_t Bg;


void bgInitTexture();
Bg* bgInit(Bounds2D bounds);
void bgFlyAway(Bg* bg);
void bgGameBg(Bg* bg);
void bgMenuBg(Bg* bg);
void bgDeInit(Bg* bg);