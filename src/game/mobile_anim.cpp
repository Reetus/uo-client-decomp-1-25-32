#include "mobile_anim.h"

#include <cstdlib>

#include "../runtime/ported.h"
#include "globals.h"

#define g_animOverlayTable GLOBAL_PTR(AnimOverlayEntry, g_animOverlayTable_storage)
#define g_cellObjects GLOBAL_PTR(int *, g_tileBuckets)       // 64 x 64 heads of per-cell object chains (next at +0x14)
#define g_shadeShift GLOBAL_AS(uint8_t, g_redShift16)
#define g_shadeColors GLOBAL_PTR(uint16_t, g_hueTable_storage)    // [color + shade * 0x2c]

// One map entry: clamp (u, v) into the 64 x 128 sprite and pack as v * 0x40 + u.
static int map_entry(int u, int v)
{
    if (u < 0)
        u = 0;
    if (u >= 0x40)
        u = 0x3f;
    if (v < 0) {
        v = 0;
        u = 0;
    }
    if (v >= 0x80)
        v = 0x7f;
    return (v << 6) + u;
}

// 0x00470520  size 0x34c  builds eight (v*0x40+u) index maps (int[64 cols]) inside `this`, at +0x4008,+0xc008,+0xf808,+0x10808,+0x14008,+0x17808,+0x18808,+0x1c008
int __thiscall CAnimOverlayBuffer::BuildIndexMaps()
{
    int *p;
    int xx, yy;
    int cA;
    int cB;
    int rowC;
    int vb, ub;

    // table A @ +0x4008 : y = 0..0x7f, x = 0..0x3f, u = x-3, v = y-0x1a
    p = reinterpret_cast<int *>(reinterpret_cast<char *>(this) + 0x4008);
    for (yy = 0; yy < 0x80; yy++) {
        for (xx = 0; xx < 0x40; xx++) {
            *p = map_entry(xx - 3, yy - 0x1a);
            p++;
        }
    }

    // table B @ +0xc008 : y = 0..0x37, u = x, v = y-0xc
    p = reinterpret_cast<int *>(reinterpret_cast<char *>(this) + 0xc008);
    for (yy = 0; yy < 0x38; yy++) {
        for (xx = 0; xx < 0x40; xx++) {
            *p = map_entry(xx, yy - 0xc);
            p++;
        }
    }

    // table C @ +0xf808 : 16 rows; rowC = 0x1c0, 0x1c8, ... < 0x240; cA starts 0x38 (+1), cB starts 0xe0 (+4)
    cA = 0x38;
    cB = 0xe0;
    rowC = 0x1c0;
    p = reinterpret_cast<int *>(reinterpret_cast<char *>(this) + 0xf808);
    do {
        int a = rowC - 0x1c0;
        int b = cB - 0xe0;
        int ush = (a + ((a >> 31) & 0xf)) >> 4;
        vb = ((b + ((b >> 31) & 0xf)) >> 4) + cA - 0xc;
        for (xx = 0; xx < 0x40; xx++) {
            *p = map_entry(xx - ush, vb);
            p++;
        }
        rowC += 8;
        cA++;
        cB += 4;
    } while (rowC < 0x240);

    // table D @ +0x10808 : y = 0x48..0x7f, u = x-8, v = y-8
    p = reinterpret_cast<int *>(reinterpret_cast<char *>(this) + 0x10808);
    for (yy = 0x48; yy < 0x80; yy++) {
        for (xx = 0; xx < 0x40; xx++) {
            *p = map_entry(xx - 8, yy - 8);
            p++;
        }
    }

    // table E @ +0x14008 : y = 0..0x37, u = x, v = y-0xc
    p = reinterpret_cast<int *>(reinterpret_cast<char *>(this) + 0x14008);
    for (yy = 0; yy < 0x38; yy++) {
        for (xx = 0; xx < 0x40; xx++) {
            *p = map_entry(xx, yy - 0xc);
            p++;
        }
    }

    // table F @ +0x17808 : 16 rows; rowC = 0xe0, 0xe4, ... < 0x120; cA starts 0x38 (+1), cB starts 0x1c0 (+8)
    cA = 0x38;
    cB = 0x1c0;
    rowC = 0xe0;
    p = reinterpret_cast<int *>(reinterpret_cast<char *>(this) + 0x17808);
    do {
        int a = rowC - 0xe0;
        int b = cB - 0x1c0;
        vb = ((a + ((a >> 31) & 0xf)) >> 4) + cA - 0xc;
        ub = (b + ((b >> 31) & 0xf)) >> 4;
        for (xx = 0; xx < 0x40; xx++) {
            *p = map_entry(ub + xx, vb);
            p++;
        }
        rowC += 4;
        cA++;
        cB += 8;
    } while (rowC < 0x120);

    // table G @ +0x18808 : y = 0x48..0x7f, u = 8..0x47, v = y-8
    p = reinterpret_cast<int *>(reinterpret_cast<char *>(this) + 0x18808);
    for (yy = 0x48; yy < 0x80; yy++) {
        xx = 8;
        do {
            *p = map_entry(xx, yy - 8);
            p++;
            xx++;
        } while (xx - 8 < 0x40);
    }

    // table H @ +0x1c008 : y = 0..0x7f, u = 3..0x42, v = y-0x1a
    p = reinterpret_cast<int *>(reinterpret_cast<char *>(this) + 0x1c008);
    for (yy = 0; yy < 0x80; yy++) {
        xx = 3;
        do {
            *p = map_entry(xx, yy - 0x1a);
            p++;
            xx++;
        } while (xx - 3 < 0x40);
    }
    return static_cast<int>(reinterpret_cast<intptr_t>(this));
}
HOOK_METHOD(0x00470520, CAnimOverlayBuffer, BuildIndexMaps);

// 0x00470870  size 0x22f  draw a 64x128 lookup-mapped overlay (this+8 pixels via index tables at this+0x4008) onto the 640-wide back buffer, then redraw the matching map object clipped
// The search for the matching cell object: idx = (1-based position of the first table entry whose id matches and
// whose +4 is non-zero) - 1, or -1 when the object fails the vtable +0x18 / +0x24 checks or no entry matches.
void __thiscall CAnimOverlayBuffer::DrawOverlay(void *param_1)
{
    int row, col;
    uint16_t *dst;
    int16_t px;
    int *obj;
    int dz, idx, id;
    AnimOverlayEntry *entry;
    int rc[4];
    int half;
    int dir;
    char *prm = static_cast<char *>(param_1);

    if (direction != 0 && direction != 6) {
        y = y + g_animOverlayTable[tableIndex].offsetA;
    } else {
        y = y + g_animOverlayTable[tableIndex].offsetB;
    }
    dst = g_backBuffer + (y * 0x280 + x);

    for (row = 0; row < 0x80; row++) {
        if (row + y < clip[1] || row + y >= clip[3]) {
            dst = dst + 0x280;
        } else {
            for (col = 0; col < 0x40; col++) {
                if (col + x >= clip[0] && col + x < clip[2]) {
                    px = static_cast<int16_t>(pixels[maps[(((direction >> 1) << 7) + row) * 0x40 + col]]);
                    if (px != 0) {
                        *dst = static_cast<uint16_t>(px);
                    }
                }
                dst = dst + 1;
            }
            dst = dst + 0x240;
        }
    }

    if (direction != 0 && direction != 6) {
        return;
    }
    if (g_animOverlayTable[tableIndex].offsetC == 0) {
        return;
    }
    obj = g_cellObjects[((*reinterpret_cast<uint8_t *>(prm + 0x22) & 0x3f) << 6) + (*reinterpret_cast<uint8_t *>(prm + 0x20) & 0x3f)];
    if (obj == nullptr) {
        return;
    }
    do {
        char *o = reinterpret_cast<char *>(obj);
        dz = static_cast<int>(*reinterpret_cast<int16_t *>(o + 0x24)) - static_cast<int>(*reinterpret_cast<int16_t *>(prm + 0x24));
        if (dz < 0)
            dz = -dz;
        if (dz <= 1 && *reinterpret_cast<int16_t *>(o + 0x20) == *reinterpret_cast<int16_t *>(prm + 0x20) &&
            *reinterpret_cast<int16_t *>(o + 0x22) == *reinterpret_cast<int16_t *>(prm + 0x22)) {
            idx = 0;
            if (PORT_VSLOT0(obj, 0x18, int)() != 0 && PORT_VSLOT0(obj, 0x24, int)() == 0) {
                id = *reinterpret_cast<uint16_t *>(o + 0x38);   // stored over the parameter slot in the original
                idx = 0;
                entry = g_animOverlayTable;
                if (g_animOverlayTable[0].id != 0) {
                    int cur = g_animOverlayTable[0].id;
                    for (;;) {
                        if (cur == id && entry->enabled != 0) {
                            idx = idx + 1;   // found
                            goto have_idx;
                        }
                        cur = entry[1].id;
                        entry = entry + 1;
                        idx = idx + 1;
                        if (cur == 0) {
                            break;
                        }
                    }
                }
            }
            idx = 0;   // 0x470a0f: not found / rejected
        have_idx:
            idx = idx - 1;
            if (idx == tableIndex) {
                break;
            }
        }
        obj = reinterpret_cast<int *>(*reinterpret_cast<int32_t *>(o + 0x14));   // next in the cell chain
    } while (obj != nullptr);

    if (obj == nullptr) {
        return;
    }
    dir = direction;
    rc[0] = clip[0];
    rc[1] = clip[1];
    rc[2] = clip[2];
    rc[3] = clip[3];
    half = (obj[3] - obj[1]) >> 1;
    if (dir == 0) {
        rc[2] = (g_animOverlayTable[tableIndex].offsetC - half) - g_scrollX + obj[3];
    } else {
        rc[0] = (half - g_animOverlayTable[tableIndex].offsetC) - g_scrollX + obj[1];
    }
    PORT_VSLOT1(obj, 0x08, void, int *)(rc);
}
HOOK_METHOD(0x00470870, CAnimOverlayBuffer, DrawOverlay);

// Pixel transforms for DecodeFrame.
struct PlainPixel {
    uint16_t operator()(uint16_t p) const { return p; }
};
struct ShadedPixel {
    int shade;
    uint16_t operator()(uint16_t p) const { return g_shadeColors[(p >> g_shadeShift) + shade * 0x2c]; }
};
struct GreyPixel {
    int shade;
    uint16_t operator()(uint16_t p) const
    {
        if (((int)p >> 5) == ((int)p & 0x3ff))
            p = g_shadeColors[((int)p >> 10) + shade * 0x2c];
        return p;
    }
};

// Walks the run list; each run is {header, len pixel bytes}. Header: dx in bits 22.., dy in bits 12..21 (signed), len in bits 0..11.
// The terminator is 0x7fff7fff. In mirrored mode a run is placed at 0x20 - dx - len and read backwards.
// The mode is a tag type (RunsMirrored() or RunsPlain()) because Visual C++ 5.0 accepts no explicit function-template argument.
struct RunsMirrored {
    enum { value = 1 };
};
struct RunsPlain {
    enum { value = 0 };
};
template <class Mode, class Fn>
static void decode_runs(Mode, uint16_t *d, const uint16_t *palette, const uint32_t *rle, Fn fn)
{
    const bool Mirror = Mode::value != 0;
    uint32_t hdr;
    while ((hdr = *rle) != 0x7fff7fff) {
        uint32_t len = hdr & 0xfff;
        int row = (static_cast<int>(hdr << 10) >> 0x16) + 0x50;
        int col;
        if (Mirror)
            col = (0x20 - (static_cast<int>(hdr) >> 0x16)) - static_cast<int>(len);
        else
            col = (static_cast<int>(hdr) >> 0x16) + 0x20;
        const uint8_t *data = reinterpret_cast<const uint8_t *>(rle) + 4;
        if (row >= 0x80)
            return;
        if (row >= 0 && static_cast<int>(len) > 0) {
            const uint8_t *endp = data + len;
            int i = 0;
            if (col < 0x40) {
                do {
                    if (col >= 0) {
                        uint8_t src = Mirror ? endp[-1] : data[i];
                        d[row * 0x40 + col] = fn(palette[src]);
                    }
                    col++;
                    i++;
                    endp--;
                } while (i < static_cast<int>(len) && col < 0x40);
            }
        }
        rle = reinterpret_cast<const uint32_t *>(data + len);
    }
}

// 0x00470b50  size 0x446  decode one RLE animation frame into the 64x128 16-bit sprite buffer at this+8 (row stride 0x40 pixels)
// The six loops of the original (plain / shaded / grey-only, each mirrored or not) share decode_runs.
void __thiscall CAnimOverlayBuffer::DecodeFrame(unsigned int mode, uint16_t *palette, uint32_t *rle)
{
    uint16_t *d = pixels;
    int mirror = unk_24010;

    if ((mode & 0x7fff) == 0) {
        if (mirror != 0)
            decode_runs(RunsMirrored(), d, palette, rle, PlainPixel());
        else
            decode_runs(RunsPlain(), d, palette, rle, PlainPixel());
    } else if ((mode & 0x8000) == 0) {
        ShadedPixel fn = {static_cast<int>(mode) - 1};
        if (mirror != 0)
            decode_runs(RunsMirrored(), d, palette, rle, fn);
        else
            decode_runs(RunsPlain(), d, palette, rle, fn);
    } else {
        GreyPixel fn = {static_cast<int>(mode & 0x7fff) - 1};
        if (mirror != 0)
            decode_runs(RunsMirrored(), d, palette, rle, fn);
        else
            decode_runs(RunsPlain(), d, palette, rle, fn);
    }
}
HOOK_METHOD(0x00470b50, CAnimOverlayBuffer, DecodeFrame);
