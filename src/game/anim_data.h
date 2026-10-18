// CAnimData: the animdata.mul table (art-tile animation records). The one
// instance lives at 0x005286b0 in the original image: 0x4000 records of
// 0x48 bytes, indexed by item graphic id. Layout from the loader at
// 0x00401000 and the per-tick update at 0x00401110.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"
#include "globals.h"

struct AnimRecord {
    uint8_t frames[0x40];   // +0x00  graphic offset per frame (signed in the file format)
    uint8_t curFrame;       // +0x40  current frame index (the file's unknown byte, initial value read from the file)
    uint8_t frameCount;     // +0x41
    uint8_t interval;       // +0x42  ticks between frames; 0 = not animated (8 until loaded)
    uint8_t startDelay;     // +0x43  added to the tick counter before the modulo test
    int32_t lastTick;       // +0x44  tick counter value of the last frame advance
};

static_assert(sizeof(AnimRecord) == 0x48, "AnimRecord size");
static_assert(offsetof(AnimRecord, curFrame) == 0x40, "AnimRecord::curFrame");
static_assert(offsetof(AnimRecord, frameCount) == 0x41, "AnimRecord::frameCount");
static_assert(offsetof(AnimRecord, interval) == 0x42, "AnimRecord::interval");
static_assert(offsetof(AnimRecord, startDelay) == 0x43, "AnimRecord::startDelay");
static_assert(offsetof(AnimRecord, lastTick) == 0x44, "AnimRecord::lastTick");

class CAnimData {
public:
    // 0x00401000  size 0x102  constructor: zero the table, set every interval to 8, then read
    // animdata.mul (groups of 8 records, each group preceded by a 4-byte header). Returns this.
    CAnimData *__thiscall Ctor();

    AnimRecord records[0x4000];
};

static_assert(sizeof(CAnimData) == 0x120000, "CAnimData size");

#define g_animData GLOBAL_AS(CAnimData, g_animData_storage)
