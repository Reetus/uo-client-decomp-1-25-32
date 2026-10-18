// CButtonGump: push button with up/down/hover art. Allocated with operator new(0xcc).
// Layout from decomp/pass2/0x00412fb0.cpp and the callers that set fields after
// construction (hover art at +0xa4, toggle flag at +0xb0, layer byte at +0x54).
// Vtable 0x0049ca90, name string "Button gump".
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CButtonGump : public CGump {
public:
    virtual int IsTaga8() { return 1; }
    // Batch u1 (small_methods_u1.cpp).
    // 0x004134a0: vtable slot 0x24: clear unk_bc.
    virtual void MouseLeave();
    // 0x004134b0: vtable slot 0x2c: auto-repeat tick of a held button; calls the callback with the parent.
    virtual void Tick();
    // 0x00412fb0  size 0x126  art button constructor (x, y, parent (not read), type, frame, callback, art up, art down, p9). RET 0x24.
    CButtonGump(int x, int y, int p3, int p4, int p5, int p6, int artUp, int artDown, int p9);
    // 0x00413110  size 0x132  rectangle button constructor (x1, y1, x2, y2, parent (not read), type, frame, callback, p9). RET 0x24.
    CButtonGump(int x1, int y1, int x2, int y2, CGump *parent, int type, int frame, int callbackAddr, int p9);
    // 0x00413250: destructor (frees the two frame bitmaps and the frame data, then the base).
    ~CButtonGump();
    // 0x004130e0: vtable slot 0, RET 4.
    void Dtor() { this->CButtonGump::~CButtonGump(); }
    // 0x00413300 / 0x00413320: vtable slots 0x18 / 0x1c, RET 8: forward (a, b) to the same slot of the parent.
    virtual void RButtonDown(int x, int y);
    virtual void RButtonUp(int x, int y);
    // 0x00413340 / 0x004133d0 / 0x00413480: vtable slots 0x10 / 0x14 / 0x20, RET 8, (x, y) not read.
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void MouseOver(int x, int y);
    // 0x00413500: vtable slot 0x08, RET 4.
    virtual void Draw(int *clip);
    // 0x004136e0: vtable slot 0x0c, RET 8: non-zero when (x, y) is on the button.
    virtual int HitTest(int x, int y);

    int32_t param_94;      // +0x94  p4
    int32_t unk_98;
    int32_t unk_9c;
    int32_t artDown;       // +0xa0
    int32_t artHover;      // +0xa4  set by callers after construction (0 = none)
    int16_t unk_a8;        // +0xa8
    uint8_t pad_aa[2];
    int32_t unk_ac;
    int32_t flag_b0;       // +0xb0  set to 1 by some callers
    int32_t param_b4;      // +0xb4  p9
    int32_t unk_b8;
    int32_t unk_bc;
    int32_t unk_c0;        // +0xc0  1 after construction
    int32_t callback;      // +0xc4  p6: code address of the click handler
    int32_t unk_c8;
};

static_assert(offsetof(CButtonGump, param_94) == 0x94, "CButtonGump::param_94");
static_assert(offsetof(CButtonGump, artDown) == 0xa0, "CButtonGump::artDown");
static_assert(offsetof(CButtonGump, artHover) == 0xa4, "CButtonGump::artHover");
static_assert(offsetof(CButtonGump, unk_a8) == 0xa8, "CButtonGump::unk_a8");
static_assert(offsetof(CButtonGump, flag_b0) == 0xb0, "CButtonGump::flag_b0");
static_assert(offsetof(CButtonGump, param_b4) == 0xb4, "CButtonGump::param_b4");
static_assert(offsetof(CButtonGump, unk_c0) == 0xc0, "CButtonGump::unk_c0");
static_assert(offsetof(CButtonGump, callback) == 0xc4, "CButtonGump::callback");
static_assert(sizeof(CButtonGump) == 0xcc, "CButtonGump size (operator new(0xcc), both constructors)");
