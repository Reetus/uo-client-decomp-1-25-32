// CToggleControl: check box / radio toggle drawn from gump art. operator new(0xb8).
// Name guessed in decomp/pass2/0x004163f0.cpp. Constructor 0x00416120 is not ported.
// Art ids: +0x50 normal, +0x94 checked, +0x98 disabled, +0x9c disabled+checked,
// +0xa0 hover (0 = none), +0xa4 hover+checked.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CToggleControl : public CGump {
public:
    virtual int IsTagc4() { return 1; }
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual int HitTest(int x, int y);
    virtual void RButtonUp(int x, int y);
    // Batch u1 (small_methods_u1.cpp).
    // 0x00416710: vtable slot 0x24: clear the hover flag and redraw when there is a hover art.
    virtual void MouseLeave();
    // 0x004163f0: redraw the control's frame0 bitmap.
    void __thiscall Redraw();
    // 0x00416640 / 0x00416670 / 0x004166c0: mouse down / up / enter handlers, RET 8, arguments not read.
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void MouseOver(int x, int y);
    // 0x00416120  size 0x20f  constructor (x, y, parent (not read), art, art checked, font, hue, text, checked, art disabled,
    // art disabled checked). RET 0x2c.
    CToggleControl(int x_, int y_, int a3, int art, int artChecked_, int font, int hue, const char *text, int checked_,
                   int artDisabled_, int artDisabledChecked_);
    // 0x00416360: destructor (frees the frame bitmap and the frame data, then the CGump destructor).
    ~CToggleControl();
    // 0x00416330: vtable slot 0, RET 4.
    void Dtor() { this->CToggleControl::~CToggleControl(); }
    // 0x00416590: vtable slot 0x08, RET 4: blit frame0 when the gump is visible and the clip intersects it.
    virtual void Draw(int *clip);

    int32_t artChecked;       // +0x94
    int32_t artDisabled;      // +0x98
    int32_t artDisabledChecked; // +0x9c
    int32_t artHover;         // +0xa0
    int32_t artHoverChecked;  // +0xa4
    int32_t unk_a8;
    int32_t hovering;         // +0xac
    int32_t checked;          // +0xb0
    int32_t disabled;         // +0xb4
};

static_assert(offsetof(CToggleControl, artChecked) == 0x94, "CToggleControl::artChecked");
static_assert(offsetof(CToggleControl, artDisabled) == 0x98, "CToggleControl::artDisabled");
static_assert(offsetof(CToggleControl, artDisabledChecked) == 0x9c, "CToggleControl::artDisabledChecked");
static_assert(offsetof(CToggleControl, artHover) == 0xa0, "CToggleControl::artHover");
static_assert(offsetof(CToggleControl, artHoverChecked) == 0xa4, "CToggleControl::artHoverChecked");
static_assert(offsetof(CToggleControl, hovering) == 0xac, "CToggleControl::hovering");
static_assert(offsetof(CToggleControl, checked) == 0xb0, "CToggleControl::checked");
static_assert(offsetof(CToggleControl, disabled) == 0xb4, "CToggleControl::disabled");
static_assert(sizeof(CToggleControl) == 0xb8, "CToggleControl size (operator new(0xb8))");
