// CShopGump second-pass ports: mouse handling, scrolling, cart editing, the two redraws, Draw,
// and the three-part bar drawer 0x0046bc90 that the redraws use for the row separators.
#include "shop_gump.h"

#include <windows.h>

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "batch06_helpers.h"
#include "bill_gump.h"
#include "font_manager.h"
#include "globals.h"
#include "obj.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "art_record_ops_ports.h"
#include "art_row_blits_ports.h"
#include "text_gump_ctor_ports.h"
#include "gump_base_ops_ports.h"
#include "art_blit_ports.h"
#include "art_row_blits_hue_ports.h"

#define sg_hueRegister GLOBAL_AS(uint32_t, g_hueOverride)   // hue index read by the hued row blitters
#define sg_tileArtTable GLOBAL_PTR(void, g_artCache_storage)
#define sg_systemMessageObj GLOBAL_PTR(void, g_msgTarget_storage)

// 0x00476130 on the tile art table: art record of an id, RET 4. Width at +4, height at +6.
#define sg_ArtGet(o, a1) PORT_THISCALL1(&ArtRecord_Get, uint8_t *, (o), unsigned)(a1)
// 0x004767c0: (pixels, stride) of the surface the art walkers draw to.
#define sg_SetBlitSurface(a1, a2) (SetBlitSurface(a1, a2))
// 0x004767d0: ECX = row blitter, EDX = art record, stack (clip, x, y), RET 0xc.
#define sg_RleArtWalk PORT_FUNC5(&RleArtWalk, void, __fastcall, void *, uint8_t *, int *, int, int)
#define SG_BLIT_PLAIN PTR_AT(void, PORT_ADDR(&RowBlitCopyAll))
#define SG_BLIT_HUED PTR_AT(void, PORT_ADDR(&RowBlitLookupAll))
// 0x00479480 on the system message object: (0, 0, text)
#define sg_SystemMessage(o, a1, a2, a3) PORT_THISCALL3(&CreateBarkGump, void, (o), int, int, const char *)(a1, a2, a3)

// An item object as the shop reads it (a CObj-like record; only these fields are touched).
struct ShopObj {
    uint8_t pad_00[0x3c];
    uint16_t qty;           // +0x3c
    uint16_t hue;           // +0x3e
    ShopObj *next;          // +0x40
    uint8_t pad_44[0x78 - 0x44];
    int32_t key;            // +0x78
    ShopObj *parent;        // +0x7c  container the item lies in
    uint8_t pad_80[0x99 - 0x80];
    uint8_t layer;          // +0x99  vendor slot of that container (0x1a / 0x1b)
    uint8_t pad_9a[0xa0 - 0x9a];
    char *name;             // +0xa0
    int32_t price;          // +0xa4
    ShopObj *firstItem;     // +0xa8  (containers)
};
static_assert(offsetof(ShopObj, qty) == 0x3c, "ShopObj::qty");
static_assert(offsetof(ShopObj, next) == 0x40, "ShopObj::next");
static_assert(offsetof(ShopObj, key) == 0x78, "ShopObj::key");
static_assert(offsetof(ShopObj, layer) == 0x99, "ShopObj::layer");
static_assert(offsetof(ShopObj, name) == 0xa0, "ShopObj::name");
static_assert(offsetof(ShopObj, firstItem) == 0xa8, "ShopObj::firstItem");

// The vendor mobile: equipment slots from +0xc4 (slot 0x1a at +0x12c, 0x1b at +0x130).
static ShopObj *vendor_slot(void *vendor, unsigned slot)
{
    return reinterpret_cast<ShopObj **>(static_cast<char *>(vendor) + 0xc4)[slot];
}

// Fields of the shop gump that the header only has as int32.
static ShopObj *&sg_firstVisible(CShopGump *g) { return RAWF(ShopObj *, g, 0xa4); }
static ShopObj *&sg_selected(CShopGump *g) { return RAWF(ShopObj *, g, 0xa8); }
static int32_t &sg_selectedFlag(CShopGump *g) { return RAWF(int32_t, g, 0xb8); }
static CBillGump *sg_bill(CShopGump *g) { return reinterpret_cast<CBillGump *>(g->billGump); }

// Fields of a SHOP_SELL_ITEM that ShopSellItem leaves in its padding.
static int32_t si_key(const ShopSellItem *it) { return RAWF(int32_t, it, 0x00); }
static uint16_t si_graphic(const ShopSellItem *it) { return RAWF(uint16_t, it, 0x04); }
static uint16_t si_hue(const ShopSellItem *it) { return RAWF(uint16_t, it, 0x06); }
static uint16_t si_count(const ShopSellItem *it) { return RAWF(uint16_t, it, 0x08); }

// 0x0046bc90  size 0x111  RET 0xc  ECX = x, EDX = y; stack (art id, frame bitmap, clip)
// Draws a horizontal bar from three consecutive gump arts: left end `artId`, a middle piece `artId + 1` repeated
// until the clip width is covered, and the right end `artId + 2` flush with the right edge. Nothing is drawn
// when any of the three arts is missing.
static void __fastcall ShopBar_Draw(int x, int y, int artId, B6Bitmap *bmp, int *clip)
{
    int wA, hA, wB, hB, wC, hC;
    b6_art_size(artId, &wA, &hA);
    b6_art_size(artId + 1, &wB, &hB);
    b6_art_size(artId + 2, &wC, &hC);
    if (wA == 0 || wB == 0 || wC == 0)
        return;
    int total = clip[2] - clip[0] + 1;
    b6_draw_art_clip(B6_ROWBLIT_COPY, artId, clip, x, y, bmp->px, bmp->w, bmp->h);
    int s = wA;
    if (s < total) {
        do {
            b6_draw_art_clip(B6_ROWBLIT_COPY, artId + 1, clip, s + x, y, bmp->px, bmp->w, bmp->h);
            s += wB;
        } while (s < total);
    }
    b6_draw_art_clip(B6_ROWBLIT_COPY, artId + 2, clip, total + x - wC, y, bmp->px, bmp->w, bmp->h);
}
HOOK_FUNC(0x0046bc90, ShopBar_Draw);

HOOK_VTABLE(0x004a0088, CShopGump);
HOOK_SDD(0x0046c0e0, CShopGump);

// 0x0046c630  size 0x2c0  CShopGump::MouseDown, RET 8
// Records the press, then walks the visible rows (the vendor's first container, then the second) to find the
// row under the click; the row height is the larger of the art height and the wrapped text height, at least 0x2c.
void __thiscall CShopGump::MouseDown(int mx, int my)
{
    char line[0x80];
    int dummyW, dummyH;
    int tw, th, tb;
    int inSecond = unk_b4;

    if (sellMode != 0) {
        SellHandleLButtonDown(mx, my);
        return;
    }
    b6_art_size(0x32, &dummyW, &dummyH);   // results unused
    b6_art_size(0x39, &dummyW, &dummyH);
    BringToFront(0);
    sg_selectedFlag(this) = 0;
    unk_e4 = 1;
    unk_6c = mx;
    unk_70 = my;
    if (unk_f0 == 1) {
        unk_f0 = 2;
    } else {
        unk_f0 = 1;
        unk_ec = static_cast<int32_t>(GetTickCount());
    }
    if (mx < unk_94 || mx > unk_9c || my < unk_98 + 6 || my > unk_a0)
        return;

    ShopObj *item = sg_firstVisible(this);
    int yrel = my - unk_98 - 6;
    uint8_t *rec = nullptr;
    if (item != nullptr)
        rec = PORT_VSLOT0(item, 0x74, uint8_t *)();
    int artH = (rec != nullptr) ? *reinterpret_cast<uint16_t *>(rec + 6) : 0;
    if (item != nullptr)
        sprintf(line, "%s at %dgp", item->name, item->price);
    else
        strcpy(line, "(null)");
    g_fontMgr.MeasureTextWrapped(9, line, 0x64, &tw, &th, &tb, 0);
    if (th > artH)
        artH = th;

    for (;;) {
        int rowH = (artH < 0x2c) ? 0x2c : artH;
        if (yrel <= rowH + 0x12) {
            if (item == nullptr)
                return;
            sg_selectedFlag(this) = 1;
            sg_selected(this) = item;
            if (inSecond != 0) {
                unk_ac = 0x1b;
                RedrawFrame();   // 0x0046d280
                return;
            }
            unk_ac = 0x1a;
            RedrawFrame();   // 0x0046d280
            return;
        }
        if (item == nullptr)
            return;
        yrel = yrel - 0x12 - rowH;
        item = item->next;
        if (item == nullptr) {
            if (inSecond == 0) {
                ShopObj *second = vendor_slot(owner, 0x1b)->firstItem;
                if (second != nullptr) {
                    inSecond = 1;
                    item = second;
                }
            }
            if (item == nullptr)
                continue;
        }
        rec = PORT_VSLOT0(item, 0x74, uint8_t *)();
        artH = (rec != nullptr) ? *reinterpret_cast<uint16_t *>(rec + 6) : 0;
        sprintf(line, "%s at %dgp", item->name, item->price);
        g_fontMgr.MeasureTextWrapped(9, line, 0x64, &tw, &th, &tb, 0);
        if (th > artH)
            artH = th;
    }
}
HOOK_VMETHOD(0x0046c630, CShopGump, MouseDown);

// 0x0046c8f0  size 0x26c  CShopGump::Sell_HandleLButtonDown, RET 8
// The same row search over the sell item array: first visible index at +0xc2, count at +0xc0, selection at +0xc4.
void __thiscall CShopGump::SellHandleLButtonDown(int mx, int my)
{
    char line[0x80];
    int dummyW, dummyH;
    int tw, th, tb;

    b6_art_size(0x32, &dummyW, &dummyH);   // results unused
    b6_art_size(0x39, &dummyW, &dummyH);
    BringToFront(0);
    sg_selectedFlag(this) = 0;
    unk_e4 = 1;
    unk_6c = mx;
    unk_70 = my;
    if (unk_f0 == 1) {
        unk_f0 = 2;
    } else {
        unk_f0 = 1;
        unk_ec = static_cast<int32_t>(GetTickCount());
    }
    if (mx < unk_94 || mx > unk_9c || my < unk_98 + 6 || my > unk_a0)
        return;

    int yrel = my - unk_98 - 6;
    uint16_t idx = unk_c2;
    uint8_t *rec = sg_ArtGet(sg_tileArtTable, si_graphic(&items[idx]) + 0x4000);
    int artH = (rec != nullptr) ? *reinterpret_cast<uint16_t *>(rec + 6) : 0;
    sprintf(line, "%s at %dgp", items[idx].name, static_cast<unsigned int>(items[idx].price));
    g_fontMgr.MeasureTextWrapped(9, line, 0x64, &tw, &th, &tb, 0);
    if (th > artH)
        artH = th;

    for (;;) {
        int rowH = (artH < 0x2c) ? 0x2c : artH;
        if (yrel <= rowH + 0x12 || idx >= itemCount) {
            if (idx >= itemCount)
                return;
            sg_selectedFlag(this) = 1;
            unk_c4 = idx;
            RedrawFrame();
            return;
        }
        yrel = yrel - 0x12 - rowH;
        idx = static_cast<uint16_t>(idx + 1);
        if (idx >= itemCount)
            continue;
        rec = sg_ArtGet(sg_tileArtTable, si_graphic(&items[idx]) + 0x4000);
        artH = (rec != nullptr) ? *reinterpret_cast<uint16_t *>(rec + 6) : 0;
        char *nm = BuildItemName(si_graphic(&items[idx]), 0);   // 0x00453050 (the first row above used the stored name)
        sprintf(line, "%s at %dgp", nm, static_cast<unsigned int>(items[idx].price));
        g_fontMgr.MeasureTextWrapped(9, line, 0x64, &tw, &th, &tb, 0);
        if (th > artH)
            artH = th;
    }
}
HOOK_METHOD(0x0046c8f0, CShopGump, SellHandleLButtonDown);

// 0x0046cb60  size 0x56  CShopGump::MouseUp, RET 8 (arguments not read)
void __thiscall CShopGump::MouseUp(int mx, int my)
{
    (void)mx;
    (void)my;
    unk_e4 = 0;
    if (unk_dc != 0) {
        unk_dc = 0;
        unk_f0 = 0;
        g_dragGump = nullptr;
        return;
    }
    if (unk_f0 == 2) {
        unk_f0 = 0;
        if (sg_selectedFlag(this) != 0)
            RepeatIncrease();   // 0x0040e050
    }
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x0046cb60, CShopGump, MouseUp);

// 0x0046cbc0  size 0xa9  CShopGump::MouseMove, RET 8
void __thiscall CShopGump::MouseMove(int mx, int my)
{
    int pressed = unk_e4;
    if (pressed != 0 && static_cast<uint32_t>(unk_f0) > 0) {
        int dx = mx - unk_6c;
        int dy = my - unk_70;
        if ((dx < 0 ? -dx : dx) > 4 || (dy < 0 ? -dy : dy) > 4) {
            unk_dc = 1;
            unk_f0 = 0;
        }
    }
    if (pressed != 0 && sg_selectedFlag(this) == 0) {
        Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);   // 0x0043b140
        sg_bill(this)->x = x + 0xaa;
        sg_bill(this)->y = y + 0xd6;
    }
}
HOOK_VMETHOD(0x0046cbc0, CShopGump, MouseMove);

// 0x0046cc70  size 0x115  CShopGump::OnScroll, RET 0xc (third argument not read)
// Sets the first visible row to pos / range of the list length: the item array index in sell mode, otherwise a
// walk down the vendor's first container and on into the second (unk_b4 records that the second one was entered).
void __thiscall CShopGump::OnScroll(int pos, int range, int unused)
{
    (void)unused;
    if (sellMode != 0) {
        int cnt = itemCount;
        int sidx = pos * cnt / range;
        if (itemCount != 0 && sidx >= cnt)
            sidx = cnt - 1;
        unk_c2 = static_cast<uint16_t>(sidx);
        RedrawFrame();
        return;
    }

    ShopObj *c1 = vendor_slot(owner, 0x1a);
    int count = 0;
    ShopObj *p = nullptr;
    if (c1 != nullptr) {
        p = c1->firstItem;
        if (p != nullptr) {
            do {
                p = p->next;
                count++;
            } while (p != nullptr);
        }
    }
    ShopObj *c2 = vendor_slot(owner, 0x1b);
    if (c2 != nullptr) {
        p = c2->firstItem;
        if (p != nullptr) {
            do {
                p = p->next;
                count++;
            } while (p != nullptr);
        }
    }
    int idx = count * pos / range;
    if (count != 0 && idx >= count)
        idx = count - 1;

    ShopObj *cur = nullptr;
    unk_b4 = 0;
    if (c1 != nullptr) {
        cur = c1->firstItem;
        if (idx != 0) {
            do {
                if (cur == nullptr)
                    goto L_second;
                cur = cur->next;
                idx--;
                if (cur == nullptr && c2 != nullptr) {
                    cur = c2->firstItem;
                    unk_b4 = 1;
                }
            } while (idx != 0);
        }
        goto L_done;
    }
L_second:
    if (idx != 0 && c2 != nullptr) {
        cur = c2->firstItem;
        unk_b4 = 1;
        while (cur != nullptr) {
            cur = cur->next;
            idx--;
            if (idx == 0)
                break;
        }
    }
L_done:
    sg_firstVisible(this) = cur;
    RedrawFrame();
}
HOOK_VMETHOD(0x0046cc70, CShopGump, OnScroll);

// 0x0046cd90  size 0x6e  CShopGump::Tick
// A single click that stayed unanswered for the double-click time ends; with nothing pressed and a row selected
// while the target cursor is up, the target callback is called with the selected item.
void __thiscall CShopGump::Tick()
{
    uint32_t now = GetTickCount();
    uint32_t dct = GetDoubleClickTime();
    if (unk_f0 == 1 && now - static_cast<uint32_t>(unk_ec) > dct) {
        unk_f0 = 0;
        if (unk_e4 != 0) {
            unk_dc = 1;
        } else if (sg_selectedFlag(this) != 0 && g_targetCallbackActive != 0 && sg_selected(this) != nullptr) {
            decomp::thiscall_at_void(g_targetCallback, sg_selected(this));
            return;
        }
    }
}
HOOK_VMETHOD(0x0046cd90, CShopGump, Tick);

// 0x0046ce00  size 0x1c1  CShopGump::AddToCart, RET 0xc
// Finds (or creates) the cart line for `key`, adds qty to it (clamped by the vendor's stock for slot 0x1a, to 1
// for slot 0x1b, and to 999) and removes the line when it falls to zero. Refreshes the bill gump either way.
void __thiscall CShopGump::AddToCart(int slotArg, int key, int qty)
{
    uint8_t slot = static_cast<uint8_t>(slotArg);
    BillEntry *before = nullptr;
    BillEntry *node = reinterpret_cast<BillEntry *>(nodeList);
    if (node != nullptr) {
        do {
            if (node->key == key)
                break;
            before = node;
            node = node->next;
        } while (node != nullptr);
    }
    ShopObj *item = vendor_slot(owner, slot)->firstItem;
    if (item != nullptr) {
        while (item->key != key) {
            item = item->next;
            if (item == nullptr)
                break;
        }
    }
    int avail = item->qty;   // ? no null check in the original

    if (node == nullptr && qty > 0 && (slot == 0x1b || avail > 0)) {
        BillEntry *n = static_cast<BillEntry *>(::operator new(0x10));   // operator new, no null check
        n->slot = slot;
        n->key = key;
        n->quantity = static_cast<uint16_t>(qty);
        if (slot == 0x1b && static_cast<uint16_t>(qty) > 1)
            n->quantity = 1;
        else if (slot == 0x1a && static_cast<int>(static_cast<uint16_t>(qty)) > avail)
            n->quantity = static_cast<uint16_t>(avail);
        n->next = nullptr;
        if (before != nullptr)
            before->next = n;
        if (nodeList == nullptr) {
            nodeList = reinterpret_cast<uint8_t *>(n);
            sg_bill(this)->entries = n;
        }
        sg_bill(this)->RecomputeTotal();   // 0x0046e2b0
        sg_bill(this)->RedrawFrame();      // 0x0046e5e0
        return;
    }
    if (node == nullptr)
        return;

    int total = qty + node->quantity;
    if (slot == 0x1b && total > 1)
        total = 1;
    else if (slot == 0x1a && total > avail)
        total = avail;
    if (total > 0x3e7)
        total = 0x3e7;
    if (total > 0) {
        node->quantity = static_cast<uint16_t>(total);
    } else {
        if (before != nullptr)
            before->next = node->next;
        if (nodeList == reinterpret_cast<uint8_t *>(node))
            nodeList = reinterpret_cast<uint8_t *>(node->next);
        CBillGump *bill = sg_bill(this);
        if (bill->entries == node) {
            BillEntry *nx = node->next;
            if (nx == nullptr)
                nx = before;
            bill->entries = nx;
        }
        operator delete(node);   // operator delete
    }
    sg_bill(this)->RecomputeTotal();   // 0x0046e2b0
    sg_bill(this)->RedrawFrame();      // 0x0046e5e0
}
HOOK_METHOD(0x0046ce00, CShopGump, AddToCart);

// 0x0046cfd0  size 0x166  CShopGump::Sell_AddToCart, RET 8
// The sell variant: the cart line is matched by the item's serial (first dword of the sell item), limited by the
// item's count (+8) and 999; a line that falls to zero is removed. Lines remember the array index.
void __thiscall CShopGump::SellAddToCart(int index, int qty)
{
    BillEntry *before = nullptr;
    BillEntry *node = reinterpret_cast<BillEntry *>(nodeList);
    if (node != nullptr) {
        int32_t key = si_key(&items[static_cast<uint16_t>(index)]);
        do {
            if (node->key == key)
                break;
            before = node;
            node = node->next;
        } while (node != nullptr);
    }
    ShopSellItem *it = &items[static_cast<uint16_t>(index)];
    int avail = si_count(it);

    if (node == nullptr && qty > 0 && avail > 0) {
        BillEntry *n = static_cast<BillEntry *>(::operator new(0x10));   // operator new, no null check
        n->quantity = static_cast<uint16_t>(qty);
        n->key = si_key(&items[static_cast<uint16_t>(index)]);
        n->index = static_cast<uint16_t>(index);
        if (static_cast<int>(static_cast<uint16_t>(qty)) > avail)
            n->quantity = static_cast<uint16_t>(avail);
        n->next = nullptr;
        if (before != nullptr)
            before->next = n;
        if (nodeList == nullptr) {
            nodeList = reinterpret_cast<uint8_t *>(n);
            sg_bill(this)->entries = n;
        }
        sg_bill(this)->RecomputeTotal();   // 0x0046e2b0
        sg_bill(this)->RedrawFrame();      // 0x0046e5e0
        return;
    }
    if (node == nullptr)
        return;

    int total = qty + node->quantity;
    if (total > avail)
        total = avail;
    if (total > 0x3e7)
        total = 0x3e7;
    if (total > 0) {
        node->quantity = static_cast<uint16_t>(total);
    } else {
        if (before != nullptr)
            before->next = node->next;
        if (nodeList == reinterpret_cast<uint8_t *>(node))
            nodeList = reinterpret_cast<uint8_t *>(node->next);
        CBillGump *bill = sg_bill(this);
        if (bill->entries == node) {
            BillEntry *nx = node->next;
            if (nx == nullptr)
                nx = before;
            bill->entries = nx;
        }
        operator delete(node);   // operator delete
    }
    sg_bill(this)->RecomputeTotal();   // 0x0046e2b0
    sg_bill(this)->RedrawFrame();      // 0x0046e5e0
}
HOOK_METHOD(0x0046cfd0, CShopGump, SellAddToCart);

// 0x0046d140  size 0x140  CShopGump::CheckCartItem, RET 4
// Buy mode only. The cart line for `key` is looked up; when the vendor no longer has the item the line is
// dropped ("Item removed from shopping list."), when a slot 0x1a item has less stock than the line asks for the
// quantity is lowered ("%s dropped to quantity (%d).").
void __thiscall CShopGump::CheckCartItem(int key)
{
    char msg[0x50];
    if (sellMode != 0)
        return;
    BillEntry *before = nullptr;
    BillEntry *node = reinterpret_cast<BillEntry *>(nodeList);
    while (node != nullptr) {
        if (node->key == key)
            break;
        before = node;
        node = node->next;
    }
    if (node == nullptr)
        return;

    uint8_t slot = node->slot;
    ShopObj *item = vendor_slot(owner, slot)->firstItem;
    if (item != nullptr) {
        while (item->key != key) {
            item = item->next;
            if (item == nullptr)
                break;
        }
    }
    if (item != nullptr) {
        if (slot != 0x1a)
            return;
        uint16_t have = item->qty;
        if (have >= node->quantity)
            return;
        node->quantity = have;
        CObj *obj = reinterpret_cast<CObj *>(item);
        sprintf(msg, "%s dropped to quantity (%d).", obj->ItemName(), static_cast<unsigned int>(have));   // 0x00453190
        sg_SystemMessage(sg_systemMessageObj, 0, 0, msg);
        sg_bill(this)->RedrawFrame();   // 0x0046e5e0
        return;
    }

    if (before != nullptr)
        before->next = node->next;
    if (nodeList == reinterpret_cast<uint8_t *>(node))
        nodeList = reinterpret_cast<uint8_t *>(node->next);
    CBillGump *bill = sg_bill(this);
    if (bill->entries == node) {
        if (node->next != nullptr)
            bill->entries = node->next;
        else
            bill->entries = before;
    }
    operator delete(node);   // operator delete
    sg_SystemMessage(sg_systemMessageObj, 0, 0, "Item removed from shopping list.");
    sg_bill(this)->RedrawFrame();   // 0x0046e5e0
}
HOOK_METHOD(0x0046d140, CShopGump, CheckCartItem);

// 0x0046d280  size 0x3f1  CShopGump::RedrawFrame
// Buy mode (sell mode continues in 0x0046d680): background art, then for each visible item of the vendor's two
// containers a separator bar, the "%s at %dgp" text, the quantity still available at the right and the item art,
// until the list rectangle is full; finally a closing bar.
void __thiscall CShopGump::RedrawFrame()
{
    char buf[0x80];
    int clip[4];   // {left, top, right, bottom}
    int sizeW, sizeH;
    int tw, th, tb;
    int inSecond = unk_b4;

    if (sellMode != 0) {
        SellRedrawFrame();
        return;
    }
    b6_art_size(0x32, &sizeW, &sizeH);   // results unused
    B6Bitmap *bmp = b6_frame_bitmap(this, 0);
    b6_draw_art(B6_ROWBLIT_COPY, unk_50, 0, 0, bmp->px, bmp->w, bmp->h);

    clip[0] = unk_94 + 2;
    clip[2] = unk_9c - 2;
    clip[1] = unk_98 + 2;
    clip[3] = unk_a0 - 2;
    ShopObj *item = sg_firstVisible(this);
    int yy = clip[1] + 6;

    if (item != nullptr) {
        for (;;) {
            if (item->parent == nullptr)
                break;
            if (yy > clip[3])
                return;
            if (item != sg_firstVisible(this)) {
                int ny = yy + 0x12;
                if (ny < clip[3])
                    ShopBar_Draw(clip[0], yy, 0x39, b6_frame_bitmap(this, 0), clip);
                yy = ny;
            }
            uint8_t *art = PORT_VSLOT0(item, 0x74, uint8_t *)();
            uint32_t artH = (art != nullptr) ? *reinterpret_cast<uint16_t *>(art + 6) : 0x2c;
            unsigned hue = (item != sg_selected(this)) ? 0x21f : 0x21;
            sprintf(buf, "%s at %dgp", item->name, item->price);
            g_fontMgr.MeasureTextWrapped(9, buf, 0x64, &tw, &th, &tb, 0);
            if (th > static_cast<int>(artH & 0xffff))
                artH = static_cast<uint32_t>(th);
            g_fontMgr.DrawTextWrapped(clip[0] + 0x32, yy + 2, 9, static_cast<int>(hue & 0xffff), buf, 0x64,
                                      b6_frame_bitmap(this, 0)->px, width, clip, 0, 0);

            BillEntry *cart = reinterpret_cast<BillEntry *>(nodeList);
            if (item->parent->layer == 0x1b) {
                if (cart != nullptr) {
                    while (cart->key != item->key) {
                        cart = cart->next;
                        if (cart == nullptr)
                            break;
                    }
                }
                int q = (cart != nullptr) ? cart->quantity : 0;
                sprintf(buf, "%d", 1 - q);
            } else {
                if (cart != nullptr) {
                    while (cart->key != item->key) {
                        cart = cart->next;
                        if (cart == nullptr)
                            break;
                    }
                }
                int q = (cart != nullptr) ? cart->quantity : 0;
                sprintf(buf, "%d", static_cast<int>(item->qty) - q);
            }
            g_fontMgr.MeasureText(9, buf, &tw, &th, &tb);
            int h2 = ((artH & 0xffff) < 0x2c) ? 0x2c : static_cast<int>(artH & 0xffff);
            g_fontMgr.DrawTextPlain(clip[2] - tw, tb + yy + h2 - 0x1d, 9, static_cast<int>(hue & 0xffff), buf,
                                    b6_frame_bitmap(this, 0)->px, width, clip);

            if (art != nullptr) {
                sg_SetBlitSurface(b6_frame_bitmap(this, 0)->px, b6_frame_bitmap(this, 0)->w);   // 0x004767c0
                uint16_t artHue = item->hue;
                if (artHue != 0) {
                    if (artHue > 0xea60)
                        sg_hueRegister = 1;
                    else
                        sg_hueRegister = static_cast<uint32_t>(artHue) - 1;
                    if (static_cast<uint16_t>(artH) < 0x2c) {
                        int off = (0x2c - static_cast<int>(artH & 0xffff)) / 2;
                        sg_RleArtWalk(SG_BLIT_HUED, art, clip, clip[0], yy + off);   // 0x004767d0
                    } else {
                        sg_RleArtWalk(SG_BLIT_HUED, art, clip, clip[0], yy);
                    }
                } else {
                    if (static_cast<uint16_t>(artH) < 0x2c) {
                        int off = (0x2c - static_cast<int>(artH & 0xffff)) / 2;
                        sg_RleArtWalk(SG_BLIT_PLAIN, art, clip, clip[0], yy + off);   // 0x004767d0
                    } else {
                        sg_RleArtWalk(SG_BLIT_PLAIN, art, clip, clip[0], yy);
                    }
                }
            }
            int h3 = (static_cast<uint16_t>(artH) < 0x2c) ? 0x2c : static_cast<int>(artH & 0xffff);
            item = item->next;
            yy = yy + h3;
            if (item != nullptr)
                continue;
            if (inSecond == 0) {
                inSecond = 1;
                item = vendor_slot(owner, 0x1b)->firstItem;
            }
            if (item == nullptr)
                break;
        }
    }
    if (yy > clip[3])
        return;
    ShopBar_Draw(clip[0], yy, 0x39, b6_frame_bitmap(this, 0), clip);
}
HOOK_METHOD(0x0046d280, CShopGump, RedrawFrame);

// 0x0046d680  size 0x3b2  CShopGump::Sell_RedrawFrame
// Sell mode: the same layout over the sell item array, from the first visible index (+0xc2) up to the count
// (+0xc0); the highlighted row is the selected index (+0xc4). Row art is item graphic + 0x4000.
void __thiscall CShopGump::SellRedrawFrame()
{
    char buf[0x80];
    int clip[4];   // {left, top, right, bottom}
    int sizeW, sizeH;
    int tw, th, tb;

    b6_art_size(0x32, &sizeW, &sizeH);   // results unused
    B6Bitmap *bmp = b6_frame_bitmap(this, 0);
    b6_draw_art(B6_ROWBLIT_COPY, unk_50, 0, 0, bmp->px, bmp->w, bmp->h);

    clip[2] = unk_9c - 2;
    clip[0] = unk_94 + 2;
    clip[1] = unk_98 + 2;
    clip[3] = unk_a0 - 2;
    uint16_t idx = unk_c2;
    int yy = clip[1] + 6;

    if (idx < itemCount) {
        for (;;) {
            if (yy > clip[3])
                return;
            if (idx != unk_c2) {
                int ny = yy + 0x12;
                if (ny < clip[3])
                    ShopBar_Draw(clip[0], yy, 0x39, b6_frame_bitmap(this, 0), clip);
                yy = ny;
            }
            ShopSellItem *it = &items[idx];
            uint8_t *art = sg_ArtGet(sg_tileArtTable, si_graphic(it) + 0x4000);
            uint32_t artH = (art != nullptr) ? *reinterpret_cast<uint16_t *>(art + 6) : 0x2c;
            unsigned hue = (idx != unk_c4) ? 0x21f : 0x21;
            sprintf(buf, "%s at %dgp", it->name, static_cast<unsigned int>(it->price));
            g_fontMgr.MeasureTextWrapped(9, buf, 0x64, &tw, &th, &tb, 0);
            if (th > static_cast<int>(artH & 0xffff))
                artH = static_cast<uint32_t>(th);
            g_fontMgr.DrawTextWrapped(clip[0] + 0x32, yy + 2, 9, static_cast<int>(hue & 0xffff), buf, 0x64,
                                      b6_frame_bitmap(this, 0)->px, width, clip, 0, 0);

            BillEntry *cart = reinterpret_cast<BillEntry *>(nodeList);
            if (cart != nullptr) {
                int32_t key = si_key(it);
                while (cart->key != key) {
                    cart = cart->next;
                    if (cart == nullptr)
                        break;
                }
            }
            int q = (cart != nullptr) ? cart->quantity : 0;
            sprintf(buf, "%d", static_cast<int>(si_count(it)) - q);
            g_fontMgr.MeasureText(9, buf, &tw, &th, &tb);
            int h2 = ((artH & 0xffff) < 0x2c) ? 0x2c : static_cast<int>(artH & 0xffff);
            g_fontMgr.DrawTextPlain(clip[2] - tw, tb + yy + h2 - 0x1d, 9, static_cast<int>(hue & 0xffff), buf,
                                    b6_frame_bitmap(this, 0)->px, width, clip);

            if (art != nullptr) {
                sg_SetBlitSurface(b6_frame_bitmap(this, 0)->px, b6_frame_bitmap(this, 0)->w);   // 0x004767c0
                uint16_t artHue = si_hue(it);
                if (artHue != 0) {
                    if (artHue > 0xea60)
                        sg_hueRegister = 1;
                    else
                        sg_hueRegister = static_cast<uint32_t>(artHue) - 1;
                    if (static_cast<uint16_t>(artH) < 0x2c) {
                        int off = (0x2c - static_cast<int>(artH & 0xffff)) / 2;
                        sg_RleArtWalk(SG_BLIT_HUED, art, clip, clip[0], yy + off);   // 0x004767d0
                    } else {
                        sg_RleArtWalk(SG_BLIT_HUED, art, clip, clip[0], yy);
                    }
                } else {
                    if (static_cast<uint16_t>(artH) < 0x2c) {
                        int off = (0x2c - static_cast<int>(artH & 0xffff)) / 2;
                        sg_RleArtWalk(SG_BLIT_PLAIN, art, clip, clip[0], yy + off);   // 0x004767d0
                    } else {
                        sg_RleArtWalk(SG_BLIT_PLAIN, art, clip, clip[0], yy);
                    }
                }
            }
            int h3 = (static_cast<uint16_t>(artH) < 0x2c) ? 0x2c : static_cast<int>(artH & 0xffff);
            yy = yy + h3;
            idx = static_cast<uint16_t>(idx + 1);
            if (idx < itemCount)
                continue;
            break;
        }
    }
    if (yy > clip[3])
        return;
    ShopBar_Draw(clip[0], yy, 0x39, b6_frame_bitmap(this, 0), clip);
}
HOOK_METHOD(0x0046d680, CShopGump, SellRedrawFrame);

// 0x0046da40  size 0xbb  CShopGump::Draw, RET 4
// Blits the frame bitmap to the back buffer (clipped), then draws each child through its vtable slot 0x08.
void __thiscall CShopGump::Draw(int *clip)
{
    int sx = 0;
    int sy = 0;
    int rect[4];
    LocalToScreen(&sx, &sy);
    rect[0] = sx;
    rect[1] = sy;
    rect[2] = sx + b6_frame_bitmap(this, 0)->w;
    rect[3] = sy + b6_frame_bitmap(this, 0)->h;
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, int *, int *)(clip, rect) != 0) {   // GumpRectIntersects
        B6Bitmap *b = b6_frame_bitmap(this, 0);
        GFX_BlitFrame(gfx::artPlain(), b->px, clip, b->w, b->h, sx, sy, g_backBuffer, 0x280, 0x1e0);
    }
    CGump *child = firstChild;
    if (child != nullptr) {
        do {
            PORT_VSLOT1(child, 0x08, void, int *)(clip);
            child = child->next;
        } while (child != nullptr);
    }
}
HOOK_VMETHOD(0x0046da40, CShopGump, Draw);

// Own copies of bodies shared with other classes in the original (the linker folds them).
int __thiscall CShopGump::HitTest(int px, int py)
{
    if (px < 0 || py < 0 || px >= width || py >= height)
        return 0;
    if (b6_frame_bitmap(this, 0)->px[py * width + px] != 0)
        return 1;
    LocalToScreen(&px, &py);   // 0x0043b4e0
    for (CGump *c = lastChild; c != nullptr; c = c->prev) {
        int lx = px;
        int ly = py;
        c->ScreenToLocal(&lx, &ly);   // 0x0043b510
        if (PORT_VSLOT2(c, 0x0c, int, int, int)(lx, ly) != 0)
            return 1;
    }
    return 0;
}

void __thiscall CShopGump::RButtonUp(int a, int b)
{
    (void)a;
    (void)b;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}

