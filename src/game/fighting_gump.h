// CFightingGump: the fighting style gump (name "fightingstyle gump", vtable 0x0049e998).
// Layout partial; see decomp/pass2/0x004314c0.cpp. The child controls (sliders, buttons)
// are classes not ported yet and are held as opaque pointers. Size is not known yet
// (allocation site not ported).
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CFightingGump : public CGump {
public:
    virtual int IsTage8() { return 1; }
    // slot override (gump virtual-slot prep): own copy of the shared body
    virtual void Draw(int *clip);
    virtual int HitTest(int x, int y);
    virtual void RButtonUp(int x, int y);
    virtual void MouseMove(int x, int y);
    virtual void Tick();
    // Batch u1 (small_methods_u1.cpp).
    // 0x00412050: send the war mode packet with the toggle frame and both slider percentages.
    void __thiscall SendWarSliders();
    // 0x004120d0: set frame 1 on toggleA and run slot 0x44 (OnToggle) for it.
    void __thiscall PressToggleA();
    // 0x00412180 / 0x004121a0: slider A one step down / up (only while visible).
    void __thiscall SliderADec();
    void __thiscall SliderAInc();
    // 0x004121c0 / 0x004121e0: slider B one step down / up.
    void __thiscall SliderBDec();
    void __thiscall SliderBInc();
    // 0x004314c0: CFightingGump(x, y). Builds the two frame bitmaps (art 0x83e and the
    // minimised 0x853) and the sliders, toggle buttons and push buttons.
    CFightingGump(int x, int y);
    // 0x00431b70: destructor; releases the two frame bitmaps and the frame data.
    ~CFightingGump();
    void Dtor() { this->CFightingGump::~CFightingGump(); }
    // 0x00431c20: start of a drag (RET 8): bring to front, remember the grab point (+0x6c / +0x70).
    virtual void MouseDown(int x, int y);
    // 0x00431c80: end of a drag or click (RET 8; both arguments ignored).
    virtual void MouseUp(int x, int y);
    // 0x00431ce0: empty handler (RET 0xc, three arguments ignored).
    // 0x00431cf0: a toggle button was clicked (RET 4): clear the others, show the sliders per toggleB.
    virtual void OnChildChanged(int child);

    int32_t unk_94;         // +0x94  cleared
    int32_t unk_98;         // +0x98  cleared
    int32_t unk_9c;         // +0x9c  cleared
    int32_t unk_a0;         // +0xa0  cleared
    int32_t unk_a4;         // +0xa4  cleared
    int32_t unk_a8;         // +0xa8  cleared
    void *sliderA;          // +0xac  slider at (0x5c, 0x6b)
    void *sliderB;          // +0xb0  slider at (0x5c, 0x96)
    void *toggleA;          // +0xb4  toggle button at (0x55, 0x40)
    void *toggleB;          // +0xb8  toggle button at (0x97, 0x40)
};

static_assert(offsetof(CFightingGump, unk_94) == 0x94, "CFightingGump::unk_94");
static_assert(offsetof(CFightingGump, unk_a4) == 0xa4, "CFightingGump::unk_a4");
static_assert(offsetof(CFightingGump, sliderA) == 0xac, "CFightingGump::sliderA");
static_assert(offsetof(CFightingGump, sliderB) == 0xb0, "CFightingGump::sliderB");
static_assert(offsetof(CFightingGump, toggleA) == 0xb4, "CFightingGump::toggleA");
static_assert(offsetof(CFightingGump, toggleB) == 0xb8, "CFightingGump::toggleB");
static_assert(sizeof(CFightingGump) == 0xbc, "CFightingGump size (operator new(0xbc))");

// 0x00431ce0: the empty three-argument procedure (default of the gump slots OnScroll/OnScrollEnd), a plain function.
void __fastcall Nop_00431ce0(void *self, void *, int a, int b, int c);
