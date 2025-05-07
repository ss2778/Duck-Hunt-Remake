#pragma once
#include "baseTypes.h"

typedef struct player_t Player;


void playerInitTextures();
Player* playerInit(Bounds2D bounds);
bool playerShoot(Player* player);
void playerUpScore(Player* player, uint32_t duckScore);
void playerSetActive(Player* player);
void playerGameOver(Player* player);
void playerReload(Player* player);
void playerDeInit(Player* player);