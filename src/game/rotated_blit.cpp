// Rotated sprite blits, sin / cos tables and the war-mode packet sender (0x00430df0 .. 0x00431480).
#include "rotated_blit.h"

#include <cmath>

#include "../runtime/ported.h"
#include "globals.h"
#include "obj.h"
#include "raw_field.h"
#include "art_record_ops_ports.h"
#include "packet_builders_b25_ports.h"
#include "client_socket.h"

#define g_artCache GLOBAL_PTR(void, g_artCache_storage)
#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)

// 0x00476160: ECX = art cache, stack (sprite, x, y), RET 0xc; the pixel (0 = transparent), in AX.
#define ArtPixel(o, a1, a2, a3) PORT_THISCALL3(&ArtRecord_PixelAt, uint16_t, (o), const uint8_t *, int, int)(a1, a2, a3)
// 0x00464be0: ECX = 0x0069e0e0; stack (buffer, mode byte, a, b, c), RET 0x14: builds packet 0x72.
#define BuildWarModePacket(o, a1, a2, a3, a4, a5) PORT_THISCALL5(&Pkt_Build_0x72_WarMode, void, (o), char *, uint8_t, int, int, int)(a1, a2, a3, a4, a5)
// 0x00478610: ECX = *0x0069e0ec, stack = packet, RET 4.
#define SendPacket(self, a1) (reinterpret_cast<CClientSocket *>(self)->SendPacket((const uint8_t *)(a1)))

// 0x00430df0  size 0x55
// cosTable[i] = (int)(cos(i * step) * 65536), sinTable[i] likewise, for i in 0..255.
void __cdecl InitSinCosTables()
{
    for (int i = 0; i < 0x100; i++) {
        double angle = i * g_sinCosStep;
        g_cosTable[i] = static_cast<int>(std::cos(angle) * g_sinCosScale);   // __ftol, 0x0048c0d0
        g_sinTable[i] = static_cast<int>(std::sin(angle) * g_sinCosScale);   // __ftol, 0x0048c0d0
    }
}
HOOK_FUNC(0x00430df0, InitSinCosTables);

// 0x00430e50  size 0x1e4
// Sprite layout: u16 width at +4, u16 height at +6, u16 row offsets (in words, from the end of
// the offset table) at +8; each row is {skip, run, run pixels} segments ending at a run of 0.
// For every destination pixel of the clipped rect the source pixel is found by rotating
// (cos / sin in 16.16) about the sprite centre and looked up in the row's segments.
void __fastcall BlitRotatedRLESprite(const uint8_t *sprite, const int *clip, int destX, int destY, int angle)
{
    int w = *reinterpret_cast<const uint16_t *>(sprite + 4);
    int h = *reinterpret_cast<const uint16_t *>(sprite + 6);
    int colStart;
    int colEnd;
    int rowStart;
    int rowEnd;

    if (clip[0] > destX)
        colStart = clip[0] - destX;
    else
        colStart = 0;
    if (clip[2] < destX + w)
        colEnd = clip[2] - destX;
    else
        colEnd = w;
    if (colStart >= colEnd)
        return;

    if (clip[1] > destY)
        rowStart = clip[1] - destY;
    else
        rowStart = 0;
    if (clip[3] < destY + h)
        rowEnd = clip[3] - destY;
    else
        rowEnd = h;

    int halfW = w >> 1;
    int halfH = h >> 1;
    const uint8_t *rle = sprite + h * 2 + 8;
    uint16_t *dst = g_backBuffer + (destY + rowStart) * 640 + colStart + destX;
    int sinv = g_sinTable[angle & 0xff];
    int cosv = g_cosTable[angle & 0xff];

    if (rowStart >= rowEnd)
        return;
    int ry = sinv * rowStart;
    int rc = cosv * rowStart;
    int rows = rowEnd - rowStart;

    do {
        uint16_t *rowDst = dst;
        int sx = cosv * colStart;
        int sy = sinv * colStart + rc;
        int cols = colEnd - colStart;
        do {
            int u = ((sx - ry) >> 16) + halfW;
            int v = halfH - (sy >> 16);
            if (u >= 0 && u < w && v >= 0 && v < h) {
                int rowOffset = *reinterpret_cast<const uint16_t *>(sprite + 8 + v * 2);
                const uint16_t *seg = reinterpret_cast<const uint16_t *>(rle + rowOffset * 2);
                if (seg[1] != 0) {
                    for (;;) {
                        u -= seg[0];
                        if (u < 0)
                            break;
                        int run = seg[1];
                        if (u < run) {
                            *dst = seg[2 + u];
                            break;
                        }
                        u -= run;
                        seg = seg + 2 + run;
                        if (seg[1] == 0)
                            break;
                    }
                }
            }
            dst++;
            sx += cosv;
            sy += sinv;
            cols--;
        } while (cols != 0);
        dst = rowDst + 640;
        rc += cosv;
        ry += sinv;
        rows--;
    } while (rows != 0);
}
HOOK_FUNC(0x00430e50, BlitRotatedRLESprite);

// 0x00431040  size 0x212
// rect = {left, top, right, bottom}. Each corner (dx, dy) relative to the centre maps to
// x = ((cos * dx - sin * dy) >> 16) + cx, y = cy - ((cos * dy + sin * dx) >> 16); the rect becomes
// the min / max over the four corners (starting from 9999 / -9999), in the order top-left,
// top-right, bottom-left, bottom-right.
void __fastcall RotateRectBounds(int angle, int *rect)
{
    int left = rect[0];
    int top = rect[1];
    int right = rect[2];
    int bottom = rect[3];
    int cx = left + ((right - left) >> 1);
    int cy = top + ((bottom - top) >> 1);
    rect[1] = 9999;
    rect[0] = 9999;
    rect[3] = -9999;
    rect[2] = -9999;

    int dxl = left - cx;
    int dyt = top - cy;
    int dxr = right - cx;
    int dyb = bottom - cy;
    int sinv, cosv, px, py;

    sinv = g_sinTable[angle];
    cosv = g_cosTable[angle];
    px = ((cosv * dxl - sinv * dyt) >> 16) + cx;
    py = cy - ((cosv * dyt + sinv * dxl) >> 16);
    if (px < 9999)
        rect[0] = px;
    if (px > -9999)
        rect[2] = px;
    if (py < 9999)
        rect[1] = py;
    if (py > -9999)
        rect[3] = py;

    sinv = g_sinTable[angle];
    cosv = g_cosTable[angle];
    px = ((cosv * dxr - sinv * dyt) >> 16) + cx;
    py = cy - ((cosv * dyt + sinv * dxr) >> 16);
    if (px < rect[0])
        rect[0] = px;
    if (px > rect[2])
        rect[2] = px;
    if (py < rect[1])
        rect[1] = py;
    if (py > rect[3])
        rect[3] = py;

    sinv = g_sinTable[angle];
    cosv = g_cosTable[angle];
    px = ((cosv * dxl - sinv * dyb) >> 16) + cx;
    py = cy - ((sinv * dxl + cosv * dyb) >> 16);
    if (px < rect[0])
        rect[0] = px;
    if (px > rect[2])
        rect[2] = px;
    if (py < rect[1])
        rect[1] = py;
    if (py > rect[3])
        rect[3] = py;

    sinv = g_sinTable[angle];
    cosv = g_cosTable[angle];
    px = ((cosv * dxr - sinv * dyb) >> 16) + cx;
    py = cy - ((cosv * dyb + sinv * dxr) >> 16);
    if (px < rect[0])
        rect[0] = px;
    if (px > rect[2])
        rect[2] = px;
    if (py < rect[1])
        rect[1] = py;
    if (py > rect[3])
        rect[3] = py;
}
HOOK_FUNC(0x00431040, RotateRectBounds);

// 0x00431260  size 0x21f
// Rotated copy of the part `src` of a sprite to (destX, destY), clipped to `clip`, sampling
// each source pixel through the art cache and skipping 0 (transparent).
// ? The right-hand clip tests (width after the left clip) + destX against clip[2], as the
// assembly does, so a sprite clipped on the left is cut too late on the right.
void __fastcall BlitRotatedSprite(const uint8_t *sprite, const int *src, int destX, int destY, uint8_t angle, const int *clip)
{
    int w = *reinterpret_cast<const uint16_t *>(sprite + 4);
    int h = *reinterpret_cast<const uint16_t *>(sprite + 6);
    int halfW = w >> 1;
    int halfH = h >> 1;
    int width = src[2] - src[0];
    int colStart = 0;
    if (destX < clip[0]) {
        colStart = clip[0] - destX;
        width -= colStart;
    }
    if (width + destX > clip[2])
        width = clip[2] - destX;

    int height = src[3] - src[1];
    int rowStart = 0;
    if (destY + height > clip[3])
        height = clip[3] - destY;
    if (destY < clip[1])
        rowStart = clip[1] - destY;

    uint16_t *dst = g_backBuffer + (rowStart + destY) * 640 + colStart + destX;
    int sinv = g_sinTable[angle & 0xff];
    int cosv = g_cosTable[angle & 0xff];
    int row = rowStart + src[1];
    if (row >= height + src[1])
        return;

    int endX = width + colStart;
    int ry = sinv * (halfH - row);
    int rc = cosv * (halfH - row);
    do {
        uint16_t *rowDst = dst;
        int col = colStart + src[0];
        if (col < endX + src[0]) {
            int colSin = col * sinv;
            int colCos = col * cosv;
            int sinHalfW = sinv * halfW;
            int cosHalfW = cosv * halfW;
            do {
                int u = ((colCos - cosHalfW - ry) >> 16) + halfW;
                if (u >= 0 && u < w) {
                    int v = halfH - ((rc + (colSin - sinHalfW)) >> 16);
                    if (v >= 0 && v < h) {
                        uint16_t px = ArtPixel(g_artCache, sprite, u, v);   // 0x00476160
                        if (px != 0)
                            *dst = px;
                    }
                }
                dst++;
                colCos += cosv;
                colSin += sinv;
                col++;
            } while (col < endX + src[0]);
        }
        dst = rowDst + 640;
        row++;
        rc += -cosv;
        ry += -sinv;
    } while (row < height + src[1]);
}
HOOK_FUNC(0x00431260, BlitRotatedSprite);

// 0x00431480  size 0x3c
// Build the 0x72 packet from the player's byte at +0x150 (merged into the low byte of EDX) and
// the three incoming values, then send it.
// ? Name and meaning of the three arguments are not known.
void __fastcall SendWarModePacket(int ecxArg, int edxArg, int stackArg)
{
    char packet[8];
    BuildWarModePacket(g_packetBuilder, packet, RAWF(uint8_t, g_player, 0x150), ecxArg, edxArg, stackArg);   // 0x00464be0
    SendPacket(g_pktSender, packet);   // 0x00478610
}
HOOK_FUNC(0x00431480, SendWarModePacket);
