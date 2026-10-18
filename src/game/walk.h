// CWalkManager: the player's walk queue (one static instance at 0x00648848).
// Layout from decomp/pass2/0x00449490.cpp and the globals the other walk code
// reads (frame 0x00648858, total 0x0064885c, dirState 0x00648868, entry
// array 0x0064886c).
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"
#include "globals.h"

struct WalkEntry {          // 0x14 bytes
    uint8_t dir;            // +0x00  direction to face / walk
    uint8_t nextDir;        // +0x01  direction of the following step (compared with dir)
    uint8_t seq;            // +0x02  sequence number sent in the move request
    uint8_t pad_03;
    int32_t acked;          // +0x04  server acknowledged this step
    uint8_t dirState;       // +0x08  copied to CWalkManager::dirState when the step is committed
    uint8_t drops;          // +0x09  non-zero when this step changes z (fall check)
    uint8_t pad_0a[2];
    uint32_t tick;          // +0x0c  GetTickCount() when the request was sent
    int8_t z;               // +0x10
    uint8_t pad_11[3];
};
static_assert(sizeof(WalkEntry) == 0x14, "WalkEntry size");
static_assert(offsetof(WalkEntry, nextDir) == 0x01, "WalkEntry::nextDir");
static_assert(offsetof(WalkEntry, acked) == 0x04, "WalkEntry::acked");
static_assert(offsetof(WalkEntry, dirState) == 0x08, "WalkEntry::dirState");
static_assert(offsetof(WalkEntry, drops) == 0x09, "WalkEntry::drops");
static_assert(offsetof(WalkEntry, seq) == 0x02, "WalkEntry::seq");
static_assert(offsetof(WalkEntry, tick) == 0x0c, "WalkEntry::tick");
static_assert(offsetof(WalkEntry, z) == 0x10, "WalkEntry::z");

class CWalkManager {
public:
    int32_t count;          // +0x04  queued steps
    int32_t cur;            // +0x08  index of the current step
    int32_t nextSeq;        // +0x0c  next move sequence number (1..255)
    int32_t frame;          // +0x10  animation frame of the current step
    int32_t total;          // +0x14  frames walked in total
    int32_t blocked;        // +0x18  1 = walking is frozen until the server resyncs
    int32_t requireAck;     // +0x1c  steps must be acknowledged before they run
    int32_t dirState;       // +0x20
    // WalkEntry entries[] follows at +0x24 (capacity not known).

    WalkEntry *Entry(int i) { return reinterpret_cast<WalkEntry *>(reinterpret_cast<char *>(this) + 0x24) + i; }

    // 0x00449490  size 0x446  per-tick walk animation step.
    void __thiscall ProcessStep();

    // Third pass, walk_manager_ops.cpp (batch 17).
    // 0x00448580  constructor: the compiler's vptr (table 0x0049f728), all counters zero.
    CWalkManager();
    // The destructor only restores the vptr. Defined inline in walk_manager_ops.cpp so the scalar deleting
    // destructor (0x004485b0) expands it as the original's does.
    virtual ~CWalkManager();
    // 0x004485e0  zero the queue counters (count, cur, nextSeq, frame, total, dirState, blocked, requireAck).
    void __thiscall ClearCounters();

    // Second pass, walk_queue.cpp.
    // 0x00448e50  RET 8: validate / adjust the direction *dir for a step from the end of the queue; *outZ gets the new z.
    int __thiscall ValidateDirection(int *dir, int *outZ);   // 0 / 1 in all of EAX
    // 0x00449100  RET 8: RequestWalk when walking is not blocked and the queue is fully played.
    void __thiscall RequestWalkIdle(int dir, int run);
    // 0x00449130  RET 8: RequestWalk(dir, run key 0x007589f0) unconditionally, or only when idle if flags has 0x40000000.
    void __thiscall RequestWalkKey(int dir, uint32_t flags);
    // 0x00449180  RET 8: queue a step in direction dir (client numbering) and send the move request; run == -1 asks the mouse.
    void __thiscall RequestWalk(int dir, int run);
    // 0x004498e0  RET 4: the server acknowledged move sequence seq.
    void __thiscall HandleMoveAck(int seq);
    // 0x00449a20  RET 0x10: clear the queue and place the player (flag == -1 keeps the view position).
    void __thiscall Reset(int flag, int x, int y, int z);
    // 0x0044a610  RET 4: queue a step and treat it as acknowledged at once.
    void __thiscall RequestWalkForced(int dir);
};

static_assert(offsetof(CWalkManager, count) == 0x04, "CWalkManager::count");
static_assert(offsetof(CWalkManager, cur) == 0x08, "CWalkManager::cur");
static_assert(offsetof(CWalkManager, nextSeq) == 0x0c, "CWalkManager::nextSeq");
static_assert(offsetof(CWalkManager, blocked) == 0x18, "CWalkManager::blocked");
static_assert(offsetof(CWalkManager, frame) == 0x10, "CWalkManager::frame");
static_assert(offsetof(CWalkManager, total) == 0x14, "CWalkManager::total");
static_assert(offsetof(CWalkManager, requireAck) == 0x1c, "CWalkManager::requireAck");
static_assert(offsetof(CWalkManager, dirState) == 0x20, "CWalkManager::dirState");

#define g_walk GLOBAL_AS(CWalkManager, g_walk_storage)
