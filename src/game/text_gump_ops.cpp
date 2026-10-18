// CTextGump second pass (batch 30): destructor, mouse handling, scrolling, repaint and draw.
// The SEH frame of the destructor (one try level) is omitted.
#include "text_gump.h"

#include <windows.h>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "batch06_helpers.h"
#include "font_manager.h"
#include "globals.h"
#include "crt.h"
#include "scroll_thumb_gump.h"
#include "unicode_text_ports.h"
#include "skill_list_gump_ops_ports.h"
#include "gump_base_ops_ports.h"
#include "vtables.h"

#define TG_FontMgr GLOBAL_AS(CFontManager, g_fontMgr_storage)
// 0x0047cd70: ECX = string, EDX = max width, stack (&width, &height, &zero), RET 0xc.
#define TG_MeasureUnicode(a1, a2, a3, a4, a5) (MeasureWrappedUnicode(a1, a2, a3, a4, a5))
// 0x0047d0b0: ECX = x, EDX = y, stack (hue, string, max width, dest, pitch, clip), RET 0x18.
#define TG_DrawUnicode(a1, a2, a3, a4, a5, a6, a7, a8) (DrawWrappedUnicode(a1, a2, a3, a4, a5, (uint16_t *)(a6), a7, a8))
// 0x00472be0: ECX = art id, EDX = 2, stack (bitmap, h1, h2), RET 0xc.
#define TG_DrawFrame(a1, a2, a3, a4, a5) (SkillList_DrawTiledFrame(a1, a2, (uint16_t *)(a3), a4, a5))
// 0x0043b540: thiscall on the gump manager, (clip, rect), RET 8.
#define TG_RectIntersects(o, a1, a2) PORT_THISCALL2(&GumpRectIntersects, int, (o), int *, int *)(a1, a2)

static inline CScrollThumbGump *TG_Thumb(CTextGump *g) { return static_cast<CScrollThumbGump *>(g->childC0); }

HOOK_VTABLE(0x004a0840, CTextGump);
HOOK_SDD(0x00479960, CTextGump);

// 0x00479990  size 0xa3  destructor body: frees both frame bitmaps and the frame table, then the base destructor.
CTextGump::~CTextGump()
{
    if (frameData[1] != 0) {
        gfx::FreeTagged(b6_ptr(frameData[1]), "text frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData[3] != 0) {
        gfx::FreeTagged(b6_ptr(frameData[3]), "text frame1 pBMP");
        frameData[3] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "text framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00479990, CTextGump);

// 0x00479a40  size 0x5d  RET 8 (x, y)
// Mouse press: remember the point, mark the button held, bring the gump to the front, then track the
// double-click window (pressState 1 = first press at pressTick, 2 = second press inside it).
void __thiscall CTextGump::MouseDown(int mx, int my)
{
    unk_6c = mx;
    unk_70 = my;
    held = 1;
    repeat = 1;
    BringToFront(0);   // 0x0043ba70
    if (pressState == 1) {
        pressState = 2;
        return;
    }
    pressState = 1;
    pressTick = static_cast<int32_t>(GetTickCount());
}
HOOK_VMETHOD(0x00479a40, CTextGump, MouseDown);

// 0x00479aa0  size 0x1bb  RET 8 (x, y)
// Click on the arrow strip right of the text: above the thumb scrolls up by ten lines, below it down by ten.
void __thiscall CTextGump::ClickScroll(int mx, int my)
{
    int w;
    int h;
    int z;
    int hitEnd = 0;   // edx
    int target;       // ebx

    if (mx < width - 0x30 || mx > width - 0x19 || my < textTop || my > unk_bc || cur == nullptr)
        return;
    childC4->frameIndex = 1;
    target = my - 8;
    if (target < TG_Thumb(this)->y) {
        for (int i = 10; i != 0; i--) {
            if (cur->prev != nullptr)
                cur = cur->prev;
            else
                hitEnd = 1;
        }
        if (hitEnd != 0)
            lineOffset = 0;
        TG_Thumb(this)->PlaceFromTextList();   // 0x00413b20
        CScrollThumbGump *t = TG_Thumb(this);
        if (t->y < target || t->y < t->minY) {
            t->y = target;
            t = TG_Thumb(this);
            if (t->y < t->minY)
                t->y = t->minY;
            t = TG_Thumb(this);
            PORT_VSLOT3(this, 0x30, void, int, int, CGump *)(t->y - t->minY, t->maxY - t->minY, t);
            RedrawText(1, 1);   // 0x0047a210
        } else {
            RedrawText(1, 1);   // 0x0047a210
        }
        return;
    }

    for (int i = 10; i != 0; i--) {
        if (cur->next != nullptr)
            cur = cur->next;
        else
            hitEnd = 1;
    }
    if (hitEnd != 0) {
        if (cur->isUnicode != 0) {
            TG_MeasureUnicode(static_cast<uint16_t *>(cur->text), unk_b8 - textLeft + 1, &w, &h, &z);   // 0x0047cd70
        } else {
            TG_FontMgr.MeasureTextWrapped(9, static_cast<char *>(cur->text), unk_b8 - textLeft + 1, &w, &h, &z, 2);   // 0x004328e0
        }
        lineOffset = h - 1;
    }
    TG_Thumb(this)->PlaceFromTextList();   // 0x00413b20
    if (TG_Thumb(this)->y > target) {
        TG_Thumb(this)->y = target;
        CScrollThumbGump *t = TG_Thumb(this);
        PORT_VSLOT3(this, 0x30, void, int, int, CGump *)(t->y - t->minY, t->maxY - t->minY, t);
    }
    RedrawText(1, 1);   // 0x0047a210
}
HOOK_METHOD(0x00479aa0, CTextGump, ClickScroll);

// 0x00479c60  size 0x7f  idle tick
void __thiscall CTextGump::Tick()
{
    if (held != 0) {
        repeat = repeat + 1;
        if (repeat == 2) {
            ClickScroll(unk_6c, unk_70);   // 0x00479aa0
            repeat = 0;
        }
    }
    uint32_t now = GetTickCount();
    uint32_t dbl = GetDoubleClickTime();
    if (pressState == 1 && now - static_cast<uint32_t>(pressTick) > dbl) {
        pressState = 0;
        if (held != 0)
            unk_94 = 1;
    }
}
HOOK_VMETHOD(0x00479c60, CTextGump, Tick);

// 0x00479ce0  size 0x1ce  RET 0xc (pos, range, thumb)
// The scroll thumb was moved to `pos` of `range`: pick the line at that fraction of the total height.
void __thiscall CTextGump::OnScroll(int pos, int range, int thumb)
{
    (void)thumb;
    int w;
    int h;
    int z;
    uint8_t heights[100];   // ebp-0x74
    int total;
    CTextLine *node;

    childC4->frameIndex = 1;
    if (pos <= 1) {
        cur = g_textListHead;
        lineOffset = 0;
        RedrawText(1, 1);   // 0x0047a210
        return;
    }
    if (pos == range) {
        cur = TG_TextListTail;
        if (cur->isUnicode != 0) {
            TG_MeasureUnicode(static_cast<uint16_t *>(cur->text), unk_b8 - textLeft + 1, &w, &h, &z);   // 0x0047cd70
            lineOffset = h - 1;
            RedrawText(1, 1);   // 0x0047a210
            return;
        }
        TG_FontMgr.MeasureTextWrapped(9, static_cast<char *>(cur->text), unk_b8 - textLeft + 1, &w, &h, &z, 2);   // 0x004328e0
        lineOffset = h - 1;
        RedrawText(1, 1);   // 0x0047a210
        return;
    }

    total = 0;
    node = g_textListHead;
    {
        uint8_t *out = heights;
        while (node != nullptr) {
            if (node->isUnicode != 0) {
                TG_MeasureUnicode(static_cast<uint16_t *>(node->text), unk_b8 - textLeft + 1, &w, &h, &z);   // 0x0047cd70
            } else {
                TG_FontMgr.MeasureTextWrapped(9, static_cast<char *>(node->text), unk_b8 - textLeft + 1, &w, &h, &z, 2);   // 0x004328e0
            }
            total += h;
            *out = static_cast<uint8_t>(h);
            out++;
            node = node->next;
        }
    }
    int target = total * pos / range;
    node = g_textListHead;
    {
        uint8_t *p = heights;
        while (target > 0) {
            int rowH = *p;
            if (target < rowH) {
                lineOffset = target;
                target = 0;
            } else {
                lineOffset = 0;
                node = node->next;
                target -= rowH;
                p++;
            }
        }
    }
    cur = node;
    RedrawText(1, 1);   // 0x0047a210
}
HOOK_VMETHOD(0x00479ce0, CTextGump, OnScroll);

// 0x0047a0b0  size 0x151  show the end of the history: start at the newest line and walk back until the
// lines above fill the text area (textBottom - textTop - 4 pixels).
void __thiscall CTextGump::ScrollToEnd()
{
    int w;
    int h;
    int z;

    cur = TG_TextListTail;
    if (cur->isUnicode != 0) {
        TG_MeasureUnicode(static_cast<uint16_t *>(cur->text), unk_b8 - textLeft + 1, &w, &h, &z);   // 0x0047cd70
    } else {
        TG_FontMgr.MeasureTextWrapped(9, static_cast<char *>(cur->text), unk_b8 - textLeft + 1, &w, &h, &z, 2);   // 0x004328e0
    }
    lineOffset = h - 1;
    int room = unk_bc - textTop - 4;   // edi
    do {
        int off = lineOffset;
        if (room >= off + 1) {
            CTextLine *prevLine = cur->prev;
            if (prevLine != nullptr) {
                cur = prevLine;
                room += -1 - off;
                if (prevLine->isUnicode != 0) {
                    TG_MeasureUnicode(static_cast<uint16_t *>(prevLine->text), unk_b8 - textLeft + 1, &w, &h, &z);   // 0x0047cd70
                } else {
                    TG_FontMgr.MeasureTextWrapped(9, static_cast<char *>(prevLine->text), unk_b8 - textLeft + 1, &w, &h, &z, 2);   // 0x004328e0
                }
                lineOffset = h - 1;
            } else {
                lineOffset = 0;
                room = 0;
            }
        } else if (room >= off) {
            room -= off;
            lineOffset = 0;
        } else {
            lineOffset = off - room;
            room = 0;
        }
    } while (room != 0);
}
HOOK_METHOD(0x0047a0b0, CTextGump, ScrollToEnd);

// 0x0047a210  size 0x1b5  RET 8 (arguments not read)
// Repaint frame 0: the frame art, the two arrow buttons' backgrounds, then the history lines from `cur`
// (starting lineOffset pixels above the top) until the bottom of the text area is passed.
void __thiscall CTextGump::RedrawText(int a, int b)
{
    (void)a;
    (void)b;
    int clip[4];   // ebp-0x24: left, top, right, bottom
    int w;
    int h;
    int h1;
    int h2;
    int lineH;
    int dummy;
    int z;
    int rowY;
    CTextLine *node;

    clip[0] = textLeft;
    clip[2] = unk_b8;
    clip[1] = textTop;
    clip[3] = unk_bc;
    gfx::ArtSize(0x82d, &w, &h);   // 0x00433e70
    h1 = h;
    gfx::ArtSize(0x82e, &w, &h);   // 0x00433e70
    h2 = h;
    TG_DrawFrame(unk_50, 2, b6_frame_bitmap(this, 0), h1 - 1, h2);   // 0x00472be0
    {
        B6Bitmap *bmp = b6_frame_bitmap(this, 0);
        gfx::DrawArt(gfx::artPlain(), 0x82a, 0x6f, h1 + 0xa, bmp->px, bmp->w, bmp->h);   // 0x00433ed0
    }
    {
        B6Bitmap *bmp = b6_frame_bitmap(this, 0);
        gfx::DrawArt(gfx::artPlain(), 0x82b, 0x1e, h1 + 0x22, bmp->px, bmp->w, bmp->h);   // 0x00433ed0
    }
    {
        B6Bitmap *bmp = b6_frame_bitmap(this, 0);
        gfx::DrawArt(gfx::artPlain(), 0x82b, 0x1f, height - h2 - 0x30, bmp->px, bmp->w, bmp->h);   // 0x00433ed0
    }
    node = cur;
    if (node == nullptr)
        return;
    rowY = clip[1] - lineOffset;
    do {
        if (node->isUnicode != 0) {
            TG_MeasureUnicode(static_cast<uint16_t *>(node->text), clip[2] - clip[0] + 1, &dummy, &lineH, &z);   // 0x0047cd70
            TG_DrawUnicode(clip[0], rowY, node->hue, static_cast<uint16_t *>(node->text), clip[2] - clip[0] + 1,
                           b6_frame_bitmap(this, 0), width, clip);   // 0x0047d0b0
        } else {
            lineH = TG_FontMgr.DrawTextWrapped(clip[0], rowY, 9, node->hue, static_cast<char *>(node->text), clip[2] - clip[0] + 1,
                                               reinterpret_cast<uint16_t *>(b6_frame_bitmap(this, 0)), width, clip, 2, 0);   // 0x00432260
        }
        node = node->next;
        rowY += lineH;
    } while (rowY <= clip[3] && node != nullptr);
}
HOOK_METHOD(0x0047a210, CTextGump, RedrawText);

// 0x0047a3d0  size 0xc6  vtable slot 0x08, RET 4
// Blit frame 0 (frame 1 when unk_60 is set) clipped, then draw the children unless unk_60 is set.
void __thiscall CTextGump::Draw(int *clip)
{
    int sx = 0;
    int sy = 0;
    int rect[4];
    B6Bitmap *bmp;

    LocalToScreen(&sx, &sy);   // 0x0043b4e0
    if (unk_60 == 0)
        bmp = b6_frame_bitmap(this, 0);
    else
        bmp = b6_frame_bitmap(this, 1);
    rect[0] = sx;
    rect[1] = sy;
    rect[2] = sx + bmp->w;
    rect[3] = sy + bmp->h;
    if (TG_RectIntersects(g_gumpManager, clip, rect) != 0) {   // 0x0043b540
        GFX_BlitFrame(gfx::artPlain(), bmp->px, clip, bmp->w, bmp->h, sx, sy, g_backBuffer, 0x280, 0x1e0);   // 0x004342e0
        if (unk_60 == 0) {
            for (CGump *c = firstChild; c != nullptr; c = c->next)
                PORT_VSLOT1(c, 0x08, void, int *)(clip);
        }
    }
}
HOOK_VMETHOD(0x0047a3d0, CTextGump, Draw);
