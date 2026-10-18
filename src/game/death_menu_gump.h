// CDeathMenuGump: the menu shown to a dead player ("DeathMenu gump", vtable 0x0049f1b8): "Play As Ghost" and
// "Resurrect With Penalties" buttons on a framed 0x118 x 0x64 panel. Allocated with operator new(0x94) (the
// caller in chunk 11 stores it in 0x006488d4); it adds no fields to CGump.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CDeathMenuGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    // slot override (gump virtual-slot prep): own copy of the shared body
    virtual void MouseUp(int x, int y);
    virtual void RButtonUp(int x, int y);
    // 0x0043e770  size 0x247  constructor, no arguments.
    CDeathMenuGump();
    // 0x0043e9c0: vtable slot 0, RET 4.
    // 0x0043e9f0: destructor (the CGump base destructor follows).
    ~CDeathMenuGump();
    void Dtor() { this->CDeathMenuGump::~CDeathMenuGump(); }
    // 0x0043ea90: draw (vtable slot 0x08), RET 4.
    virtual void Draw(int *clip);
    // 0x0043eb60: hit test (vtable slot 0x0c), RET 8.
    virtual int HitTest(int x, int y);
    // 0x0043ebb0: mouse down (x, y not read), RET 8: bring to front.
    virtual void MouseDown(int x, int y);
    // 0x0043ebc0: mouse enter (x, y not read), RET 8: set mouse cursor 0x11.
    virtual void MouseOver(int x, int y);
};

static_assert(sizeof(CDeathMenuGump) == 0x94, "CDeathMenuGump size (operator new(0x94))");
