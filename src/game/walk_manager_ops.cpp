// CWalkManager constructor / destructor, the cell height helpers (GetZVal and friends) and the cell object
// collector used by the walk code. Procedures 0x00448580 .. 0x004488d0 of chunk_14_00445800.
#include "walk.h"

#include <windows.h>

#include "../runtime/ported.h"
#include "globals.h"
#include "obj.h"
#include "crt.h"
#include "raw_field.h"
#include "terrain_tile.h"
#include "world_obj.h"
#include "window_helpers_31_ports.h"
#include "dirty_rects_ports.h"
#include "vtables.h"

#define g_cellBuckets GLOBAL_PTR(CWorldObj *, g_tileBuckets)    // 64 x 64 world cell lists

// 0x00482c70: message box, ECX = text, EDX = caption, stack = exit flag (non-zero ends the program).
#define Walk_MessageBox(a1, a2, a3) (ShowMessageBoxAndExit(a1, a2, a3))
// 0x0044fe30 (ECX = four signed corner heights): 1 when |h0 - h2| <= |h1 - h3|.
#define Walk_DiagonalPick(o) PORT_THISCALL0(&RectIsTall, int, (o))()

// 0x00448580  size 0x23
CWalkManager::CWalkManager()
{
    count = 0;
    cur = 0;
    nextSeq = 0;
    frame = 0;
    total = 0;
    dirState = 0;
    blocked = 0;
    requireAck = 0;
}
HOOK_CTOR(0x00448580, CWalkManager, ());

// 0x004485e0  size 0x1b
void __thiscall CWalkManager::ClearCounters()
{
    count = 0;
    cur = 0;
    nextSeq = 0;
    frame = 0;
    total = 0;
    dirState = 0;
    blocked = 0;
    requireAck = 0;
}
HOOK_METHOD(0x004485e0, CWalkManager, ClearCounters);

// 0x00448600  size 0x18b
// Height of a tile cell from its four corner heights z[0..3] (signed bytes). `flag` picks the diagonal whose
// mean is the centre height (z2 + z0 when set, z3 + z1 otherwise). mode 0 returns the centre; mode 2 returns
// the corner / edge value for direction dir (0..7: z1+z0, z1, z2+z1, z2, z2+z3, z3, z3+z0, z0, each pair
// halved); mode 1 returns the mean of that value and the centre. Any other mode or a direction above 7 shows
// the "Bugger" message box (a bad mode only the first time) and returns the centre. RET 8.
int __fastcall Walk_GetZVal(const int8_t *z, int flag, int mode, int dir)
{
    int centre;
    if (flag != 0)
        centre = z[2] + z[0];
    else
        centre = z[3] + z[1];
    centre = centre >> 1;
    if (mode == 0)
        return centre;
    if (mode == 1) {
        switch (dir) {
        case 0:
            return (((z[1] + z[0]) >> 1) + centre) >> 1;
        case 1:
            return (z[1] + centre) >> 1;
        case 2:
            return (((z[2] + z[1]) >> 1) + centre) >> 1;
        case 3:
            return (z[2] + centre) >> 1;
        case 4:
            return (((z[2] + z[3]) >> 1) + centre) >> 1;
        case 5:
            return (z[3] + centre) >> 1;
        case 6:
            return (((z[3] + z[0]) >> 1) + centre) >> 1;
        case 7:
            return (centre + z[0]) >> 1;
        default:
            Walk_MessageBox("Someone tried to call GetZVal with theDir not between 0 and 7", "Bugger", 0);   // 0x00482c70
            return centre;
        }
    }
    if (mode == 2) {
        switch (dir) {
        case 0:
            return (z[1] + z[0]) >> 1;
        case 1:
            return z[1];
        case 2:
            return (z[2] + z[1]) >> 1;
        case 3:
            return z[2];
        case 4:
            return (z[2] + z[3]) >> 1;
        case 5:
            return z[3];
        case 6:
            return (z[3] + z[0]) >> 1;
        case 7:
            return z[0];
        default:
            Walk_MessageBox("Someone tried to call GetZVal with theDir not between 0 and 7", "Bugger", 0);   // 0x00482c70
            return centre;
        }
    }
    if (g_getZValWarned == 0)
        Walk_MessageBox("Someone tried to call GetZVal with theOffset not between 0 and 2", "Bugger", 0);   // 0x00482c70
    g_getZValWarned = g_getZValWarned + 1;
    return centre;
}
HOOK_FUNC(0x00448600, Walk_GetZVal);

// 0x004487e0  size 0x2f
// qsort comparator: entries compare by the 16-bit field at +2, then by the one at +6 (signed).
int __cdecl CompareColumnEntries(const void *a, const void *b)
{
    int16_t a2 = RAWF(int16_t, a, 2);
    int16_t b2 = RAWF(int16_t, b, 2);
    if (a2 == b2)
        return static_cast<int>(RAWF(int16_t, a, 6)) - static_cast<int>(RAWF(int16_t, b, 6));
    return static_cast<int>(a2) - static_cast<int>(b2);
}
HOOK_FUNC(0x004487e0, CompareColumnEntries);

// 0x00448810  size 0x2f
uint32_t __thiscall CTerrainTile::GetTopZ()
{
    int8_t *h = &height[0];
    if (Walk_DiagonalPick(h) != 0)   // 0x0044fe30
        return (height[2] + h[0]) >> 1;
    return (height[3] + height[1]) >> 1;
}
HOOK_VMETHOD(0x00448810, CTerrainTileOps, GetTopZ);

// 0x00448840  size 0x29
uint32_t __thiscall CObj::GetTopZ()
{
    int h = PORT_VSLOT0(this, 0x6c, int)();
    uint32_t flags = PORT_VSLOT0(this, 0x40, uint32_t)();
    uint32_t eax = flags;
    if (flags & 0x400) {
        h = h / 2;
        eax = static_cast<uint32_t>(h);
    }
    eax = (eax & 0xffff0000u) | static_cast<uint16_t>(z);
    return static_cast<int>(eax) + h;
}
HOOK_VMETHOD(0x00448840, CObj, GetTopZ);

// 0x00448870  size 0x2d
uint16_t __thiscall CTerrainTile::GetHeightAt(int i)
{
    int hNext = *reinterpret_cast<int8_t *>(reinterpret_cast<char *>(this) + 0x6c + (((i >> 1) + 1) & 3));
    if ((i & 1) != 0)
        return hNext;
    return (*reinterpret_cast<int8_t *>(reinterpret_cast<char *>(this) + 0x6c + (i >> 1)) + hNext) >> 1;
}
HOOK_VMETHOD(0x00448870, CTerrainTileOps, GetHeightAt);

struct WalkProbe {          // 12 bytes written by Walk_CollectCell
    uint16_t flags;         // +0x00  1 / 6 / 7 for land, a bit set (1, 2, 4, 6) for objects, 0 / 1 for mobiles
    uint16_t z;             // +0x02  bottom
    uint16_t top;           // +0x04  top
    uint16_t height;        // +0x06  top - bottom (land), height (objects), 0x10 (mobiles)
    CWorldObj *obj;         // +0x08
};
static_assert(sizeof(WalkProbe) == 12, "WalkProbe size");

// 0x004488d0  size 0x2aa
// Collect (at most 0x100) probes for the map cell (x, y): first the objects of the world cell list from its
// last object backwards (+0x18) that pass the virtual tests, then a probe for every mobile standing on the
// cell. `mode` (0 walk, 1, 2) changes which flags are reported. Returns the number of probes written.
// ECX = out, EDX = x, stack (y, mode), RET 8.
// The cell walk of 0x00430790 is spelled out here because it must return the last object of the list.
int __fastcall Walk_CollectCell(WalkProbe *out, int x, int y, int mode)
{
    int count = 0;
    WalkProbe *p = out;
    CWorldObj *esi;

    // 0x00430790: the last object of the cell list.
    CWorldObj *head = g_cellBuckets[(y & 0x3f) * 0x40 + (x & 0x3f)];
    if (head == nullptr) {
        esi = nullptr;
    } else {
        esi = head;
        while (esi->next != nullptr)
            esi = esi->next;
    }

    if (esi != nullptr) {
        do {
            if (count >= 0x100)
                break;
            if (PORT_VSLOT0(esi, 0x34, int)() == 0) {
                if (PORT_VSLOT0(esi, 0x1c, int)() != 0) {
                    uint32_t id = RAWF(uint16_t, esi, 0x38);
                    bool skip;
                    if (id > 0x1b5)
                        skip = id == 0x1db;
                    else
                        skip = id >= 0x1ae || id == 2;
                    if (!skip) {
                        uint32_t tileFlags = *reinterpret_cast<uint32_t *>(g_landTable + id * 7 * 4);
                        int kind = 1;
                        if (mode == 2)
                            kind = 6;
                        else if ((tileFlags & 0x40) == 0)
                            kind = 7;
                        int lo = static_cast<int16_t>(PORT_VSLOT0(esi, 0x48, int)());
                        int hi = static_cast<int16_t>(PORT_VSLOT0(esi, 0x44, int)());
                        count++;
                        p->top = static_cast<uint16_t>(hi);
                        p->flags = static_cast<uint16_t>(kind);
                        p->z = static_cast<uint16_t>(lo);
                        p->height = static_cast<uint16_t>(hi - lo);
                        p->obj = esi;
                        p++;
                    }
                } else if (PORT_VSLOT0(esi, 0x18, int)() != 0) {
                    int m = 0;
                    if (PORT_VSLOT0(esi, 0x24, int)() == 0) {
                        uint32_t fl = PORT_VSLOT0(esi, 0x40, uint32_t)();
                        if (mode == 2) {
                            m = 6;
                        } else {
                            if (fl & 0x240)
                                m = 1;
                            if ((fl & 0x40) == 0) {
                                if (fl & 0x200)
                                    m |= 2;
                                if (fl & 0x400)
                                    m |= 4;
                            }
                            if (mode == 1) {
                                uint32_t gid = RAWF(uint16_t, esi, 0x38);
                                bool special;
                                if (gid > 0x6f6)
                                    special = gid == 0x846 || gid == 0x873;
                                else
                                    special = gid >= 0x6f5 || gid == 0x692;
                                if (special)
                                    m &= ~1;
                                else if (fl & 0x20000000)
                                    m &= ~1;
                            }
                        }
                        if (m != 0) {
                            int h = PORT_VSLOT0(esi, 0x6c, int)();
                            int top = PORT_VSLOT0(esi, 0x44, int)();
                            count++;
                            p->z = static_cast<uint16_t>(esi->z);
                            p->flags = static_cast<uint16_t>(m);
                            p->top = static_cast<uint16_t>(top);
                            p->height = static_cast<uint16_t>(h);
                            p->obj = esi;
                            p++;
                        }
                    }
                }
            }
            esi = esi->prev;
        } while (esi != nullptr);
    }

    CMobile *mob = g_mobileListHead;
    if (mob != nullptr) {
        WalkProbe *q = out + count;
        do {
            int flag = 0;
            if (x == mob->x && y == mob->y) {
                bool add = false;
                if (mode == 0) {
                    if (RAWF(int, g_player, 0x1ec) != RAWF(int, g_player, 0x1f0)) {
                        flag = 1;
                        add = true;
                    }
                } else if (mode <= 0) {
                    add = true;
                } else if (mode > 2) {
                    add = true;
                }
                if (add) {
                    uint16_t mz = static_cast<uint16_t>(mob->z);
                    count++;
                    q->flags = static_cast<uint16_t>(flag);
                    q->z = mz;
                    q->top = static_cast<uint16_t>(mz + 0x10);
                    q->height = 0x10;
                    q->obj = esi;   // ? the original stores ESI (the cell walk's last object, 0 when it ran out), not the mobile
                    q++;
                }
            }
            mob = mob->listNext;
        } while (mob != nullptr);
    }
    return count;
}
HOOK_FUNC(0x004488d0, Walk_CollectCell);

inline CWalkManager::~CWalkManager()
{
}
HOOK_VTABLE(0x0049f728, CWalkManager);
HOOK_SDD(0x004485b0, CWalkManager);   // slot 0

// 0x004487d0  size 0x7
// The destructor as the atexit thunk 0x0044aaf0 jumps to it (ECX = g_WalkManager): only the vptr store. A plain
// function that expands the inline destructor, since an inline destructor has no out-of-line copy of its own.
void __fastcall WalkManager_SetVtable_004487d0(void *self, void *)
{
    reinterpret_cast<CWalkManager *>(self)->CWalkManager::~CWalkManager();
}
HOOK_FUNC(0x004487d0, WalkManager_SetVtable_004487d0);
