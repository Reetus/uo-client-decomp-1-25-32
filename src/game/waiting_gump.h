// CGump_0043de30: the draggable "waiting" style gump whose draw / mouse handlers sit at 0x0043de30..0x0043df80.
// The class is not identified (the merged first pass called it CWaitingGump); fields are the ones these four
// handlers touch. Derived classes keep their own fields from +0x94.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CGump_0043de30 : public CGump {
public:
    virtual int IsTag80() { return 0; }
    virtual int IsTag108() { return 1; }
    // slot override (gump virtual-slot prep): own copy of the shared body
    virtual int HitTest(int x, int y);
    virtual void MouseUp(int x, int y);
    // 0x0043de30: draw handler (vtable slot +0x08), one stack argument (clip), RET 4.
    virtual void Draw(int *clip);
    // 0x0043df00: mouse down (x, y), RET 8: bring to front unless it is the gump at 0x006488d4, start a drag.
    virtual void MouseDown(int x, int y);
    // 0x0043df40: mouse move (x, y), RET 8: move the gump while a drag is active and +0x98 is 0.
    virtual void MouseMove(int x, int y);
    // 0x0043df80: key (code), RET 4: Enter (0x0d) calls 0x004111c0.
    virtual void OnKey(int key);

    // Second pass, waiting_gump_ops.cpp: this is the "waiting gump" (vtable 0x0049f0a0, 0x2ac bytes).
    // 0x0043d740  size 0x427  constructor, RET 0x1c: (x, y, text, closeTarget, withButtons, smallStyle, p7).
    CGump_0043de30(int x, int y, const char *text, int closeTarget, int withButtons, int smallStyle, int p7);
    // 0x0043dba0  destructor (the out-of-line body; the base destructor follows implicitly).
    ~CGump_0043de30();
    // 0x0043dba0: the destructor body, called by ScalarDeletingDtor.
    void Dtor() { this->CGump_0043de30::~CGump_0043de30(); }
    // 0x0043dd00: copy frame1 over frame0 and draw the text into frame0.
    void __thiscall RenderText();

    int32_t dragging;       // +0x94  1 while dragging
    int32_t noDrag;         // +0x98  drag is ignored while non-zero (also: small style)
    int32_t unk_9c;         // +0x9c
    char text[0x200];       // +0xa0
    int32_t closeTarget;    // +0x2a0
    CGump *prevFocus;       // +0x2a4  focus gump to restore
    int32_t cursorHeld;     // +0x2a8  the wait cursor counter was raised
};

static_assert(offsetof(CGump_0043de30, dragging) == 0x94, "CGump_0043de30::dragging");
static_assert(offsetof(CGump_0043de30, noDrag) == 0x98, "CGump_0043de30::noDrag");
static_assert(offsetof(CGump_0043de30, text) == 0xa0, "CGump_0043de30::text");
static_assert(offsetof(CGump_0043de30, closeTarget) == 0x2a0, "CGump_0043de30::closeTarget");
static_assert(offsetof(CGump_0043de30, cursorHeld) == 0x2a8, "CGump_0043de30::cursorHeld");
static_assert(sizeof(CGump_0043de30) == 0x2ac, "CGump_0043de30 size (operator new(0x2ac))");
