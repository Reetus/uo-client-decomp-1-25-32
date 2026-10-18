// CStatusGump: the status window of a mobile (vtable 0x0049fd40, name string
// "status gump" at 0x004ae41c). Layout partial, from decomp/pass2/0x00468060
// (constructor), 0x00468480 (destructor), 0x00468650 (left-button-up),
// 0x00468d40 (bar blit) and 0x00468e00 (RedrawFrame).
//
// Status gumps of other mobiles live in a ring of 8 at 0x007225c0 (count at
// 0x007225e0). Gumps can be snapped to each other: +0xf0 is the head of the
// snap group, +0xf4 the next gump of the group chain, +0xfc/+0x100/+0x104/
// +0x108 the four neighbour links and +0x10c..+0x118 the group's extreme
// members (min x, max x, min y, max y).
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CStatusGump : public CGump {
public:
    virtual int IsTagac() { return 1; }
    // slot override (gump virtual-slot prep): own copy of the shared body
    virtual int HitTest(int x, int y);
    // 0x00468060  size 0x412  constructor (mobile, pinned); RET 8.
    CStatusGump(void *mobile, int pinned);
    // 0x00468480  size 0xf8  destructor.
    ~CStatusGump();
    void Dtor() { this->CStatusGump::~CStatusGump(); }
    // Second pass, batch 25 (status_gump_ops.cpp).
    // 0x00468580  RET 8 (arguments not read): delete the whole snap group, or just this gump.
    virtual void RButtonUp(int x, int y);
    // 0x004685d0  left button down at (x, y), RET 8.
    virtual void MouseDown(int x, int y);
    // 0x00468a90  mouse move, RET 8: drag the gump (and its group), snap to a neighbour.
    virtual void MouseMove(int x, int y);
    // 0x00468c70  the rename edit control was committed: validate the name and send packet 0x75.
    virtual void OnOk();
    // 0x00468cf0  per-tick update: a pending click older than the double-click time becomes a move.
    virtual void Tick();
    // 0x004693e0  draw pass, RET 4.
    virtual void Draw(int *clip);
    // 0x004695e0  RET 4: the snap side (1 left, 2 right, 4 top, 8 bottom) at which this gump touches
    // `other`, or 0.
    uint32_t __thiscall SnapSideTo(CStatusGump *other);
    // 0x004696a0  take this gump out of its snap group and recompute the group's extreme members.
    void __thiscall DetachGroup();
    // 0x00468650  size 0x411  left button released (x and y are not used); RET 8.
    virtual void MouseUp(int x, int y);
    // 0x00468d40  size 0xb6  draw a proportional bar (art 0x805) at (x, y) and register its rect. RET 0x20.
    void __thiscall DrawBar(int bx, int by, int barWidth, int barHeight, uint16_t cur, uint16_t max, uint16_t *bmp, int hueId);
    // 0x00468e00  size 0x5da  redraw the frame. The four stack arguments callers push are unused; RET 0x10.
    void __thiscall RedrawFrame(int a1, int a2, int a3, int a4);

    int32_t unk_94;
    int32_t unk_98;
    char nameText[0x1e];    // +0x9c  mobile name ("" until the server sends it)
    uint8_t sex;            // +0xba  0 draws "M", otherwise "F"
    uint8_t pad_bb;
    uint16_t stat_bc;       // +0xbc  shown at (0x56, row 0x3e)
    uint16_t stat_be;       // +0xbe  shown at (0x56, row 0x4a)
    uint16_t stat_c0;       // +0xc0  shown at (0x56, row 0x56)
    uint16_t hits;          // +0xc2  "%u/%u" with +0xc4 at (0xad, row 0x3e); first bar
    uint16_t hitsMax;       // +0xc4
    uint16_t mana;          // +0xc6  "%u/%u" with +0xc8 at (0xad, row 0x56); third bar (own gump)
    uint16_t manaMax;       // +0xc8
    uint16_t stam;          // +0xca  "%u/%u" with +0xcc at (0xad, row 0x4a); second bar
    uint16_t stamMax;       // +0xcc
    uint32_t stat_d0;       // +0xd0  "%u" at (0xad, row 0x62)
    uint16_t stat_d4;       // +0xd4  "%u" at (0x56, row 0x6e)
    uint16_t stat_d6;       // +0xd6  "%u" at (0xad, row 0x6e)
    int32_t moved;          // +0xd8  set while the gump is being dragged; cleared on release
    int32_t unk_dc;
    int32_t dragging;       // +0xe0
    int32_t unk_e4;
    int32_t unk_e8;
    int32_t clickState;     // +0xec  2 after a plain click
    CStatusGump *groupHead; // +0xf0
    CStatusGump *groupNext; // +0xf4
    CStatusGump *snapTo;    // +0xf8  the gump this one was dropped next to
    CStatusGump *link_fc;   // +0xfc
    CStatusGump *link_100;  // +0x100
    CStatusGump *link_104;  // +0x104
    CStatusGump *link_108;  // +0x108
    CStatusGump *minX;      // +0x10c
    CStatusGump *maxX;      // +0x110
    CStatusGump *minY;      // +0x114
    CStatusGump *maxY;      // +0x118
    void *editControl;      // +0x11c  zero until the edit control exists (see 0x004694d0)
    uint8_t snapSide;       // +0x120  1 left, 2 right, 4 top, 8 bottom
};

static_assert(offsetof(CStatusGump, nameText) == 0x9c, "CStatusGump::nameText");
static_assert(offsetof(CStatusGump, sex) == 0xba, "CStatusGump::sex");
static_assert(offsetof(CStatusGump, stat_bc) == 0xbc, "CStatusGump::stat_bc");
static_assert(offsetof(CStatusGump, hits) == 0xc2, "CStatusGump::hits");
static_assert(offsetof(CStatusGump, hitsMax) == 0xc4, "CStatusGump::hitsMax");
static_assert(offsetof(CStatusGump, mana) == 0xc6, "CStatusGump::mana");
static_assert(offsetof(CStatusGump, manaMax) == 0xc8, "CStatusGump::manaMax");
static_assert(offsetof(CStatusGump, stam) == 0xca, "CStatusGump::stam");
static_assert(offsetof(CStatusGump, stamMax) == 0xcc, "CStatusGump::stamMax");
static_assert(offsetof(CStatusGump, stat_d0) == 0xd0, "CStatusGump::stat_d0");
static_assert(offsetof(CStatusGump, stat_d4) == 0xd4, "CStatusGump::stat_d4");
static_assert(offsetof(CStatusGump, stat_d6) == 0xd6, "CStatusGump::stat_d6");
static_assert(offsetof(CStatusGump, moved) == 0xd8, "CStatusGump::moved");
static_assert(offsetof(CStatusGump, dragging) == 0xe0, "CStatusGump::dragging");
static_assert(offsetof(CStatusGump, clickState) == 0xec, "CStatusGump::clickState");
static_assert(offsetof(CStatusGump, groupHead) == 0xf0, "CStatusGump::groupHead");
static_assert(offsetof(CStatusGump, groupNext) == 0xf4, "CStatusGump::groupNext");
static_assert(offsetof(CStatusGump, snapTo) == 0xf8, "CStatusGump::snapTo");
static_assert(offsetof(CStatusGump, link_fc) == 0xfc, "CStatusGump::link_fc");
static_assert(offsetof(CStatusGump, link_100) == 0x100, "CStatusGump::link_100");
static_assert(offsetof(CStatusGump, link_104) == 0x104, "CStatusGump::link_104");
static_assert(offsetof(CStatusGump, link_108) == 0x108, "CStatusGump::link_108");
static_assert(offsetof(CStatusGump, minX) == 0x10c, "CStatusGump::minX");
static_assert(offsetof(CStatusGump, maxY) == 0x118, "CStatusGump::maxY");
static_assert(offsetof(CStatusGump, editControl) == 0x11c, "CStatusGump::editControl");
static_assert(offsetof(CStatusGump, snapSide) == 0x120, "CStatusGump::snapSide");
static_assert(sizeof(CStatusGump) == 0x124, "CStatusGump size (operator new(0x124))");
