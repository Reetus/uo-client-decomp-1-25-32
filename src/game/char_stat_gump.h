// CCharStatGump: the character creation stat page (three stats that always total 65).
// Layout from decomp/pass2/0x0041ac00.cpp: stat edit boxes +0xa0 / +0xa4 / +0xa8 (value at CInputControlGump::value,
// range 10..45), their scroll thumbs +0xac / +0xb0 / +0xb4 and read-only mirror boxes +0xb8 (of +0xa0),
// +0xbc (of +0xa8), +0xc0 (of +0xa4). The constructor and the class size are not part of this batch.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"
#include "input_control_gump.h"
#include "scroll_thumb_gump.h"

class CCharStatGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    virtual int IsTagbc() { return 1; }
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual void Draw(int *clip);
    virtual int HitTest(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void RButtonUp(int x, int y);
    virtual void Slot64(int a);
    // Batch u1 (small_methods_u1.cpp).
    // 0x0041b5a0 / 0x0041b5b0: vtable slots 0x4c / 0x50, jump thunks to NextFromName / CancelCreation.
    virtual void OnOk();
    virtual void OnCancel();
    // 0x0041b5c0: vtable slot 0x60: show art_c8.
    virtual void Slot60();
    // 0x0041ac00: RET 0xc. A thumb moved to pos of range; the other two stats are rebalanced.
    virtual void OnScroll(int a, int b, int c);
    // 0x0040e660  ? validate the name, copy name / password to the globals, show the skill page. (class inferred: +0x98 / +0x9c / +0xc8)
    void __thiscall NextFromName();
    // 0x0040e7d0  ? cancel character creation: back to the character select gump, delete all creation pages
    void __thiscall CancelCreation();

    // 0x0041a040: no stack arguments. Builds the page; the caller stores the result in 0x006488f8.
    CCharStatGump();
    // 0x0041aa30: destructor (clears 0x006488f8, frees frame0 and the frame data, then the base destructor).
    ~CCharStatGump();
    void Dtor() { this->CCharStatGump::~CCharStatGump(); }
    // 0x0041aa00: vtable slot 0, RET 4.
    // 0x0041ab70: vtable slot 0x10 (mouse down), RET 8, (x, y) local position.
    virtual void MouseDown(int x, int y);
    // 0x0041b280: vtable slot 0x44, RET 4. A control changed; the profession box applies a preset.
    virtual void OnChildChanged(int child);
    // 0x0041b460: vtable slot 0x48, no arguments. Tab: the focus moves between the two name boxes.
    virtual void OnTab();
    // 0x0041b510: vtable slot 0x58, RET 4. Moves the focus mark between the two name boxes.
    virtual void OnFocus(int field);
    // 0x0041b5d0: RET 0x24. Stats (a1..a3) are clamped to total 0x41 and shown in +0xa0 / +0xa4 / +0xa8;
    // skill ids (a4..a6) and values (a7..a9) go to the CInitSkillGump when it exists.
    void __thiscall ApplyPreset(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);

    int32_t dragging;                // +0x94  set to 1 when the press hit no thumb (window drag)
    CInputControlGump *edit_98;      // +0x98  name box, focus mark in OnFocusChanged
    CInputControlGump *edit_9c;      // +0x9c  second name box
    CInputControlGump *edit_a0;      // +0xa0
    CInputControlGump *edit_a4;      // +0xa4
    CInputControlGump *edit_a8;      // +0xa8
    CScrollThumbGump *thumb_ac;      // +0xac  thumb of edit_a0
    CScrollThumbGump *thumb_b0;      // +0xb0  thumb of edit_a4
    CScrollThumbGump *thumb_b4;      // +0xb4  thumb of edit_a8
    CInputControlGump *mirror_b8;    // +0xb8  mirrors edit_a0
    CInputControlGump *mirror_bc;    // +0xbc  mirrors edit_a8
    CInputControlGump *mirror_c0;    // +0xc0  mirrors edit_a4
    CInputControlGump *profession_c4; // +0xc4  profession picker box; value = profession index 1..14
    CGump *art_c8;                   // +0xc8  decorative art child (art 0x4bb)
};

static_assert(offsetof(CCharStatGump, edit_a0) == 0xa0, "CCharStatGump::edit_a0");
static_assert(offsetof(CCharStatGump, edit_a4) == 0xa4, "CCharStatGump::edit_a4");
static_assert(offsetof(CCharStatGump, edit_a8) == 0xa8, "CCharStatGump::edit_a8");
static_assert(offsetof(CCharStatGump, thumb_ac) == 0xac, "CCharStatGump::thumb_ac");
static_assert(offsetof(CCharStatGump, thumb_b0) == 0xb0, "CCharStatGump::thumb_b0");
static_assert(offsetof(CCharStatGump, thumb_b4) == 0xb4, "CCharStatGump::thumb_b4");
static_assert(offsetof(CCharStatGump, mirror_b8) == 0xb8, "CCharStatGump::mirror_b8");
static_assert(offsetof(CCharStatGump, mirror_bc) == 0xbc, "CCharStatGump::mirror_bc");
static_assert(offsetof(CCharStatGump, mirror_c0) == 0xc0, "CCharStatGump::mirror_c0");
static_assert(offsetof(CCharStatGump, dragging) == 0x94, "CCharStatGump::dragging");
static_assert(offsetof(CCharStatGump, edit_98) == 0x98, "CCharStatGump::edit_98");
static_assert(offsetof(CCharStatGump, edit_9c) == 0x9c, "CCharStatGump::edit_9c");
static_assert(offsetof(CCharStatGump, profession_c4) == 0xc4, "CCharStatGump::profession_c4");
static_assert(offsetof(CCharStatGump, art_c8) == 0xc8, "CCharStatGump::art_c8");
static_assert(sizeof(CCharStatGump) == 0xcc, "CCharStatGump size (operator new(0xcc))");
