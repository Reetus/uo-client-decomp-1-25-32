#include "gump_frame.h"

#include <cstdlib>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "app_support_ports.h"

// 0x0044b650: rand() % modulus. ECX = modulus, plain RET. The original CRT rand state is used.
#define Orig_RandMod(a1) (App_RandomMod(a1))

// 0x0041f540  size 0x8b0
// Piece layout: base+0 TL, +1 top edge, +2 TR, +3 left edge, +4 centre, +5 right edge, +6 BL, +7 bottom edge, +8 BR.
// Ornaments (flag == 0 only): +0xb/+0xc on the top edge (y = 0), +0x11/+0x12 on the bottom edge (y = H - h),
// +0xd/+0xe on the left edge (x = 0), +0xf/+0x10 on the right edge (x = W - w).
void __fastcall PaintGumpFrame(int baseArtId, uint16_t *bmp, int flag)
{
    int W = bmp[0];
    int H = bmp[1];
    uint16_t *pix = bmp + 2;
    int clip[4];
    int d;
    int w0, w1, w2, h0, h2, w3, h1, w5, h5, h3, h4, w4, w6, w7, w8, h6, h7, h8;
    int h7b, bottom, right, x, y;
    int wa, ha, wb, hb;
    int xa, xb;
    void *const art = gfx::artPlain();
    void *const art2 = gfx::artAlt();

    gfx::ArtSize(baseArtId, &w0, &d);
    gfx::ArtSize(baseArtId + 2, &w2, &d);
    right = W - w2;
    gfx::DrawArt(art, baseArtId, 0, 0, pix, bmp[0], bmp[1]);   // top-left corner
    gfx::ArtSize(baseArtId + 1, &w1, &d);
    if (w1 == 0)
        return;

    // top edge
    clip[0] = w0; clip[1] = 0; clip[2] = right; clip[3] = H;
    x = w0;
    if (w0 < right) {
        do {
            if (right < w1 + x)
                gfx::DrawArtClipped(art, baseArtId + 1, clip, x, 0, pix, bmp[0], bmp[1]);
            else
                gfx::DrawArt(art, baseArtId + 1, x, 0, pix, bmp[0], bmp[1]);
            x = x + w1;
        } while (x < right);
    }
    gfx::DrawArt(art, baseArtId + 2, right, 0, pix, bmp[0], bmp[1]);   // top-right corner

    // left edge
    gfx::ArtSize(baseArtId, &d, &h0);
    gfx::ArtSize(baseArtId + 6, &d, &h6);
    bottom = H - h6;
    gfx::ArtSize(baseArtId + 3, &d, &h3);
    clip[0] = 0; clip[1] = h0; clip[2] = W; clip[3] = bottom;
    y = h0;
    if (h0 < bottom) {
        do {
            if (bottom < h3 + y)
                gfx::DrawArtClipped(art, baseArtId + 3, clip, 0, y, pix, bmp[0], bmp[1]);
            else
                gfx::DrawArt(art, baseArtId + 3, 0, y, pix, bmp[0], bmp[1]);
            y = y + h3;
        } while (y < bottom);
    }
    gfx::DrawArt(art, baseArtId + 6, 0, bottom, pix, bmp[0], bmp[1]);   // bottom-left corner

    // right edge
    gfx::ArtSize(baseArtId + 2, &d, &h2);
    gfx::ArtSize(baseArtId + 8, &d, &h8);
    bottom = H - h8;
    gfx::ArtSize(baseArtId + 5, &w5, &h5);
    clip[0] = 0; clip[1] = h2; clip[2] = W; clip[3] = bottom;
    y = h2;
    if (h2 < bottom) {
        do {
            if (bottom < h5 + y)
                gfx::DrawArtClipped(art, baseArtId + 5, clip, W - w5, y, pix, bmp[0], bmp[1]);
            else
                gfx::DrawArt(art, baseArtId + 5, W - w5, y, pix, bmp[0], bmp[1]);
            y = y + h5;
        } while (y < bottom);
    }
    gfx::ArtSize(baseArtId + 8, &w8, &d);
    gfx::DrawArt(art, baseArtId + 8, W - w8, bottom, pix, bmp[0], bmp[1]);   // bottom-right corner

    // bottom edge
    gfx::ArtSize(baseArtId + 6, &w6, &d);
    gfx::ArtSize(baseArtId + 8, &w8, &d);
    right = W - w8;
    gfx::ArtSize(baseArtId + 7, &w7, &h7);
    clip[0] = w6; clip[1] = 0; clip[2] = right; clip[3] = H;
    x = w6;
    if (w6 < right) {
        do {
            if (right < w7 + x)
                gfx::DrawArtClipped(art, baseArtId + 7, clip, x, H - h7, pix, bmp[0], bmp[1]);
            else
                gfx::DrawArt(art, baseArtId + 7, x, H - h7, pix, bmp[0], bmp[1]);
            x = x + w7;
        } while (x < right);
    }

    // centre fill
    gfx::ArtSize(baseArtId + 1, &d, &h1);
    gfx::ArtSize(baseArtId + 7, &d, &h7b);
    h7b = H - h7b;
    gfx::ArtSize(baseArtId + 3, &w3, &d);
    gfx::ArtSize(baseArtId + 5, &w5, &d);
    right = W - w5;
    gfx::ArtSize(baseArtId + 4, &w4, &h4);
    clip[0] = w3; clip[1] = h1; clip[2] = right; clip[3] = h7b;
    y = h1;
    if (h1 < h7b) {
        do {
            x = w3;
            if (x < right) {
                do {
                    if (right < w4 + x || h7b <= y + h4)
                        gfx::DrawArtClipped(art, baseArtId + 4, clip, x, y, pix, bmp[0], bmp[1]);
                    else
                        gfx::DrawArt(art, baseArtId + 4, x, y, pix, bmp[0], bmp[1]);
                    x = x + w4;
                } while (x < right);
            }
            y = y + h4;
        } while (y < h7b);
    }

    if (flag != 0)
        return;

    // ornaments on the top edge (pieces 0xb, 0xc)
    gfx::ArtSize(baseArtId + 0xb, &wa, &ha);
    gfx::ArtSize(baseArtId + 0xc, &wb, &hb);
    if (wa != 0 && wb != 0 && W > wb + wa + 0x50) {
        xa = Orig_RandMod(W - wa - 0x14) + 10;
        xb = Orig_RandMod(W - wb - 0x14) + 10;
        if (abs(xa - xb) < wa + 10) {
            do {
                xb = Orig_RandMod(W - wb - 0x14) + 10;
            } while (abs(xa - xb) < wa + 10);
        }
        gfx::DrawArt(art2, baseArtId + 0xb, xa, 0, pix, W, H);
        gfx::DrawArt(art2, baseArtId + 0xc, xb, 0, pix, W, H);
    }

    // ornaments on the bottom edge (pieces 0x11, 0x12)
    gfx::ArtSize(baseArtId + 0x11, &wa, &ha);
    gfx::ArtSize(baseArtId + 0x12, &wb, &hb);
    if (wa != 0 && wb != 0 && W > wb + wa + 0x50) {
        xa = Orig_RandMod(W - wa - 0x14) + 10;
        xb = Orig_RandMod(W - wb - 0x14) + 10;
        if (abs(xa - xb) < wa + 10) {
            do {
                xb = Orig_RandMod(W - wb - 0x14) + 10;
            } while (abs(xa - xb) < wa + 10);
        }
        gfx::DrawArt(art2, baseArtId + 0x11, xa, H - ha, pix, W, H);
        gfx::DrawArt(art2, baseArtId + 0x12, xb, H - hb, pix, W, H);
    }

    // ornaments on the left edge (pieces 0xd, 0xe); vertical placement, heights tested
    gfx::ArtSize(baseArtId + 0xd, &wa, &ha);
    gfx::ArtSize(baseArtId + 0xe, &wb, &hb);
    if (wa != 0 && wb != 0 && H > ha + hb + 0x50) {
        xa = Orig_RandMod(H - ha - 0x14) + 10;   // here xa / xb are y positions
        xb = Orig_RandMod(H - hb - 0x14) + 10;
        if (abs(xa - xb) < ha + 10) {
            do {
                xb = Orig_RandMod(H - hb - 0x14) + 10;
            } while (abs(xa - xb) < ha + 10);
        }
        gfx::DrawArt(art2, baseArtId + 0xd, 0, xa, pix, W, H);
        gfx::DrawArt(art2, baseArtId + 0xe, 0, xb, pix, W, H);
    }

    // ornaments on the right edge (pieces 0xf, 0x10)
    gfx::ArtSize(baseArtId + 0xf, &wa, &ha);
    gfx::ArtSize(baseArtId + 0x10, &wb, &hb);
    if (wa != 0 && wb != 0 && H > ha + hb + 0x50) {
        xa = Orig_RandMod(H - ha - 0x14) + 10;
        xb = Orig_RandMod(H - hb - 0x14) + 10;
        if (abs(xa - xb) < ha + 10) {
            do {
                xb = Orig_RandMod(H - hb - 0x14) + 10;
            } while (abs(xa - xb) < ha + 10);
        }
        gfx::DrawArt(art2, baseArtId + 0xf, W - wa, xa, pix, W, H);
        gfx::DrawArt(art2, baseArtId + 0x10, W - wb, xb, pix, W, H);
    }
}
HOOK_FUNC(0x0041f540, PaintGumpFrame);
