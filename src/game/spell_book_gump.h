// CSpellBookGump: the spellbook window, derived from CBookGump. vtable
// 0x0049c850, gump name "spellbook gump". Layout from decomp/pass2/
// 0x0040a450.cpp .. 0x0040b2d0.cpp. The book object (`owner`, a CDynamicObj)
// keeps its contained spell scrolls in a list starting at owner+0xa8 (next link
// at +0x40, spell id + 1 in the u16 at +0x3c); owner+0xac points back here.
// The allocation size is not established, so there is no sizeof assert.
#pragma once

#include <cstddef>
#include <cstdint>
#include <new>

#include "book_gump.h"

class CSpellBookGump : public CBookGump {
public:
    virtual int IsTag68() { return 1; }
    virtual int IsTagd0() { return 1; }
    // Batch u1 (small_methods_u1.cpp).
    // 0x0040b240 (slot 0x118): next spread (page + 2 while page + 1 < pageCount), redraw (slot 0x114), page sound 0x55.
    virtual void NextPage();
    // 0x0040b290 (slot 0x11c): previous spread (page - 2 while page > 1), redraw, page sound 0x55.
    virtual void PrevPage();
    // 0x0040a450: build the book art, the "next circle" button and draw the first spread.
    CSpellBookGump(int posX, int posY, CObj *book);
    // 0x0040a750: ~CSpellBookGump; the CBookGump dtor (0x004071a0) runs last.
    ~CSpellBookGump();
    void Dtor() { this->CSpellBookGump::~CSpellBookGump(); }
    // 0x0040a810: mouse down: remember the press and pick the spell under the cursor.
    virtual void MouseDown(int x, int y);
    // 0x0040aa30: mouse move while pressed: drag the window, or after > 4 px
    // spawn/grab a spell icon gump for the selected spell.
    virtual void MouseMove(int x, int y);
    // 0x0040b2d0: redraw both pages of the current spread into the frame bitmap.
    virtual void RedrawSpread();
    // 0x0040a9d0 (slot 0x14): mouse up; turns clicks into slot 0x120 / 0x124 calls.
    virtual void MouseUp(int x, int y);
    // 0x0040abb0 (slot 0x120): single click: circle tabs, page corners, or a spell in an index page.
    virtual void OnClickCorner(int x, int y);
    // 0x0040adf0 (slot 0x124): double click: jump to first / last spread or pick a spell (iconify, notify server).
    virtual void OnCornerJump(int x, int y);
    // 0x0040b180 (slot 0x2c): per-tick single-click timeout.
    virtual void Tick();
    // 0x0040b1e0 (slot 0x0c): hit test against the visible frame bitmap (non-zero pixel).
    virtual int HitTest(int x, int y);
    // 0x0040c510: index (in the book's spell list) of the spell listed under (x, y) on an index page, -1 if none.
    int __thiscall GetIndexSpellAt(int mouseX, int mouseY);

    int32_t unk_134;       // +0x134
    uint8_t pad_138[4];
    int32_t mouseDown;     // +0x13c  1 while the left button is held
    int32_t unk_140;       // +0x140
    uint32_t clickTick;    // +0x144  GetTickCount() of the first click
    int32_t clickState;    // +0x148  1 after the first click, 2 after the second
    uint16_t spellCount;   // +0x14c  number of spells in the book
    uint16_t indexPages;   // +0x14e  number of index pages (8)
    uint8_t pad_150[0x178 - 0x150];
    int32_t selectedSpell; // +0x178  spell index (0-based) under the cursor, -1 = none
};

static_assert(offsetof(CSpellBookGump, unk_134) == 0x134, "CSpellBookGump::unk_134");
static_assert(offsetof(CSpellBookGump, mouseDown) == 0x13c, "CSpellBookGump::mouseDown");
static_assert(offsetof(CSpellBookGump, unk_140) == 0x140, "CSpellBookGump::unk_140");
static_assert(offsetof(CSpellBookGump, clickTick) == 0x144, "CSpellBookGump::clickTick");
static_assert(offsetof(CSpellBookGump, clickState) == 0x148, "CSpellBookGump::clickState");
static_assert(offsetof(CSpellBookGump, spellCount) == 0x14c, "CSpellBookGump::spellCount");
static_assert(offsetof(CSpellBookGump, indexPages) == 0x14e, "CSpellBookGump::indexPages");
static_assert(offsetof(CSpellBookGump, selectedSpell) == 0x178, "CSpellBookGump::selectedSpell");
static_assert(sizeof(CSpellBookGump) == 0x17c, "CSpellBookGump size (operator new(0x17c))");
