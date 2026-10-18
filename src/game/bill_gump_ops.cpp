// CBillGump second-pass ports: constructor, destructors, mouse handling, signature animation, scrolling,
// total and the per-line button synchronisation. (RedrawFrame is in bill_gump.cpp.)
#include "bill_gump.h"

#include <windows.h>

#include <cstring>

#include "../runtime/ported.h"
#include "batch06_helpers.h"
#include "button_gump.h"
#include "font_manager.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "scroll_thumb_gump.h"
#include "shop_gump.h"
#include "ui_callbacks_ports.h"
#include "skill_gump.h"
#include "vtables.h"


// Button callbacks (addresses in the original image).
#define BG_CB_PREV PORT_ADDR(&UiCb_BillPrev)    // UiCb_BillPrev
#define BG_CB_NEXT PORT_ADDR(&UiCb_BillNext)    // UiCb_BillNext
#define BG_CB_SIGN PORT_ADDR(&CBillGump::StartSigning)
#define BG_CB_CLEAR PORT_ADDR(&CBillGump::ClearCart)   // CBillGump::ClearCart
#define BG_CB_INC PORT_ADDR(&CSkillGump::OnIncrease)
#define BG_CB_DEC PORT_ADDR(&CSkillGump::OnDecrease)

// 0x0046e2b0  size 0x95  CBillGump::RecomputeTotal
// total = sum over the shop's cart lines of price * quantity: from the sell item array (sell bill) or from the
// vendor's item with the line's serial (buy bill; no null check on the lookup, as in the original).
void __thiscall CBillGump::RecomputeTotal()
{
    CShopGump *sh = shop;
    total = 0;
    BillEntry *node = reinterpret_cast<BillEntry *>(sh->nodeList);
    if (node == nullptr)
        return;
    do {
        if (sellMode != 0) {
            ShopSellItem *list = sh->items;
            int price = list[node->index].price;
            total += price * node->quantity;
        } else {
            char *vendor = reinterpret_cast<char *>(sh->owner);
            char *container = *reinterpret_cast<char **>(vendor + 0xc4 + node->slot * 4);
            char *item = *reinterpret_cast<char **>(container + 0xa8);
            for (; item != nullptr; item = *reinterpret_cast<char **>(item + 0x40)) {
                if (node->key == *reinterpret_cast<int32_t *>(item + 0x78))
                    break;
            }
            int price = *reinterpret_cast<int32_t *>(item + 0xa4);
            total += price * node->quantity;
        }
        node = node->next;
    } while (node != nullptr);
}
HOOK_METHOD(0x0046e2b0, CBillGump, RecomputeTotal);

// 0x0046db00  size 0x33f  CBillGump::CBillGump(int x, int y, CShopGump *shop, int sell)  RET 0x10
// Sizes the gump from art 0x871 (buy) or 0x873 (sell), allocates its frame, adds the scroll thumb and the four
// rectangle buttons, and draws it.
CBillGump::CBillGump(int x_, int y_, CShopGump *shop_, int sellFlag)
{
    sellMode = sellFlag;
    name = const_cast<char *>("bill gump");
    dirty = 0;
    unk_d0 = 0;
    pressed = 0;
    unk_c8 = 0;
    dragStarted = 0;
    clickState = 0;
    clickTick = 0;
    signing = 0;
    x = x_;
    y = y_;
    owner = nullptr;
    shop = shop_;
    entries = reinterpret_cast<BillEntry *>(shop_->nodeList);
    RecomputeTotal();   // 0x0046e2b0
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    if (sellMode != 0)
        unk_50 = 0x873;
    else
        unk_50 = 0x871;
    b6_art_size(unk_50, &width, &height);
    rowStride = width;
    frameIndex = 0;
    frameCount = 1;
    int32_t *fd = static_cast<int32_t *>(b6_alloc(g_frameDataAllocator, 8, "framedata for CBillGump"));
    frameData = fd;
    memset(fd, 0, frameCount * 8);
    b6_set_frame_bitmap(this, 0, b6_alloc(g_frameDataAllocator, height * width * 2 + 4, "shopgump frame"));
    memset(b6_frame_bitmap(this, 0)->px, 0, height * width * 2);
    b6_frame_bitmap(this, 0)->w = static_cast<uint16_t>(width);
    b6_frame_bitmap(this, 0)->h = static_cast<uint16_t>(height);

    listLeft = 0x20;
    listRight = 0xe4;
    listTop = 0x42;
    listBottom = 0x9c;

    CScrollThumbGump *thumb;
    thumb = new CScrollThumbGump(0xec, 0x42, 1, 0x9a, (int)(this), 0x828);
    scrollThumb = reinterpret_cast<uint8_t *>(thumb);
    CGump_AddChild(thumb, this, 1);
    gfx::ScrollThumbSet(reinterpret_cast<CScrollThumbGump *>(scrollThumb), 0, 1);

    CButtonGump *btn = new CButtonGump(0xe8, 0x32, 0xfb, 0x3e, this, 2, 0, BG_CB_PREV, 0);
    CGump_AddChild(btn, this, 1);

    btn = new CButtonGump(0xe8, 0x9c, 0xfb, 0xab, this, 2, 0, BG_CB_NEXT, 0);
    CGump_AddChild(btn, this, 1);

    btn = new CButtonGump(0x22, 0xc4, 0x36, 0xda, this, 0, 0, BG_CB_SIGN, 0);
    CGump_AddChild(btn, this, 1);

    btn = new CButtonGump(0xcf, 0xc8, 0xdf, 0xd7, this, 0, 0, BG_CB_CLEAR, 0);
    CGump_AddChild(btn, this, 1);

    RedrawFrame();   // 0x0046e5e0
}
HOOK_CTOR(0x0046db00, CBillGump, (int, int, CShopGump *, int));

HOOK_VTABLE(0x004a01a0, CBillGump);
HOOK_SDD(0x0046de40, CBillGump);

// 0x0046de70  size 0x67  CBillGump::~CBillGump
// Detaches itself from the shop gump and deletes that too, then the CGump destructor runs.
CBillGump::~CBillGump()
{
    if (shop != nullptr) {
        shop->billGump = nullptr;
        if (shop != nullptr)
            PORT_VSLOT1(shop, 0x00, void, int)(1);   // scalar deleting destructor
    }
}
HOOK_DTOR(0x0046de70, CBillGump);

// 0x0046dee0  size 0x64  left button down, RET 8: raise the shop gump, remember the press, count the click
void __thiscall CBillGump::MouseDown(int mx, int my)
{
    shop->BringToFront(0);   // 0x0043ba70
    unk_6c = mx;
    unk_a8 = 0;
    pressed = 1;
    unk_70 = my;
    if (clickState == 1) {
        clickState = 2;
        return;
    }
    clickState = 1;
    clickTick = GetTickCount();
}
HOOK_VMETHOD(0x0046dee0, CBillGump, MouseDown);

// 0x0046df50  size 0x42  left button up, RET 8 (arguments not read)
void __thiscall CBillGump::MouseUp(int mx, int my)
{
    (void)mx;
    (void)my;
    pressed = 0;
    if (dragStarted != 0) {
        dragStarted = 0;
        clickState = 0;
        g_dragGump = nullptr;
        return;
    }
    if (clickState == 2)
        clickState = 0;
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x0046df50, CBillGump, MouseUp);

// 0x0046dfa0  size 0xa9  mouse move, RET 8: a drag starts after 4 pixels; the shop gump follows the bill gump
void __thiscall CBillGump::MouseMove(int mx, int my)
{
    int wasPressed = pressed;
    if (wasPressed != 0 && static_cast<uint32_t>(clickState) > 0) {
        if (abs(mx - unk_6c) > 4 || abs(my - unk_70) > 4) {
            dragStarted = 1;
            clickState = 0;
        }
    }
    if (wasPressed != 0 && unk_a8 == 0) {
        Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);   // 0x0043b140
        shop->x = x - 0xaa;
        shop->y = y - 0xd6;
    }
}
HOOK_VMETHOD(0x0046dfa0, CBillGump, MouseMove);

// 0x0046e050  size 0x1a6  CBillGump::Update
// While +0xac is set: renders the player's name (font 5) into a scratch bitmap and copies the first +0xbc pixels of
// every row into the frame bitmap at row 194, column 72; +0xbc grows by 6 per call up to the text width. When the
// whole name is shown the animation stops and the shop gump sends its order.
void __thiscall CBillGump::Update()
{
    int tw, th, tb;
    int clip[4];

    if (signing == 0)
        return;
    int stillGrowing = 0;
    if (strlen(g_playerName) == 0)
        strcpy(g_playerName, "NoName");
    g_fontMgr.MeasureText(5, g_playerName, &tw, &th, &tb);
    tw = tw + 2;
    if (tw > 0x7c)
        tw = 0x7c;
    sigWidth += 6;
    int progress = sigWidth;
    if (progress < tw)
        stillGrowing = 1;
    if (progress > tw)
        sigWidth = tw;

    uint16_t *buf = static_cast<uint16_t *>(b6_alloc(g_frameDataAllocator, th * tw * 2, "signature"));
    memset(buf, 0, th * tw * 2);
    clip[0] = 0;
    clip[1] = 0;
    clip[2] = tw - 1;
    clip[3] = th - 1;
    g_fontMgr.DrawTextPlain(0, tb, 5, 0x386, g_playerName, buf, tw, clip);

    uint16_t *src = buf;
    uint16_t *dst = b6_frame_bitmap(this, 0)->px + 194 * width + 72;
    int row = 0;
    if (th > 0) {
        do {
            B6_ROWBLIT_COPY(dst, src, sigWidth);
            src += tw;
            row++;
            dst += width;
        } while (row < th);
    }
    b6_free(g_frameDataAllocator, buf, "signature");
    if (stillGrowing == 0) {
        signing = 0;
        shop->SendOrder();   // 0x0040e390
    }
}
HOOK_VMETHOD(0x0046e050, CBillGump, Update);

// 0x0046e200  size 0x5d  CBillGump::OnScroll, RET 0xc (third argument not read)
// Moves the first visible line to pos / range of the cart length.
void __thiscall CBillGump::OnScroll(int pos, int range, int unused)
{
    (void)unused;
    int count = 0;
    for (BillEntry *e = reinterpret_cast<BillEntry *>(shop->nodeList); e != nullptr; e = e->next)
        count++;
    BillEntry *head = reinterpret_cast<BillEntry *>(shop->nodeList);
    int idx = count * pos / range;
    if (count != 0 && idx >= count)
        idx = count - 1;
    entries = head;
    while (idx != 0) {
        entries = entries->next;
        idx--;
    }
    RedrawFrame();   // 0x0046e5e0
}
HOOK_VMETHOD(0x0046e200, CBillGump, OnScroll);

// 0x0046e260  size 0x48  CBillGump::Tick
// A single click that outlived the double-click time ends; if the button is still down it becomes a drag.
void __thiscall CBillGump::Tick()
{
    uint32_t now = GetTickCount();
    uint32_t dct = GetDoubleClickTime();
    if (clickState == 1 && now - clickTick > dct) {
        clickState = 0;
        if (pressed != 0)
            dragStarted = 1;
    }
}
HOOK_VMETHOD(0x0046e260, CBillGump, Tick);

// 0x0046e350  size 0x287  CBillGump: line buttons
// First pass: for every visible cart line make sure a "+" and a "-" button exist (children that answer slot 0xa8,
// carry the flag at +0xac and the line's serial at +0x9c): existing ones are moved to the line's y and shortened to
// fit the list, or deleted when the line is below the list; missing ones are created. Second pass: delete the
// buttons whose serial is no longer among the visible lines. The gump list is walked backwards from the last
// child through the global cursor 0x0064cd7c, which the destructor keeps current. The SEH frame is omitted.
void __thiscall CBillGump::SyncLineButtons()
{
    int sizeW, sizeH;
    BillEntry *lastItem = nullptr;
    b6_art_size(0x32, &sizeW, &sizeH);   // results unused
    BillEntry *item = entries;
    int yy = 0;

    if (item != nullptr) {
        yy = listTop + 0xa;
        do {
            CGump *saved;
            int found = 0;
            CGump *c = lastChild;
            saved = g_gumpDeleteCursor;
            lastItem = item;
            while (c != nullptr) {
                g_gumpDeleteCursor = c->prev;
                CButtonGump *b = static_cast<CButtonGump *>(c);
                if (PORT_VSLOT0(c, 0xa8, int)() != 0 && b->unk_ac != 0 && b->unk_9c == item->key) {
                    if (yy > listBottom - 2) {
                        if (c != nullptr)
                            PORT_VSLOT1(c, 0x00, void, int)(1);   // scalar deleting destructor
                    } else {
                        c->y = yy;
                        int hh = listBottom - yy - 2;
                        if (hh > 0x11)
                            hh = 0x11;
                        c->height = hh;
                        found = 1;
                    }
                }
                c = g_gumpDeleteCursor;
            }
            g_gumpDeleteCursor = saved;
            if (found == 0) {
                CButtonGump *b = new CButtonGump(listRight - 0x2a, yy, reinterpret_cast<int>(this), 2, 0, BG_CB_INC, 0x37, 0x37, 0);
                b->unk_c0 = 0;
                b->unk_ac = 1;
                b->unk_9c = item->key;
                CGump_AddChild(b, this, 1);

                b = new CButtonGump(listRight - 0x16, yy, reinterpret_cast<int>(this), 2, 0, BG_CB_DEC, 0x38, 0x38, 0);
                b->unk_ac = 1;
                b->unk_c0 = 0;
                b->unk_9c = item->key;
                CGump_AddChild(b, this, 1);
            }
            yy += 0x1c;
            item = item->next;
        } while (item != nullptr);
    }

    CGump *c = lastChild;
    CGump *saved = g_gumpDeleteCursor;
    while (c != nullptr) {
        g_gumpDeleteCursor = c->prev;
        if (PORT_VSLOT0(c, 0xa8, int)() != 0 && static_cast<CButtonGump *>(c)->unk_ac != 0) {
            int found = 0;
            BillEntry *e = entries;
            if (e != nullptr && lastItem != nullptr) {
                BillEntry *stop = lastItem->next;
                if (e != stop) {
                    int key = static_cast<CButtonGump *>(c)->unk_9c;
                    BillEntry *nx;
                    do {
                        nx = e->next;
                        if (e->key == key)
                            found = 1;
                        e = nx;
                    } while (nx != stop);
                }
            }
            if (found == 0) {
                if (c != nullptr)
                    PORT_VSLOT1(c, 0x00, void, int)(1);   // scalar deleting destructor
            }
        }
        c = g_gumpDeleteCursor;
    }
    g_gumpDeleteCursor = saved;
}
HOOK_METHOD(0x0046e350, CBillGump, SyncLineButtons);

// Own copies of bodies shared with other classes in the original (the linker folds them).
void __thiscall CBillGump::RButtonUp(int a, int b)
{
    (void)a;
    (void)b;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}

