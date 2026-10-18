// CCourseGump.
#include "course_gump.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "batch02_helpers.h"
#include "globals.h"
#include "crt.h"
#include "packet_builders_batch24_ports.h"
#include "client_socket.h"
#include "obj.h"
#include "gump_frame.h"
#include "mapped_file.h"
#include "bitmap16_draw_ports.h"

#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)          // this for the packet builders
// 0x00463e40: build the map-pin packet into pkt (map serial, action, pin index, x, y).
#define Packet_MapPin(o, a1, a2, a3, a4, a5, a6) PORT_THISCALL6(&Build_Packet_0x56, void, (o), char *, int, int, int, int, int)(a1, a2, a3, a4, a5, a6)
// 0x00478610: send a built packet.
#define Net_Send(self, a1) (reinterpret_cast<CClientSocket *>(self)->SendPacket((const uint8_t *)(a1)))
// 0x00453c60: apply the pin change to the map item (action, index, x, y). ECX = owner.
#define MapItem_ApplyPin(self, a1, a2, a3, a4) (reinterpret_cast<CObjVt49f960 *>(self)->ListOp(a1, a2, a3, a4))
// 0x0041f540: draw a 9-slice frame (art id in ECX) into a bitmap (EDX); stack: flag.
#define DrawFrameArt(a1, a2, a3) (PaintGumpFrame(a1, (uint16_t *)(a2), a3))
// multimap.rle file object (0x90 bytes): new MappedFile(path), data(), close(), delete.

#define Rle_Data(self) ((int *)(self)->OpenReadOnly())
#define Rle_Close(self) ((self)->Close())
// 0x00432cf0: draw a 16-bit line into a bitmap (x0, y0, x1, y1, colour, bitmap).
#define FontDrawLine(o, a1, a2, a3, a4, a5, a6) PORT_THISCALL6(&DrawLine16, void, (o), int, int, int, int, int, void *)(a1, a2, a3, a4, a5, a6)

// 0x00427dd0  size 0x1f9
void __thiscall CCourseGump::AddPin(int px, int py)
{
    char pkt[0x40];               // pass2: 0x20 bytes of locals; enlarged, the builder output size is not known  // ?
    int best = 10;
    int mx = 0;
    int my = 0;
    int bestIdx;
    int idx;
    void *map = owner;

    if (RAW_AT(int32_t, map, 0xac) >= 0x32)          // signed: 50 pins max
        return;
    if (plotEnabled == 0)
        return;

    MapPin *a = RAW_AT(MapPin *, map, 0xa8);
    bestIdx = -1;
    idx = 1;
    if (a != nullptr) {
        MapPin *b;
        while ((b = a->next) != nullptr) {
            int ax = a->x;
            int bx = b->x;
            int ay = a->y;
            int by = b->y;
            int dx = bx - ax;
            int dy = by - ay;
            int len = static_cast<int>(std::sqrt(static_cast<double>(dx * dx + dy * dy)));     // FILD / FSQRT / __ftol
            int d, e1, e2;
            int t;

            // no guard against len == 0: IDIV by len as in the binary
            t = ((ay - py) * dx - (ax - px) * dy) / len;
            d = t < 0 ? -t : t;
            t = ((by - py) * dy - (bx - px) * (ax - bx)) / len;
            e1 = t < 0 ? -t : t;
            t = ((ay - by) * (ay - py) - (ax - px) * dx) / len;
            e2 = t < 0 ? -t : t;

            if (e2 < len && e1 < len && d < best) {
                best = d;
                bestIdx = idx;
                mx = (bx + ax) / 2;                      // round toward zero
                my = (by + ay) / 2;
            }
            idx = idx + 1;
            a = b;
        }
    }

    if (bestIdx == -1) {
        Packet_MapPin(g_packetBuilder, pkt, RAW_AT(int32_t, owner, 0x78), 1, 0, px, py);
        Net_Send(g_pktSender, pkt);
        MapItem_ApplyPin(owner, 1, 0, px, py);
    } else {
        Packet_MapPin(g_packetBuilder, pkt, RAW_AT(int32_t, owner, 0x78), 2, bestIdx, mx, my);
        Net_Send(g_pktSender, pkt);
        MapItem_ApplyPin(owner, 2, bestIdx, mx, my);
    }
    Redraw();
}
HOOK_METHOD(0x00427dd0, CCourseGump, AddPin);

// 0x00428a90  size 0x5bc
void __thiscall CCourseGump::Redraw()
{
    int artW, artH;
    int rect[4];
    char buf[0x10];
    int w2, h2;
    int idx;
    MapPin *pin;

    unk_c0 = 0;
    if (unk_50 == 0x139d) {
        // ---- map art: density image from multimap.rle ----
        unsigned char *density;
        unsigned char *dcur;
        MappedFile *rle;
        int *hdr;
        unsigned char *rp;
        int imgW, imgH, total;
        int x0, y0, x1, y1;
        int sx, sy;
        int col, row, done;
        int maxLevel;
        int run;
        unsigned char b;
        int cnt;
        int rowBase, y0Scaled;
        int pal[0x100];
        uint16_t *dst;
        int ix;
        int tmp;

        DrawFrameArt(0x1432, i32_ptr(frameData[1]), 0);

        density = static_cast<unsigned char *>(::operator new(static_cast<size_t>(mapWidth * mapHeight)));
        dcur = density;
        memset(density, 0, static_cast<size_t>(mapWidth * mapHeight));

        rle = new MappedFile(g_app.ResolveDataPath(const_cast<char *>("multimap.rle")));
        hdr = Rle_Data(rle);
        imgH = hdr[1];
        imgW = hdr[0];
        total = imgH * imgW;
        rp = reinterpret_cast<unsigned char *>(hdr + 2);

        tmp = regionX0 * imgW;
        x0 = static_cast<int>((static_cast<__int64>(tmp) * 0x66666667i64) >> 32) >> 11;      // IMUL / SAR 0xb
        x0 = x0 + static_cast<int>(static_cast<unsigned int>(x0) >> 31);                      // = tmp / 0x1400
        tmp = regionY0 * imgH;
        y0 = (tmp + ((tmp >> 31) & 0xfff)) >> 12;                                             // = tmp / 0x1000
        done = 0;
        tmp = regionX1 * imgW;
        x1 = static_cast<int>((static_cast<__int64>(tmp) * 0x66666667i64) >> 32) >> 11;
        x1 = x1 + static_cast<int>(static_cast<unsigned int>(x1) >> 31);
        row = 0;
        tmp = regionY1 * imgH;
        y1 = (tmp + ((tmp >> 31) & 0xfff)) >> 12;

        tmp = x1 - x0;
        if (tmp == 0)
            tmp = 1;
        sx = (mapWidth << 8) / tmp;
        tmp = y1 - y0;
        if (tmp == 0)
            tmp = 1;
        sy = (mapHeight << 8) / tmp;

        col = 0;
        maxLevel = 1;
        if (total > 0) {
            do {
                b = *rp;
                rp = rp + 1;
                run = b & 0x7f;
                if ((b & 0x80) == 0) {
                    col = col + run;
                    if (col >= imgW) {
                        col = col - imgW;
                        row = row + 1;
                    }
                } else if (run > 0) {
                    cnt = run;
                    rowBase = sy * row;
                    y0Scaled = sy * y0;
                    do {
                        if (col >= x0 && col < x1 && row >= y0 && row < y1) {
                            ix = (((rowBase - y0Scaled) >> 8) * mapWidth) + (((col - x0) * sx) >> 8);
                            {
                                unsigned char c = density[ix];
                                if (static_cast<int>(c) < 0xff) {
                                    density[ix] = static_cast<unsigned char>(c + 1);
                                    if (static_cast<int>(c) == maxLevel)
                                        maxLevel = maxLevel + 1;
                                }
                            }
                        }
                        col = col + 1;
                        if (col == imgW) {
                            col = 0;
                            row = row + 1;
                            rowBase = rowBase + sy;
                        }
                        cnt = cnt - 1;
                    } while (cnt != 0);
                }
                done = done + run;
            } while (done < total);
        }

        if (maxLevel >= 1) {
            int num = maxLevel * 31 - 31;
            for (int k = 0; k < maxLevel; k++) {
                int e = num / maxLevel;
                num = num - 31;
                pal[k] = g_mapPalette[e];
            }
        }
        if (g_is565 != 0) {
            if (maxLevel > 1) {
                for (int k = 0; k < maxLevel - 1; k++) {
                    int v = pal[k];
                    pal[k] = ((v & 0x7fe0) << 1) | (v & 0x1f);          // 555 -> 565
                }
            }
        }

        {
            uint16_t *bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
            // 31 rows + 24 pixels + the 4-byte header
            dst = reinterpret_cast<uint16_t *>(reinterpret_cast<char *>(bmp) + static_cast<unsigned int>(bmp[0]) * 31 * 2 + 0x34);
            for (int j = 0; j < mapHeight; j++) {
                for (int i = 0; i < mapWidth; i++) {
                    unsigned char c = *dcur;
                    if (c != 0)
                        *dst = static_cast<uint16_t>(pal[c - 1]);
                    dcur = dcur + 1;
                    dst = dst + 1;
                }
                dst = dst + 0x2c;
            }
        }

        operator delete(density);
        Rle_Close(rle);
        delete rle;
    } else {
        uint16_t *bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
        DrawGumpArt(ROWBLIT_COPY, unk_50, 0, 0, bmp + 2, bmp[0], bmp[1]);
    }

    // ---- pins ----
    GumpArtSize(0x139c, &artW, &artH);        // pin sprite size
    artW = artW >> 1;
    artH = artH >> 1;
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = width;
    rect[3] = height;

    pin = RAW_AT(MapPin *, owner, 0xa8);
    if (pin != nullptr) {
        // pass 1: connecting lines
        while (pin->next != nullptr) {
            MapPin *nextPin = pin->next;
            if (pinSelected == 0 || pin != selectedPin)
                FontDrawLine(g_fontManager, pin->x, pin->y, nextPin->x, nextPin->y, 0x7fff, i32_ptr(frameData[1]));
            pin = pin->next;
        }

        // pass 2: pin sprites and index numbers
        idx = 1;
        pin = RAW_AT(MapPin *, owner, 0xa8);
        while (pin != nullptr) {
            uint16_t *bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
            int len;
            DrawGumpArt(ROWBLIT_COPY, 0x139c, pin->x - artW, pin->y - artH, bmp + 2, bmp[0], bmp[1]);
            sprintf(buf, "%d", idx);
            len = static_cast<int>(strlen(buf));
            FontDrawText(g_fontManager, pin->x - len * 16, pin->y - 5, 3, 0, buf,
                         static_cast<uint16_t *>(i32_ptr(frameData[1])) + 2, width, rect);
            idx = idx + 1;
            pin = pin->next;
        }
    }

    // ---- cursor / edit-pin sprite ----
    if (cursorActive != 0) {
        uint16_t *bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
        DrawGumpArt(ROWBLIT_ALT, 0x139c, cursorX - artW, cursorY - artH, bmp + 2, bmp[0], bmp[1]);
    }

    // ---- corner overlay (art 0x139d) ----
    GumpArtSize(0x139d, &w2, &h2);
    {
        uint16_t *bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
        DrawGumpArt(ROWBLIT_COPY, 0x139d, static_cast<int>(bmp[0]) - w2 - 0x1a, static_cast<int>(bmp[1]) - h2 - 0x22,
                    bmp + 2, bmp[0], bmp[1]);
    }
}
HOOK_METHOD(0x00428a90, CCourseGump, Redraw);
