// Walk queue, second pass (batch 18): the column probe that decides where the player can stand,
// direction validation, move requests, move acknowledgements, reset, pathfinding start.
// The SEH frames of the originals (none here) are not involved; stack buffers keep the original sizes.
#include "walk.h"

#include <cstdio>
#include <cstring>
#include <windows.h>

#include "../runtime/ported.h"
#include "globals.h"
#include "map_world.h"
#include "obj.h"
#include "crt.h"
#include "path_search.h"
#include "raw_field.h"
#include "vcall.h"
#include "world_renderer.h"
#include "walk_manager_ops_ports.h"
#include "packet_builders_batch24_ports.h"
#include "client_socket.h"
#include "client_misc_ops_ports.h"
#include "mouse_manager.h"
#include "dirty_rects_ports.h"
#include "map_gump_ops_ports.h"

#define wq_map GLOBAL_AS(CMap, g_map_storage)
#define wq_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)
#define wq_mouseManager GLOBAL_PTR(void, g_mouse_storage)
#define wq_lastWalkClock GLOBAL_AS(int, g_combatTimeB)      // set from 0x006f47d0 at every request
#define wq_backpackGump GLOBAL_AS(void *, g_openContainerGump)
#define wq_gumpTail GLOBAL_AS(void *, g_gumpListTail)
#define wq_pathObj GLOBAL_AS(CPathSearch, g_pathSearch_storage)

// One entry of the column list built by 0x004488d0 (12 bytes).
struct WqColEntry {
    uint16_t flags;     // +0x00  bit 0 stand-on surface, bit 1, bit 2 blocking volume
    int16_t bottom;     // +0x02
    int16_t top;        // +0x04
    int16_t height;     // +0x06
    void *obj;          // +0x08
};
static_assert(sizeof(WqColEntry) == 12, "WqColEntry size");

// 0x004488d0: collect the column entries of tile (x, y) into `out`; returns how many. ECX = out, EDX = x, RET 8.
// Not part of this batch.
#define wq_collectColumn(a1, a2, a3, a4) (Walk_CollectCell((WalkProbe *)(a1), a2, a3, a4))

// Packet 0x02 (move request) and 0x22 (resync) builders on the packet object, and the send call.
static inline void wq_buildMove(char *pkt, int dir, int seq)
{
    PORT_THISCALL3(&Build_Packet_0x02, void, wq_packetBuilder, char *, int, int)(pkt, dir, seq);   // Build_Packet_0x02
}
static void wq_buildResync(char *pkt)
{
    PORT_THISCALL1(&Build_Packet_0x22, void, wq_packetBuilder, char *)(pkt);   // Build_Packet_0x22
}
static inline void wq_send(char *pkt)
{
    reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(pkt));
}

// Wrap a tile position into the map; skipped for x >= 0x1400 (noWrap).
static void wq_wrap(int &x, int &y, int noWrap)
{
    if (noWrap == 0) {
        if (x < 0)
            x += 0x1400;
        if (x >= 0x1400)
            x -= 0x1400;
        if (y < 0)
            y += 0x1000;
        if (y >= 0x1000)
            y -= 0x1000;
    }
}

// 0x00448b80  size 0x102  ECX = int *floorZ, EDX = int *ceilZ, RET 0x14
// Probe the tile next to (x, y) behind direction dir (dir ^ 4): *floorZ gets the highest surface at or below z,
// *ceilZ the blocking height above it (+2).
static void __fastcall Walk_ColumnProbe(int *floorZ, int *ceilZ, int x, int y, int z, unsigned dir, int mode)
{
    WqColEntry col[0x100];   // original: 0xc04 bytes of locals
    dir &= 7;
    int dx = g_dirDx[dir ^ 4];
    int dy = g_dirDy[dir ^ 4];
    *floorZ = -0x80;
    *ceilZ = z;
    int n = wq_collectColumn(col, x + dx, y + dy, mode);
    for (WqColEntry *e = col; e != col + n; e++) {
        int top = e->top;
        if (e->obj != nullptr && decomp::vcall(PORT_RET(int), e->obj, 0x1c) != 0 && top <= z) {
            int h = static_cast<int16_t>(decomp::vcall(PORT_RET(int), e->obj, 0x4c, static_cast<int>(dir)));
            if (*floorZ < h)
                *floorZ = h;
            if (*ceilZ < h)
                *ceilZ = h;
        } else {
            unsigned fl = e->flags;
            if ((fl & 1) != 0 && top <= z && *floorZ < top)
                *floorZ = top;
            if ((fl & 4) != 0 && z == top) {
                int bottom = e->bottom;
                int ceil = e->height + bottom;
                if (*ceilZ < ceil)
                    *ceilZ = ceil;
                if (*floorZ > bottom)
                    *floorZ = bottom;
            }
        }
    }
    *ceilZ += 2;
}
HOOK_FUNC(0x00448b80, Walk_ColumnProbe);

// 0x00448c90  size 0x1b5  ECX = x, EDX = y, RET 0xc
// Can the player step onto tile (x, y) from height z in direction dir? On success *outZ is the standing height.
int __fastcall Walk_FindStandZ(int x, int y, int z, int dir, int *outZ)
{
    WqColEntry col[0x102];   // the original has 0x100 entries (frame 0xc14) and its sentinel store can land one entry past them; kept two larger until the function matches
    int floorZ;              // ebp-4
    int ceilZ;               // ebp-0x10
    int mode = 0;
    if (decomp::vcall(PORT_RET(int), g_player, 0x8c) != 0)
        mode = 1;
    if (Misc_Return0() == 0 && RAWF(uint16_t, g_player, 0x38) == 0x3db)   // ? 0x00478cd0 takes no arguments and returns 0
        mode = 1;
    Walk_ColumnProbe(&floorZ, &ceilZ, x, y, z, static_cast<unsigned>(dir), mode);
    int n = wq_collectColumn(col, x, y, mode);
    qsort(col, static_cast<unsigned>(n), 12, CompareColumnEntries);
    WqColEntry *sentinel = &col[n];
    sentinel->flags = 1;
    sentinel->bottom = 0x80;
    sentinel->top = 0x80;
    sentinel->height = 0x80;
    sentinel->obj = nullptr;
    n++;
    if (z < floorZ)
        z = floorZ;
    dir = floorZ;   // dir is dead from here: the original keeps the running step height in its slot
    *outZ = -0x80;
    int best = 1000000;
    int maxTop = -0x80;
    for (WqColEntry *p = col; p != col + n; p++) {
        if ((p->flags & 1) == 0)
            continue;
        int pz = p->bottom;
        if (pz - dir >= 0x10) {
            for (WqColEntry *q = p - 1; q >= col; q--) {
                uint16_t qf = q->flags;
                if ((qf & 6) == 0)
                    continue;
                int qt = q->top;
                if (qt < maxTop)
                    continue;
                if (pz - qt < 0x10)
                    continue;
                if ((qt <= ceilZ && (qf & 2) != 0) || ((qf & 4) != 0 && q->bottom <= ceilZ)) {
                    int d = z - qt;
                    if (d < 0)
                        d = -d;
                    if (d < best) {
                        best = d;
                        *outZ = qt;
                    }
                }
            }
        }
        int pt = p->top;
        if (dir < pt)
            dir = pt;
        if (maxTop < pt)
            maxTop = pt;
    }
    return *outZ != -0x80;
}
HOOK_FUNC(0x00448c90, Walk_FindStandZ);

// 0x00448e50  size 0x2a6
// Walk the queue to find where the player will be after the queued steps, then check the step in *dir
// (and, for diagonals, the two neighbouring directions). May change *dir; *outZ gets the new z.
int __thiscall CWalkManager::ValidateDirection(int *dir, int *outZ)
{
    int altZ = 0;       // ebp-0x18
    int tmpZ;           // ebp-8
    int fail;           // ebp-0xc
    int n = count;
    int facing;
    if (n == 0)
        facing = g_player->facing & 7;
    else
        facing = Entry(n - 1)->dir;
    int px = wq_map.viewX;
    int pz = wq_map.viewZ;
    int py = wq_map.viewY;
    int noWrap = (px >= 0x1400) ? 1 : 0;
    for (int i = 0; i < n; i++) {
        WalkEntry *e = Entry(i);
        if (e->acked == 0 && e->dir == e->nextDir) {
            px += g_dirDx[e->dir];
            py += g_dirDy[e->dir];
            pz = e->z;
        }
    }

    int d = *dir;
    if ((d & 1) == 0) {
        int x = g_dirDx[d] + px;
        int y = g_dirDy[d] + py;
        wq_wrap(x, y, noWrap);
        if (Walk_FindStandZ(x, y, pz, d, outZ))
            return true;
        return *dir != facing;
    }

    int x = g_dirDx[d] + px;
    int y = g_dirDy[d] + py;
    wq_wrap(x, y, noWrap);
    fail = 0;
    if (!Walk_FindStandZ(x, y, pz, d, outZ))
        fail = 1;

    int cand = *dir;    // ebx
    x = g_dirDx[cand - 1] + px;
    y = g_dirDy[cand - 1] + py;
    wq_wrap(x, y, noWrap);
    if (!Walk_FindStandZ(x, y, pz, (cand - 1) & 7, &tmpZ)) {
        fail = 1;
    } else {
        altZ = tmpZ;
        cand = *dir - 1;
    }

    d = *dir;
    x = g_dirDx[d + 1] + px;
    y = g_dirDy[d + 1] + py;
    wq_wrap(x, y, noWrap);
    int resultZ;        // edx
    int failed;         // ecx
    if (!Walk_FindStandZ(x, y, pz, (d + 1) & 7, &tmpZ)) {
        resultZ = altZ;
        failed = 1;
    } else {
        resultZ = tmpZ;
        failed = fail;
        cand = (*dir + 1) & 7;
    }
    if (failed == 0)
        return true;
    if (cand == *dir)
        return *dir != facing;
    *dir = cand;
    *outZ = resultZ;
    return true;
}
HOOK_METHOD(0x00448e50, CWalkManager, ValidateDirection);

// 0x00449100  size 0x2a
void __thiscall CWalkManager::RequestWalkIdle(int dir, int run)
{
    if (blocked == 0 && g_walk.cur == g_walk.count)
        RequestWalk(dir, run);
}
HOOK_METHOD(0x00449100, CWalkManager, RequestWalkIdle);

// 0x00449130  size 0x4e
void __thiscall CWalkManager::RequestWalkKey(int dir, uint32_t flags)
{
    if ((flags & 0x40000000) != 0) {
        int run = wq_runKey;
        if (blocked == 0 && g_walk.cur == g_walk.count)
            RequestWalk(dir, run);
    } else {
        RequestWalk(dir, wq_runKey);
    }
}
HOOK_METHOD(0x00449130, CWalkManager, RequestWalkKey);

// 0x00449180  size 0x1ca
// Queue a step (dir is the client numbering, converted with (dir - 1) & 7), send the move request packet and
// drop the gumps of objects that are no longer reachable.
void __thiscall CWalkManager::RequestWalk(int dir, int run)
{
    char pkt[4];
    wq_lastWalkClock = g_combatTimeA;
    if (blocked == 1)
        return;
    dir = (dir - 1) & 7;
    if (count == 4)
        return;
    if (g_pktSender == nullptr)
        return;
    int newZ;
    if (!ValidateDirection(&dir, &newZ))
        return;

    Entry(count)->seq = static_cast<uint8_t>(nextSeq);
    Entry(count)->acked = 0;
    int r = run;
    if (run == -1)
        r = (reinterpret_cast<CMouseManager *>(wq_mouseManager)->IsNearScreenCenter() == 0) ? 1 : 0;   // IsNearScreenCenter
    Entry(count)->dirState = static_cast<uint8_t>(r);
    if (count == 0)
        Entry(0)->nextDir = g_player->facing & 7;
    else
        Entry(count)->nextDir = Entry(count - 1)->dir;
    int now = GetTickCount();
    Entry(count)->tick = now;
    Entry(count)->z = static_cast<int8_t>(newZ);
    WalkEntry *e = Entry(count);
    int playerZ = g_player->z;
    if (static_cast<unsigned>(dir) == e->nextDir && playerZ - newZ >= 0xb)
        e->drops = 1;
    else
        e->drops = 0;
    Entry(count)->dir = static_cast<uint8_t>(dir);
    count = count + 1;
    if (Entry(count - 1)->dirState != 0)
        dir |= 0x80;
    wq_buildMove(pkt, dir, static_cast<uint8_t>(nextSeq));
    wq_send(pkt);
    nextSeq = (nextSeq + 1) & 0xff;
    if (nextSeq == 0)
        nextSeq = 1;

    if (wq_backpackGump != nullptr) {
        decomp::vdelete(wq_backpackGump);
        wq_backpackGump = nullptr;
    }
    void *g = wq_gumpTail;
    while (g != nullptr) {
        void *prev = RAWF(void *, g, 0x3c);
        if (decomp::vcall(PORT_RET(int), g, 0x90) != 0) {
            CDynamicObj *owner = RAWF(CDynamicObj *, g, 0x34);
            if (owner != nullptr && owner->IsReachable() == 0)
                decomp::vdelete(g);
        }
        g = prev;
    }
}
HOOK_METHOD(0x00449180, CWalkManager, RequestWalk);

// 0x00449350  size 0x118  ECX = direction, EDX = step number
// Set the screen scroll offsets (0x0069dda4 / 0x0069dda8) for step n of the walk animation in direction dir.
void __fastcall Walk_ScrollOffset(unsigned dir, int n)
{
    switch (dir) {
    case 0:
        g_viewOffsetX = (n * 11 * 2) / wq_stepFrames;
        g_viewOffsetY = (-(n * 11) * 2) / wq_stepFrames;
        break;
    case 1:
        g_viewOffsetX = (n * 11 * 4) / wq_stepFrames;
        g_viewOffsetY = 0;
        break;
    case 2:
        g_viewOffsetX = (n * 11 * 2) / wq_stepFrames;
        g_viewOffsetY = g_viewOffsetX;
        break;
    case 3:
        g_viewOffsetX = 0;
        g_viewOffsetY = (n * 11 * 4) / wq_stepFrames;
        break;
    case 4:
        g_viewOffsetX = (-(n * 11) * 2) / wq_stepFrames;
        g_viewOffsetY = (n * 11 * 2) / wq_stepFrames;
        break;
    case 5:
        g_viewOffsetX = (-(n * 11) * 4) / wq_stepFrames;
        g_viewOffsetY = 0;
        break;
    case 6:
        g_viewOffsetX = (-(n * 11) * 2) / wq_stepFrames;
        g_viewOffsetY = g_viewOffsetX;
        break;
    case 7:
        g_viewOffsetX = 0;
        g_viewOffsetY = (-(n * 11) * 4) / wq_stepFrames;
        break;
    default:
        break;
    }
}
HOOK_FUNC(0x00449350, Walk_ScrollOffset);

// 0x004498e0  size 0x13d
// The server confirmed move sequence `seq`: update the latency average, apply the step to the map and either
// drop the finished entry or mark it acknowledged. An unknown sequence resyncs (packet 0x22) and clears the queue.
void __thiscall CWalkManager::HandleMoveAck(int seq)
{
    char pkt[8];
    int n = count;
    int i = 0;
    if (n > 0) {
        for (;;) {
            if (Entry(i)->seq == seq)
                break;
            i++;
            if (i >= n)
                break;
        }
    }
    if (i == n) {
        wq_buildResync(pkt);
        wq_send(pkt);
        blocked = 1;
        cur = 0;
        count = 0;
        return;
    }
    WalkEntry *e = Entry(i);
    int elapsed = static_cast<int>(GetTickCount() - e->tick);
    if (elapsed >= 0 && elapsed < 0xea60)
        wq_ackLatency = (static_cast<uint32_t>(elapsed) + wq_ackLatency) >> 1;
    unsigned a = e->dir;
    unsigned b = e->nextDir;
    if (a == b)
        wq_map.Move(g_dirDx[a], g_dirDy[a], e->z);
    if (i < cur) {
        if (i != 0) {
            wq_buildResync(pkt);
            wq_send(pkt);
            blocked = 1;
            cur = 0;
            count = 0;
            return;
        }
        if (count > 1) {
            WalkEntry *dst = Entry(0);
            int j = 1;
            do {
                memcpy(dst, dst + 1, sizeof(WalkEntry));   // rep movsd, 5 dwords
                j++;
                dst++;
            } while (j < count);
        }
        count = count - 1;
        cur = cur - 1;
        return;
    }
    Entry(i)->acked = 1;
}
HOOK_METHOD(0x004498e0, CWalkManager, HandleMoveAck);

// 0x00449a20  size 0xed
// Clear the walk queue and move the player. flag == -1: put the player on the current view position;
// otherwise set the map view to (x, y, z) first.
void __thiscall CWalkManager::Reset(int flag, int x, int y, int z)
{
    decomp::vcall_void(g_player, 0x14);
    frame = 0;
    nextSeq = 0;
    count = 0;
    cur = 0;
    g_viewOffsetX = 0;
    g_viewOffsetY = 0;
    blocked = 0;
    if (flag == -1) {
        g_player->x = static_cast<int16_t>(wq_map.viewX);
        g_player->y = static_cast<int16_t>(wq_map.viewY);
        decomp::vcall_void(g_player, 0x10);
        InvalidateAll();   // InvalidateAll
        decomp::vcall_void(g_player, 0x88);
        g_worldRenderer.UpdateRoofHideLimit();
        return;
    }
    wq_map.SetPos(x, y, z, wq_map.mapId);
    g_player->x = static_cast<int16_t>(x);
    g_player->y = static_cast<int16_t>(y);
    g_player->z = static_cast<int16_t>(z);
    decomp::vcall_void(g_player, 0x10);
    InvalidateAll();   // InvalidateAll
    decomp::vcall_void(g_player, 0x88);
    g_worldRenderer.UpdateRoofHideLimit();
}
HOOK_METHOD(0x00449a20, CWalkManager, Reset);

// 0x00449b10  size 0x19e  ECX = x, EDX = y, stack z, RET 4
// Throttled (every 20th call) pathfinding request: run the A* search from the view position to (x, y, z),
// remember the end of the node chain and announce it as a bark.
void __fastcall Walk_PathfindTo(uint16_t x16, uint16_t y16, uint32_t z32)
{
    char text[0x200];
    int guard = wq_pathThrottle - 1;
    wq_pathThrottle = guard;
    if (guard > 1)
        return;
    int z = z32 & 0xffff;
    int y = y16;
    int x = x16;
    wq_pathThrottle = 0x14;
    sprintf(text, "%d %d %d", x, y, z);   // 0x0048b1d0
    g_player->CreateBarkKeepTwo(text, 0, 0, nullptr);

    // The result of this loop (the player position after the queued steps) is not used by the original.
    {
        int qx = wq_map.viewX;
        int qy = wq_map.viewY;
        WalkEntry *e = g_walk.Entry(0);
        for (int left = g_walk.count; left > 0; left--, e++) {
            if (e->acked == 0 && e->dir == e->nextDir) {
                qx += g_dirDx[e->dir];
                qy += g_dirDy[e->dir];
            }
        }
        (void)qx;
        (void)qy;
    }

    PathNode *node = wq_pathObj.Run(wq_map.viewX, wq_map.viewY, wq_map.viewZ, x, y, z, 0x1f4, 1);
    g_pathCurrentNode = node;
    if (node == nullptr) {
        g_pathActive = 0;
        return;
    }
    g_pathActive = 1;
    PathReverseLinks();   // link the chain from the current node: each parent's +0x28 points back
    while (node->parent != nullptr)
        node = node->parent;
    g_pathCurrentNode = node;
    g_pathX = node->x;
    g_pathY = node->y;
    g_pathZ = node->z;
    g_pathFacing = g_player->facing;
    g_pathStartNode = node;
    sprintf(text, "Pathfinding to %d, %d, %d.", x, y, z);   // 0x0048b1d0
    g_player->CreateBarkKeepTwo(text, 0, 0, nullptr);
}
HOOK_FUNC(0x00449b10, Walk_PathfindTo);

// 0x0044a5f0  size 0x1e  ECX = direction byte
// Forced walk request on the global walk manager.
void __fastcall Walk_ForcedThunk(uint8_t dir)
{
    g_walk.RequestWalkForced(dir);
}
HOOK_FUNC(0x0044a5f0, Walk_ForcedThunk);

// 0x0044a610  size 0x267
// Like RequestWalk but dir is used as given and no packet is sent for the step: the step is queued and
// handled at once as if the server had acknowledged it (the HandleMoveAck path).
void __thiscall CWalkManager::RequestWalkForced(int dir)
{
    int newZ;
    char pkt[4];
    wq_lastWalkClock = g_combatTimeA;
    if (blocked == 1)
        return;
    if (count == 4) {
        wq_buildResync(pkt);
        wq_send(pkt);
        blocked = 1;
        cur = 0;
        count = 0;
    }
    if (g_pktSender == nullptr)
        return;
    if (!ValidateDirection(&dir, &newZ))
        return;

    Entry(count)->seq = static_cast<uint8_t>(nextSeq);
    Entry(count)->acked = 0;
    int nearCenter = reinterpret_cast<CMouseManager *>(wq_mouseManager)->IsNearScreenCenter();   // IsNearScreenCenter
    Entry(count)->dirState = (nearCenter == 0) ? 1 : 0;
    if (count == 0)
        Entry(0)->nextDir = g_player->facing & 7;
    else
        Entry(count)->nextDir = Entry(count - 1)->dir;
    Entry(count)->tick = GetTickCount();
    Entry(count)->z = static_cast<int8_t>(newZ);
    int playerZ = g_player->z;
    WalkEntry *e = Entry(count);
    if (static_cast<unsigned>(dir) == e->nextDir && playerZ - newZ >= 0xb)
        e->drops = 1;
    else
        e->drops = 0;
    Entry(count)->dir = static_cast<uint8_t>(dir);
    count = count + 1;
    int sentSeq = nextSeq;
    if (Entry(count - 1)->dirState != 0)
        dir |= 0x80;
    nextSeq = (sentSeq + 1) & 0xff;
    if (nextSeq == 0)
        nextSeq = 1;

    int i = 0;
    if (count > 0) {
        for (;;) {
            if (Entry(i)->seq == sentSeq)
                break;
            i++;
            if (i >= count)
                break;
        }
    }
    if (i == count) {
        wq_buildResync(pkt);
    } else {
        WalkEntry *f = Entry(i);
        unsigned d = f->dir;
        dir = static_cast<int>(d);
        if (d == f->nextDir)
            wq_map.Move(g_dirDx[d], g_dirDy[d], f->z);
        if (i >= cur) {
            Entry(i)->acked = 1;
            return;
        }
        if (i == 0) {
            if (count > 1) {
                WalkEntry *dst = Entry(0);
                int j = 1;
                do {
                    memcpy(dst, dst + 1, sizeof(WalkEntry));   // rep movsd, 5 dwords
                    j++;
                    dst++;
                } while (j < count);
            }
            count = count - 1;
            cur = cur - 1;
            return;
        }
        wq_buildResync(pkt);
    }
    wq_send(pkt);
    blocked = 1;
    cur = 0;
    count = 0;
}
HOOK_METHOD(0x0044a610, CWalkManager, RequestWalkForced);
