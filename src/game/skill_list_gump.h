// CSkillListGump: the skills window with collapsible groups (named CSkillGump in
// decomp/port_batches/batch_06.tsv; renamed here because skill_gump.h already
// defines a different CSkillGump for 0x0040e280). Layout from
// decomp/pass2/0x00474210.cpp, 0x00474850.cpp, 0x00474ac0.cpp, 0x00474cc0.cpp
// and 0x004755b0.cpp. The constructor is not ported yet.
//
// Global tables in the original image (all indexed as in the .cpp):
//   0x00747660 group count, 0x00747668 int[50] per-skill state, 0x007466c0 skill names (0x50 each),
//   0x00747730 u8[] row -> skill, 0x00747798 row count, 0x007477a0 group names (0x11 each),
//   0x00747870 int[50] skill -> group (-1 = no group), 0x00747938 u8[50] flags (bit 0 = show button),
//   0x007588b8 u16[50] the player's skill values.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CSkillListGump : public CGump {
public:
    virtual int IsTag68() { return 1; }
    virtual int IsTagb0() { return 1; }
    // 0x00474210  RET 0xc: commit a pending group rename, then move the cursor (scrollRange * num / den) steps.
    virtual void OnScroll(int a, int b, int c);
    // 0x00474850  per-frame update: click timeout, group-name save, drag auto-scroll
    virtual void Tick();
    // 0x00474ac0  delete the old row buttons and create the lock / up-down icon buttons
    void __thiscall RebuildButtons();
    // 0x00474cc0  draw the list into the frame bitmap (row == -1: everything, else only that skill row)
    void __thiscall Redraw(int row);
    // 0x0040df80  add a group named "New Group" (at most 12), select it, redraw and save skillgrp.mul
    void __thiscall AddGroup();
    // 0x004755b0  RET 0x10: which row is at (x, y); *isGroup / *index receive the result
    int __thiscall HitTestRow(int x_, int y_, int *isGroup, int *index);

    // Batch 28 (skill_list_gump_ops.cpp). vtable 0x004a0600.
    // 0x00472da0  RET 8: constructor (x, y); builds the frames, six buttons and the scroll thumb (unk_94).
    CSkillListGump(int px, int py);
    // 0x00473390  vtable slot 0x00, RET 4.
    // 0x004733c0  destructor (the CGump base destructor follows).
    ~CSkillListGump();
    void Dtor() { this->CSkillListGump::~CSkillListGump(); }
    // 0x004735b0  vtable slot 0x1c, RET 8 (arguments not read): delete `this` through its own slot 0 when non-null.
    virtual void RButtonUp(int x, int y);
    // 0x004735d0 / 0x004736a0 / 0x004738c0  vtable slots 0x10 / 0x14 / 0x28, RET 8: mouse down / up / move at (x, y).
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void MouseMove(int x, int y);
    // 0x00473b90  RET 8: single click at (x, y) (toggle a group, select a group, finish a rename).
    void __thiscall OnClick(int mx, int my);
    // 0x00474010  RET 8 (arguments not read): double click; un-iconify or open the EditGroup gump.
    void __thiscall OnDoubleClick(int mx, int my);
    // 0x004740b0  vtable slot 0x38, RET 8: resize to (width, height), reallocating frame0.
    virtual void Resize(int w, int h);
    // 0x004744b0  vtable slot 0x3c, RET 4: key press; 0xe02e (Delete) removes the selected group.
    virtual void OnKey(int key);
    // 0x004753c0 / 0x004754d0  vtable slots 0x08 / 0x0c.
    virtual void Draw(int *clip);
    virtual int HitTest(int x, int y);
    // 0x004758e0  RET 4 (argument not read): start renaming group `renameGroup` with an edit control over its row.
    void __thiscall BeginRename(int group);
    // 0x00475a20  vtable slot 0x4c, plain RET: commit the rename edit, save skillgrp.mul, clear the selection.
    virtual void OnOk();
    // 0x004760f0  RET 4: choose skillValue[] (non-zero) or the player's skills.
    void __thiscall SetUseOwnValues(int v);
    // 0x00476100  RET 8: skillValue[skill] = value for 0 <= skill < 50.
    void __thiscall SetSkillValue(int skill, uint16_t value);

    int32_t unk_94;             // +0x94
    uint8_t *renameEdit;        // +0x98  edit control used while renaming a group (text at its +0xec)
    int32_t listLeft;           // +0x9c
    int32_t listTop;            // +0xa0
    int32_t listRight;          // +0xa4
    int32_t listBottom;         // +0xa8
    int32_t onGroupHeader;      // +0xac  1 = cursor is on a group header, 0 = on a skill row
    int32_t groupExpanded[12];  // +0xb0
    int32_t unk_e0;             // +0xe0
    int32_t unk_e4;             // +0xe4
    int32_t renaming;           // +0xe8  a group name is being edited
    int32_t cursor;             // +0xec  group number or row number (see onGroupHeader)
    int32_t unk_f0;             // +0xf0
    int32_t selIndex;           // +0xf4  selected row, -1 none
    int32_t selGroup;           // +0xf8  selected group, -2 none
    int32_t scrollPos;          // +0xfc
    int32_t scrollRange;        // +0x100
    int32_t renameGroup;        // +0x104  group being renamed, -1 none
    int32_t dragging;           // +0x108
    int32_t unk_10c;
    int32_t unk_110;
    int32_t unk_114;
    uint32_t clickTick;         // +0x118
    int32_t clickPending;       // +0x11c
    uint16_t skillValue[50];    // +0x120
    int32_t useOwnValues;       // +0x184  non-zero: skillValue[] instead of the player's values
};

static_assert(offsetof(CSkillListGump, renameEdit) == 0x98, "CSkillListGump::renameEdit");
static_assert(offsetof(CSkillListGump, listLeft) == 0x9c, "CSkillListGump::listLeft");
static_assert(offsetof(CSkillListGump, listTop) == 0xa0, "CSkillListGump::listTop");
static_assert(offsetof(CSkillListGump, listRight) == 0xa4, "CSkillListGump::listRight");
static_assert(offsetof(CSkillListGump, listBottom) == 0xa8, "CSkillListGump::listBottom");
static_assert(offsetof(CSkillListGump, onGroupHeader) == 0xac, "CSkillListGump::onGroupHeader");
static_assert(offsetof(CSkillListGump, groupExpanded) == 0xb0, "CSkillListGump::groupExpanded");
static_assert(offsetof(CSkillListGump, unk_e0) == 0xe0, "CSkillListGump::unk_e0");
static_assert(offsetof(CSkillListGump, unk_e4) == 0xe4, "CSkillListGump::unk_e4");
static_assert(offsetof(CSkillListGump, renaming) == 0xe8, "CSkillListGump::renaming");
static_assert(offsetof(CSkillListGump, cursor) == 0xec, "CSkillListGump::cursor");
static_assert(offsetof(CSkillListGump, unk_f0) == 0xf0, "CSkillListGump::unk_f0");
static_assert(offsetof(CSkillListGump, selIndex) == 0xf4, "CSkillListGump::selIndex");
static_assert(offsetof(CSkillListGump, selGroup) == 0xf8, "CSkillListGump::selGroup");
static_assert(offsetof(CSkillListGump, scrollPos) == 0xfc, "CSkillListGump::scrollPos");
static_assert(offsetof(CSkillListGump, scrollRange) == 0x100, "CSkillListGump::scrollRange");
static_assert(offsetof(CSkillListGump, renameGroup) == 0x104, "CSkillListGump::renameGroup");
static_assert(offsetof(CSkillListGump, dragging) == 0x108, "CSkillListGump::dragging");
static_assert(offsetof(CSkillListGump, clickTick) == 0x118, "CSkillListGump::clickTick");
static_assert(offsetof(CSkillListGump, clickPending) == 0x11c, "CSkillListGump::clickPending");
static_assert(offsetof(CSkillListGump, skillValue) == 0x120, "CSkillListGump::skillValue");
static_assert(offsetof(CSkillListGump, useOwnValues) == 0x184, "CSkillListGump::useOwnValues");
static_assert(sizeof(CSkillListGump) == 0x188, "CSkillListGump size (operator new(0x188))");
