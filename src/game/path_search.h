// Best-first (A*) path search over the walk grid: the search object, its
// 0x50-byte nodes and the 8-byte cells of the g-cost propagation stack.
// Layout partial; see decomp/pass2/0x00466120, 0x00466160, 0x00466450 and
// 0x00466640. The constructor (0x004660b0, "zeroes lists, allocs 8-byte stack
// head") and the node expansion (0x00466390) are not ported here.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"

struct PathNode;

// One cell of the propagation stack: allocated with calloc(1, 8).
struct PathCell {
    PathNode *node;         // +0x00
    PathCell *next;         // +0x04
};
static_assert(sizeof(PathCell) == 8, "PathCell size");

// Head of the propagation stack (an 8-byte block, calloc'd by the search constructor).
struct PathStack {
    int32_t unk_00;         // +0x00
    PathCell *top;          // +0x04
};
static_assert(offsetof(PathStack, top) == 4, "PathStack::top");

// A search node, calloc(1, 0x50).
struct PathNode {
    float f;                // +0x00  g + h
    float h;                // +0x04  straight-line distance estimate to the goal
    float g;                // +0x08  cost so far
    int32_t unk_0c;
    int32_t x;              // +0x10
    int32_t y;              // +0x14
    int32_t z;              // +0x18
    uint8_t pad_1c[8];
    PathNode *parent;       // +0x24
    int32_t unk_28;
    PathNode *nbr[8];       // +0x2c  neighbours that were considered; null terminated
    PathNode *next;         // +0x4c  next node of the open / closed list
};
static_assert(sizeof(PathNode) == 0x50, "PathNode size");
static_assert(offsetof(PathNode, f) == 0x00, "PathNode::f");
static_assert(offsetof(PathNode, h) == 0x04, "PathNode::h");
static_assert(offsetof(PathNode, g) == 0x08, "PathNode::g");
static_assert(offsetof(PathNode, x) == 0x10, "PathNode::x");
static_assert(offsetof(PathNode, y) == 0x14, "PathNode::y");
static_assert(offsetof(PathNode, z) == 0x18, "PathNode::z");
static_assert(offsetof(PathNode, parent) == 0x24, "PathNode::parent");
static_assert(offsetof(PathNode, nbr) == 0x2c, "PathNode::nbr");
static_assert(offsetof(PathNode, next) == 0x4c, "PathNode::next");

class CPathSearch {
public:
    // 0x00466120  size 0x39  free the nodes of the open and closed lists.
    void __thiscall FreeLists();
    // 0x00466160  size 0x180  run the search from (sx, sy, sz) to (gx, gy, gz); returns the goal node or null.
    // Gives up after 0x1388 expansions; once `pollAfter` expansions are exceeded it asks 0x00478cd0
    // (returns 0 -> give up). `tolerance` is the x/y slack around the goal.
    PathNode *__thiscall Run(int sx, int sy, int sz, int gx, int gy, int gz, int pollAfter, int tolerance);
    // 0x00466450  size 0x1e9  consider the neighbour (x, y, z) of `parent`.
    void __thiscall ConsiderNeighbour(PathNode *parent, int x, int y, int z, int gx, int gy, int gz);
    // 0x00466640  size 0x169  propagate an improved g cost from `start` to the nodes that hang off it.
    void __thiscall PropagateCost(PathNode *start);

    PathStack *stack;       // +0x00  propagation stack head
    PathNode *open;         // +0x04  open list sentinel (sorted by f, next at node+0x4c)
    PathNode *closed;       // +0x08  closed list sentinel
    uint8_t pad_0c[0x1c];
    int32_t expanded;       // +0x28  number of expanded nodes
};
static_assert(offsetof(CPathSearch, stack) == 0x00, "CPathSearch::stack");
static_assert(offsetof(CPathSearch, open) == 0x04, "CPathSearch::open");
static_assert(offsetof(CPathSearch, closed) == 0x08, "CPathSearch::closed");
static_assert(offsetof(CPathSearch, expanded) == 0x28, "CPathSearch::expanded");
