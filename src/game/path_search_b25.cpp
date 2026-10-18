// A* search helpers, second pass (batch 25): constructor, stack free,
// the diagonal step check and the node expansion.
// 0x004660b0, 0x00466100, 0x004662f0, 0x00466390.
//
// The class CPathSearch is declared in path_search.h (not edited here), so the
// methods below are __fastcall free functions: `self` arrives in ECX and the
// callee cleans the same stack bytes as the __thiscall original.
#include "path_search.h"

#include "../runtime/ported.h"
#include "crt.h"
#include "window_helpers_31_ports.h"
#include "walk_queue_ports.h"
#include "globals.h"

// 0x00482c70 out-of-memory box: ECX = 0x004adcfc, EDX = 0x004add18, stack 0 (RET 4).
static inline void b25_OutOfMemory()
{
    ShowMessageBoxAndExit((const char *)(GLOBAL_PTR(void, g_004adcfc_storage)), (const char *)(GLOBAL_PTR(void, g_004add18_storage)), 0);
}


// 0x00466450 CPathSearch::ConsiderNeighbour (ported in path_search.cpp).

// Direction deltas (8 entries of int each); the diagonal check reads index dir - 1 unmasked.
#define b25_dirDx(i) (g_dirDx[(i)])
#define b25_dirDy(i) (g_dirDy[(i)])

// ---------------------------------------------------------------------------
// 0x004660b0  size 0x44
// Constructor: zero the fields at +0x14, +0, +4, +8, calloc(1, 8) into +0; out-of-memory box on failure.
// Returns this (EAX).
// ---------------------------------------------------------------------------
CPathSearch *__fastcall AStarSearch_Ctor(CPathSearch *self)
{
    reinterpret_cast<int32_t *>(self)[5] = 0;
    reinterpret_cast<void **>(self)[0] = nullptr;
    self->open = nullptr;
    self->closed = nullptr;
    void *p = calloc(1, 8);   // 0x0048c820
    reinterpret_cast<void **>(self)[0] = p;
    if (p == nullptr)
        b25_OutOfMemory();   // 0x00482c70
    return self;
}
HOOK_FUNC(0x004660b0, AStarSearch_Ctor);

// ---------------------------------------------------------------------------
// 0x00466100  size 0x1a
// Free the block at +0 (CRT free 0x0048c8d0) and clear the pointer.
// ---------------------------------------------------------------------------
void __fastcall AStarSearch_FreeStack(CPathSearch *self)
{
    void *&p = reinterpret_cast<void **>(self)[0];
    if (p != nullptr) {
        free(p);   // 0x0048c8d0
        p = nullptr;
    }
}
HOOK_FUNC(0x00466100, AStarSearch_FreeStack);

// ---------------------------------------------------------------------------
// 0x004662f0  size 0x95
// ECX = x, EDX = y, stack (z, dir, outZ), RET 0xc. Diagonal step check: the tile in direction
// `dir`, then the tile in direction dir - 1 (not masked, so dir 0 reads before the delta tables),
// then the one in (dir + 1) & 7. Returns 0 at the first that fails, else the result of the third.
// ---------------------------------------------------------------------------
static int __fastcall AStarSearch_StepCheck(int x, int y, int z, int dir, int *outZ)
{
    if (!Walk_FindStandZ(x + b25_dirDx(dir), y + b25_dirDy(dir), z, dir, outZ))
        return 0;
    if (!Walk_FindStandZ(x + b25_dirDx(dir - 1), y + b25_dirDy(dir - 1), z, dir - 1, outZ))
        return 0;
    int next = (dir + 1) & 7;
    return Walk_FindStandZ(x + b25_dirDx(next), y + b25_dirDy(next), z, next, outZ);
}
HOOK_FUNC(0x004662f0, AStarSearch_StepCheck);

// ---------------------------------------------------------------------------
// 0x00466390  size 0xbc
// ECX = search, stack (node, gx, gy, gz), RET 0x10. For each of the 8 directions: odd ones go
// through the diagonal check 0x004662f0, even ones straight to 0x00448c90; when the step is
// possible the neighbour is handed to 0x00466450 (ConsiderNeighbour) with the resulting z.
// ---------------------------------------------------------------------------
void __fastcall AStarSearch_ExpandNode(CPathSearch *self, int /*edx*/, PathNode *node, int gx, int gy, int gz)
{
    int dir = 0;
    PathNode *cur = node;
    int *outZ = reinterpret_cast<int *>(&node);
    do {
        if ((dir & 1) != 0) {
            int nodeY = cur->y;
            int nodeX = cur->x;
            if (AStarSearch_StepCheck(nodeX, nodeY, cur->z, dir, outZ) == 0)
                continue;
            self->ConsiderNeighbour(cur, b25_dirDx(dir) + nodeX, b25_dirDy(dir) + nodeY, *outZ, gx, gy, gz);   // 0x00466450
        } else {
            int ny = b25_dirDy(dir) + cur->y;
            int nx = b25_dirDx(dir) + cur->x;
            if (Walk_FindStandZ(nx, ny, cur->z, dir, outZ) == 0)
                continue;
            self->ConsiderNeighbour(cur, nx, ny, *outZ, gx, gy, gz);   // 0x00466450
        }
    } while (++dir < 8);
}
HOOK_FUNC(0x00466390, AStarSearch_ExpandNode);
