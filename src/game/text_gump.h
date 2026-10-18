// CTextGump (name low-confidence): a gump with a text frame bitmap, a
// background copy, and two child controls. Layout from
// decomp/pass2/0x00479eb0.cpp; the constructor is not ported yet.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

// One line of the text history list (heads at 0x006486b4 / tail 0x006486b8). Only the fields the gump reads.
struct CTextLine {
    void *text;                 // +0x00  char * or uint16_t * (see isUnicode)
    int32_t hue;                // +0x04
    uint8_t pad_08[0x10 - 0x08];
    int32_t isUnicode;          // +0x10
    uint8_t pad_14[4];
    CTextLine *next;            // +0x18
    CTextLine *prev;            // +0x1c
};
static_assert(offsetof(CTextLine, isUnicode) == 0x10, "CTextLine::isUnicode");
static_assert(offsetof(CTextLine, next) == 0x18, "CTextLine::next");
static_assert(offsetof(CTextLine, prev) == 0x1c, "CTextLine::prev");

class CTextGump : public CGump {
public:
    virtual int IsTag94() { return 1; }
    // 0x00479eb0  resize the gump and its frame / background bitmaps, re-anchor the children
    virtual void Resize(int w, int h);
    // 0x00479500: constructor, RET 8 (x, y); allocated with operator new(0xd4). Builds the two frame bitmaps from gump
    // art 0x820 / 0x830, the scroll thumb (+0xc0) and the four buttons (+0xc4 is the one at the bottom, art 0x82c).
    CTextGump(int x_, int y_);

    // Second pass (text_gump_ops.cpp).
    // 0x00479990: destructor body.
    ~CTextGump();
    void Dtor() { this->CTextGump::~CTextGump(); }
    // 0x00479a40: RET 8 (x, y): start of a mouse press; remembers the point and the time.
    virtual void MouseDown(int x, int y);
    // 0x00479aa0: RET 8 (x, y): click on the arrow / track strip, scrolls by ten lines.
    void __thiscall ClickScroll(int mx, int my);
    // 0x00479c60: idle tick: repeats the press after two ticks, ends the double-click window.
    virtual void Tick();
    // 0x00479ce0: RET 0xc (pos, range, thumb): the scroll thumb moved.
    virtual void OnScroll(int a, int b, int c);
    // 0x0047a0b0: show the end of the history.
    void __thiscall ScrollToEnd();
    // 0x0047a210: RET 8 (both arguments unread): repaint frame 0.
    void __thiscall RedrawText(int a, int b);
    // 0x0047a3d0: vtable slot 0x08, RET 4.
    virtual void Draw(int *clip);

    int32_t unk_94;             // +0x94  set to 1 when a press turns into a drag / double-click wait ends
    int32_t unk_98;
    int32_t held;               // +0x9c  mouse button is down
    int32_t unk_a0;
    int32_t pressTick;          // +0xa4  GetTickCount() of the first press
    int32_t pressState;         // +0xa8  0 idle, 1 first press seen, 2 second press
    int32_t unk_ac;
    int32_t textLeft;           // +0xb0
    int32_t textTop;            // +0xb4
    int32_t unk_b8;             // +0xb8  width - 0x31, forced odd
    int32_t unk_bc;             // +0xbc  height - 0x4e
    CGump *childC0;             // +0xc0
    CGump *childC4;             // +0xc4
    CTextLine *cur;             // +0xc8  first line shown
    int32_t lineOffset;         // +0xcc  pixels of that line scrolled off the top
    int32_t repeat;             // +0xd0  idle ticks since the press
    virtual int HitTest(int x, int y);   // slot HitTest, copy of CSkillListGump::HitTestPoint
    virtual void MouseUp(int x, int y);   // slot MouseUp, copy of CFightingGump::EndDrag
    virtual void RButtonUp(int x, int y);   // slot RButtonUp, copy of CCourseGump::CloseAndDelete
    virtual void MouseMove(int x, int y);   // slot MouseMove, copy of CTipsGump::MouseMove
};

static_assert(offsetof(CTextGump, held) == 0x9c, "CTextGump::held");
static_assert(offsetof(CTextGump, pressTick) == 0xa4, "CTextGump::pressTick");
static_assert(offsetof(CTextGump, textLeft) == 0xb0, "CTextGump::textLeft");
static_assert(offsetof(CTextGump, textTop) == 0xb4, "CTextGump::textTop");
static_assert(offsetof(CTextGump, cur) == 0xc8, "CTextGump::cur");
static_assert(offsetof(CTextGump, repeat) == 0xd0, "CTextGump::repeat");
static_assert(offsetof(CTextGump, unk_b8) == 0xb8, "CTextGump::unk_b8");
static_assert(offsetof(CTextGump, unk_bc) == 0xbc, "CTextGump::unk_bc");
static_assert(offsetof(CTextGump, childC0) == 0xc0, "CTextGump::childC0");
static_assert(offsetof(CTextGump, childC4) == 0xc4, "CTextGump::childC4");
static_assert(sizeof(CTextGump) == 0xd4, "CTextGump size (operator new(0xd4))");
