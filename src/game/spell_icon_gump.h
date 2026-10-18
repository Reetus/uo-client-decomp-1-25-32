// CSpellIconGump: the draggable spell icon dropped out of a spellbook ("spellicon gump", vtable
// 0x0049c978, operator new(0xb4)). Art id is spell index + 0x8c0. Single click is counted with the
// double-click time; the second click sends the cast request (string packet 0x56, "%d" of spell + 1).
// Layout from the constructor 0x0040c660 and the handlers 0x0040c8a0 .. 0x0040ca50.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CSpellIconGump : public CGump {
public:
    virtual int IsTag104() { return 1; }
    // 0x0040c660: RET 0xc. (x, y, spell index 0-based). Returns this.
    CSpellIconGump(int posX, int posY, int spellIndex);
    // 0x0040c810: ~CSpellIconGump; the CGump destructor runs last.
    ~CSpellIconGump();
    void Dtor() { this->CSpellIconGump::~CSpellIconGump(); }
    // 0x0040c8a0 (slot 0x10): mouse down; starts a drag and counts the click.
    virtual void MouseDown(int x, int y);
    // 0x0040c910 (slot 0x14): mouse up; the second click of a double click casts the spell.
    virtual void MouseUp(int x, int y);
    // 0x0040c9b0 (slot 0x28): mouse move; drags the window while dragging.
    virtual void MouseMove(int x, int y);
    // 0x0040c9e0 (slot 0x2c): tick; click timeout.
    virtual void Tick();
    // 0x0040ca50 (slot 0x08): blit the frame bitmap at the screen origin.
    virtual void Draw(int *clip);

    int32_t dragStarted;   // +0x94  set by Tick when the button is still down after the double-click time
    int32_t unk_98;        // +0x98
    int32_t dragging;      // +0x9c  1 between mouse down and mouse up
    int32_t unk_a0;        // +0xa0
    uint32_t clickTick;    // +0xa4  GetTickCount() of the first click
    int32_t clickState;    // +0xa8  1 after the first click, 2 after the second
    int32_t spell;         // +0xac  spell index (0-based)
    int32_t unk_b0;        // +0xb0  counter reset to 0 when it reaches 2 (Tick) and set to 1 on mouse down
    virtual int HitTest(int x, int y);   // slot HitTest, copy of CDeleteCharGump::HitTestFrame
    virtual void RButtonUp(int x, int y);   // slot RButtonUp, copy of CCourseGump::CloseAndDelete
};

static_assert(offsetof(CSpellIconGump, dragStarted) == 0x94, "CSpellIconGump::dragStarted");
static_assert(offsetof(CSpellIconGump, dragging) == 0x9c, "CSpellIconGump::dragging");
static_assert(offsetof(CSpellIconGump, clickTick) == 0xa4, "CSpellIconGump::clickTick");
static_assert(offsetof(CSpellIconGump, clickState) == 0xa8, "CSpellIconGump::clickState");
static_assert(offsetof(CSpellIconGump, spell) == 0xac, "CSpellIconGump::spell");
static_assert(offsetof(CSpellIconGump, unk_b0) == 0xb0, "CSpellIconGump::unk_b0");
static_assert(sizeof(CSpellIconGump) == 0xb4, "CSpellIconGump size (operator new(0xb4))");

// 0x0040caf0: a lone `ret`, the default of the gump slots OnTab/OnOk/OnCancel/Update/Slot60 and the entry destructor body of
// part_list_ops.cpp (called on a PartEntry: a plain function, so that no virtual dispatch can reach it).
void __fastcall Nop_0040caf0(void *self);
