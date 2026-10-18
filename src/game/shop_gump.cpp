#include "shop_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch06_helpers.h"
#include "globals.h"
#include "scroll_thumb_gump.h"
#include "button_gump.h"
#include "bill_gump.h"
#include "obj.h"
#include "gump.h"
#include "ui_callbacks_ports.h"
#include "vtables.h"
#include "crt.h"

// 0x00413740 scroll thumb ctor, 6 stack args. 0x00413c00 scrollbar update on the thumb (value, max).
#define ScrollThumb_Update(self, a1, a2) (reinterpret_cast<CScrollThumbGump *>(self)->SetPosition(a1, a2))
// 0x00413110 button ctor, 9 stack args: (x, y, x2, y2, parent, 2, 0, callback, 0).
// 0x0046d280 on the shop gump: populate the list.
#define Shop_Populate(self) (self->*PORT_METHOD0(CShopGump, void, &CShopGump::RedrawFrame))()
// 0x00453ad0 on the vendor's container objects, no args.
#define Container_Clear(self) (reinterpret_cast<CContainerObj *>(self)->DestroyChildren())

// The part of the constructors both variants share up to and including the vendor link.
static void link_vendor(CShopGump *self, void *vendor)
{
    self->owner = static_cast<CObj *>(vendor);
    if (vendor != nullptr) {
        if (PORT_VSLOT0(vendor, 0x2c, int)() != 0) {
            char *o = reinterpret_cast<char *>(self->owner);
            if (*reinterpret_cast<int32_t *>(o + 0x88) != 0)
                *reinterpret_cast<CShopGump **>(o + 0x8c) = self;
            else
                *reinterpret_cast<CShopGump **>(o + 0x88) = self;
        }
    }
}

// Frame setup of both constructors: art 0x870 (buy) or 0x872 (sell), frame data and the frame bitmap.
static void build_frame(CShopGump *self, int artId)
{
    self->firstChild = nullptr;
    self->parent = nullptr;
    self->lastChild = nullptr;
    self->next = nullptr;
    self->prev = nullptr;
    self->unk_50 = artId;
    b6_art_size(artId, &self->width, &self->height);
    self->rowStride = self->width;
    self->frameIndex = 0;
    self->frameCount = 1;
    int32_t *fd = static_cast<int32_t *>(b6_alloc(g_frameDataAllocator, 8, "framedata for CShopGump"));
    self->frameData = fd;
    memset(fd, 0, self->frameCount * 8);
    b6_set_frame_bitmap(self, 0, b6_alloc(g_frameDataAllocator, self->width * self->height * 2 + 4, "shopgump frame"));
    memset(b6_frame_bitmap(self, 0)->px, 0, (self->width * self->height) * 2);
    b6_frame_bitmap(self, 0)->w = static_cast<uint16_t>(self->width);
    b6_frame_bitmap(self, 0)->h = static_cast<uint16_t>(self->height);
}

// 0x0046bdb0  size 0x32d  CShopGump::CShopGump(int x, int y, CDynamicObj *vendor)  (buy variant), RET 0xc
// The SEH frame (try levels around the four operator-new blocks) is omitted.
CShopGump::CShopGump(int x_, int y_, void *vendor)
{
    name = const_cast<char *>("shop gump");
    dirty = 0;
    unk_e8 = 0;
    unk_e4 = 0;
    unk_e0 = 0;
    unk_dc = 0;
    y = y_;
    unk_f0 = 0;
    unk_ec = 0;
    unk_bc = 0;
    sellMode = 0;   // 0 = buy mode (sell variant sets 1)
    itemCount = 0;
    items = nullptr;
    unk_d0 = 0;
    x = x_;
    link_vendor(this, vendor);

    build_frame(this, 0x870);

    unk_94 = 0x1e;
    unk_9c = 0xe4;
    unk_98 = 0x3a;
    unk_a0 = 0xec;
    nodeList = nullptr;

    void *mem;
    uint8_t *thumb;
    thumb = reinterpret_cast<uint8_t *>(new CScrollThumbGump(0xec, 0x42, 1, 0xbc, (int)(this), 0x828));
    scrollThumb = thumb;
    CGump_AddChild(thumb, this, 1);
    ScrollThumb_Update(scrollThumb, 0, 1);

    mem = new CButtonGump(0xe8, 0x30, 0xfb, 0x3d, this, 2, 0, (int)(reinterpret_cast<void *>(PORT_ADDR(&UiCb_ShopPrev))), 0);
    CGump_AddChild(mem, this, 1);

    mem = new CButtonGump(0xea, 0xc0, 0xfc, 0xce, this, 2, 0, (int)(reinterpret_cast<void *>(PORT_ADDR(&UiCb_ShopNext))), 0);
    CGump_AddChild(mem, this, 1);

    // vendor container slot (+0x12c) -> +0xa8
    unk_a4 = *reinterpret_cast<int32_t *>(*reinterpret_cast<char **>(reinterpret_cast<char *>(owner) + 0x12c) + 0xa8);
    unk_ac = 0x1a;
    unk_a8 = *reinterpret_cast<int32_t *>(*reinterpret_cast<char **>(reinterpret_cast<char *>(owner) + 0x12c) + 0xa8);
    unk_b4 = 0;
    Shop_Populate(this);

    uint8_t *bill = reinterpret_cast<uint8_t *>(new CBillGump(x + 0xaa, y + 0xd6, this, 0));
    billGump = bill;
    CGump_AddChild(bill, nullptr, 1);
}
HOOK_CTOR(0x0046bdb0, CShopGump, (int, int, void *));

// 0x0046c110  size 0x403  CShopGump::CShopGump(int x, int y, CDynamicObj *vendor, ushort count, SHOP_SELL_ITEM *items)  (sell variant), RET 0x14
// Copies the item array and every item name, then builds the same controls as the buy variant.
// The SEH frame is omitted.
CShopGump::CShopGump(int x_, int y_, void *vendor, uint16_t count, ShopSellItem *items_)
{
    name = const_cast<char *>("shop gump");
    dirty = 0;
    unk_e8 = 0;
    unk_e4 = 0;
    unk_e0 = 0;
    unk_dc = 0;
    unk_f0 = 0;
    unk_ec = 0;
    unk_bc = 0;
    sellMode = 1;   // sell mode
    itemCount = count;

    if (count != 0) {
        items = static_cast<ShopSellItem *>(::operator new(static_cast<size_t>(count) << 4));
        memcpy(items, items_, static_cast<size_t>(itemCount) << 4);
        int i = 0;
        if (itemCount != 0) {
            ShopSellItem *src = items_;
            do {
                size_t len = strlen(src->name) + 1;
                items[i].name = static_cast<char *>(::operator new(len));
                strcpy(items[i].name, src->name);
                i = i + 1;
                src = src + 1;
            } while (i < static_cast<int>(itemCount));
        }
    } else {
        items = nullptr;
    }

    x = x_;
    y = y_;
    link_vendor(this, vendor);

    build_frame(this, 0x872);

    unk_94 = 0x1e;
    unk_9c = 0xe4;
    unk_98 = 0x3a;
    unk_a0 = 0xec;
    nodeList = nullptr;
    unk_d0 = 0;

    void *mem;
    uint8_t *thumb;
    thumb = reinterpret_cast<uint8_t *>(new CScrollThumbGump(0xec, 0x42, 1, 0xbc, (int)(this), 0x828));
    scrollThumb = thumb;
    CGump_AddChild(thumb, this, 1);
    ScrollThumb_Update(scrollThumb, 0, 1);

    mem = new CButtonGump(0xe8, 0x30, 0xfb, 0x3d, this, 2, 0, (int)(reinterpret_cast<void *>(PORT_ADDR(&UiCb_ShopPrev))), 0);
    CGump_AddChild(mem, this, 1);

    mem = new CButtonGump(0xea, 0xc0, 0xfc, 0xce, this, 2, 0, (int)(reinterpret_cast<void *>(PORT_ADDR(&UiCb_ShopNext))), 0);
    CGump_AddChild(mem, this, 1);

    unk_a8 = 0;
    unk_a4 = 0;
    unk_c4 = 0;
    unk_c2 = 0;
    Shop_Populate(this);

    uint8_t *bill = reinterpret_cast<uint8_t *>(new CBillGump(x + 0xaa, y + 0xd6, this, 1));   // sell flag
    billGump = bill;
    CGump_AddChild(bill, nullptr, 1);
}
HOOK_CTOR(0x0046c110, CShopGump, (int, int, void *, uint16_t, ShopSellItem *));

// 0x0046c520  size 0x103  CShopGump::~CShopGump
// Frees the item names, the item array and the node list, clears the vendor's containers, deletes the bill gump.
// The SEH frame is omitted.
CShopGump::~CShopGump()
{
    int i = 0;
    int off = 0;
    unk_bc = 1;
    if (itemCount != 0) {
        do {
            operator delete(*reinterpret_cast<void **>(reinterpret_cast<char *>(items) + off + 0xc));   // free item name
            i = i + 1;
            off = off + 0x10;
        } while (i < static_cast<int>(itemCount));
    }
    if (items != nullptr)
        operator delete(items);

    if (nodeList != nullptr) {
        do {
            uint8_t *node = nodeList;
            uint8_t *nxt = *reinterpret_cast<uint8_t **>(node + 0xc);
            operator delete(node);
            nodeList = nxt;
        } while (nodeList != nullptr);
    }

    if (owner != nullptr) {
        char *o = reinterpret_cast<char *>(owner);
        Container_Clear(*reinterpret_cast<void **>(o + 0x12c));
        Container_Clear(*reinterpret_cast<void **>(o + 0x130));
    }

    if (billGump != nullptr) {
        *reinterpret_cast<int32_t *>(billGump + 0xb0) = 0;
        if (billGump != nullptr)
            b6_delete_gump(billGump);
    }
}
HOOK_DTOR(0x0046c520, CShopGump);
