#pragma once
#include "Object.h"

typedef enum roundState_t State;

typedef struct roundmgr_t Round;

typedef void (*roundMakeCB)(uint8_t);
typedef void (*roundPerfCB)(uint32_t);
typedef void (*roundNoArgCB)();
typedef bool (*roundBoolCB)();
typedef void (*roundSoundCB)(soundIds);
void roundSetCBs(roundSoundCB soundcb, roundNoArgCB flyAwaycb, roundMakeCB makeDuckscb, roundNoArgCB losecb,
				 roundBoolCB duckCheckcb, roundNoArgCB flyAwayOvercb, roundPerfCB perfcb, roundNoArgCB reloadcb);
void roundClearCBs();
void roundSetActive(Round* round);
void roundSetInctive(Round* round);
void roundInitTextures();
Round* roundInit(Bounds2D bounds);
void roundDuckHit(Round* round);
State roundGetState(Round* round);
void roundDeInit(Round* round);