// CDragGump: the gump that follows the cursor while an item is held.
// Layout partial; see decomp/pass2/0x00429b70.cpp.
#pragma once

#include <cstddef>
#include <cstdint>
#include <new>

#include "gump.h"

class CDragGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    virtual int IsTag8c() { return 1; }
    // slot override (gump virtual-slot prep): own copy of the shared body
    virtual void Tick();
    // 0x00429b70: mouse released at (x, y) (screen coordinates). Works out what is under
    // the cursor and sends the matching drop / equip / use packet, then deletes this gump.
    virtual void MouseUp(int x, int y);

    // 0x00429430  size 0x3aa  constructor (no arguments, plain ECX): renders the held item's art
    // (or gump art for graphics 0x3584..0x35a1) into a frame0 bitmap, centres on the cursor.
    CDragGump();
    // Construction in place (a hand-allocated 0x9c bytes), for object_manager.cpp.
    CDragGump *__thiscall Ctor();
    // 0x00429810  destructor: clears the held-item globals, releases the hovered gump (the CGump base destructor follows).
    ~CDragGump();
    void Dtor() { this->CDragGump::~CDragGump(); }
    // 0x004298e0  vtable-slot thunk: remembers this as the current drag gump, calls slot +0x28 (x, y).
    virtual void MouseOver(int x, int y);
    // 0x00429900  mouse move (also vtable slot +0x28): follow the cursor, pick the drop target.
    virtual void MouseMove(int x, int y);
    // 0x00429a60  left button down: forward to the world view gump under the cursor.
    virtual void RButtonDown(int x, int y);
    // 0x00429ae0  left button up: forward to the world view gump under the cursor.
    virtual void RButtonUp(int x, int y);

    CGump *sourceGump;      // +0x94  gump the item was picked from (drop target hovered by the mouse move)
    int32_t mouseDown;      // +0x98  left button state forwarded by the world view
};

static_assert(offsetof(CDragGump, sourceGump) == 0x94, "CDragGump::sourceGump");
static_assert(offsetof(CDragGump, mouseDown) == 0x98, "CDragGump::mouseDown");
static_assert(sizeof(CDragGump) == 0x9c, "CDragGump size (operator new(0x9c))");
