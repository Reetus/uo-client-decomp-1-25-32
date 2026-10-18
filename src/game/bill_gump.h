// CBillGump: the purchase / sale bill shown next to the shop gump.
// Layout from decomp/pass2/0x0046e5e0.cpp and the constructor at 0x0046db00.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CShopGump;

// One bill line (linked from CBillGump::entries).
struct BillEntry {
    uint8_t slot;       // +0x00  vendor equipment slot of the container holding the item
    uint8_t pad_01[3];
    int32_t key;        // +0x04  matched against the item's +0x78
    uint16_t index;     // +0x08  sell: index into the shop's item array
    uint16_t quantity;  // +0x0a
    BillEntry *next;    // +0x0c
};
static_assert(offsetof(BillEntry, key) == 0x04, "BillEntry::key");
static_assert(offsetof(BillEntry, index) == 0x08, "BillEntry::index");
static_assert(offsetof(BillEntry, quantity) == 0x0a, "BillEntry::quantity");
static_assert(offsetof(BillEntry, next) == 0x0c, "BillEntry::next");

class CBillGump : public CGump {
public:
    virtual int IsTag68() { return 1; }
    virtual int IsTaga4() { return 1; }
    // slot override (gump virtual-slot prep): own copy of the shared body
    virtual void RButtonUp(int x, int y);
    // Batch u1 (small_methods_u1.cpp).
    // 0x0040e370: button callback: start the signature animation when it is not running.
    void __thiscall StartSigning();
    // 0x0046e5e0  redraw the text area of the frame bitmap
    void __thiscall RedrawFrame();
    // 0x0040e420  remove every line of the cart from the shop, redraw it and reset the scroll thumb (+0xc0)
    void __thiscall ClearCart();

    // Second pass, bill_gump_ops.cpp.
    // 0x0046db00  size 0x33f  constructor, RET 0x10  (x, y, shop gump, sell flag)
    CBillGump(int x_, int y_, CShopGump *shop_, int sellFlag);
    // 0x0046de70: destructor; detaches and deletes the shop gump too.
    ~CBillGump();
    void Dtor() { this->CBillGump::~CBillGump(); }
    // 0x0046dee0 / 0x0046df50 / 0x0046dfa0  RET 8: left button down / up, mouse move.
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void MouseMove(int x, int y);
    // 0x0046e050: one step of the signature animation; sends the order when it is complete.
    virtual void Update();
    // 0x0046e200  RET 0xc (pos, range, third not read): scroll the bill lines.
    virtual void OnScroll(int a, int b, int c);
    // 0x0046e260: double-click timeout.
    virtual void Tick();
    // 0x0046e2b0: recompute `total` from the shop's cart.
    void __thiscall RecomputeTotal();
    // 0x0046e350: create / move / delete the per-line +/- buttons so that they match the visible lines.
    void __thiscall SyncLineButtons();
    // 0x0046e970  vtable slot 8, RET 4: blit the frame bitmap, then draw the children (the list area clipped separately)
    virtual void Draw(int *clip);
    // 0x0046eac0  vtable slot 0x0c, RET 8: (x, y) hits an opaque frame pixel or one of the children
    virtual int HitTest(int x, int y);

    int32_t listLeft;       // +0x94
    int32_t listTop;        // +0x98
    int32_t listRight;      // +0x9c
    int32_t listBottom;     // +0xa0
    int32_t sellMode;       // +0xa4  0 = buy bill, else sell bill
    int32_t unk_a8;         // +0xa8  cleared on a press
    int32_t signing;        // +0xac  signature animation running (0x0046e050)
    CShopGump *shop;        // +0xb0
    BillEntry *entries;     // +0xb4
    int32_t total;          // +0xb8
    int32_t sigWidth;       // +0xbc  pixels of the signature drawn so far
    uint8_t *scrollThumb;   // +0xc0
    int32_t dragStarted;    // +0xc4
    int32_t unk_c8;         // +0xc8
    int32_t pressed;        // +0xcc
    int32_t unk_d0;         // +0xd0
    uint32_t clickTick;     // +0xd4  GetTickCount() of the first click
    int32_t clickState;     // +0xd8  0 / 1 / 2 clicks
};

static_assert(offsetof(CBillGump, listLeft) == 0x94, "CBillGump::listLeft");
static_assert(offsetof(CBillGump, listTop) == 0x98, "CBillGump::listTop");
static_assert(offsetof(CBillGump, listRight) == 0x9c, "CBillGump::listRight");
static_assert(offsetof(CBillGump, listBottom) == 0xa0, "CBillGump::listBottom");
static_assert(offsetof(CBillGump, sellMode) == 0xa4, "CBillGump::sellMode");
static_assert(offsetof(CBillGump, shop) == 0xb0, "CBillGump::shop");
static_assert(offsetof(CBillGump, entries) == 0xb4, "CBillGump::entries");
static_assert(offsetof(CBillGump, total) == 0xb8, "CBillGump::total");
static_assert(offsetof(CBillGump, signing) == 0xac, "CBillGump::signing");
static_assert(offsetof(CBillGump, scrollThumb) == 0xc0, "CBillGump::scrollThumb");
static_assert(offsetof(CBillGump, clickState) == 0xd8, "CBillGump::clickState");
static_assert(sizeof(CBillGump) == 0xdc, "CBillGump size (operator new(0xdc))");
