// CStackGump: the "stack gump" (amount prompt for splitting a stack). vtable 0x0049fe58.
// Layout from decomp/pass2/0x00469890.cpp and 0x00469b10.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CStackGump : public CGump {
public:
    virtual int IsTag9c() { return 1; }
    // slot override (gump virtual-slot prep): own copy of the shared body
    virtual void Draw(int *clip);
    virtual int HitTest(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void RButtonUp(int x, int y);
    virtual void MouseMove(int x, int y);
    // 0x00469890  RET 0xc.  owner is the stack item (CObj); owner+0x90 gets this gump.
    CStackGump(int x_, int y_, void *owner_);
    // 0x00469b10  destructor
    ~CStackGump();
    void Dtor() { this->CStackGump::~CStackGump(); }
    // 0x00469bb0  RET 8: store (a, b) at +0x6c / +0x70, mark +0x94, raise, take the input focus and redraw.
    virtual void MouseDown(int x, int y);
    // 0x00469bf0  RET 0xc: value = maxValue * pos / range (third stack argument not read); refresh text and frame.
    virtual void OnScroll(int a, int b, int c);
    // 0x00469c30  RET 4: Enter confirms (0x0040d920); digits and backspace edit the amount text.
    virtual void OnKey(int key);
    // 0x00469d40  RET 4 (argument not read): redraw the frame bitmap: art, the amount text and the caret.
    void __thiscall RedrawFrame(int unused);

    int32_t unk_94;             // +0x94  cleared
    int32_t unk_98;             // +0x98  cleared
    int32_t value;              // +0x9c  initial: owner's u16 at +0x3c
    int32_t maxValue;           // +0xa0  same value
    char text[8];               // +0xa4  decimal of the amount
    uint8_t *scrollThumb;       // +0xac  scroll thumb control
};

static_assert(offsetof(CStackGump, unk_94) == 0x94, "CStackGump::unk_94");
static_assert(offsetof(CStackGump, unk_98) == 0x98, "CStackGump::unk_98");
static_assert(offsetof(CStackGump, value) == 0x9c, "CStackGump::value");
static_assert(offsetof(CStackGump, maxValue) == 0xa0, "CStackGump::maxValue");
static_assert(offsetof(CStackGump, text) == 0xa4, "CStackGump::text");
static_assert(offsetof(CStackGump, scrollThumb) == 0xac, "CStackGump::scrollThumb");
static_assert(sizeof(CStackGump) == 0xb0, "CStackGump size (operator new(0xb0))");
