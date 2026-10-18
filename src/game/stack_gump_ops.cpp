// CStackGump second-pass ports: activate, slider, key and frame redraw.
#include "stack_gump.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../runtime/ported.h"
#include "batch06_helpers.h"
#include "font_manager.h"
#include "globals.h"
#include "raw_field.h"
#include "batch01_helpers.h"
#include "gump_base_ops_ports.h"
#include "global_views.h"
#include "scroll_thumb_gump.h"
#include "ui_callbacks_ports.h"

#define sg_focusGump GLOBAL_AS(CGump *, g_focusedInput)
// 0x0040d920: UiCb_PickUpOwner, __fastcall(gump)
#define sg_PickUpOwner(a1) (UiCb_PickUpOwner(a1))

// 0x00469bb0  size 0x3e  RET 8
// Stores the two arguments in the base fields +0x6c / +0x70, sets +0x94, raises the gump, takes the input focus, redraws.
void __thiscall CStackGump::MouseDown(int a, int b)
{
    unk_94 = 1;
    unk_6c = a;
    unk_70 = b;
    BringToFront(0);
    PORT_THISCALL1(&Gump_SetFocus, void, GLOBAL_PTR(void, g_gumpMgr_storage), CGump *)(this);   // 0x0043a820 on the gump manager
    RedrawFrame(0);
}
HOOK_VMETHOD(0x00469bb0, CStackGump, MouseDown);

// 0x00469bf0  size 0x3d  CStackGump::OnScroll  RET 0xc
// value = maxValue * pos / range, formatted into the amount text; the third stack argument is not read.
void __thiscall CStackGump::OnScroll(int pos, int range, int unused)
{
    (void)unused;
    value = maxValue * pos / range;
    sprintf(text, "%d", value);   // 0x0048b1d0
    RedrawFrame(0);
}
HOOK_VMETHOD(0x00469bf0, CStackGump, OnScroll);

// 0x00469c30  size 0x10d  CStackGump::OnKey  RET 4
// Enter runs the pick-up callback; digits and backspace edit the amount text, which is clamped to maxValue.
void __thiscall CStackGump::OnKey(int key)
{
    int edit = 0;

    if (key == 0xd) {
        sg_PickUpOwner(this);
    } else if ((key >= 0x30 && key <= 0x39) || key == 8) {
        edit = key;
    }
    if (edit == 0)
        return;

    if (unk_98 == 0) {
        unk_98 = 1;
        strcpy(text, g_emptyString);
    }
    int len = static_cast<int>(strlen(text));
    if (edit == 8 && len != 0) {
        text[len - 1] = 0;
    } else if (edit != 8 && !(edit == 0x30 && len == 0)) {
        text[len] = static_cast<char>(edit);
        text[len + 1] = 0;
        if (atoi(text) > maxValue)   // 0x0048c060
            text[len] = 0;
    }
    value = atoi(text);   // 0x0048c060
    RedrawFrame(0);
    reinterpret_cast<CScrollThumbGump *>(scrollThumb)->SetPosition(value, maxValue);
}
HOOK_VMETHOD(0x00469c30, CStackGump, OnKey);

// 0x00469d40  size 0x16b  CStackGump::RedrawFrame  RET 4 (argument not read)
// Draws the background art, then the amount text, then a "_" caret while this gump has the focus.
void __thiscall CStackGump::RedrawFrame(int unused)
{
    (void)unused;
    char buf[8];
    int clip[4];   // {left, top, right, bottom}
    int tw;
    int th;
    int base;

    clip[0] = 0;
    clip[2] = width - 1;
    clip[1] = 0;
    clip[3] = height - 1;
    B6Bitmap *bmp = b6_frame_bitmap(this, 0);
    b6_draw_art(B6_ROWBLIT_COPY, unk_50, 0, 0, bmp->px, bmp->w, bmp->h);

    clip[2] = width;
    clip[3] = height;
    clip[0] = 0;
    clip[1] = 0;
    strcpy(buf, text);
    g_fontMgr.MeasureText(1, const_cast<char *>("Aq"), &tw, &th, &base);
    if (strlen(buf) != 0) {
        g_fontMgr.MeasureText(1, buf, &tw, &th, &base);
        g_fontMgr.DrawTextPlain(0x1d, base + 0x2a, 1, 0x386, buf, b6_frame_bitmap(this, 0)->px, width, clip);
    } else {
        tw = 0;
    }
    if (sg_focusGump == this) {
        strcpy(buf, "_");
        g_fontMgr.DrawTextPlain(tw + 0x1e, base + 0x2a, 1, 0x386, buf, b6_frame_bitmap(this, 0)->px, width, clip);
    }
}
HOOK_METHOD(0x00469d40, CStackGump, RedrawFrame);

// Own copies of bodies shared with other classes in the original (the linker folds them).
void __thiscall CStackGump::Draw(int *clip)
{
    int ox = 0;
    int oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);   // absolute screen origin

    uint16_t *bmp = gfx::bitmapOf(this);
    int rect[4];
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = bmp[0] + ox;
    rect[3] = oy + bmp[1];

    // 0x0043b540: thiscall on the gump manager object, (clip, &rect), RET 8
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, void *, int *)(clip, rect) != 0) {
        GFX_BlitFrame(gfx::artPlain(), bmp + 2, clip, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);

        for (CGump *c = firstChild; c != nullptr; c = c->next)
            PORT_VSLOT1(c, kGumpSlot_Draw, void, void *)(clip);   // child->draw(clip)
    }
}

int __thiscall CStackGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

void __thiscall CStackGump::MouseUp(int, int)
{
    RAWF(int32_t, this, 0x94) = 0;
    g_dragGump = nullptr;
}

void __thiscall CStackGump::RButtonUp(int px, int py)
{
    (void)px;
    (void)py;
    g_dragGump = nullptr;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}

void __thiscall CStackGump::MouseMove(int mx, int my)
{
    if (RAWF(int32_t, this, 0x94) != 0)
        Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);   // 0x0043b140
}

