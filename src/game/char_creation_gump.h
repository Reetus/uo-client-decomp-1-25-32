// CCharCreationGump: the first character creation page (name, hair / beard style, hues, preview).
// Vtable 0x0049d558, name "CharCreation gump", art 0x4ec as background.
// Layout from decomp/pass2/0x00418d60.cpp (ctor), 0x004197d0.cpp and 0x00419cb0.cpp. The class size is not
// part of this batch (the allocation is in the caller).
#pragma once

#include <cstddef>
#include <cstdint>

#include "button_gump.h"
#include "gump.h"
#include "input_control_gump.h"
#include "scroll_thumb_gump.h"

class CCharCreationGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    virtual int IsTagb8() { return 1; }
    // Gump prep P5: own copy of a slot body the original shares between gumps.
    virtual void Slot40(int a);
    virtual void RButtonUp(int x, int y);
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual int HitTest(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void OnTab();
    virtual void Slot64(int a);
    // Batch u1 (small_methods_u1.cpp).
    // 0x00411bf0: toggle the gender value of genderBox (+0xe8) and run slot 0x44 for it.
    void __thiscall ToggleGender();
    // 0x00419c70 / 0x00419c80: vtable slots 0x4c / 0x50, jump thunks to ShowCityChoice / ShowInitSkill.
    virtual void OnOk();
    virtual void OnCancel();
    // 0x00419c90: vtable slot 0x60: show art_c4 and art_c8.
    virtual void Slot60();
    // 0x00418d60  size 0x8d3  constructor, no stack arguments (allocation 0xf0).
    CCharCreationGump();
    // 0x00419670: destructor (clears g_charCreationGump, frees the frame bitmap and table, then the base).
    ~CCharCreationGump();
    // 0x004197d0: mouse-down handler (vtable slot), RET 8, (x, y).
    virtual void MouseDown(int x, int y);
    // 0x00419cb0: repaint the character preview into frame0.
    void __thiscall RedrawPreview();
    // 0x00419640: vtable slot 0, RET 4.
    void Dtor() { this->CCharCreationGump::~CCharCreationGump(); }
    // 0x00419700: vtable slot 0x08, RET 4.
    virtual void Draw(int *clip);
    // 0x004198a0: RET 0xc, (position, range, thumb): a scroll thumb moved; selects skin / hair / beard hue.
    virtual void OnScroll(int a, int b, int c);
    // 0x00419a90: vtable slot 0x44, RET 4: the gender / hair / beard box changed.
    virtual void OnChildChanged(int child);
    // 0x00411c20: hair style button: hide art_c4 and open the hair style list over hairBox.
    void __thiscall OpenHairPicker();
    // 0x00411d90: beard style button: hide art_c8 and open the beard style list over beardBox.
    void __thiscall OpenBeardPicker();
    // 0x0040e4f0  ? show the city choice gump (create it if missing), hide this page  (class inferred from +0xc4 / +0xc8)
    void __thiscall ShowCityChoice();
    // 0x0040e5c0  ? show the skill page (create it if missing), hide this page
    void __thiscall ShowInitSkill();

    int32_t dragging;               // +0x94  set to 1 when the press hit no thumb (window drag)
    CButtonGump *button_98;         // +0x98
    CButtonGump *button_9c;         // +0x9c
    CInputControlGump *hairBox;     // +0xa0  hair style name; value = hair style index (0 = none)
    CInputControlGump *beardBox;    // +0xa4  beard style name; value = beard style index (0 = none)
    CInputControlGump *genderBox;   // +0xa8  hidden; value = gender (0 / 1)
    CScrollThumbGump *thumb_ac;     // +0xac
    CScrollThumbGump *thumb_b0;     // +0xb0
    CScrollThumbGump *thumb_b4;     // +0xb4
    CScrollThumbGump *thumb_b8;     // +0xb8  hidden initially
    CScrollThumbGump *thumb_bc;     // +0xbc
    CScrollThumbGump *thumb_c0;     // +0xc0  hidden initially
    CGump *art_c4;                  // +0xc4  decorative art child (art 0x4ed)
    CGump *art_c8;                  // +0xc8  decorative art child (art 0x4ee)
    int32_t unk_cc;
    int32_t unk_d0;
    int32_t unk_d4;
    int32_t unk_d8;
    int32_t unk_dc;
    int32_t unk_e0;
    int32_t skinHueIndex;           // +0xe4  0x3e9 initially; RedrawPreview uses skinHueIndex + 1 (0 = unhued)
    int32_t hairHue;                // +0xe8  0x44d initially; -1 = unhued
    int32_t beardHue;               // +0xec  0x44d initially; -1 = unhued
};

static_assert(offsetof(CCharCreationGump, dragging) == 0x94, "CCharCreationGump::dragging");
static_assert(offsetof(CCharCreationGump, button_98) == 0x98, "CCharCreationGump::button_98");
static_assert(offsetof(CCharCreationGump, button_9c) == 0x9c, "CCharCreationGump::button_9c");
static_assert(offsetof(CCharCreationGump, hairBox) == 0xa0, "CCharCreationGump::hairBox");
static_assert(offsetof(CCharCreationGump, beardBox) == 0xa4, "CCharCreationGump::beardBox");
static_assert(offsetof(CCharCreationGump, genderBox) == 0xa8, "CCharCreationGump::genderBox");
static_assert(offsetof(CCharCreationGump, thumb_ac) == 0xac, "CCharCreationGump::thumb_ac");
static_assert(offsetof(CCharCreationGump, thumb_b0) == 0xb0, "CCharCreationGump::thumb_b0");
static_assert(offsetof(CCharCreationGump, thumb_b4) == 0xb4, "CCharCreationGump::thumb_b4");
static_assert(offsetof(CCharCreationGump, thumb_b8) == 0xb8, "CCharCreationGump::thumb_b8");
static_assert(offsetof(CCharCreationGump, thumb_bc) == 0xbc, "CCharCreationGump::thumb_bc");
static_assert(offsetof(CCharCreationGump, thumb_c0) == 0xc0, "CCharCreationGump::thumb_c0");
static_assert(offsetof(CCharCreationGump, art_c4) == 0xc4, "CCharCreationGump::art_c4");
static_assert(offsetof(CCharCreationGump, art_c8) == 0xc8, "CCharCreationGump::art_c8");
static_assert(offsetof(CCharCreationGump, skinHueIndex) == 0xe4, "CCharCreationGump::skinHueIndex");
static_assert(offsetof(CCharCreationGump, hairHue) == 0xe8, "CCharCreationGump::hairHue");
static_assert(offsetof(CCharCreationGump, beardHue) == 0xec, "CCharCreationGump::beardHue");
static_assert(sizeof(CCharCreationGump) == 0xf0, "CCharCreationGump size (operator new(0xf0))");
