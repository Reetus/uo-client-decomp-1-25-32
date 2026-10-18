// CSignGump: a sign / plaque gump ("sign gump", vtable 0x0049ef70): the sign art with the wrapped
// text drawn into frame0. gumps.cxx 0x0043d420..0x0043d710.
// Allocation size not established, so there is no sizeof assert.
#pragma once

#include <cstddef>
#include <cstdint>
#include <new>

#include "gump.h"

class CSignGump : public CGump {
public:
    virtual int IsTagf8() { return 1; }
    // 0x0043d420: constructor, RET 0x18: (x, y, artId, a4 (not read), text, owner).
    CSignGump(int x, int y, int artId, int a4, char *text, CObj *owner);
    // 0x0043d5c0: destructor (the CGump base destructor follows).
    ~CSignGump();
    void Dtor() { this->CSignGump::~CSignGump(); }
    // 0x0043d650: draw handler (vtable slot +0x08), RET 4.
    virtual void Draw(int *clip);
    // 0x0043d6e0: mouse down (vtable slot 0x10), RET 8: bring to front, start dragging from (x, y).
    virtual void MouseDown(int x, int y);
    // 0x0043d710: mouse move (vtable slot 0x28), RET 8: move with the drag.
    virtual void MouseMove(int x, int y);

    int32_t dragging;       // +0x94
    virtual int HitTest(int x, int y);   // slot HitTest, copy of CStringPickerGump::HitTest
    virtual void MouseUp(int x, int y);   // slot MouseUp, copy of CMacroCmdGump::OnMouseUp
    virtual void RButtonUp(int x, int y);   // slot RButtonUp, copy of CCourseGump::CloseAndDelete
};

static_assert(offsetof(CSignGump, dragging) == 0x94, "CSignGump::dragging");
static_assert(sizeof(CSignGump) == 0x98, "CSignGump size (operator new(0x98))");
