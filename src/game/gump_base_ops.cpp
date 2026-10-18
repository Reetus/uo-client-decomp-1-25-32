// CGump base class methods and the rect helpers next to them. gumps.cxx, 0x0043a720..0x0043c640.
//
// The SEH frames of the original are omitted (the base constructors and destructors here have
// none of their own).
#include "gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch02_helpers.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "art_record_ops_ports.h"
#include "gump_art_ops_ports.h"
#include "art_row_blits_ports.h"
#include "gump_cascade_ports.h"
#include "mouse_manager.h"
#include "text_entry.h"
#include "generic_gump.h"
#include "stack_gump.h"
#include "input_control_gump.h"
#include "bb_msg_gump.h"
#include "skill_list_gump.h"
#include "edit_list_gump.h"
#include "vtables.h"
#include "window_helpers_31_ports.h"

#define g_focusGump GLOBAL_AS(CGump *, g_focusedInput)

#define Gb_ArtCache GLOBAL_PTR(void, g_artCache_storage)
#define Gb_TileArt(o, a1) PORT_THISCALL1(&ArtRecord_Get, uint8_t *, (o), uint32_t)(a1)
#define Gb_TilePixel(o, a1, a2, a3) PORT_THISCALL3(&ArtRecord_PixelAt, uint16_t, (o), uint8_t *, int, int)(a1, a2, a3)      // (record, x, y)
#define Gb_GumpPixel(a1, a2, a3) (GumpArtPixel(a1, a2, a3))    // ECX = art id, EDX = x, stack y
#define Gb_GumpArtSize(a1, a2, a3) (GumpArtSizeImpl(a1, a2, a3))  // ECX = art id, EDX = &w, stack &h
#define Gb_SetTarget(a1, a2) (SetBlitSurface((uint16_t *)(a1), a2))
#define Gb_CascadeFree(a1, a2) (GumpCascade_Free((int32_t)(a1), (int32_t)(a2)))           // ECX = +0x8c, EDX = +0x90
#define Gb_MouseDetach(self, a1) (reinterpret_cast<CMouseManager *>(self)->ForgetHover(a1))                    // on the mouse manager 0x0068d2e0
#define Gb_MouseMgr GLOBAL_PTR(void, g_mouse_storage)
#define Gb_TextRefresh(self) (reinterpret_cast<CTextEntry *>(self)->RefreshLine())

// ---------------------------------------------------------------------------
// Rect helpers (rects are {left, top, right, bottom}).
// ---------------------------------------------------------------------------

// 0x0043a990  size 0x78
// Grow dst to contain src; an all-zero dst is replaced by src. ECX not read, RET 8.
void __stdcall GumpRectUnion(int *dst, const int *src)
{
    if (dst[1] == 0 && dst[0] == 0 && dst[2] == 0 && dst[3] == 0) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        return;
    }
    if (src[0] < dst[0])
        dst[0] = src[0];
    if (src[1] < dst[1])
        dst[1] = src[1];
    if (src[2] > dst[2])
        dst[2] = src[2];
    if (src[3] > dst[3])
        dst[3] = src[3];
}
HOOK_FUNC(0x0043a990, GumpRectUnion);

// 0x0043b540  size 0x41
// Rectangles overlap (ECX not read), RET 8.
int __stdcall GumpRectIntersects(const int *a, const int *b)
{
    if (a[2] > b[0] && a[0] < b[2] && a[3] > b[1] && a[1] < b[3])
        return 1;
    return 0;
}
HOOK_FUNC(0x0043b540, GumpRectIntersects);

// 0x0043b590  size 0x58
// ECX = rect, EDX = clip, stack = out, RET 4. Each edge of `in` is kept when it lies inside the
// clip's span on that axis, otherwise the clip's edge is used.
void __fastcall GumpRectClip(const int *in, const int *clip, int *out)
{
    int v = in[0];
    if (v < clip[0] || v >= clip[2])
        v = clip[0];
    out[0] = v;
    v = in[2];
    if (v <= clip[0] || v > clip[2])
        v = clip[2];
    out[2] = v;
    v = in[1];
    if (v < clip[1] || v >= clip[3])
        v = clip[1];
    out[1] = v;
    v = in[3];
    if (v <= clip[1] || v > clip[3])
        v = clip[3];
    out[3] = v;
}
HOOK_FUNC(0x0043b590, GumpRectClip);

// 0x0043b760  size 0x4b
// ECX = rows, EDX = width, stack (dst, src, dstStride, srcStride) in pixels, RET 0x10:
// copy `rows` rows with the row blitter 0x004764c0.
void __fastcall GumpBlitRows(int rows, int width, uint16_t *dst, uint16_t *src, int dstStride, int srcStride)
{
    if (rows <= 0)
        return;
    int n = rows;
    do {
        ROWBLIT_COPY(dst, src, width);   // 0x004764c0
        dst += dstStride;
        src += srcStride;
        n--;
    } while (n != 0);
}
HOOK_FUNC(0x0043b760, GumpBlitRows);

// 0x0043adf0  size 0x2eb
// Content rectangle {left, top, right, bottom} of a container gump id; default {0, 0, 200, 200}.
// RET 8, ECX not read.
void __stdcall GetContainerGumpRect(int gumpId, int *rc)
{
    int l = 0, t = 0, r = 200, b = 200;
    switch (gumpId) {
    case 0x3c:
        l = 0x2c; t = 0x41; r = 0xba; b = 0x9f;
        break;
    case 7:
        l = 0x1e; t = 0x1e; r = 0x10e; b = 0xaa;
        break;
    case 9:
        l = 0x14; t = 0x55; r = 0x7c; b = 0xc4;
        break;
    case 0x91a:
        l = 0; t = 0; r = 0x11a; b = 0xe6;
        break;
    case 0x92e:
        l = 0; t = 0; r = 0x11a; b = 0xd2;
        break;
    case 0x3d:
        l = 0x1d; t = 0x22; r = 0x89; b = 0x80;
        break;
    case 0x3e:
        l = 0x21; t = 0x24; r = 0x8e; b = 0x94;
        break;
    case 0x3f:
        l = 0x13; t = 0x2f; r = 0xb6; b = 0x7b;
        break;
    case 0x40:
        l = 0x10; t = 0x26; r = 0x98; b = 0x7d;
        break;
    case 0x41:
        l = 0x23; t = 0x26; r = 0x91; b = 0x74;
        break;
    case 0x42:
    case 0x49:
    case 0x4a:
        l = 0x12; t = 0x69; r = 0xa2; b = 0xb2;
        break;
    case 0x43:
    case 0x4b:
        l = 0x10; t = 0x33; r = 0xb8; b = 0x7c;
        break;
    case 0x44:
        l = 0x14; t = 0xa; r = 0xaa; b = 0x64;
        break;
    case 0x47:
        l = 0x10; t = 0xa; r = 0x94; b = 0x8a;
        break;
    case 0x48:
    case 0x51:
        l = 0x10; t = 0xa; r = 0x9a; b = 0x5e;
        break;
    case 0x4c:
        l = 0x2e; t = 0x4a; r = 0xc4; b = 0xb8;
        break;
    case 0x4d:
        l = 0x4c; t = 0xc; r = 0x8c; b = 0x44;
        break;
    case 0x4e:
    case 0x4f:
        l = 0x18; t = 0x60; r = 0xc4; b = 0x98;
        break;
    case 0x52:
        l = 0; t = 0; r = 0x6e; b = 0x3e;
        break;
    default:
        break;
    }
    rc[0] = l;
    rc[1] = t;
    rc[2] = r;
    rc[3] = b;
}
HOOK_FUNC(0x0043adf0, GetContainerGumpRect);

// ---------------------------------------------------------------------------
// Keyboard focus.
// ---------------------------------------------------------------------------

// 0x0043a720  size 0xf3
// Clear the focused input gump (0x006488d0) and run its "done editing" handler, chosen by which
// type predicate (vtable slots 0x6c, 0x9c, 0xc8, 0xcc, 0xe4, 0xb0, 0x100) the gump answers.
// Then refreshes the input line when 0x005285c8 is set. ECX not read.
void __cdecl Gump_CommitFocus()
{
    CGump *g = g_focusGump;
    if (g == nullptr)
        return;
    int suppress = g_suppressFlag;
    g_focusGump = nullptr;
    if (suppress != 0)
        return;

    if (decomp::vcall(PORT_RET(int), g, 0x6c) != 0) {
        reinterpret_cast<CGenericGump *>(g)->ClearEntryFocus();
    } else if (decomp::vcall(PORT_RET(int), g, 0x9c) != 0) {
        reinterpret_cast<CStackGump *>(g)->RedrawFrame(0);   // CStackGump::RedrawFrame
    } else if (decomp::vcall(PORT_RET(int), g, 0xc8) != 0) {
        if (g->unk_50 == 0x98c)
            g->unk_50 = 0x98b;
        reinterpret_cast<CInputControlGump *>(g)->Redraw();   // CInputControlGump::Redraw
    } else if (decomp::vcall(PORT_RET(int), g, 0xcc) != 0) {
        decomp::vcall_void(g, 0x114);
    } else if (decomp::vcall(PORT_RET(int), g, 0xe4) != 0) {
        reinterpret_cast<CBBMsgGump *>(g)->RedrawText();   // CBBMsgGump::RedrawText
    } else if (decomp::vcall(PORT_RET(int), g, 0xb0) != 0) {
        reinterpret_cast<CSkillListGump *>(g)->CSkillListGump::OnOk();
    } else if (decomp::vcall(PORT_RET(int), g, 0x100) != 0) {
        reinterpret_cast<CEditListGump *>(g)->ClearSelection();   // CEditListGump::ClearSelection
    }
    if (g_inputActive != 0)
        Gb_TextRefresh(g_textEntry);   // 0x00443590
}
HOOK_FUNC(0x0043a720, Gump_CommitFocus);

// 0x0043a820  size 0x2f
// Make `gump` the focus gump, committing the previous one. RET 4.
void __stdcall Gump_SetFocus(CGump *gump)
{
    if (g_focusGump != gump) {
        Gump_CommitFocus();
        void *te = g_textEntry;
        g_focusGump = gump;
        if (te != nullptr)
            Gb_TextRefresh(te);   // 0x00443590
    }
}
HOOK_FUNC(0x0043a820, Gump_SetFocus);

// ---------------------------------------------------------------------------
// CGump methods.
// ---------------------------------------------------------------------------

// 0x0043a850  size 0x140
// Vtable slot 0x0c: (x, y) in local coordinates lies on a non-transparent pixel. RET 8.
// Slot 0x88 selects tile art (the dragged item's, or the owner's) instead of the gump's own art.
int __thiscall CGump::HitTest(int px, int py)
{
    int w, h;
    if (decomp::vcall(PORT_RET(int), this, 0x88) != 0) {
        int base, add;
        if (decomp::vcall(PORT_RET(int), this, 0x8c) != 0) {
            add = g_heldByteAt99;
            base = g_heldGraphic;
        } else {
            CObj *o = owner;
            add = RAWF(int8_t, o, 0x3a);
            base = RAWF(uint16_t, o, 0x38);
        }
        uint8_t *art = Gb_TileArt(Gb_ArtCache, static_cast<uint32_t>(add + base + 0x4000));
        w = *reinterpret_cast<uint16_t *>(art + 4);
        h = *reinterpret_cast<uint16_t *>(art + 6);
        if (px < 0 || py < 0 || px >= w || py >= h)
            return 0;
        return Gb_TilePixel(Gb_ArtCache, art, px, py) != 0 ? 1 : 0;   // 0x00476160
    }
    if (unk_50 >= 0) {
        Gb_GumpArtSize(unk_50, &w, &h);   // 0x00433e70
        if (px < 0 || py < 0 || px >= w || py >= h)
            return 0;
        return Gb_GumpPixel(unk_50, px, py) != 0 ? 1 : 0;   // 0x00434410
    }
    const uint8_t *bmp = reinterpret_cast<const uint8_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[frameIndex * 2 + 1]))) + 4;
    w = width;
    h = height;
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return reinterpret_cast<const uint16_t *>(bmp)[w * py + px] != 0 ? 1 : 0;
}
HOOK_VMETHOD(0x0043a850, CGump, HitTest);

// 0x0043b4e0  size 0x2e
// Add this gump's offset and each ancestor's to (*x, *y). RET 8.
void __thiscall CGump::LocalToScreen(int *px, int *py)
{
    for (CGump *g = this; g != nullptr; g = g->parent) {
        *px += g->x;
        *py += g->y;
    }
}
HOOK_METHOD(0x0043b4e0, CGump, LocalToScreen);

// 0x0043b510  size 0x2e
// Subtract this gump's offset and each ancestor's from (*x, *y). RET 8.
void __thiscall CGump::ScreenToLocal(int *px, int *py)
{
    for (CGump *g = this; g != nullptr; g = g->parent) {
        *px -= g->x;
        *py -= g->y;
    }
}
HOOK_METHOD(0x0043b510, CGump, ScreenToLocal);

// 0x0043b5f0  size 0x16c
// Vtable slot 0x04 (draw pass 1), RET 4. When the gump's screen rect overlaps `clip`, mark it
// dirty and copy the clipped rect from the back buffer into the ring buffer, recording the copy in
// the flush list (entry count at 0x004d3334, at most 0x1000).
void __thiscall CGump::PreDraw(int *clip)
{
    int sx = 0;
    int sy = 0;
    if (visible == 0)
        return;
    for (CGump *g = this; g != nullptr; g = g->parent) {
        sx += g->x;
        sy += g->y;
    }
    int r[4];
    r[0] = sx;
    r[1] = sy;
    r[2] = width + sx;
    r[3] = height + sy;
    if (sx < 0)
        r[0] = 0;
    if (sy < 0)
        r[1] = 0;
    if (r[2] > 0x280)
        r[2] = 0x280;
    if (r[3] > 0x1e0)
        r[3] = 0x1e0;

    if (!(clip[2] > r[0] && clip[0] < r[2] && clip[3] > r[1] && clip[1] < r[3]))
        return;

    int o[4];
    GumpRectClip(clip, r, o);   // 0x0043b590
    dirty = 1;
    int row = o[1];
    uint8_t *bb = reinterpret_cast<uint8_t *>(g_backBuffer);
    uint8_t *src = bb + (row * 640 + o[0]) * 2;
    int off = static_cast<int>((src - bb) >> 1);
    uint8_t *dst = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(g_ringBase + static_cast<uint32_t>(off) * 2));
    int count = g_flushCount;
    if (count < 0x1000) {
        g_flushTabA[count] = src;
        int *e = g_flushRects + count * 4;
        g_flushTabB[count] = dst;
        count++;
        e[0] = o[0];
        g_flushCount = count;
        e[1] = o[1];
        e[2] = o[2];
        e[3] = o[3];
    }
    if (row < o[3]) {
        do {
            memcpy(dst, src, static_cast<size_t>(o[2] - o[0]) * 2);
            dst += 0x500;
            src += 0x500;
            row++;
        } while (row < o[3]);
    }
}
HOOK_VMETHOD(0x0043b5f0, CGump, PreDraw);

// 0x0043b7b0  size 0x1b8
// Vtable slot 0x08 (draw pass 2), RET 4. Blit the clipped part of the current frame bitmap to the
// back buffer (with the row blitter 0x004764c0 when slot 0x7c answers true, memcpy otherwise),
// then draw every child with the clipped rect.
// ? when the gump's rect does not overlap `clip`, the original passes its uninitialised clipped-rect
//   local to the children; this port does the same.
void __thiscall CGump::Draw(int *clip)
{
    int o[4];
    int sx = 0;
    int sy = 0;
    for (CGump *g = this; g != nullptr; g = g->parent) {
        sx += g->x;
        sy += g->y;
    }
    if (visible == 0)
        return;

    int r[4];
    r[0] = sx;
    r[1] = sy;
    r[2] = width + sx;
    r[3] = height + sy;
    Gb_SetTarget(g_backBuffer, 0x280);   // 0x004767c0
    if (clip[2] > r[0] && clip[0] < r[2] && clip[3] > r[1] && clip[1] < r[3]) {
        GumpRectClip(clip, r, o);   // 0x0043b590
        int xoff = 0;
        int yoff = 0;
        if (r[0] != o[0])
            xoff = o[0] - r[0];
        if (r[1] != o[1])
            yoff = o[1] - r[1];
        uint8_t *bmp = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[frameIndex * 2 + 1])));
        uint8_t *src = bmp + (yoff * rowStride + xoff) * 2 + 4;
        uint8_t *dst = reinterpret_cast<uint8_t *>(g_backBuffer) + ((yoff + sy) * 640 + xoff + sx) * 2;
        if (decomp::vcall(PORT_RET(int), this, 0x7c) != 0) {
            int count = o[2] - o[0];
            int rows = o[3] - o[1];
            if (rows > 0) {
                int stride = rowStride * 2;
                do {
                    ROWBLIT_COPY(reinterpret_cast<uint16_t *>(dst), reinterpret_cast<uint16_t *>(src), count);   // 0x004764c0
                    dst += 0x500;
                    src += stride;
                    rows--;
                } while (rows != 0);
            }
        } else {
            for (int row = o[1]; row < o[3]; row++) {
                memcpy(dst, src, static_cast<size_t>(o[2] - o[0]) * 2);
                dst += 0x500;
                src += rowStride * 2;
            }
        }
    }
    for (CGump *c = firstChild; c != nullptr; c = c->next)
        PORT_VSLOT1(c, 0x08, void, int *)(o);
}
HOOK_VMETHOD(0x0043b7b0, CGump, Draw);

// 0x0043ba70  size 0x24a
// Move the gump to the end of its sibling list (or of the top-level list), then bring forward
// the gumps whose +0xb4 is this gump, and the one in slot 0xa0's +0xd8. RET 4. Slots 0xb8 and
// 0xf4 veto the whole operation. The dirty rect built along the way is only a local in the original.
void __thiscall CGump::BringToFront(int flag)
{
    if (decomp::vcall(PORT_RET(int), this, 0xb8) != 0)
        return;
    if (decomp::vcall(PORT_RET(int), this, 0xf4) != 0)
        return;

    if (flag == 0) {
        CGump *f = g_focusGump;
        if (f != nullptr && f->parent != this)
            Gump_CommitFocus();   // 0x0043a720
    }

    if (next != nullptr) {
        if (prev != nullptr)
            prev->next = next;
        next->prev = prev;
        CGump *p = parent;
        if (p != nullptr) {
            if (p->firstChild == this)
                p->firstChild = next;
            p->lastChild->next = this;
            prev = p->lastChild;
            p->lastChild = this;
        } else {
            if (g_gumpListHead == this)
                g_gumpListHead = next;
            g_gumpListTail->next = this;
            prev = g_gumpListTail;
            g_gumpListTail = this;
        }
        next = nullptr;

        int sx = 0;
        int sy = 0;
        for (CGump *g = this; g != nullptr; g = g->parent) {
            sx += g->x;
            sy += g->y;
        }
        int r[4];
        r[0] = sx;
        r[1] = sy;
        if (decomp::vcall(PORT_RET(int), this, 0x84) == 0) {
            int right, bottom;
            if (unk_60 == 0) {
                right = width + sx;
                bottom = height;
            } else {
                right = unk_24 + sx;
                bottom = unk_28;
            }
            bottom += sy;
            r[2] = right;
            r[3] = bottom;
            if (sx < 0)
                r[0] = 0;
            if (sy < 0)
                r[1] = 0;
            if (right > 0x27f)
                r[2] = 0x27f;
            if (bottom > 0x1df)
                r[3] = 0x1df;

            CGump *e = g_gumpListHead;
            if (e != nullptr) {
                while (e != this) {
                    CGump *nx = e->next;
                    if (decomp::vcall(PORT_RET(int), e, 0x84) != 0 && RAWF(CGump *, e, 0xb4) == this) {
                        e->BringToFront(0);   // 0x0043ba70
                        int r2[4];
                        r2[0] = e->x;
                        r2[1] = e->y;
                        r2[2] = e->width + e->x;
                        r2[3] = e->height + e->y;
                        if (e->x < 0)
                            r2[0] = 0;
                        if (e->y < 0)
                            r2[1] = 0;
                        if (r2[2] > 0x27f)
                            r2[2] = 0x27f;
                        if (r2[3] > 0x1df)
                            r2[3] = 0x1df;
                        GumpRectUnion(r, r2);   // 0x0043a990
                    }
                    if (nx == nullptr)
                        break;
                    e = nx;
                }
            }
        } else {
            int right = width + sx;
            int bottom = height + sy;
            r[2] = right;
            r[3] = bottom;
            if (sx < 0)
                r[0] = 0;
            if (sy < 0)
                r[1] = 0;
            if (right > 0x27f)
                r[2] = 0x27f;
            if (bottom > 0x1df)
                r[3] = 0x1df;
        }
    }

    if (decomp::vcall(PORT_RET(int), this, 0xa0) != 0)
        RAWF(CGump *, this, 0xd8)->BringToFront(0);   // 0x0043ba70
}
HOOK_METHOD(0x0043ba70, CGump, BringToFront);

// 0x0043bcc0  size 0x5a
// Link as the last child of `parent`, or at the tail of the top-level list when parent is null.
// RET 8 (the second argument is not read).
void __thiscall CGump::LinkInto(CGump *p, int)
{
    next = nullptr;
    parent = p;
    if (p == nullptr) {
        CGump *tail = g_gumpListTail;
        prev = tail;
        if (tail != nullptr)
            tail->next = this;
        CGump *head = g_gumpListHead;
        g_gumpListTail = this;
        if (head == nullptr)
            g_gumpListHead = this;
        return;
    }
    CGump *last = p->lastChild;
    prev = last;
    if (last != nullptr)
        last->next = this;
    CGump *first = p->firstChild;
    p->lastChild = this;
    if (first == nullptr)
        p->firstChild = this;
}
HOOK_METHOD(0x0043bcc0, CGump, LinkInto);

// 0x0043c2a0  size 0xf9
// Full constructor: (x, y, a3 (not read), owner, frameCount, width, height), RET 0x1c. Allocates
// a zeroed frame table of frameCount slots; unk_50 is -1 (own bitmaps) unless the owner's slot 0x28
// answers true, in which case it is 0. SEH frame omitted.
CGump::CGump(int px, int py, int, CObj *own, int frames, int w, int h)
{
    unk_08 = 0;
    unk_0c = 0;
    frameIndex = 0;
    frameCount = frames;
    this->width = w;
    this->height = h;
    rowStride = (w + 7) & ~7;
    y = py;
    unk_80 = py;
    unk_88 = py;
    unk_24 = 0;
    unk_28 = 0;
    x = px;
    owner = own;
    prev = nullptr;
    next = nullptr;
    lastChild = nullptr;
    parent = nullptr;
    firstChild = nullptr;
    layer = 0;
    unk_58 = 0;
    dirty = 0;
    unk_60 = 0;
    unk_64 = 0;
    visible = 1;
    unk_6c = 0;
    unk_70 = 0;
    unk_74 = 0;
    unk_78 = 0;
    unk_7c = px;
    unk_84 = px;
    unk_8c = 0;
    unk_90 = 0;
    name = const_cast<char *>("normal gump");   // 0x004a8390
    int32_t *fd = FrameData_Alloc(g_frameDataAllocator, frameCount << 3, "framedata in CGump::CGump()");
    frameData = fd;
    memset(fd, 0, static_cast<size_t>(frameCount) << 3);
    unk_50 = -1;
    if (own != nullptr) {
        if (decomp::vcall(PORT_RET(int), own, 0x28) != 0)
            unk_50 = 0;
    }
}
HOOK_CTOR(0x0043c2a0, CGump, (int, int, int, CObj *, int, int, int));

HOOK_VTABLE(0x0049ed40, CGump);
HOOK_SDD(0x0043c3a0, CGump);

// 0x0043c3d0  size 0x92
// No-argument base constructor: every field cleared (unk_50 = -1, visible = 1), name set; the vptr store is the compiler's.
// Not shared with the constructor above (two out-of-line bodies, as the original).
CGump::CGump()
{
    unk_08 = 0;
    unk_0c = 0;
    frameIndex = 0;
    frameCount = 0;
    rowStride = 0;
    width = 0;
    height = 0;
    unk_24 = 0;
    unk_28 = 0;
    x = 0;
    y = 0;
    owner = nullptr;
    frameData = nullptr;
    prev = nullptr;
    next = nullptr;
    lastChild = nullptr;
    parent = nullptr;
    firstChild = nullptr;
    unk_50 = -1;
    layer = 0;
    unk_58 = 0;
    dirty = 0;
    unk_60 = 0;
    unk_64 = 0;
    visible = 1;
    unk_6c = 0;
    unk_70 = 0;
    unk_74 = 0;
    unk_78 = 0;
    unk_7c = 0;
    unk_80 = 0;
    unk_84 = 0;
    unk_88 = 0;
    unk_8c = 0;
    unk_90 = 0;
    name = const_cast<char *>("normal gump");   // 0x004a8390
}
HOOK_CTOR(0x0043c3d0, CGump, ());

// 0x0043c470  size 0x1cc
// Destructor body: release the cascade slot, clear every global that may point at this gump,
// delete the children (with 0x006f4794 raised), unlink from the sibling or top-level list, free
// the frame bitmaps and the frame table.
CGump::~CGump()
{
    if (unk_8c != 0)
        Gb_CascadeFree(unk_8c, unk_90);   // 0x0040d300
    if (g_updateCursor == this)
        g_updateCursor = prev;
    if (g_gumpDeleteCursor == this)
        g_gumpDeleteCursor = prev;
    Gb_MouseDetach(Gb_MouseMgr, this);   // 0x00446d00
    if (g_pointerGump == this)
        g_pointerGump = nullptr;
    if (g_focusGump == this)
        PORT_THISCALL0(&Gump_CommitFocus, void, GLOBAL_PTR(void, g_gumpMgr_storage))();   // 0x0043a720

    for (CGump *g = g_gumpListTail; g != nullptr; g = g->prev) {
        if (decomp::vcall(PORT_RET(int), g, 0x108) != 0) {
            if (RAWF(CGump *, g, 0x2a0) == this)
                RAWF(CGump *, g, 0x2a0) = nullptr;
        }
    }

    CObj *o = owner;
    if (o != nullptr) {
        if (RAWF(CGump *, o, 0x88) == this)
            RAWF(CGump *, o, 0x88) = nullptr;
        if (decomp::vcall(PORT_RET(int), owner, 0x30) != 0) {
            o = owner;
            if (RAWF(CGump *, o, 0xac) == this)
                RAWF(CGump *, o, 0xac) = nullptr;
        }
    }

    int savedSuppress = g_suppressFlag;
    CGump *savedCursor = g_gumpDeleteCursor;
    g_suppressFlag = 1;
    CGump *c = lastChild;
    if (c != nullptr) {
        do {
            g_gumpDeleteCursor = c->prev;
            if (c != nullptr)
                PORT_VSLOT1(c, 0x00, void, int)(1);
            c = g_gumpDeleteCursor;
        } while (c != nullptr);
    }
    g_gumpDeleteCursor = savedCursor;
    g_suppressFlag = savedSuppress;

    CGump *p = parent;
    if (p != nullptr) {
        if (p->lastChild == this)
            p->lastChild = prev;
        p = parent;
        if (p->firstChild == this)
            p->firstChild = next;
    } else {
        if (g_gumpListTail == this)
            g_gumpListTail = prev;
        if (g_gumpListHead == this)
            g_gumpListHead = next;
    }
    if (prev != nullptr)
        prev->next = next;
    if (next != nullptr)
        next->prev = prev;

    if (frameData != nullptr) {
        for (int i = 0; i < frameCount; i++) {
            int32_t bmp = frameData[i * 2 + 1];
            if (bmp != 0) {
                FrameData_Free(g_frameDataAllocator, reinterpret_cast<void *>(static_cast<uintptr_t>(static_cast<uint32_t>(bmp))), "frame pBMP");   // 0x00446310
                frameData[i * 2 + 1] = 0;
            }
        }
        FrameData_Free(g_frameDataAllocator, frameData, "framedata");   // 0x00446310
        frameData = nullptr;
    }
}
HOOK_DTOR(0x0043c470, CGump);

// 0x0043c640  size 0x19
// Vtable slot 0x14, RET 8 (the arguments are not read): forget the drag gump when it is this one.
void __thiscall CGump::MouseUp(int, int)
{
    if (GLOBAL_AS(CGump *, g_dragGump) == this)
        GLOBAL_AS(CGump *, g_dragGump) = nullptr;
}
HOOK_VMETHOD(0x0043c640, CGump, MouseUp);

// 0x0043c660  size 0xd
// Vtable slot 0x24 (the same body in 50 vtables): SetMouseCursor(0) on the mouse manager 0x0068d2e0. Plain RET.
void __thiscall CGump::MouseLeave()
{
    PORT_THISCALL1(&CMouseManager::SetMouseCursor, int, GLOBAL_PTR(void, g_mouse_storage), int)(0);
}
HOOK_VMETHOD(0x0043c660, CGump, MouseLeave);

// 0x0040c990  size 0x13
// Vtable slot 0x20 (the default of 43 vtables): the mouse is over the gump, set the cursor of region 0.
void __thiscall CGump::MouseOver(int, int)
{
    PORT_THISCALL1(&SetCursorForRegion, int, reinterpret_cast<void *>(GADDR(g_app_storage)), int)(0);
}
HOOK_VMETHOD(0x0040c990, CGump, MouseOver);

// ---- defaults of the other slots (see gump.h): the original's shared empty procedures, folded by the linker
void __thiscall CGump::MouseDown(int, int)
{
}
void __thiscall CGump::RButtonDown(int, int)
{
}
void __thiscall CGump::RButtonUp(int, int)
{
}
void __thiscall CGump::MouseMove(int, int)
{
}
void __thiscall CGump::Tick()
{
}
void __thiscall CGump::OnScroll(int, int, int)
{
}
void __thiscall CGump::OnScrollEnd(int, int, int)
{
}
void __thiscall CGump::Resize(int, int)
{
}
void __thiscall CGump::OnKey(int)
{
}
void __thiscall CGump::Slot40(int)
{
}
void __thiscall CGump::OnChildChanged(int)
{
}
void __thiscall CGump::OnTab()
{
}
void __thiscall CGump::OnOk()
{
}
void __thiscall CGump::OnCancel()
{
}
void __thiscall CGump::OnFocus(int)
{
}
void __thiscall CGump::OnEnter(int)
{
}
void __thiscall CGump::Update()
{
}
void __thiscall CGump::Slot60()
{
}
void __thiscall CGump::Slot64(int)
{
}
