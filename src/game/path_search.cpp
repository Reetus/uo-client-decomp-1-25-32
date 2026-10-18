// A* path search: free lists, driver, neighbour consideration and g-cost
// propagation. 0x00466120, 0x00466160, 0x00466450, 0x00466640.
//
// Nodes come from calloc in the original CRT (calloc / free).
// __ftol of the original is a plain truncating float -> int conversion.
#include "path_search.h"

#include <cmath>

#include "../runtime/ported.h"
#include "crt.h"
#include "window_helpers_31_ports.h"
#include "client_misc_ops_ports.h"
#include "path_search_b25_ports.h"
#include "globals.h"

// Out-of-memory handler: ECX = 0x004adcfc, EDX = 0x004add18, one stack argument 0 (RET 4).
static inline void OutOfMemory()
{
    ShowMessageBoxAndExit((const char *)(GLOBAL_PTR(void, g_004adcfc_storage)), (const char *)(GLOBAL_PTR(void, g_004add18_storage)), 0);
}
static inline int PollContinue() { return Misc_Return0(); }

static inline PathNode *new_node()
{
    PathNode *n = static_cast<PathNode *>(calloc(1, 0x50));
    if (n == nullptr)
        OutOfMemory();
    return n;
}

// Append `n` to the first free slot of parent->nbr. The scan stops after 8 slots,
// so with all 8 taken the store lands on index 8, which aliases parent->next
// (as in the original).
static void link_neighbour(PathNode *parent, PathNode *n)
{
    int i = 0;
    PathNode **p = &parent->nbr[0];
    while (*p != nullptr) {
        i++;
        p++;
        if (!(i < 8))
            break;
    }
    reinterpret_cast<PathNode **>(reinterpret_cast<uint8_t *>(parent) + 0x2c)[i] = n;
}

// ---------------------------------------------------------------------------
// 0x00466120  size 0x39
// Free the nodes of the two singly linked lists at +4 and +8 (next at node+0x4c).
// ---------------------------------------------------------------------------
void __thiscall CPathSearch::FreeLists()
{
    PathNode *node = open;
    PathNode *nextNode;
    if (node != nullptr) {
        do {
            nextNode = node->next;
            free(node);
            node = nextNode;
        } while (nextNode != nullptr);
    }
    node = closed;
    if (node != nullptr) {
        do {
            nextNode = node->next;
            free(node);
            node = nextNode;
        } while (nextNode != nullptr);
    }
}
HOOK_METHOD(0x00466120, CPathSearch, FreeLists);

// ---------------------------------------------------------------------------
// 0x00466160  size 0x180
// Best-first search driver. RET 0x20: eight stack arguments.
// ---------------------------------------------------------------------------
PathNode *__thiscall CPathSearch::Run(int sx, int sy, int sz, int gx, int gy, int gz, int pollAfter, int tolerance)
{
    PathNode *node;

    expanded = 0;
    open = static_cast<PathNode *>(calloc(1, 0x50));
    if (open == nullptr)
        OutOfMemory();
    closed = static_cast<PathNode *>(calloc(1, 0x50));
    if (closed == nullptr)
        OutOfMemory();
    node = new_node();

    node->g = 0;
    int dy = sx - gx;
    int dx = sy - gy;
    int dz = sz - gz;
    int distSq = dy * dy + dx * dx + dz * dz;
    float dist = static_cast<float>(sqrt(static_cast<double>(distSq)));
    node->x = sx;
    node->y = sy;
    node->z = sz;
    node->h = dist;
    node->f = dist;
    open->next = node;

    for (;;) {
        PathNode *head = open;
        PathNode *first = head->next;
        if (first == nullptr) {
            node = nullptr;
        } else {
            PathNode *tmp = first->next;
            node = first;
            head->next = tmp;                 // pop from the open list
            first->next = closed->next;
            closed->next = first;               // push onto the closed list
        }
        if (node == nullptr)
            return nullptr;

        expanded++;
        if (expanded > pollAfter) {
            if (PollContinue() == 0)
                return nullptr;
        }
        if (expanded > 0x1388)
            return nullptr;

        if (gx >= node->x - tolerance && gx <= node->x + tolerance &&
            gy >= node->y - tolerance && gy <= node->y + tolerance &&
            gz >= node->z - 8 && gz <= node->z + 8)
            return node;

        PORT_THISCALL4(&AStarSearch_ExpandNode, void, this, PathNode *, int, int, int)(node, gx, gy, gz);   // expand `node`
    }
}
HOOK_METHOD(0x00466160, CPathSearch, Run);

// ---------------------------------------------------------------------------
// 0x00466450  size 0x1e9
// Consider the neighbour (x, y, z) of `parent`. RET 0x1c: seven stack arguments.
// ---------------------------------------------------------------------------
void __thiscall CPathSearch::ConsiderNeighbour(PathNode *parent, int x, int y, int z, int gx, int gy, int gz)
{
    int newG = static_cast<int>(parent->g) + 1;           // __ftol(parent->g) + 1
    PathNode *n = nullptr;
    PathNode *p;

    for (p = open->next; p != nullptr; p = p->next) {
        if (p->x == x && p->y == y && p->z == z) {
            n = p;
            break;
        }
    }

    if (n != nullptr) {
        // found on the open list: no re-sort
        link_neighbour(parent, n);
        if (static_cast<float>(newG) < n->g) {
            n->g = static_cast<float>(newG);
            n->parent = parent;
            n->f = n->h + static_cast<float>(newG);
        }
        return;
    }

    for (p = closed->next; p != nullptr; p = p->next) {
        if (p->x == x && p->y == y && p->z == z) {
            n = p;
            break;
        }
    }

    if (n != nullptr) {
        // found on the closed list
        link_neighbour(parent, n);
        if (static_cast<float>(newG) < n->g) {
            n->g = static_cast<float>(newG);
            n->parent = parent;
            n->f = n->h + static_cast<float>(newG);
            PropagateCost(n);
        }
        return;
    }

    // brand new node
    n = new_node();
    n->parent = parent;
    n->z = z;
    int dx = x - gx;
    int dy = y - gy;
    int dz = z - gz;
    n->g = static_cast<float>(newG);
    int distSq = dx * dx + dy * dy + dz * dz;
    n->x = x;
    n->y = y;
    n->h = static_cast<float>(sqrt(static_cast<double>(distSq)));
    float f = n->h + static_cast<float>(newG);
    n->f = f;

    // sorted insert into the open list (by f)
    PathNode **link = &open->next;
    p = *link;
    while (p != nullptr && p->f < f) {
        link = &p->next;
        p = p->next;
    }
    n->next = *link;
    *link = n;

    link_neighbour(parent, n);
}
HOOK_METHOD(0x00466450, CPathSearch, ConsiderNeighbour);

// ---------------------------------------------------------------------------
// 0x00466640  size 0x169
// Propagate an improved g cost from `start` through its neighbour links, using
// the stack at this->stack (cells of 8 bytes). RET 4.
// The constant at 0x0049fc20 is -1.0f, so "g - C" is g + 1.0f.
// ---------------------------------------------------------------------------
void __thiscall CPathSearch::PropagateCost(PathNode *start)
{
    int i;
    int g = static_cast<int>(start->g);                   // __ftol(start->g)
    for (i = 0; i < 8; i++) {
        PathNode *nb = start->nbr[i];
        if (nb == nullptr)
            break;
        if (static_cast<float>(g + 1) < nb->g) {
            nb->parent = start;
            nb->g = static_cast<float>(g) - (-1.0f);
            nb->f = nb->h + (static_cast<float>(g) - (-1.0f));
            PathCell *cell = static_cast<PathCell *>(calloc(1, 8));
            if (cell == nullptr)
                OutOfMemory();
            cell->node = nb;
            cell->next = stack->top;
            stack->top = cell;
        }
    }

    PathCell *cell = stack->top;
    while (cell != nullptr) {
        PathNode *cur = cell->node;
        stack->top = cell->next;
        free(cell);
        for (i = 0; i < 8; i++) {
            PathNode *nb = cur->nbr[i];
            if (nb == nullptr)
                break;
            float ng = cur->g - (-1.0f);
            if (ng < nb->g) {
                nb->g = ng;
                nb->parent = cur;
                nb->f = nb->h + ng;
                PathCell *c2 = static_cast<PathCell *>(calloc(1, 8));
                if (c2 == nullptr)
                    OutOfMemory();
                c2->node = nb;
                c2->next = stack->top;
                stack->top = c2;
            }
        }
        cell = stack->top;
    }
}
HOOK_METHOD(0x00466640, CPathSearch, PropagateCost);
