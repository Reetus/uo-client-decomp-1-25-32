// CShopGump: vendor buy / sell window. vtable 0x004a0088, name "shop gump".
// Layout from decomp/pass2/0x0046bdb0.cpp, 0x0046c110.cpp, 0x0046c520.cpp and
// the CBillGump::RedrawFrame accesses (0x0046e5e0).
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

// SHOP_SELL_ITEM: 0x10 bytes. Only the price and the (heap copied) name are touched here.
struct ShopSellItem {
    uint8_t pad_00[0x0a];
    uint16_t price;     // +0x0a
    char *name;         // +0x0c  malloc'd copy made by the sell constructor
};
static_assert(offsetof(ShopSellItem, price) == 0x0a, "ShopSellItem::price");
static_assert(offsetof(ShopSellItem, name) == 0x0c, "ShopSellItem::name");
static_assert(sizeof(ShopSellItem) == 0x10, "ShopSellItem size");

class CShopGump : public CGump {
public:
    virtual int IsTag68() { return 1; }
    virtual int IsTaga0() { return 1; }
    // slot override (gump virtual-slot prep): own copy of the shared body
    virtual int HitTest(int x, int y);
    virtual void RButtonUp(int x, int y);
    // 0x0046bdb0  buy variant, RET 0xc. vendor is the CDynamicObj.
    CShopGump(int x_, int y_, void *vendor);
    // 0x0046c110  sell variant, RET 0x14.
    CShopGump(int x_, int y_, void *vendor, uint16_t count, ShopSellItem *items_);
    // 0x0046c520
    ~CShopGump();
    void Dtor() { this->CShopGump::~CShopGump(); }
    // 0x0040e050  auto-repeat tick of the + button: add 1 / 2 / 5 / 10 of the selected item to the cart
    void __thiscall RepeatIncrease();
    // 0x0040e390  send the buy (0x3b) or sell (0x9f) order for the cart
    void __thiscall SendOrder();

    // Second pass, shop_gump_ops.cpp.
    // 0x0046c630  RET 8 (mx, my): left button down; picks the list row (buy variant, or 0x0046c8f0 in sell mode).
    virtual void MouseDown(int x, int y);
    // 0x0046c8f0  RET 8 (mx, my): the same for the sell item array.
    void __thiscall SellHandleLButtonDown(int mx, int my);
    // 0x0046cb60  RET 8 (arguments not read): left button up.
    virtual void MouseUp(int x, int y);
    // 0x0046cbc0  RET 8 (mx, my): mouse move; drags the gump and moves the bill gump with it.
    virtual void MouseMove(int x, int y);
    // 0x0046cc70  RET 0xc (pos, range, third not read): scroll the list to pos/range of its length.
    virtual void OnScroll(int a, int b, int c);
    // 0x0046cd90: double-click timeout.
    virtual void Tick();
    // 0x0046ce00  RET 0xc: add qty (negative removes) of the item `key` held in vendor slot `slot` to the cart.
    void __thiscall AddToCart(int slot, int key, int qty);
    // 0x0046cfd0  RET 8: sell variant, `index` into the item array.
    void __thiscall SellAddToCart(int index, int qty);
    // 0x0046d140  RET 4: after the vendor's stock changed, clamp or drop the cart line of `key` (buy mode).
    void __thiscall CheckCartItem(int key);
    // 0x0046d280: redraw the frame bitmap (sell mode goes on to 0x0046d680).
    void __thiscall RedrawFrame();
    // 0x0046d680: the sell variant of the redraw.
    void __thiscall SellRedrawFrame();
    // 0x0046da40  RET 4: blit the frame bitmap, then draw the children.
    virtual void Draw(int *clip);

    int32_t unk_94;             // +0x94  0x1e
    int32_t unk_98;             // +0x98  0x3a
    int32_t unk_9c;             // +0x9c  0xe4
    int32_t unk_a0;             // +0xa0  0xec
    int32_t unk_a4;             // +0xa4  buy: vendor container value; sell: 0
    int32_t unk_a8;             // +0xa8  same
    uint8_t unk_ac;             // +0xac  buy: 0x1a
    uint8_t pad_ad[3];
    int32_t sellMode;           // +0xb0  0 = buy, 1 = sell
    int32_t unk_b4;             // +0xb4  cleared in the buy constructor
    int32_t pad_b8;
    int32_t unk_bc;             // +0xbc  cleared; the destructor sets it to 1
    uint16_t itemCount;         // +0xc0  sell item count
    uint16_t unk_c2;            // +0xc2  sell: cleared
    uint16_t unk_c4;            // +0xc4  sell: cleared
    uint8_t pad_c6[2];
    ShopSellItem *items;        // +0xc8  sell: copy of the caller's array
    uint8_t *nodeList;          // +0xcc  list of nodes, next at +0xc
    uint16_t unk_d0;            // +0xd0
    uint8_t pad_d2[2];
    uint8_t *scrollThumb;       // +0xd4
    uint8_t *billGump;          // +0xd8  CBillGump
    int32_t unk_dc;             // +0xdc
    int32_t unk_e0;             // +0xe0
    int32_t unk_e4;             // +0xe4
    int32_t unk_e8;             // +0xe8
    int32_t unk_ec;             // +0xec
    int32_t unk_f0;             // +0xf0
};

static_assert(offsetof(CShopGump, unk_94) == 0x94, "CShopGump::unk_94");
static_assert(offsetof(CShopGump, unk_a4) == 0xa4, "CShopGump::unk_a4");
static_assert(offsetof(CShopGump, unk_a8) == 0xa8, "CShopGump::unk_a8");
static_assert(offsetof(CShopGump, unk_ac) == 0xac, "CShopGump::unk_ac");
static_assert(offsetof(CShopGump, sellMode) == 0xb0, "CShopGump::sellMode");
static_assert(offsetof(CShopGump, unk_b4) == 0xb4, "CShopGump::unk_b4");
static_assert(offsetof(CShopGump, unk_bc) == 0xbc, "CShopGump::unk_bc");
static_assert(offsetof(CShopGump, itemCount) == 0xc0, "CShopGump::itemCount");
static_assert(offsetof(CShopGump, unk_c2) == 0xc2, "CShopGump::unk_c2");
static_assert(offsetof(CShopGump, unk_c4) == 0xc4, "CShopGump::unk_c4");
static_assert(offsetof(CShopGump, items) == 0xc8, "CShopGump::items");
static_assert(offsetof(CShopGump, nodeList) == 0xcc, "CShopGump::nodeList");
static_assert(offsetof(CShopGump, unk_d0) == 0xd0, "CShopGump::unk_d0");
static_assert(offsetof(CShopGump, scrollThumb) == 0xd4, "CShopGump::scrollThumb");
static_assert(offsetof(CShopGump, billGump) == 0xd8, "CShopGump::billGump");
static_assert(offsetof(CShopGump, unk_dc) == 0xdc, "CShopGump::unk_dc");
static_assert(offsetof(CShopGump, unk_f0) == 0xf0, "CShopGump::unk_f0");
static_assert(sizeof(CShopGump) == 0xf4, "CShopGump size (operator new(0xf4), both constructors)");
