// CHueGump: the hue picker ("hue gump", vtable 0x0049f4c8, operator new(0xdc) at the callers). A frame from art
// 0x906 with a scroll thumb (hue page), two arrow buttons and an OK button. 0x00442080 redraws it. The class is
// the same object that char_flow_gumps.h calls CHueGump_00410d30 (its StepDown / StepUp handlers).
#pragma once

#include <cstddef>
#include <cstdint>

#include "button_gump.h"
#include "gump.h"
#include "scroll_thumb_gump.h"

class CHueGump : public CGump {
public:
    virtual int IsTagfc() { return 1; }
    // slot override (gump virtual-slot prep): own copy of the shared body
    virtual int HitTest(int x, int y);
    // 0x00441850: RET 0x18, (x, y, unk_50 value, mode, title, hue).
    CHueGump(int x_, int y_, int artArg, int mode_, const char *titleText, uint16_t hue_);

    // Second pass, hue_gump_ops.cpp.
    // 0x00441c00: RET 0xc, (serial -> +0xbc, word -> +0xc0, word -> +0xc2). Centred on the 640x480 screen; mode flags
    // +0xa4 = 0, +0xa8 = 0, +0xac = 1, +0xb0 = 1. Told apart from the other constructor by its arity.
    CHueGump(int serial, uint16_t arg0, uint16_t arg1);
    // 0x00441bd0: vtable slot 0, RET 4.
    // 0x00441f50: destructor (sends the dye reply when +0xb0 is set, stores the hue into the config gump).
    ~CHueGump();
    void Dtor() { this->CHueGump::~CHueGump(); }
    // 0x00442080: redraw the hue grid, the selection mark and the preview into the gump bitmap.
    void __thiscall Redraw();
    // 0x00442600: vtable slot 0x08, RET 4.
    virtual void Draw(int *clip);
    // 0x004426c0: vtable slot 0x1c, RET 8 (arguments not read): closes the gump unless +0xb0 is set.
    virtual void RButtonUp(int x, int y);
    // 0x004426f0 / 0x004427b0 / 0x004427d0: vtable slots 0x10 / 0x14 / 0x28, RET 8.
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void MouseMove(int x, int y);
    // 0x00442890: vtable slot 0x30, RET 0xc: the page slider moved (value, range, source).
    virtual void OnScroll(int a, int b, int c);

    CScrollThumbGump *thumb;    // +0x94  page slider
    CButtonGump *btnDown;       // +0x98  rectangle button, calls 0x00410d30
    CButtonGump *btnUp;         // +0x9c  rectangle button, calls 0x00410d70
    int32_t unk_a0;
    int32_t isFirst;            // +0xa4  1 when the mode argument is 0
    int32_t mode;               // +0xa8  fourth constructor argument
    int32_t unk_ac;
    int32_t unk_b0;
    int32_t unk_b4;
    int32_t unk_b8;
    int32_t unk_bc;
    int32_t unk_c0;
    char *title;                // +0xc4  private copy of the title text
    int32_t unk_c8;             // +0xc8  0x22
    int32_t unk_cc;             // +0xcc  0x22
    int32_t unk_d0;             // +0xd0  0xc1
    int32_t unk_d4;             // +0xd4  0x71
    uint16_t hue;               // +0xd8
    uint16_t page;              // +0xda  (hue - 2) % 5
};

static_assert(offsetof(CHueGump, thumb) == 0x94, "CHueGump::thumb");
static_assert(offsetof(CHueGump, isFirst) == 0xa4, "CHueGump::isFirst");
static_assert(offsetof(CHueGump, title) == 0xc4, "CHueGump::title");
static_assert(offsetof(CHueGump, hue) == 0xd8, "CHueGump::hue");
static_assert(offsetof(CHueGump, page) == 0xda, "CHueGump::page");
static_assert(sizeof(CHueGump) == 0xdc, "CHueGump size (operator new(0xdc), both constructors)");
