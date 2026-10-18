// CStatusGump: scalar deleting destructor, Close, mouse handlers, rename
// commit, update, draw pass, snap-side test and group detach.
// (Porting pass 2, batch 25.)
// 0x00468030, 0x00468580, 0x004685d0, 0x00468a90, 0x00468c70, 0x00468cf0,
// 0x004693e0, 0x004695e0, 0x004696a0.
#include <windows.h>

#include "../runtime/ported.h"
#include "globals.h"
#include "batch02_helpers.h"
#include "gump_manager.h"
#include "name_filter.h"
#include "obj.h"
#include "crt.h"
#include "packet_cursor.h"
#include "raw_field.h"
#include "status_gump.h"
#include "vcall.h"
#include "gump_base_ops_ports.h"
#include "packet_builders_b25_ports.h"
#include "gump_art_ops_ports.h"
#include "art_row_blits_ports.h"

#define g_gumpTail GLOBAL_AS(CStatusGump *, g_gumpListTail)           // last top-level gump (previous at +0x3c)
#define g_rowPlain PTR_AT(void, PORT_ADDR(&RowBlitCopyNonZero))

static int b25s_RectIntersects(const int *a, const int *b) { return GumpRectIntersects(a, b); }
static void b25s_BlitClipped(const void *src, const int *clip, int w, int h, int x, int y, const void *dest, int stride, int destH)
{
    PORT_FUNC10(&GumpBlitBitmapClipped, void, __fastcall, void *, const void *, const int *, int, int, int, int, const void *, int, int)(
        g_rowPlain, src, clip, w, h, x, y, dest, stride, destH);
}

HOOK_VTABLE(0x0049fd40, CStatusGump);
HOOK_SDD(0x00468030, CStatusGump);

// ---------------------------------------------------------------------------
// 0x00468580  size 0x48
// RET 8, arguments not read. Clears the drag gump, then deletes (vtable slot 0, flags 1) every gump
// of the snap group (the next link is read before each delete) or, outside a group, this gump.
// ---------------------------------------------------------------------------
void __thiscall CStatusGump::RButtonUp(int, int)
{
    g_dragGump = nullptr;
    CStatusGump *g = groupHead;
    if (g != nullptr) {
        CStatusGump *following;
        do {
            following = g->groupNext;
            decomp::vdelete(g);
            g = following;
        } while (following != nullptr);
        return;
    }
    decomp::vdelete(this);
}
HOOK_VMETHOD(0x00468580, CStatusGump, RButtonUp);

// ---------------------------------------------------------------------------
// 0x004685d0  size 0x7a
// Left button down: start a drag (dragging = 1, grab point in +0x6c / +0x70), raise this gump or
// the whole group, count the click (clickState 1 then 2) and remember the tick of the first one (+0xe8).
// ---------------------------------------------------------------------------
void __thiscall CStatusGump::MouseDown(int mx, int my)
{
    CStatusGump *g = groupHead;
    dragging = 1;
    unk_6c = mx;
    unk_70 = my;
    if (g != nullptr) {
        do {
            g->BringToFront(0);   // 0x0043ba70
            g = g->groupNext;
        } while (g != nullptr);
    } else {
        BringToFront(0);   // 0x0043ba70
    }
    if (clickState == 1) {
        clickState = 2;
        return;
    }
    clickState = 1;
    unk_e8 = static_cast<int32_t>(GetTickCount());
}
HOOK_VMETHOD(0x004685d0, CStatusGump, MouseDown);

// ---------------------------------------------------------------------------
// 0x00468a90  size 0x1bf
// Mouse move. While dragging, moving more than 4 pixels from the grab point marks the gump as moved and
// cancels the click. A grouped gump moves the whole group (the others by the same delta). A lone pinned
// gump (unk_60) looks for another pinned status gump it touches (0x004695e0) and snaps to its side.
// ---------------------------------------------------------------------------
void __thiscall CStatusGump::MouseMove(int mx, int my)
{
    int32_t wasDragging = dragging;
    if (wasDragging != 0 && static_cast<uint32_t>(clickState) > 0) {
        int dx = mx - unk_6c;
        int dy = my - unk_70;
        int adx = (dx ^ (dx >> 31)) - (dx >> 31);
        int ady = (dy ^ (dy >> 31)) - (dy >> 31);
        if (adx > 4 || ady > 4) {
            moved = 1;
            clickState = 0;
        }
    }
    if (wasDragging == 0)
        return;

    if (groupHead != nullptr) {
        int oldY = y;
        int oldX = x;
        Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);   // 0x0043b140
        for (CStatusGump *g = groupHead; g != nullptr; g = g->groupNext) {
            if (g != this)
                g->Move(x - oldX, y - oldY, 1, nullptr, 0);   // 0x0043b140
        }
        return;
    }

    Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);   // 0x0043b140
    if (unk_60 == 0)
        return;
    snapTo = nullptr;
    CStatusGump *other = g_gumpTail;
    uint32_t side = 0;
    for (; other != nullptr; other = reinterpret_cast<CStatusGump *>(other->prev)) {
        if (other == this)
            continue;
        if (decomp::vcall(PORT_RET(int), other, 0xac) == 0)
            continue;
        if (other->unk_60 == 0)
            continue;
        side = SnapSideTo(other) & 0xff;   // 0x004695e0
        if (side != 0)
            break;
    }
    if (other == nullptr)
        return;
    snapSide = static_cast<uint8_t>(side);
    snapTo = other;
    switch (side) {
    case 1:
        x = other->unk_24 + other->x;
        y = other->y;
        break;
    case 2:
        x = other->x - unk_24;
        y = other->y;
        break;
    case 4:
        x = other->x;
        y = other->y + other->unk_28;
        break;
    case 8:
        x = other->x;
        y = other->y - unk_28;
        break;
    default:
        break;
    }
}
HOOK_VMETHOD(0x00468a90, CStatusGump, MouseMove);

// ---------------------------------------------------------------------------
// 0x00468c70  size 0x77
// The rename edit control (+0x11c, text at +0xec) was committed: reject unacceptable names with a box,
// otherwise send packet 0x75 (serial of the owner, the name) and commit the input focus.
// ---------------------------------------------------------------------------
void __thiscall CStatusGump::OnOk()
{
    uint8_t buf[0x28];

    char *newName = static_cast<char *>(editControl) + 0xec;
    if (NameIsRejected(newName) != 0) {   // 0x0044adf0
        PORT_THISCALL4(&CreateGump2ac, void *, GLOBAL_PTR(void, g_gumpMgr_storage), int, int, int, int)(static_cast<int>(GADDR(str_004a65ec)), 0, 0, 0);   // 0x00439f60 "Unacceptable name."
        return;
    }
    newName = static_cast<char *>(editControl) + 0xec;
    // 0x004650b0 packet 0x75 builder (ECX = builder object, not read; stdcall buf, serial, name)
    PORT_THISCALL3(&Pkt_Build_0x75_RenameCharacter, void, GLOBAL_PTR(void, g_packetBuilder_storage), uint8_t *, uint32_t, const void *)(buf, RAWF(uint32_t, owner, 0x78), newName);
    pkt::Send(buf);   // 0x00478610
    PORT_THISCALL0(&Gump_CommitFocus, void, GLOBAL_PTR(void, g_gumpMgr_storage))();   // 0x0043a720
}
HOOK_VMETHOD(0x00468c70, CStatusGump, OnOk);

// ---------------------------------------------------------------------------
// 0x00468cf0  size 0x48
// Per-tick: a first click older than the double-click time is dropped; if the button is still down
// (dragging) the gump counts as moved.
// ---------------------------------------------------------------------------
void __thiscall CStatusGump::Tick()
{
    uint32_t now = GetTickCount();
    uint32_t dct = GetDoubleClickTime();
    if (clickState != 1)
        return;
    if (now - static_cast<uint32_t>(unk_e8) <= dct)
        return;
    int32_t down = dragging;
    clickState = 0;
    if (down != 0)
        moved = 1;
}
HOOK_VMETHOD(0x00468cf0, CStatusGump, Tick);

// ---------------------------------------------------------------------------
// 0x004693e0  size 0xee
// Draw pass 2 (vtable slot 8), RET 4. Blit frame bitmap 3 (unk_60 set) or 1, then draw the children
// that belong to the current state: a child whose slot 0xa8 answers non-zero is drawn when its
// +0xb4 equals this gump's unk_60; a child that answers zero is drawn when unk_60 is set.
// ---------------------------------------------------------------------------
void __thiscall CStatusGump::Draw(int *clip)
{
    uint16_t *bmp;
    int rect[4];
    int sx = 0;
    int sy = 0;

    LocalToScreen(&sx, &sy);   // 0x0043b4e0
    if (unk_60 == 0)
        bmp = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[1])));
    else
        bmp = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[3])));
    rect[1] = sy;
    rect[0] = sx;
    rect[2] = bmp[0] + sx;
    rect[3] = bmp[1] + sy;
    if (PORT_THISCALL2(&GumpRectIntersects, int, GLOBAL_PTR(void, g_gumpMgr_storage), const int *, const int *)(clip, rect) == 0)   // 0x0043b540
        return;
    PORT_FUNC10(&GumpBlitBitmapClipped, void, __fastcall, void *, void *, void *, int, int, int, int, void *, int, int)(g_rowPlain, bmp + 2, clip, bmp[0], bmp[1], sx, sy, g_backBuffer, 0x280, 0x1e0);   // 0x004342e0, 0x004764c0
    for (CGump *child = firstChild; child != nullptr; child = child->next) {
        if ((decomp::vcall(PORT_RET(int), child, 0xa8) != 0 && RAWF(int32_t, child, 0xb4) == unk_60) ||
            (decomp::vcall(PORT_RET(int), child, 0xa8) == 0 && unk_60 != 0))
            decomp::vcall_void(child, 8, clip);
    }
}
HOOK_VMETHOD(0x004693e0, CStatusGump, Draw);

// ---------------------------------------------------------------------------
// 0x004695e0  size 0xb4
// RET 4, result in AL. How `other` touches this gump: with d = (this - other) in x and y, both
// strictly inside (-size, size) of this gump (+0x24 / +0x28): the dominant axis decides. x dominant:
// d.x >= 0 gives 1 when other's link +0x100 is free, d.x < 0 gives 2 when its link +0xfc is free;
// y dominant: d.y >= 0 gives 4 (link +0x108 free), d.y < 0 gives 8 (link +0x104 free). Otherwise 0.
// ---------------------------------------------------------------------------
uint32_t __thiscall CStatusGump::SnapSideTo(CStatusGump *other)
{
    int dx = x - other->x;
    int dy = y - other->y;
    int w = unk_24;
    if (dx <= -w || dx >= w)
        return 0;
    int h = unk_28;
    if (dy <= -h || dy >= h)
        return 0;
    int adx = (dx ^ (dx >> 31)) - (dx >> 31);
    int ady = (dy ^ (dy >> 31)) - (dy >> 31);
    if (adx > ady) {
        if (dx >= 0) {
            if (other->link_100 == nullptr)
                return 1;
            return 0;
        }
        if (other->link_fc == nullptr)
            return 2;
        return 0;
    }
    if (dy >= 0) {
        if (other->link_108 == nullptr)
            return 4;
        return 0;
    }
    if (other->link_104 == nullptr)
        return 8;
    return 0;
}
HOOK_METHOD(0x004695e0, CStatusGump, SnapSideTo);

// ---------------------------------------------------------------------------
// 0x004696a0  size 0x1e2
// Take this gump out of its snap group (chain through +0xf4, head in +0xf0), recompute the group's
// extreme members (+0x10c min x, +0x110 max x, +0x114 min y, +0x118 max y), or dissolve the group when
// only its head is left, then unlink the neighbour links (+0xfc/+0x100, +0x104/+0x108) of this gump.
// ---------------------------------------------------------------------------
void __thiscall CStatusGump::DetachGroup()
{
    CStatusGump *head = groupHead;
    CStatusGump *nxt = head->groupNext;
    CStatusGump *pred;

    if (head == this) {
        // this is the head: the next member becomes the head of the rest
        pred = nxt;
        for (; nxt != nullptr; nxt = nxt->groupNext)
            nxt->groupHead = pred;
        groupHead = pred;
    } else {
        pred = head;
        if (nxt != this) {
            do {
                pred = nxt;
                nxt = pred->groupNext;
            } while (nxt != this);
        }
        nxt = pred->groupNext;
        pred->groupNext = nxt->groupNext;
    }

    head = groupHead;
    if (head->groupNext == nullptr) {
        // only the head is left: dissolve the group
        groupHead->groupHead = nullptr;
        groupHead->link_fc = nullptr;
        groupHead->link_100 = nullptr;
        groupHead->link_104 = nullptr;
        groupHead->link_108 = nullptr;
        groupHead->minX = nullptr;
        groupHead->maxX = nullptr;
        groupHead->minY = nullptr;
        groupHead->maxY = nullptr;
    } else {
        head->minX = head;
        groupHead->maxX = groupHead;
        groupHead->minY = groupHead;
        groupHead->maxY = groupHead;
        for (CStatusGump *m = groupHead->groupNext; m != nullptr; m = m->groupNext) {
            if (m->x < groupHead->minX->x)
                groupHead->minX = m;
            if (m->x > groupHead->maxX->x)
                groupHead->maxX = m;
            if (m->y < groupHead->minY->y)
                groupHead->minY = m;
            if (m->y > groupHead->maxY->y)
                groupHead->maxY = m;
        }
    }

    if (link_fc != nullptr)
        link_fc->link_100 = nullptr;
    if (link_100 != nullptr)
        link_100->link_fc = nullptr;
    if (link_104 != nullptr)
        link_104->link_108 = nullptr;
    if (link_108 != nullptr)
        link_108->link_104 = nullptr;
    groupNext = nullptr;
    groupHead = nullptr;
    link_100 = nullptr;
    link_fc = nullptr;
    link_108 = nullptr;
    link_104 = nullptr;
}
HOOK_METHOD(0x004696a0, CStatusGump, DetachGroup);

// Own copies of bodies shared with other classes in the original (the linker folds them).
int __thiscall CStatusGump::HitTest(int px, int py)
{
    uint16_t *bmp;
    if (unk_60 == 0)
        bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
    else
        bmp = static_cast<uint16_t *>(i32_ptr(frameData[3]));
    int w = bmp[0];
    int h = bmp[1];
    bmp += 2;
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return bmp[w * py + px] != 0;
}

