// CRadarGump second-pass ports: destructor wrapper, mouse, frame rotation, blink, draw, hit test, update.
#include "radar_gump.h"

#include <windows.h>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "batch06_helpers.h"
#include "globals.h"
#include "raw_field.h"
#include "crt.h"
#include "client_misc_ops_ports.h"
#include "circle_trans_ports.h"
#include "gump_base_ops_ports.h"

// 0x00478cd0: returns 0 (stub)
#define rg_ShowDebugInfo() (Misc_Return0())
// 0x004242b0: empty procedure, two stack arguments
#define rg_Nullsub(o, a1, a2) PORT_THISCALL2(&CircleTrans_Nullsub, void, (o), int, int)(a1, a2)
// 0x0043b540: rectangles {l, t, r, b} overlap, __stdcall(a, b)
#define rg_RectIntersects(a1, a2) (GumpRectIntersects(a1, a2))

HOOK_VTABLE(0x0049ff70, CRadarGump);
HOOK_SDD(0x0046a210, CRadarGump);

// 0x0046a330  size 0x58  CRadarGump::MouseDown, RET 8
// Remembers the press position, raises the gump, and steps the click state 0 -> 1 (with a timestamp) -> 2.
void __thiscall CRadarGump::MouseDown(int mx, int my)
{
    unk_9c = 1;
    unk_6c = mx;
    unk_70 = my;
    BringToFront(0);
    if (unk_a8 == 1) {
        unk_a8 = 2;
        return;
    }
    unk_a8 = 1;
    unk_a4 = static_cast<int32_t>(GetTickCount());
}
HOOK_VMETHOD(0x0046a330, CRadarGump, MouseDown);

// 0x0046a390  size 0x64  CRadarGump::MouseUp, RET 8
// Ends a drag; a second click (state 2) calls the stub 0x00478cd0 and, when it is non-zero, the empty 0x004242b0.
void __thiscall CRadarGump::MouseUp(int mx, int my)
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
        if (rg_ShowDebugInfo() != 0)
            rg_Nullsub(this, mx, my);
    }
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x0046a390, CRadarGump, MouseUp);

// 0x0046ac90  size 0x15b  CRadarGump::RotateFrame
// Redraws the radar art into the rotated frame, then fills every pixel of it that still has the default
// colour from the map frame (frame 0), walking diagonally; finally starts the blink counter.
void __thiscall CRadarGump::RotateFrame()
{
    unk_200be = 0;
    unk_200bc = 0;
    B6Bitmap *rot = reinterpret_cast<B6Bitmap *>(rotatedFrame);
    b6_draw_art(B6_ROWBLIT_COPY, unk_50, 0, 0, rot->px, rot->w, rot->h);

    int rowCounter = 0;
    int k = 0;                 // source row of frame 0
    int startX = width / 2 - 1;
    int yy = 0;
    int j = 0;
    if (height > 0) {
        for (;;) {
            uint16_t *src = b6_frame_bitmap(this, 0)->px + k * halfWidth;
            B6Bitmap *dstBmp = reinterpret_cast<B6Bitmap *>(rotatedFrame);
            uint16_t *dst = dstBmp->px + (yy * width + startX);
            if (width > 0) {
                int base = yy - startX;
                int xc = startX;
                int i = 0;
                int m = 0;
                do {
                    xc++;
                    src++;
                    i++;
                    dst += width + 1;
                    if (i >= halfWidth || xc < 0 || xc >= width)
                        break;
                    int py = base + xc;
                    if (py < 0 || py >= height)
                        break;
                    if (static_cast<int>(*dst) == g_radarDefaultTile)
                        *dst = *src;
                    m++;
                } while (m < width);
            }
            rowCounter++;
            if (rowCounter & 1) {
                yy++;
            } else {
                startX--;
                k++;
            }
            if (k >= halfHeight)
                break;
            j++;
            if (j >= height)
                break;
        }
    }
    unk_200be = 0x64;
    BlinkPlayerPixel();
}
HOOK_METHOD(0x0046ac90, CRadarGump, RotateFrame);

// 0x0046adf0  size 0xc3  CRadarGump::BlinkPlayerPixel
// Every fifth call: swap the centre pixel of the rotated frame between white and the colour saved in +0x200bc.
void __thiscall CRadarGump::BlinkPlayerPixel()
{
    unk_200be = static_cast<int16_t>(unk_200be + 1);
    if (static_cast<uint16_t>(unk_200be) < 5)
        return;
    uint16_t saved = static_cast<uint16_t>(unk_200bc);
    unk_200be = 0;
    B6Bitmap *bmp = reinterpret_cast<B6Bitmap *>(rotatedFrame);
    unsigned centre = (static_cast<unsigned>(bmp->h) >> 1) * bmp->w + (static_cast<unsigned>(bmp->w) >> 1);
    if (saved == 0) {
        unk_200bc = static_cast<int16_t>(bmp->px[centre]);
        bmp->px[centre] = (g_is565 != 0) ? 0xffff : 0x7fff;
        return;
    }
    bmp->px[centre] = saved;
    unk_200bc = 0;
}
HOOK_METHOD(0x0046adf0, CRadarGump, BlinkPlayerPixel);

// 0x0046aec0  size 0xa6  CRadarGump::Draw, RET 4
void __thiscall CRadarGump::Draw(int *clip)
{
    int sx = 0;
    int sy = 0;
    int rect[4];
    LocalToScreen(&sx, &sy);
    B6Bitmap *bmp = (unk_60 == 0) ? reinterpret_cast<B6Bitmap *>(rotatedFrame) : b6_frame_bitmap(this, 1);
    rect[0] = sx;
    rect[1] = sy;
    rect[2] = sx + bmp->w;
    rect[3] = sy + bmp->h;
    if (rg_RectIntersects(clip, rect) != 0)
        GFX_BlitFrame(gfx::artPlain(), bmp->px, clip, bmp->w, bmp->h, sx, sy, g_backBuffer, 0x280, 0x1e0);
}
HOOK_VMETHOD(0x0046aec0, CRadarGump, Draw);

// 0x0046af70  size 0x5c  CRadarGump::HitTest, RET 8
int __thiscall CRadarGump::HitTest(int px, int py)
{
    B6Bitmap *bmp = (unk_60 == 0) ? reinterpret_cast<B6Bitmap *>(rotatedFrame) : b6_frame_bitmap(this, 1);
    int w = bmp->w;
    int h = bmp->h;
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return bmp->px[w * py + px] != 0;
}
HOOK_VMETHOD(0x0046af70, CRadarGump, HitTest);

// 0x0046afd0  size 0x1f  CRadarGump::RedrawIfDue (was Update; a method named like the root slot 0x5c would override it)
void __thiscall CRadarGump::RedrawIfDue()
{
    if (GetTickCount() - lastRedrawTick >= 500)
        Redraw();
}
HOOK_METHOD(0x0046afd0, CRadarGump, RedrawIfDue);

// Own copies of bodies shared with other classes in the original (the linker folds them).
void __thiscall CRadarGump::RButtonUp(int px, int py)
{
    (void)px;
    (void)py;
    g_dragGump = nullptr;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}

void __thiscall CRadarGump::Tick()
{
    DWORD now = GetTickCount();
    UINT dct = GetDoubleClickTime();
    if (RAWF(int32_t, this, 0xa8) == 1 && (now - static_cast<DWORD>(RAWF(int32_t, this, 0xa4))) > dct) {
        RAWF(int32_t, this, 0xa8) = 0;
        if (RAWF(int32_t, this, 0x9c) != 0)
            RAWF(int32_t, this, 0x94) = 1;
    }
}

