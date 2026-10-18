// CObjPickerGump second-pass methods: scalar deleting destructor, list redraw, draw, mouse handlers and
// the slide animation tick (0x0046f020 .. 0x0046f700).
#include "obj_picker_gump.h"
#include "macro_gump.h"
#include "app.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch06_helpers.h"
#include "globals.h"
#include "art_record_ops_ports.h"
#include "palette_manager_ports.h"
#include "bitmap16_draw_ports.h"
#include "art_row_blits_ports.h"
#include "gump_base_ops_ports.h"
#include "window_helpers_31_ports.h"
#include "input_control_gump.h"
#include "art_blit_ports.h"
#include "gump_art_ops_ports.h"
#include "art_row_blits_hue_ports.h"
#include "crt.h"


// 0x00476130: art record by id (record: +4 width, +6 height). The ECX it is given is not read; RET 4.
#define Pk_ArtRecord(a1) PORT_THISCALL1(&ArtRecord_Get, uint8_t *, GLOBAL_PTR(void, g_artCache_storage), int)(a1)
// 0x00436870 PackRGB16: three stack args (r, g, b), RET 0xc. The caller pushes the destination bitmap first,
// under the three colour arguments, and it stays on the stack as the sixth argument of the fill below.
#define Pk_PackRGB(a1, a2, a3) (PackRGB16(a1, a2, a3))
// 0x00432c70: fill (x0, y0, x1, y1, color, bitmap), RET 0x18.
#define Pk_FillRect(a1, a2, a3, a4, a5, a6) (Bitmap16_FillRect(a1, a2, a3, a4, a5, (uint16_t *)(a6)))
// 0x004767c0: ECX = destination pixels, EDX = stride in pixels.
#define Pk_SetTarget(a1, a2) (SetBlitSurface((uint16_t *)(a1), a2))
// 0x004767d0: RLE art walk. ECX = row blitter, EDX = record, stack (clip, x, y), RET 0xc.
#define Pk_RleWalk PORT_FUNC5(&RleArtWalk, void, __fastcall, void *, void *, int *, int, int)
// 0x0043b540 / 0x004342e0 / 0x0043b4e0 / 0x00482be0 (cursor selector, RET 4).
#define Pk_RectIntersects(a1, a2) PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, int *, int *)(a1, a2)
#define Pk_BlitClipped PORT_FUNC10(&GumpBlitBitmapClipped, void, __fastcall, void *, void *, int *, int, int, int, int, void *, int, int)
#define Pk_SetCursor(a1) (g_app.*PORT_METHOD1(CApp, void, &SetCursorForRegion, int))(a1)

HOOK_VTABLE(0x004a02b8, CObjPickerGump);
HOOK_SDD(0x0046f020, CObjPickerGump);

// 0x0046f1a0  size 0x1f2  CObjPickerGump::RedrawList
// Clears the strip rectangle (pickRect) of the frame bitmap, then draws the entries from index unk_b8 on, each
// as the art record (id + 0x4000, or 0x4000 when absent) at the running x, centred vertically, hued when the
// entry has a hue. With 565 pixels the strip rows are then converted from 555.
// ? the original pushes the bitmap pointer under the arguments of 0x00436870 (they are popped by its RET 0xc),
//   so it is the sixth argument of the fill call; written here as that argument.
void __thiscall CObjPickerGump::RedrawList()
{
    int curX;
    int idx;
    int w;
    int curY;
    int h;
    uint8_t *rec;

    PORT_THISCALL6(&Bitmap16_FillRect, void, b6_font, int, int, int, int, int, void *)(pickRect[0], pickRect[1], pickRect[2], pickRect[3],
        b6_make_color(b6_pixel_format, 8, 8, 8), b6_frame_bitmap(this, 0));   // 0x00432c70, 0x00436870
    curX = pickRect[0];
    if (unk_ac != 0)
        curX = curX - slideOffset;
    idx = unk_b8;
    Pk_SetTarget(reinterpret_cast<uint8_t *>(b6_frame_bitmap(this, 0)) + 4, width);   // 0x004767c0
    while (idx < count && curX <= pickRect[2]) {
        if (ids[idx] <= 0x4000) {
            rec = Pk_ArtRecord(ids[idx] + 0x4000);   // 0x00476130
            if (rec == nullptr) {
                rec = Pk_ArtRecord(0x4000);   // 0x00476130
                if (rec == nullptr) {
                    idx++;
                    continue;
                }
            }
            curY = pickRect[1];
            w = *reinterpret_cast<uint16_t *>(rec + 4);
            h = *reinterpret_cast<uint16_t *>(rec + 6);
            if (h + curY < pickRect[3]) {
                int d = pickRect[3] - h - curY;
                curY += (d - (d >> 31)) >> 1;
            }
            if (hues[idx] != 0) {
                g_hueOverride = hues[idx];
                Pk_RleWalk(reinterpret_cast<void *>(PORT_ADDR(&RowBlitLookupAll)), rec, pickRect, curX, curY);   // 0x004767d0
            } else {
                Pk_RleWalk(reinterpret_cast<void *>(PORT_ADDR(&RowBlitCopyAll)), rec, pickRect, curX, curY);   // 0x004767d0
            }
            curX += w;
        }
        idx++;
    }

    if (g_is565 != 0) {
        int row = pickRect[1];
        int px = pickRect[0];
        uint16_t *p = reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(b6_frame_bitmap(this, 0)) + 4) +
                      ((pickRect[1] - 1) * width + pickRect[0]);
        while (row <= pickRect[3]) {
            while (px <= pickRect[2]) {
                uint32_t v = *p;
                p++;
                px++;
                p[-1] = static_cast<uint16_t>(((v & 0xffe0) << 1) | (v & 0x1f));
            }
            px = pickRect[0];
            p += (pickRect[0] - pickRect[2] + width) - 1;
            row++;
        }
    }
}
HOOK_METHOD(0x0046f1a0, CObjPickerGump, RedrawList);

// 0x0046f3a0  size 0xbd  vtable slot 8, RET 4
// When visible and the frame bitmap's screen rect intersects clip, blit it clipped to the back buffer,
// then draw every child with the same clip.
void __thiscall CObjPickerGump::Draw(int *clip)
{
    int r[4];

    if (visible == 0)
        return;
    int sx = 0;
    int sy = 0;
    LocalToScreen(&sx, &sy);   // 0x0043b4e0
    B6Bitmap *bmp = b6_frame_bitmap(this, 0);
    r[0] = sx;
    r[1] = sy;
    r[2] = sx + bmp->w;
    r[3] = sy + bmp->h;
    if (!Pk_RectIntersects(clip, r))   // 0x0043b540
        return;
    Pk_BlitClipped(reinterpret_cast<void *>(PORT_ADDR(&RowBlitCopyNonZero)), bmp->px, clip, bmp->w, bmp->h, sx, sy, g_backBuffer, 0x280,
                   0x1e0);   // 0x004342e0
    for (CGump *c = firstChild; c != nullptr; c = c->next)
        PORT_VSLOT1(c, 0x08, void, int *)(clip);
}
HOOK_VMETHOD(0x0046f3a0, CObjPickerGump, Draw);

// Slot override: same body as CObjPickerGump::Draw above; the linker folds the copies.
void __thiscall CMacroCmdGump::Draw(int *clip)
{
    int r[4];

    if (visible == 0)
        return;
    int sx = 0;
    int sy = 0;
    LocalToScreen(&sx, &sy);   // 0x0043b4e0
    B6Bitmap *bmp = b6_frame_bitmap(this, 0);
    r[0] = sx;
    r[1] = sy;
    r[2] = sx + bmp->w;
    r[3] = sy + bmp->h;
    if (!Pk_RectIntersects(clip, r))   // 0x0043b540
        return;
    Pk_BlitClipped(reinterpret_cast<void *>(PORT_ADDR(&RowBlitCopyNonZero)), bmp->px, clip, bmp->w, bmp->h, sx, sy, g_backBuffer, 0x280,
                   0x1e0);   // 0x004342e0
    for (CGump *c = firstChild; c != nullptr; c = c->next)
        PORT_VSLOT1(c, 0x08, void, int *)(clip);
}

// 0x0046f460  size 0x54  RET 8
// Clears +0x9c. When +0x94 is set it is cleared with +0xa8 and the drag gump; else a pending +0xa8 == 2 is
// cleared and turned into a click at (px, py).
void __thiscall CObjPickerGump::MouseUp(int px, int py)
{
    unk_9c = 0;
    if (unk_94 != 0) {
        unk_94 = 0;
        unk_a8 = 0;
        g_dragGump = nullptr;
        return;
    }
    if (unk_a8 == 2) {
        unk_a8 = 0;
        ClickStrip(px, py);   // 0x0046f700
    }
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x0046f460, CObjPickerGump, MouseUp);

// 0x0046f4c0  size 0x14b  RET 8
// Sets cursor 0, finds the entry under (px, py) in the strip (walking the entries from unk_b8, each as wide as
// its art record, 0x2c when the record is missing) and, when it differs from unk_c4, copies its name (or the
// string at 0x004b6b40 when none) into the list control's text at +0xec and redraws the control.
void __thiscall CObjPickerGump::MouseOver(int px, int py)
{
    int newSel = -1;
    uint16_t i;
    int rem;
    uint8_t *rec;

    Pk_SetCursor(0);   // 0x00482be0
    if (unk_ac == 0 && px >= pickRect[0] && px <= pickRect[2] && py >= pickRect[1] && py <= pickRect[3]) {
        i = static_cast<uint16_t>(unk_b8);
        rem = px - pickRect[0];
        while (i < count) {
            if (rem <= 0)
                break;
            rec = Pk_ArtRecord(ids[i] + 0x4000);   // 0x00476130
            if (rec == nullptr)
                rec = Pk_ArtRecord(0x4000);   // 0x00476130
            if (rec != nullptr)
                rem -= *reinterpret_cast<uint16_t *>(rec + 4);
            else
                rem -= 0x2c;
            i++;
        }
        if (rem < 0)
            newSel = i - 1;
    }

    if (newSel != unk_c4) {
        unk_c4 = newSel;
        if (newSel == -1)
            strcpy(reinterpret_cast<char *>(listControl) + 0xec, reinterpret_cast<const char *>(GADDR(g_emptyString)));
        else
            strcpy(reinterpret_cast<char *>(listControl) + 0xec, names[newSel]);
        reinterpret_cast<CInputControlGump *>(listControl)->Redraw();   // CInputControlGump::Redraw
    }
}
HOOK_VMETHOD(0x0046f4c0, CObjPickerGump, MouseOver);

// 0x0046f610  size 0xee  plain RET
// One step of the slide: while unk_ac is set, move slideOffset by slideDir (clamped to +-3 near the ends of
// the current entry's art) and, when the offset leaves the entry, advance unk_b8 and stop; redraw the strip.
void __thiscall CObjPickerGump::Update()
{
    int dir;
    int step;
    int pos;
    uint8_t *rec;

    if (unk_ac == 0)
        return;
    rec = Pk_ArtRecord(ids[unk_b8] + 0x4000);   // 0x00476130
    if (rec == nullptr)
        rec = Pk_ArtRecord(0x4000);   // 0x00476130
    dir = slideDir;
    step = dir;
    if (dir > 0) {
        if (rec != nullptr) {
            if (slideOffset >= static_cast<int>(*reinterpret_cast<uint16_t *>(rec + 4)) - 6)
                step = 3;
        } else {
            if (slideOffset >= 0x26)
                step = 3;
        }
    } else {
        if (slideOffset <= 6)
            step = -3;
    }
    slideOffset += step;
    pos = slideOffset;
    if (dir > 0) {
        if (rec != nullptr && pos < static_cast<int>(*reinterpret_cast<uint16_t *>(rec + 4)) - 1) {
            RedrawList();   // 0x0046f1a0
            return;
        }
        unk_b8 = unk_b8 + 1;
        unk_ac = 0;
        RedrawList();   // 0x0046f1a0
        return;
    }
    if (rec == nullptr || pos <= 0)
        unk_ac = 0;
    RedrawList();   // 0x0046f1a0
}
HOOK_VMETHOD(0x0046f610, CObjPickerGump, Update);

// 0x0046f700  size 0xe0  RET 8
// Click at (px, py) on the strip: finds the entry under it as in MouseMove (a missing record counts as
// width 0 here) and, when there is one, stores its 1-based number in selection and calls vtable slot 0x1c
// with (0, 0).
void __thiscall CObjPickerGump::ClickStrip(int px, int py)
{
    uint16_t i;
    int rem;
    uint8_t *rec;

    if (unk_ac != 0)
        return;
    if (px < pickRect[0] || px > pickRect[2])
        return;
    if (py < pickRect[1] || py > pickRect[3])
        return;
    i = static_cast<uint16_t>(unk_b8);
    rem = px - pickRect[0];
    while (i < count) {
        if (rem <= 0)
            break;
        rec = Pk_ArtRecord(ids[i] + 0x4000);   // 0x00476130
        if (rec == nullptr)
            rec = Pk_ArtRecord(0x4000);   // 0x00476130
        if (rec != nullptr)
            rem -= *reinterpret_cast<uint16_t *>(rec + 4);
        i++;
    }
    if (rem >= 0)
        return;
    selection = i;
    PORT_VSLOT2(this, 0x1c, void, int, int)(0, 0);
}
HOOK_METHOD(0x0046f700, CObjPickerGump, ClickStrip);
