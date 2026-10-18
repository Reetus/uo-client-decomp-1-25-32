// CTipsGump: the "tips gump" (tip of the day / hint window): a text frame, a scroll thumb, two scroll
// arrows, and, for the tip of the day, next / previous buttons and a "Don't show again" check box.
// vtable 0x004a0958, operator new(0x2018). Layout from the constructor 0x0047a4a0.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"
#include "scroll_thumb_gump.h"
#include "toggle_control.h"

class CTipsGump : public CGump {
public:
    // Gump prep P5: own copy of a slot body the original shares between gumps.
    virtual void Draw(int *clip);
    // 0x0047a4a0: RET 0x14, (x, y, lineCount, lines, isTipOfDay). x and y are not read. `lines` is lineCount
    // strings of 0x50 bytes each.
    CTipsGump(int x_, int y_, int lineCount, const char *lines, int isTipOfDay);
    // 0x0047aaa0: destructor (the CGump base destructor follows).
    ~CTipsGump();
    void Dtor() { this->CTipsGump::~CTipsGump(); }
    // 0x0047ab80: RET 8 (x, y): mouse press.
    virtual void MouseDown(int x, int y);
    // 0x0047abe0: RET 8 (x, y): click on the arrow strip, moves ten lines.
    void __thiscall ClickScroll(int mx, int my);
    // 0x0047ad00: RET 8 (x, y): mouse move, starts a drag of the gump.
    virtual void MouseMove(int x, int y);
    // 0x0047ad90: idle tick.
    virtual void Tick();
    // 0x0047ae10: RET 0xc (pos, range, thumb): the scroll thumb moved.
    virtual void OnScroll(int a, int b, int c);
    // 0x0047ae70: RET 8 (width, height).
    virtual void Resize(int w, int h);
    virtual int HitTest(int x, int y);   // 0x0047b1e0  slot 0x0c
    // 0x0047b040: repaint frame 0.
    void __thiscall RedrawText();

    int32_t unk_94;
    int32_t unk_98;
    int32_t held;               // +0x9c
    int32_t unk_a0;
    int32_t pressTick;          // +0xa4
    int32_t pressState;         // +0xa8  0 idle, 1 first press, 2 second press
    CToggleControl *dontShow;   // +0xac  "Don't show again" (tip of the day only)
    int32_t unk_b0;
    int32_t textLeft;           // +0xb4
    int32_t textTop;            // +0xb8
    int32_t textRight;          // +0xbc
    int32_t textBottom;         // +0xc0
    CScrollThumbGump *thumb;    // +0xc4
    char lines[100][0x50];      // +0xc8
    int32_t topLine;            // +0x2008
    int32_t lineCount;          // +0x200c
    int32_t repeat;             // +0x2010
    int32_t isTipOfDay;         // +0x2014
    virtual void MouseUp(int x, int y);   // slot MouseUp, copy of CFightingGump::EndDrag
    virtual void RButtonUp(int x, int y);   // slot RButtonUp, copy of CCourseGump::CloseAndDelete
};

static_assert(offsetof(CTipsGump, held) == 0x9c, "CTipsGump::held");
static_assert(offsetof(CTipsGump, pressTick) == 0xa4, "CTipsGump::pressTick");
static_assert(offsetof(CTipsGump, dontShow) == 0xac, "CTipsGump::dontShow");
static_assert(offsetof(CTipsGump, textLeft) == 0xb4, "CTipsGump::textLeft");
static_assert(offsetof(CTipsGump, textBottom) == 0xc0, "CTipsGump::textBottom");
static_assert(offsetof(CTipsGump, thumb) == 0xc4, "CTipsGump::thumb");
static_assert(offsetof(CTipsGump, lines) == 0xc8, "CTipsGump::lines");
static_assert(offsetof(CTipsGump, topLine) == 0x2008, "CTipsGump::topLine");
static_assert(offsetof(CTipsGump, isTipOfDay) == 0x2014, "CTipsGump::isTipOfDay");
static_assert(sizeof(CTipsGump) == 0x2018, "CTipsGump size (operator new(0x2018))");
