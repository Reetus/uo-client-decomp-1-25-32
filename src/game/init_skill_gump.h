// CInitSkillGump: the character creation skill page (three skills that always total 100).
// Layout from decomp/pass2/0x0041c3a0.cpp: skill edit boxes +0xa8 / +0xac / +0xb0 (value 0..0x32) and their
// scroll thumbs +0xb4 / +0xb8 / +0xbc.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"
#include "input_control_gump.h"
#include "scroll_thumb_gump.h"

class CInitSkillGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    // Gump prep P5: slot target formerly a free function (vtable_stubs_u2.cpp).
    virtual void OnOk();
    virtual void OnCancel();
    virtual void Slot60();
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual void Draw(int *clip);
    virtual int HitTest(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void RButtonUp(int x, int y);
    virtual void OnChildChanged(int child);
    // Batch u1 (small_methods_u1.cpp).
    // 0x0040ea70: button callback: show the character stat gump again, hide this page and its art child.
    void __thiscall ShowStatPage();
    // 0x0041c3a0: RET 0xc. A thumb moved to pos of range; the other two skills are rebalanced to keep 100.
    virtual void OnScroll(int a, int b, int c);
    // 0x0040e930  ? reject duplicate skills, else show the appearance page (class inferred: +0x9c / +0xa0 / +0xa4)
    void __thiscall NextFromSkills();
    // 0x00411200  open a skill choice list for the control at +0x9c
    void __thiscall OpenSkillChoice0();
    // 0x004112d0  open a skill choice list for the control at +0xa0
    void __thiscall OpenSkillChoice1();

    // 0x0041b920: no stack arguments. Builds the page; the caller stores the result in 0x006488fc.
    CInitSkillGump();
    // 0x0041c260: destructor (clears 0x006488fc, frees frame0 and the frame data).
    ~CInitSkillGump();
    void Dtor() { this->CInitSkillGump::~CInitSkillGump(); }
    // 0x0041c230: vtable slot 0, RET 4.
    // 0x0041c310: vtable slot 0x10 (mouse down), RET 8, (x, y) local position.
    virtual void MouseDown(int x, int y);

    int32_t dragging;                // +0x94  set to 1 when the press hit no thumb (window drag)
    CGump *art_98;                   // +0x98  decorative art child (art 0x4cf)
    CInputControlGump *name_9c;      // +0x9c  skill name box of skill_a8
    CInputControlGump *name_a0;      // +0xa0  skill name box of skill_ac
    CInputControlGump *name_a4;      // +0xa4  skill name box of skill_b0
    CInputControlGump *skill_a8;     // +0xa8  K0
    CInputControlGump *skill_ac;     // +0xac  K1
    CInputControlGump *skill_b0;     // +0xb0  K2
    CScrollThumbGump *thumb_b4;      // +0xb4  thumb of skill_a8
    CScrollThumbGump *thumb_b8;      // +0xb8  thumb of skill_ac
    CScrollThumbGump *thumb_bc;      // +0xbc  thumb of skill_b0
};

static_assert(offsetof(CInitSkillGump, skill_a8) == 0xa8, "CInitSkillGump::skill_a8");
static_assert(offsetof(CInitSkillGump, skill_ac) == 0xac, "CInitSkillGump::skill_ac");
static_assert(offsetof(CInitSkillGump, skill_b0) == 0xb0, "CInitSkillGump::skill_b0");
static_assert(offsetof(CInitSkillGump, thumb_b4) == 0xb4, "CInitSkillGump::thumb_b4");
static_assert(offsetof(CInitSkillGump, thumb_b8) == 0xb8, "CInitSkillGump::thumb_b8");
static_assert(offsetof(CInitSkillGump, thumb_bc) == 0xbc, "CInitSkillGump::thumb_bc");
static_assert(offsetof(CInitSkillGump, dragging) == 0x94, "CInitSkillGump::dragging");
static_assert(offsetof(CInitSkillGump, art_98) == 0x98, "CInitSkillGump::art_98");
static_assert(offsetof(CInitSkillGump, name_9c) == 0x9c, "CInitSkillGump::name_9c");
static_assert(offsetof(CInitSkillGump, name_a0) == 0xa0, "CInitSkillGump::name_a0");
static_assert(offsetof(CInitSkillGump, name_a4) == 0xa4, "CInitSkillGump::name_a4");
static_assert(sizeof(CInitSkillGump) == 0xc0, "CInitSkillGump size (operator new(0xc0))");
