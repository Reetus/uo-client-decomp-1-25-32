// CMapGump: the map gump ("map gump", vtable 0x0049f5e0). Derived fields start
// at +0x94; only the offsets the ported code touches are named. Size 0xdc
// (operator new(0xdc) at 0x00436c70). decomp/pass2/0x00444180.cpp,
// 0x00444ee0.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CMapGump : public CGump {
public:
    virtual int IsTag68() { return 1; }
    virtual int IsTag70() { return 1; }
    virtual int IsTag80() { return 0; }
    // slot override (gump virtual-slot prep): own copy of the shared body
    virtual int HitTest(int x, int y);
    int32_t unk_94;     // +0x94
    int32_t unk_98;     // +0x98  cleared on the item / blocked paths of OnLButtonDown
    int32_t unk_9c;     // +0x9c
    int32_t unk_a0;     // +0xa0
    int32_t unk_a4;     // +0xa4  set to 1 on left button down
    int32_t unk_a8;     // +0xa8
    int32_t unk_ac;     // +0xac  cleared on the item / blocked paths of OnLButtonDown
    int32_t unk_b0;     // +0xb0
    int32_t unk_b4;     // +0xb4
    int32_t unk_b8;     // +0xb8
    int32_t unk_bc;     // +0xbc
    int32_t unk_c0;     // +0xc0
    int32_t unk_c4;     // +0xc4
    int32_t unk_c8;     // +0xc8
    int32_t unk_cc;     // +0xcc
    int32_t unk_d0;     // +0xd0
    int32_t unk_d4;     // +0xd4
    int32_t unk_d8;     // +0xd8

    // 0x00444180  size 0x108  constructor.
    CMapGump();
    // 0x00444ee0  size 0x19a  left button down at (x, y): pick up / start dragging the object under the cursor.
    void __thiscall OnLButtonDown(int x, int y);

    // Second pass, map_gump_ops.cpp. Vtable 0x0049f5e0 (the world view gump): the slots below are named by the
    // CGump event slot they fill; 0x00444ee0 above (called from the drag paths) is not a slot.
    // 0x00444290: slot 0, RET 4.
    // 0x004442c0: sets the vtable, then the CGump destructor.
    ~CMapGump();
    void Dtor() { this->CMapGump::~CMapGump(); }
    // 0x00444300 / 0x004443f0: slots 0x10 / 0x14 (left button down / up), RET 8.
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    // 0x004444d0 / 0x00444540: slots 0x18 / 0x1c (right button down / up), RET 8.
    virtual void RButtonDown(int x, int y);
    virtual void RButtonUp(int x, int y);
    // 0x004445f0: slot 0x20 (mouse move), RET 8: refresh the hover highlights at +0xd4 / +0xd8.
    virtual void MouseOver(int x, int y);
    // 0x00444700: slot 0x24: drop both hover highlights.
    virtual void MouseLeave();
    // 0x00444740: slot 0x28 (mouse move with a button held), RET 8.
    virtual void MouseMove(int x, int y);
    // 0x00444810: slot 0x2c (idle): auto-walk, click timeouts.
    virtual void Tick();
    // 0x00444a70  RET 8: single click at (x, y): answer the target cursor or look at / follow the object.
    void __thiscall OnSingleClick(int x, int y);
    // 0x00444d50  RET 4: double click on `obj` (use, attack or open).
    void __thiscall OnDoubleClick(CObj *obj);
    // 0x004450a0  RET 8 (this is not read): start auto-walking to the object under (x, y).
    void __thiscall StartPathfind(int x, int y);
};

static_assert(offsetof(CMapGump, unk_94) == 0x94, "CMapGump::unk_94");
static_assert(offsetof(CMapGump, unk_98) == 0x98, "CMapGump::unk_98");
static_assert(offsetof(CMapGump, unk_9c) == 0x9c, "CMapGump::unk_9c");
static_assert(offsetof(CMapGump, unk_a0) == 0xa0, "CMapGump::unk_a0");
static_assert(offsetof(CMapGump, unk_a4) == 0xa4, "CMapGump::unk_a4");
static_assert(offsetof(CMapGump, unk_a8) == 0xa8, "CMapGump::unk_a8");
static_assert(offsetof(CMapGump, unk_ac) == 0xac, "CMapGump::unk_ac");
static_assert(offsetof(CMapGump, unk_b0) == 0xb0, "CMapGump::unk_b0");
static_assert(offsetof(CMapGump, unk_b4) == 0xb4, "CMapGump::unk_b4");
static_assert(offsetof(CMapGump, unk_b8) == 0xb8, "CMapGump::unk_b8");
static_assert(offsetof(CMapGump, unk_bc) == 0xbc, "CMapGump::unk_bc");
static_assert(offsetof(CMapGump, unk_c0) == 0xc0, "CMapGump::unk_c0");
static_assert(offsetof(CMapGump, unk_c4) == 0xc4, "CMapGump::unk_c4");
static_assert(offsetof(CMapGump, unk_c8) == 0xc8, "CMapGump::unk_c8");
static_assert(offsetof(CMapGump, unk_cc) == 0xcc, "CMapGump::unk_cc");
static_assert(offsetof(CMapGump, unk_d4) == 0xd4, "CMapGump::unk_d4");
static_assert(offsetof(CMapGump, unk_d8) == 0xd8, "CMapGump::unk_d8");
static_assert(sizeof(CMapGump) == 0xdc, "CMapGump size (operator new(0xdc))");

// 0x004449f0  size 0x76  confirmation dialog "This will lower notoriety !" (variant A).
// ECX = the owner object, stored in DAT_005285f8 (unclear which class: ECX is only stored).
void __fastcall ConfirmLowerNotorietyA(void *self);
