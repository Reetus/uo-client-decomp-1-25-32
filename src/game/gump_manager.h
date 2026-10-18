// CGumpManager: the one object (at 0x006488c0) that owns the top-level gump
// list, the saved desktop layout and the gump factory. gumps.cxx.
// Layout partial: from decomp/pass2/0x0043a340, 0x0043bd20, 0x0043c6c0.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"
#include "gump.h"
#include "globals.h"

class CObj;

// One saved gump of the desktop layout file; 0x1c bytes each at manager+0x70.
struct DesktopEntry {
    uint8_t type;         // +0x00  record type from the file
    uint8_t pad_01[7];
    int32_t flag;         // +0x08  (flag byte == 1)
    uint8_t pad_0c[2];
    uint16_t s1;          // +0x0e
    uint16_t s2;          // +0x10
    uint8_t pad_12[2];
    int32_t x;            // +0x14
    int32_t y;            // +0x18
};
static_assert(sizeof(DesktopEntry) == 0x1c, "DesktopEntry size");
static_assert(offsetof(DesktopEntry, flag) == 0x08, "DesktopEntry::flag");
static_assert(offsetof(DesktopEntry, s1) == 0x0e, "DesktopEntry::s1");
static_assert(offsetof(DesktopEntry, s2) == 0x10, "DesktopEntry::s2");
static_assert(offsetof(DesktopEntry, x) == 0x14, "DesktopEntry::x");
static_assert(offsetof(DesktopEntry, y) == 0x18, "DesktopEntry::y");

class CGumpManager {
public:
    // 0x0043a340: redraw a screen rect (l, t, r, b) into the ring back buffer.
    void __thiscall DrawGumps(int *rc, int flush);
    // 0x0043bd20: gump factory keyed by type id; reuses an existing gump of the same object/type.
    CGump *__thiscall CreateGump(CObj *obj, int type, CGump *parent, int containerFlag);
    // 0x0043c6c0: load "desktop.uoN" and recreate the saved gumps.
    void __thiscall LoadDesktop();

    // Second pass, gump_manager_ops.cpp.
    // 0x00439e90: shift every gump that answers slot 0xf0 (missile gumps) by (dx, dy), RET 8.
    void __thiscall ShiftMissileGumps(int dx, int dy);
    // 0x00439f10: delete every gump that answers slot 0xf0.
    void __thiscall DeleteMissileGumps();
    // 0x00439ff0: delete every gump that answers slot 0x80.
    void __thiscall DeleteGumpsSlot80();
    // 0x0043a040: create the memory DC and select the desktop bitmap (+0x4474) when +0x5c is set.
    void __thiscall CreateMemDC();
    // 0x0043a0a0: find the gump of `owner` in the tree starting at `node` (walking +0x3c, recursing into +0x44), RET 0x10.
    CGump *__thiscall FindGumpForObject(CObj *owner, int flagA, int flagB, CGump *node);
    // 0x0043a220: top-level gump hit test at screen (x, y), skipping `exclude`, RET 0x10.
    CGump *__thiscall GumpAtPoint(int x, int y, CGump *exclude, int flag);
    // 0x0043aa10: per-frame update: expire timed gumps, update the radar, call slot 0x5c of every gump.
    void __thiscall Update();
    // 0x0043aaa0: reposition and stack the barks of every top-level bark gump.
    void __thiscall RepositionBarks();
    // 0x0043c670: delete every gump that answers neither slot 0x70 nor slot 0x74.
    void __thiscall DeleteTransientGumps();
    // 0x0043cd60: write the desktop layout file desktop.uoN.
    void __thiscall SaveDesktop();

    // Second pass, gump_manager_ctor.cpp.
    // 0x00437000: constructor (the compiler's vptr, table 0x0049ebc8): allocates the 640x480 back buffer 0x004d3330,
    // clears the state.
    CGumpManager();
    // Deletes every top-level gump, the memory DC and the back buffer. Defined inline in gump_manager_ctor.cpp so the
    // scalar deleting destructor (0x004370e0) expands it as the original's does.
    virtual ~CGumpManager();

    CGump *listTail;          // +0x04  == g_gumpListTail (0x006488c4)
    CGump *listHead;          // +0x08  == g_gumpListHead (0x006488c8)
    uint8_t pad_0c[0x10];
    CGump *configGump;        // +0x1c  type 0x1388
    CGump *radarGump;         // +0x20  types 0x1392 / 0x1393
    uint8_t pad_24[0x38];
    int32_t unk_5c;           // +0x5c  when 0 the flush tail records the dirty rect and calls 0x00430be0 / 0x00482ca0
    uint8_t pad_60[4];
    int32_t desktopLoaded;    // +0x64
    uint8_t pad_68[4];
    int32_t entryCount;       // +0x6c
    DesktopEntry entries[1];  // +0x70  count unknown, array runs up to +0x44c0
    uint8_t pad_8c[0x44c0 - 0x8c];
    int32_t cascadeX;         // +0x44c0
    int32_t cascadeY;         // +0x44c4
};

static_assert(offsetof(CGumpManager, listTail) == 0x04, "CGumpManager::listTail");
static_assert(offsetof(CGumpManager, listHead) == 0x08, "CGumpManager::listHead");
static_assert(offsetof(CGumpManager, configGump) == 0x1c, "CGumpManager::configGump");
static_assert(offsetof(CGumpManager, radarGump) == 0x20, "CGumpManager::radarGump");
static_assert(offsetof(CGumpManager, unk_5c) == 0x5c, "CGumpManager::unk_5c");
static_assert(offsetof(CGumpManager, desktopLoaded) == 0x64, "CGumpManager::desktopLoaded");
static_assert(offsetof(CGumpManager, entryCount) == 0x6c, "CGumpManager::entryCount");
static_assert(offsetof(CGumpManager, entries) == 0x70, "CGumpManager::entries");
static_assert(offsetof(CGumpManager, cascadeX) == 0x44c0, "CGumpManager::cascadeX");
static_assert(offsetof(CGumpManager, cascadeY) == 0x44c4, "CGumpManager::cascadeY");

// 0x00437140: the destructor for the atexit thunk (a plain function expanding the inline destructor).
void __fastcall GumpManager_Dtor_00437140(void *self, void *);

#define g_gumpMgr GLOBAL_AS(CGumpManager, g_gumpMgr_storage)

// 0x0043ab00: item art id -> container gump id (stdcall, argument masked to 16 bits).
int __stdcall ContainerGumpIdForItem(unsigned int itemId);
// 0x00439f60: create a 0x2ac-byte gump, store it in 0x006488d4, add it as a top-level gump.
void *__stdcall CreateGump2ac(int p1, int p2, int p3, int p4);
