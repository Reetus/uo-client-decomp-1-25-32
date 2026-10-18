// CTipsGump second pass (batch 30): constructor, destructor, mouse handling, scrolling, resize and repaint.
// The SEH frames (try levels around operator new) are omitted.
#include "tips_gump.h"

#include <windows.h>

#include <cstdlib>
#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "batch06_helpers.h"
#include "button_gump.h"
#include "font_manager.h"
#include "crt.h"
#include "skill_list_gump_ops_ports.h"
#include "window_helpers_31_ports.h"
#include "config_file_ports.h"
#include "book_gump.h"
#include "gump_callbacks.h"
#include "class_0049c690.h"
#include "ui_callbacks_ports.h"
#include "vtables.h"
#include "globals.h"

#define TP_FontMgr GLOBAL_AS(CFontManager, g_fontMgr_storage)
#define TP_Self static_cast<int>(reinterpret_cast<uintptr_t>(this))
// 0x00472be0: ECX = art id, EDX = 2, stack (bitmap, h1, h2), RET 0xc.
#define TP_DrawFrame(a1, a2, a3, a4, a5) (SkillList_DrawTiledFrame(a1, a2, (uint16_t *)(a3), a4, a5))
// 0x00482be0: set the mouse cursor, one stack argument, ECX not read.
#define TP_SetCursor(a1) (SetCursorForRegion((unsigned)(a1)))
// 0x00424ac0: ECX = setting id, EDX = text.
#define TP_WriteConfig(a1, a2) (WriteConfigSetting(a1, a2))

// 0x0047a4a0  size 0x5c2
// Builds the tip window: copies the lines, makes frame 0 (text area) and frame 1 (background art), then the
// scroll thumb, two arrow buttons, two end buttons and, for the tip of the day, next / previous and the check box.
CTipsGump::CTipsGump(int x_, int y_, int lineCount_, const char *src, int tipFlag)
{
    (void)x_;
    (void)y_;
    int artW;
    int btnH;
    CButtonGump *btn;
    topLine = 0;
    lineCount = lineCount_;
    repeat = 0;
    isTipOfDay = tipFlag;
    if (tipFlag != 0) {
        x = 0x140;
        y = 0xb4;
    } else {
        x = 0x14;
        y = 0x14;
    }
    {
        char *dst = lines[0];
        int i = 0;
        if (lineCount > 0) {
            do {
                strcpy(dst, src);
                src += 0x50;
                i++;
                dst += 0x50;
            } while (i < lineCount);
        }
    }
    name = const_cast<char *>("tips gump");
    dirty = 0;
    unk_58 = 0;
    unk_a0 = 0;
    held = 0;
    unk_98 = 0;
    unk_94 = 0;
    pressState = 0;
    pressTick = 0;
    unk_60 = 0;
    unk_50 = 0x820;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    gfx::ArtSize(0x820, &width, &height);   // 0x00433e70
    rowStride = (width + 7) & ~7;
    height = 0x12c;
    width = rowStride;
    gfx::ArtSize(0x9cb, &unk_24, &unk_28);   // 0x00433e70
    frameIndex = 0;
    frameCount = 2;
    frameData = static_cast<int32_t *>(gfx::AllocTagged(0x10, "framedata for CTipsGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    b6_set_frame_bitmap(this, 0, gfx::AllocTagged(width * height * 2 + 4, "text frame0"));
    memset(b6_frame_bitmap(this, 0)->px, 0, static_cast<size_t>(width * height) * 2);
    b6_frame_bitmap(this, 0)->w = static_cast<uint16_t>(width);
    b6_frame_bitmap(this, 0)->h = static_cast<uint16_t>(height);
    b6_set_frame_bitmap(this, 1, gfx::AllocTagged(unk_24 * unk_28 * 2 + 4, "text frame1"));
    memset(b6_frame_bitmap(this, 1)->px, 0, static_cast<size_t>(unk_24 * unk_28) * 2);
    b6_frame_bitmap(this, 1)->w = static_cast<uint16_t>(unk_24);
    b6_frame_bitmap(this, 1)->h = static_cast<uint16_t>(unk_28);
    {
        B6Bitmap *bmp = b6_frame_bitmap(this, 1);
        if (isTipOfDay != 0)
            gfx::DrawArt(gfx::artPlain(), 0x9cb, 0, 0, bmp->px, bmp->w, bmp->h);   // 0x00433ed0
        else
            gfx::DrawArt(gfx::artPlain(), 0x9d3, 0, 0, bmp->px, bmp->w, bmp->h);   // 0x00433ed0
    }
    textLeft = 0x26;
    textTop = 0x44;
    textRight = width - 0x31;
    if (textRight % 2 == 0)
        textRight = textRight - 1;
    textBottom = height - 0x4e;
    RedrawText();   // 0x0047b040
    gfx::ArtSize(0x82e, &artW, &btnH);   // 0x00433e70

    thumb = new CScrollThumbGump(0x100, 0x48, 1, height - btnH - 0x38, (int)(this), 0x1f);   // 0x00413740
    gfx::AddChild(thumb, this);   // 0x0043bcc0

    btn = new CButtonGump(0x89, 0, TP_Self, 0, 0, PORT_ADDR(&CBookGump::Iconify), 0x82d, 0x82d, 0);   // 0x00412fb0
    gfx::AddChild(btn, this);

    btn = new CButtonGump(0x89, height - btnH - 3, TP_Self, 0, 0, PORT_ADDR(&CGumpCallbacks::AddBackdropChild), 0x82e, 0x82e, 0);   // 0x00412fb0
    btn->artHover = 0x82f;
    gfx::AddChild(btn, this);

    btn = new CButtonGump(0xf9, 0x38, TP_Self, 2, 0, PORT_ADDR(&Nop_00406720), 0x824, 0x824, 0);   // 0x00412fb0
    gfx::AddChild(btn, this);

    btn = new CButtonGump(0xf9, height - btnH - 0x34, TP_Self, 2, 0, PORT_ADDR(&Nop_00406720), 0x825, 0x825, 0);   // 0x00412fb0
    gfx::AddChild(btn, this);

    dontShow = nullptr;
    if (isTipOfDay != 0) {
        btn = new CButtonGump(0xfb, height - btnH - 0x18, TP_Self, 2, 0, PORT_ADDR(&UiCb_TipNext), 0x9cd, 0x9cd, 0);   // 0x00412fb0
        btn->unk_c0 = 0;
        gfx::AddChild(btn, this);

        btn = new CButtonGump(0x23, height - btnH - 0x18, TP_Self, 2, 0, PORT_ADDR(&UiCb_TipPrev), 0x9cc, 0x9cc, 0);   // 0x00412fb0
        btn->unk_c0 = 0;
        gfx::AddChild(btn, this);

        dontShow = new CToggleControl(0x60, height - btnH - 0x1b, TP_Self, 0x9ce, 0x9cf, 9, 0x288, "Don't show again",
                                         g_tipEnabled == 0, 0, 0);   // 0x00416120
        gfx::AddChild(dontShow, this);
    }
}
HOOK_CTOR(0x0047a4a0, CTipsGump, (int, int, int, const char *, int));

HOOK_VTABLE(0x004a0958, CTipsGump);
HOOK_SDD(0x0047aa70, CTipsGump);

// 0x0047aaa0  size 0xdf  destructor body. For the tip of the day the "Don't show again" state is saved to the
// config (setting 0x2a, "yes" / "no") and kept in the global at 0x004a7780.
CTipsGump::~CTipsGump()
{
    if (isTipOfDay != 0) {
        int show = (dontShow->checked == 0) ? 1 : 0;
        const char *text = "yes";
        g_tipEnabled = show;
        if (show == 0)
            text = "no";
        TP_WriteConfig(0x2a, text);   // 0x00424ac0
    }
    if (frameData[1] != 0) {
        gfx::FreeTagged(b6_ptr(frameData[1]), "tips frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData[3] != 0) {
        gfx::FreeTagged(b6_ptr(frameData[3]), "tips frame1 pBMP");
        frameData[3] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "tips framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x0047aaa0, CTipsGump);

// 0x0047ab80  size 0x5d  RET 8 (x, y)
void __thiscall CTipsGump::MouseDown(int mx, int my)
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
HOOK_VMETHOD(0x0047ab80, CTipsGump, MouseDown);

// 0x0047abe0  size 0x115  RET 8 (x, y)
// Click on the arrow strip: above the thumb moves ten lines back, below it ten forward.
void __thiscall CTipsGump::ClickScroll(int mx, int my)
{
    int target;

    if (mx < width - 0x30 || mx > width - 0x19 || my < textTop || my > textBottom || lineCount == 0)
        return;
    target = my - 8;
    if (target < thumb->y) {
        for (int i = 10; i != 0; i--) {
            if (topLine != 0)
                topLine = topLine - 1;
        }
        thumb->SetPosition(topLine, lineCount);   // 0x00413c00
        if (thumb->y < target || thumb->y < thumb->minY) {
            thumb->y = target;
            if (thumb->y < thumb->minY)
                thumb->y = thumb->minY;
            PORT_VSLOT3(this, 0x30, void, int, int, CGump *)(thumb->y - thumb->minY, thumb->maxY - thumb->minY, thumb);
            RedrawText();   // 0x0047b040
        } else {
            RedrawText();   // 0x0047b040
        }
        return;
    }
    for (int i = 10; i != 0; i--) {
        int n = topLine + 1;
        if (n < lineCount)
            topLine = n;
    }
    thumb->SetPosition(topLine, lineCount);   // 0x00413c00
    if (thumb->y > target) {
        thumb->y = target;
        PORT_VSLOT3(this, 0x30, void, int, int, CGump *)(thumb->y - thumb->minY, thumb->maxY - thumb->minY, thumb);
    }
    RedrawText();   // 0x0047b040
}
HOOK_METHOD(0x0047abe0, CTipsGump, ClickScroll);

// 0x0047ad00  size 0x88  RET 8 (x, y)
// Mouse move: more than 4 pixels from the press starts a drag; while the button is held the gump follows.
void __thiscall CTipsGump::MouseMove(int mx, int my)
{
    TP_SetCursor(0);   // 0x00482be0
    int wasHeld = held;
    if (wasHeld != 0 && pressState != 0) {
        int dx = mx - unk_6c;
        if (dx < 0)
            dx = -dx;
        int dy = my - unk_70;
        if (dy < 0)
            dy = -dy;
        if (dx > 4 || dy > 4) {
            unk_94 = 1;
            pressState = 0;
        }
    }
    if (wasHeld != 0)
        Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);   // 0x0043b140
}
HOOK_VMETHOD(0x0047ad00, CTipsGump, MouseMove);

// 0x0047ad90  size 0x7f  idle tick
void __thiscall CTipsGump::Tick()
{
    if (held != 0) {
        repeat = repeat + 1;
        if (repeat == 2) {
            ClickScroll(unk_6c, unk_70);   // 0x0047abe0
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
HOOK_VMETHOD(0x0047ad90, CTipsGump, Tick);

// 0x0047ae10  size 0x54  RET 0xc (pos, range, thumb)
void __thiscall CTipsGump::OnScroll(int pos, int range, int thumb)
{
    (void)thumb;
    if (pos <= 1) {
        topLine = 0;
        RedrawText();   // 0x0047b040
        return;
    }
    if (pos == range) {
        topLine = lineCount - 1;
        RedrawText();   // 0x0047b040
        return;
    }
    topLine = lineCount * pos / range;
    RedrawText();   // 0x0047b040
}
HOOK_VMETHOD(0x0047ae10, CTipsGump, OnScroll);

// 0x0047ae70  size 0x1cb  RET 8 (width, height)
// Resize: re-anchor the children (those that answer vtable slot 0x98 by their own +0x9c / +0xa0 / +0xa8 fields,
// the others when they lie in the far half), then reallocate frame 0 and the background copy.
void __thiscall CTipsGump::Resize(int width_, int height_)
{
    rowStride = (width_ + 7) & ~7;
    for (CGump *child = lastChild; child != nullptr; child = child->prev) {
        char *c = reinterpret_cast<char *>(child);
        if (PORT_VSLOT0(child, 0x98, int)() != 0) {
            int oldX = child->x;
            *reinterpret_cast<int32_t *>(c + 0x9c) = oldX - width + rowStride;
            *reinterpret_cast<int32_t *>(c + 0xa0) = oldX - width + rowStride;
            child->x = *reinterpret_cast<int32_t *>(c + 0x9c);
            *reinterpret_cast<int32_t *>(c + 0xa8) = *reinterpret_cast<int32_t *>(c + 0xa8) + (height_ - height);
        } else {
            if (child->x > width / 2)
                child->x = child->x - width + rowStride;
            if (child->y > height / 2)
                child->y = height_ + child->y - height;
        }
    }
    if (frameData[1] != 0) {
        gfx::FreeTagged(b6_ptr(frameData[1]), "tips frame0 pBMP");
        frameData[1] = 0;
    }
    if (unk_58 != 0) {
        gfx::FreeTagged(b6_ptr(unk_58), "gumpbak tipsgump");
        unk_58 = 0;
    }
    height = height_;
    width = rowStride;
    b6_set_frame_bitmap(this, 0, gfx::AllocTagged(height * width * 2 + 4, "text frame0"));
    memset(b6_frame_bitmap(this, 0)->px, 0, static_cast<size_t>(height * width) * 2);
    b6_frame_bitmap(this, 0)->w = static_cast<uint16_t>(width);
    b6_frame_bitmap(this, 0)->h = static_cast<uint16_t>(height);
    unk_58 = static_cast<int32_t>(reinterpret_cast<uintptr_t>(gfx::AllocTagged(height * width * 2, "gump background")));
    textRight = width - 0x31;
    if (textRight % 2 == 0)
        textRight = textRight - 1;
    textBottom = height - 0x4e;
    thumb->SetPosition(topLine, lineCount);   // 0x00413c00
    RedrawText();   // 0x0047b040
}
HOOK_VMETHOD(0x0047ae70, CTipsGump, Resize);

// 0x0047b040  size 0x193  repaint frame 0: frame art, the lines from `topLine` down to the bottom of the text area.
void __thiscall CTipsGump::RedrawText()
{
    int clip[4];   // ebp-0x1c: left, top, right, bottom
    int w;
    int h;
    int h1;
    int h2;
    int y_;
    int idx;
    const char *line;

    clip[0] = textLeft;
    clip[2] = textRight;
    clip[1] = textTop;
    clip[3] = textBottom;
    gfx::ArtSize(0x82d, &w, &h);   // 0x00433e70
    h1 = h;
    gfx::ArtSize(0x82e, &w, &h);   // 0x00433e70
    h2 = h;
    TP_DrawFrame(unk_50, 2, b6_frame_bitmap(this, 0), h1 - 1, h2);   // 0x00472be0
    if (isTipOfDay != 0) {
        B6Bitmap *bmp = b6_frame_bitmap(this, 0);
        gfx::DrawArt(gfx::artPlain(), 0x9ca, 0x7f, h1 + 9, bmp->px, bmp->w, bmp->h);   // 0x00433ed0
    } else {
        B6Bitmap *bmp = b6_frame_bitmap(this, 0);
        gfx::DrawArt(gfx::artPlain(), 0x9d2, 0x70, h1 + 0xb, bmp->px, bmp->w, bmp->h);   // 0x00433ed0
    }
    {
        B6Bitmap *bmp = b6_frame_bitmap(this, 0);
        gfx::DrawArt(gfx::artPlain(), 0x82b, 0x1e, h1 + 0x22, bmp->px, bmp->w, bmp->h);   // 0x00433ed0
    }
    {
        B6Bitmap *bmp = b6_frame_bitmap(this, 0);
        gfx::DrawArt(gfx::artPlain(), 0x82b, 0x1f, height - h2 - 0x30, bmp->px, bmp->w, bmp->h);   // 0x00433ed0
    }
    y_ = clip[1];
    idx = topLine;
    line = lines[0] + idx * 0x50;
    do {
        TP_FontMgr.DrawTextWrapped(clip[0], y_, 6, 0, const_cast<char *>(line), clip[2] - clip[0] + 1,
                                   reinterpret_cast<uint16_t *>(b6_frame_bitmap(this, 0)), width, clip, 2, 0);   // 0x00432260
        y_ += 0xe;
        idx++;
        line += 0x50;
    } while (y_ <= clip[3] && idx < lineCount);
}
HOOK_METHOD(0x0047b040, CTipsGump, RedrawText);

// 0x0047b1e0  size 0x4b  vtable slot 0x0c, RET 8
int __thiscall CTipsGump::HitTest(int px, int py)
{
    B6Bitmap *bmp = (unk_60 == 0) ? b6_frame_bitmap(this, 0) : b6_frame_bitmap(this, 1);
    int w = bmp->w;
    int h = bmp->h;
    if (px >= 0 && py >= 0 && px < w && py < h)
        return 1;
    return 0;
}
HOOK_VMETHOD(0x0047b1e0, CTipsGump, HitTest);
