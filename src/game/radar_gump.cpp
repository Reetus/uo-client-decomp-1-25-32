#include "radar_gump.h"

#include <windows.h>

#include <cstdlib>
#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "batch06_helpers.h"
#include "globals.h"
#include "crt.h"
#include "file_manager.h"
#include "gump.h"
#include "window_helpers_31_ports.h"
#include "vtables.h"

#define g_objectListHead GLOBAL_AS(int, g_list70Head)     // next object at +0x60
#define g_mapBlockCache GLOBAL_PTR(void, g_fileManager_storage)

// 0x00440480 (file, index) -> cached record; 0x004405d0 (fileA, fileB, index, &size, &tmp) -> record.
#define MapCache_Get(self, a1, a2) (reinterpret_cast<CFileManager *>(self)->GetRecordPtr(a1, a2))
#define MapCache_GetPair(self, a1, a2, a3, a4, a5) (reinterpret_cast<CFileManager *>(self)->GetRecord(a1, a2, a3, a4, (int *)(a5)))

// Shade `colour` by the current light level and pack it (the three passes of Redraw share this).
static uint16_t radar_shade(unsigned colour)
{
    int lvl = g_lightAmbient - g_playerLight;
    if (lvl < 0 || (g_player != nullptr && PORT_VSLOT0(g_player, 0x8c, int)() != 0))
        lvl = 0;
    unsigned pa, pb;
    if (g_is565 != 0) {
        pa = (GLOBAL_PTR(uint32_t, g_shadeTableR)[(colour >> 10) * 0x20 + lvl] | GLOBAL_PTR(uint32_t, g_shadeTableG)[((colour >> 5) & 0x1f) * 0x20 + lvl]) << 1;
        pb = GLOBAL_PTR(uint32_t, g_shadeTableB)[(colour & 0x1f) * 0x20 + lvl];
    } else {
        pa = GLOBAL_PTR(uint32_t, g_shadeTableR)[(colour >> 10) * 0x20 + lvl] | GLOBAL_PTR(uint32_t, g_shadeTableB)[(colour & 0x1f) * 0x20 + lvl];
        pb = GLOBAL_PTR(uint32_t, g_shadeTableG)[((colour >> 5) & 0x1f) * 0x20 + lvl];
    }
    return static_cast<uint16_t>((pa | pb) | 1);
}

// 0x00469eb0  size 0x35c  CRadarGump::CRadarGump(int x, int y, int artId)
// Builds the radar gump: frames, z-buffer, colour table from radarcol.mul, first Redraw.
CRadarGump::CRadarGump(int x_, int y_, int artId)
{
    g_radarDefaultTile = 0x421;
    if (g_is565 != 0)
        g_radarDefaultTile = 0x841;
    g_radarGump = this;
    name = const_cast<char *>("radar gump");
    dirty = 0;
    owner = nullptr;
    unk_a0 = 0;
    unk_9c = 0;
    y = y_;
    unk_98 = 0;
    unk_94 = 0;
    unk_a8 = 0;
    unk_a4 = 0;
    x = x_;
    unk_60 = 0;
    unk_50 = artId;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    b6_art_size(artId, &width, &height);
    rowStride = width;
    halfHeight = static_cast<int16_t>(height / 2);
    halfWidth = static_cast<int16_t>(width / 2);
    zBuffer = static_cast<uint8_t *>(b6_alloc(g_frameDataAllocator, halfHeight * halfWidth, "radar zbuf"));
    unk_24 = 0x28;
    unk_28 = 0x14;
    frameIndex = 0;
    frameCount = 2;
    frameData = static_cast<int32_t *>(b6_alloc(g_frameDataAllocator, 0x10, "framedata for CRadarGump"));
    memset(frameData, 0, frameCount * 8);
    void *frame0 = b6_alloc(g_frameDataAllocator, halfWidth * halfHeight * 2 + 4, "radar frame0");
    b6_set_frame_bitmap(this, 0, frame0);
    memset(b6_frame_bitmap(this, 0)->px, 0, halfWidth * halfHeight * 2);
    b6_frame_bitmap(this, 0)->w = static_cast<uint16_t>(halfWidth);
    b6_frame_bitmap(this, 0)->h = static_cast<uint16_t>(halfHeight);
    void *frame1 = b6_alloc(g_frameDataAllocator, unk_24 * unk_28 * 2 + 4, "radar frame1");
    b6_set_frame_bitmap(this, 1, frame1);
    memset(b6_frame_bitmap(this, 1)->px, 0, unk_24 * unk_28 * 2);
    b6_frame_bitmap(this, 1)->w = static_cast<uint16_t>(unk_24);
    b6_frame_bitmap(this, 1)->h = static_cast<uint16_t>(unk_28);
    B6Bitmap *rot = static_cast<B6Bitmap *>(b6_alloc(g_frameDataAllocator, width * height * 2 + 4, "radar rotated frame"));
    rotatedFrame = reinterpret_cast<uint16_t *>(rot);
    rot->w = static_cast<uint16_t>(width);
    rot->h = static_cast<uint16_t>(height);
    memset(rot->px, 0, width * height * 2);
    rot = reinterpret_cast<B6Bitmap *>(rotatedFrame);
    b6_draw_art(B6_ROWBLIT_COPY, unk_50, 0, 0, rot->px, rot->w, rot->h);
    memset(b6_frame_bitmap(this, 1)->px, 0x14, unk_24 * unk_28 * 2);
    unk_200be = 0;
    unk_200bc = 0;
    originX = static_cast<int16_t>(g_viewCenterX - halfWidth / 2);
    originY = static_cast<int16_t>(g_viewCenterY - halfHeight / 2);
    FILE *fp = fopen(g_app.ResolveDataPath(const_cast<char *>("radarcol.mul")), g_fopenReadMode);
    if (fp != nullptr) {
        fread(colorTable, 2, 0x10000, fp);
        fclose(fp);
    }
    lastRedrawTick = 0;
    Redraw();
}
HOOK_CTOR(0x00469eb0, CRadarGump, (int, int, int));

// 0x0046a240  size 0xe9  CRadarGump::~CRadarGump
// Frees the rotated frame, z-buffer and frame bitmaps, then the CGump destructor runs.
CRadarGump::~CRadarGump()
{
    g_radarGump = nullptr;
    if (rotatedFrame != nullptr) {
        b6_free(g_frameDataAllocator, rotatedFrame, "radar rotFrame");
        rotatedFrame = nullptr;
    }
    if (zBuffer != nullptr) {
        b6_free(g_frameDataAllocator, zBuffer, "radar zBuf");
        zBuffer = nullptr;
    }
    if (b6_frame_bitmap(this, 0) != nullptr) {
        b6_free(g_frameDataAllocator, b6_frame_bitmap(this, 0), "radar frame0 pBMP");
        b6_set_frame_bitmap(this, 0, nullptr);
    }
    if (b6_frame_bitmap(this, 1) != nullptr) {
        b6_free(g_frameDataAllocator, b6_frame_bitmap(this, 1), "radar frame1 pBMP");
        b6_set_frame_bitmap(this, 1, nullptr);
    }
    if (frameData != nullptr) {
        b6_free(g_frameDataAllocator, frameData, "radar framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x0046a240, CRadarGump);

// 0x0046a400  size 0xe4  CRadarGump mouse move / drag (ret 8)
// Starts a drag once the mouse moved more than 4 pixels from the press point, then moves the gump (clamped).
void __thiscall CRadarGump::MouseMove(int mx, int my)
{
    (g_app.*PORT_METHOD1(CApp, void, &SetCursorForRegion, int))(0);
    if (unk_9c != 0 && unk_a8 > 0) {
        if (abs(mx - unk_6c) > 4 || abs(my - unk_70) > 4) {
            unk_94 = 1;
            unk_a8 = 0;
        }
    }
    int nx = (mx - unk_6c) + x;
    int ny = (my - unk_70) + y;
    if (nx + width < 0xa0)
        nx = 0xa0 - width;
    if (ny + height < 0xa0)
        ny = 0xa0 - height;
    if (nx > 0x1e0)
        nx = 0x1e0;
    if (ny > 0x140)
        ny = 0x140;
    if (unk_9c != 0)
        this->CGump::Move(nx - x, ny - y, 1, 0, 1);
}
HOOK_VMETHOD(0x0046a400, CRadarGump, MouseMove);

// 0x0046a4f0  size 0x798  CRadarGump::Redraw
// Renders terrain, then statics, then the object list into frame0 and the z-buffer, stamps the tick, rotates.
void __thiscall CRadarGump::Redraw()
{
    int w = halfWidth;
    int h = halfHeight;
    int dungeon = (g_viewCenterX >= 0x1400) ? 1 : 0;
    int ox = g_viewCenterX - w / 2;
    int oy = g_viewCenterY - h / 2;
    int limX = ox + w;
    int limY = oy + h;
    uint8_t *zbuf = zBuffer;
    memset(zbuf, 0, h * w);
    memset(b6_frame_bitmap(this, 0)->px, 0, static_cast<int16_t>(w) * static_cast<int16_t>(h) * 2);
    uint8_t *z = zbuf;
    uint16_t *px = b6_frame_bitmap(this, 0)->px;
    int yy = oy;
    int row = 0;

    // ---- terrain pass ----
    if (h > 0) {
        do {
            int xx = ox;
            int col = 0;
            if (w > 0) {
                do {
                    unsigned tile;
                    int8_t tz;
                    if (dungeon != 0 && (xx < 0x1400 || xx >= 0x1800 || yy < 0 || yy > 0x1000)) {
                        tile = 0x8001;
                        tz = 0;
                    } else {
                        int bx = xx;
                        int by = yy;
                        if (dungeon == 0) {
                            if (xx < 0)
                                bx = xx + 0x1400;
                            if (bx >= 0x1400)
                                bx = bx - 0x1400;
                            if (yy < 0)
                                by = yy + 0x1000;
                            if (by >= 0x1000)
                                by = by - 0x1000;
                        }
                        uint8_t *blk = MapCache_Get(g_mapBlockCache, 0, (bx / 8) * g_mapBlocksY + by / 8);
                        int c = (xx & 7) + (yy & 7) * 8;
                        tile = *reinterpret_cast<uint16_t *>(blk + c * 3 + 4);
                        tz = static_cast<int8_t>(blk[(c + 2) * 3]);
                    }
                    unsigned colour;
                    if (tile < 0x4000 && tile != 2)
                        colour = colorTable[tile];
                    else
                        colour = static_cast<unsigned>(g_radarDefaultTile);
                    if (colour == 0x7fff)
                        colour = static_cast<uint16_t>(b6_make_color(b6_pixel_format, 0xf3, 0xf3, 0xf3));
                    *px = radar_shade(colour);
                    *z = static_cast<uint8_t>(tz + 0x80);
                    px++;
                    z++;
                    xx++;
                    col++;
                } while (col < halfWidth);
            }
            yy++;
            row++;
        } while (row < halfHeight);
    }

    // ---- statics pass ----
    int bx0 = ox >> 3;
    if (bx0 < 0)
        bx0 = 0;
    int by0 = oy >> 3;
    if (by0 < 0)
        by0 = 0;
    int bx1 = (halfWidth >> 3) + bx0;
    if (bx1 > g_mapBlocksX - 1)
        bx1 = g_mapBlocksX - 1;
    int by1 = (halfHeight >> 3) + by0;
    if (by1 > g_mapBlocksY - 1)
        by1 = g_mapBlocksY - 1;
    int bY = by0;
    while (bY <= by1) {
        int bX = bx0;
        while (bX <= bx1) {
            if (((bX * 8 >= 0x1400) ? 1 : 0) == dungeon) {
                int cnt;
                char tmp[4];
                uint8_t *s = MapCache_GetPair(g_mapBlockCache, 1, 2, bX * g_mapBlocksY + bY, &cnt, tmp);
                if (cnt != 0 && cnt > 0) {
                    int xbase = bX * 8;
                    int ybase = bY * 8;
                    do {
                        int sy = static_cast<int8_t>(s[3]) + ybase;
                        int sx = static_cast<int8_t>(s[2]) + xbase;
                        unsigned tile = *reinterpret_cast<uint16_t *>(s);
                        unsigned hue = *reinterpret_cast<uint16_t *>(s + 5);
                        int sz = static_cast<int8_t>(s[4]);
                        if (hue > 0x400)
                            hue = 0;
                        s += 7;
                        if (sx >= ox && sx < limX && sy >= oy && sy < limY) {
                            int off = (sy - oy) * halfWidth - ox + sx;
                            uint8_t *zp = zBuffer + off;
                            if (sz + 0x80 >= static_cast<int>(*zp)) {
                                *zp = static_cast<uint8_t>(sz + 0x80);
                                unsigned colour;
                                if (hue == 0)
                                    colour = colorTable[0x4000 + tile];
                                else
                                    colour = g_hueColors[colorTable[0x4000 + tile] + hue * 0x2c];
                                if (colour == 0x7fff)
                                    colour = static_cast<uint16_t>(b6_make_color(b6_pixel_format, 0xf3, 0xf3, 0xf3));
                                b6_frame_bitmap(this, 0)->px[off] = radar_shade(colour);
                            }
                        }
                        cnt -= 7;
                    } while (cnt > 0);
                }
            }
            bX++;
        }
        bY++;
    }

    // ---- object list pass ----
    int obj = g_objectListHead;
    while (obj != 0) {
        unsigned tile = *reinterpret_cast<uint16_t *>(obj + 0x38);
        int ex = *reinterpret_cast<int16_t *>(obj + 0x20);
        int ey = *reinterpret_cast<int16_t *>(obj + 0x22);
        int ez = *reinterpret_cast<int16_t *>(obj + 0x24);
        unsigned hue = *reinterpret_cast<uint16_t *>(obj + 0x3e);
        if (ex >= ox && ex < limX && ey >= oy && ey < limY) {
            int off = (ey - oy) * halfWidth - ox + ex;
            uint8_t *zp = zBuffer + off;
            if (ez + 0x80 >= static_cast<int>(*zp)) {
                *zp = static_cast<uint8_t>(ez + 0x80);
                unsigned colour;
                if (hue == 0)
                    colour = colorTable[0x4000 + tile];
                else
                    colour = g_hueColors[colorTable[0x4000 + tile] + hue * 0x2c];
                if (colour == 0x7fff)
                    colour = static_cast<uint16_t>(b6_make_color(b6_pixel_format, 0xf3, 0xf3, 0xf3));
                b6_frame_bitmap(this, 0)->px[off] = radar_shade(colour);
            }
        }
        obj = *reinterpret_cast<int *>(obj + 0x60);
    }
    lastRedrawTick = GetTickCount();
    this->CRadarGump::RotateFrame();   // rotate frame
}
HOOK_METHOD(0x0046a4f0, CRadarGump, Redraw);
