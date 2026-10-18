// LoadVerdata: load verdata.mul and apply its patch records to the in-memory
// tables. 0x0043f5e0. SEH frame handler 0x0049a5d6 (state 0 around the
// new(0x90) + stream ctor, state 1 around new(0x704) + ctor) omitted.
#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "mapped_file.h"
#include "font_block.h"

#define g_verdataIndex GLOBAL_PTR(int, g_verdataTable)        // chains of 0x14-byte nodes, index file * 0x400 + (block & 0x3ff)
#define g_bitmapSets GLOBAL_PTR(uint8_t *, g_fontMgr_storage)    // per file-0x21 block: object of 0x704 bytes

// 0x0043f5e0  size 0x3f4
// A record is five ints {file, block, offset, length, extra}.
//   file <= 0x11: chain node {block, offset, length, extra, next} into the index
//   file 0x1e: 32 art records, 0x1c-byte layout (block < 0x200) or 0x28-byte layout
//   file 0x1f: 8 tiledata slots, 0x40 bytes to (slot - 0x40) then 4 bytes to slot..slot+3
//   file 0x21: exactly 0xe0 bitmaps; the source pointer is NOT advanced past the
//     pixel data (original behaviour: the next header is read 3 bytes further on)
void LoadVerdata()
{
    if (g_verdataEnabled == 0)
        return;

    FILE *fp = fopen(g_app.ResolveDataPath(const_cast<char *>("verdata.mul")), g_fopenReadMode);
    if (fp == nullptr)
        return;
    fclose(fp);

    MappedFile *stream = new MappedFile(g_app.ResolveDataPath(const_cast<char *>("verdata.mul")));   // stream ctor
    g_verdataUnk = stream;

    g_verdataBase = static_cast<uint8_t *>(decomp::thiscall_at(PORT_RET(void *), PORT_ADDR(&MappedFile::OpenReadOnly), stream));
    if (g_verdataBase == nullptr)
        return;

    uint8_t *base = g_verdataBase;
    g_verdataCount = *reinterpret_cast<int *>(base);
    int *rec = reinterpret_cast<int *>(base + 4);
    int i = 0;
    if (g_verdataCount <= 0)
        return;

    for (;;) {
        int file = rec[0];
        int block = rec[1];
        int off = rec[2];
        int len = rec[3];
        int extra = rec[4];
        rec = rec + 5;
        base = g_verdataBase;

        if (file <= 0x11) {
            int *node = static_cast<int *>(::operator new(0x14));
            node[0] = block;
            node[1] = off;
            node[2] = len;
            node[3] = extra;
            int *head = g_verdataIndex + (((block & 0x3ff) + (file << 10)));
            node[4] = *head;
            *head = (int)(intptr_t)node;
        } else if (file == 0x1e) {
            uint8_t *src = base + off + 4;
            if (block < 0x200) {
                int d = block * 0x380;
                for (int k = 0x20; k != 0; k--) {
                    RAWF(int32_t, g_landTable, d) = *reinterpret_cast<int32_t *>(src);
                    src = src + 4;
                    RAWF(int16_t, g_landTable, d + 4) = *reinterpret_cast<int16_t *>(src);
                    src = src + 2;
                    strncpy(reinterpret_cast<char *>(g_landTable + d + 6), reinterpret_cast<char *>(src), 0x14);
                    src = src + 0x14;
                    d = d + 0x1c;
                }
            } else {
                int d = (block - 0x200) * 0x500;
                for (int k = 0x20; k != 0; k--) {
                    RAWF(int32_t, g_tileData, d) = *reinterpret_cast<int32_t *>(src);
                    src = src + 4;
                    RAWF(uint8_t, g_tileData, d + 4) = *src;
                    src = src + 1;
                    RAWF(uint8_t, g_tileData, d + 5) = *src;
                    src = src + 1;
                    RAWF(int32_t, g_tileData, d + 8) = *reinterpret_cast<int32_t *>(src);
                    src = src + 4;
                    RAWF(int16_t, g_tileData, d + 0xc) = *reinterpret_cast<int16_t *>(src);
                    src = src + 2;
                    RAWF(int16_t, g_tileData, d + 0xe) = *reinterpret_cast<int16_t *>(src);
                    src = src + 2;
                    RAWF(int16_t, g_tileData, d + 0x10) = *reinterpret_cast<int16_t *>(src);
                    src = src + 2;
                    RAWF(uint8_t, g_tileData, d + 0x12) = *src;
                    src = src + 1;
                    strncpy(reinterpret_cast<char *>(g_tileData + d + 0x13), reinterpret_cast<char *>(src), 0x14);
                    src = src + 0x14;
                    d = d + 0x28;
                }
            }
        } else if (file == 0x1f) {
            int s0 = block * 8;
            int s1 = block * 8 + 8;
            uint8_t *src = base + off + 4;
            if (s0 < s1) {
                uint8_t *slot = g_005286f0 + s0 * 0x48;
                int cnt = s1 - s0;
                do {
                    uint8_t *dst = slot - 0x40;
                    for (int n = 0x40; n != 0; n--) {
                        *dst = *src;
                        src = src + 1;
                        dst = dst + 1;
                    }
                    slot[0] = *src;
                    src = src + 1;
                    slot[1] = *src;
                    src = src + 1;
                    slot = slot + 0x48;
                    slot[-0x46] = *src;
                    src = src + 1;
                    slot[-0x45] = *src;
                    src = src + 1;
                    cnt = cnt - 1;
                } while (cnt != 0);
            }
        } else if (file == 0x21) {
            uint8_t *old = g_bitmapSets[block];
            uint8_t *p = base + off;
            delete reinterpret_cast<CFontBlock *>(old);
            g_bitmapSets[block] = reinterpret_cast<uint8_t *>(new CFontBlock());

            int k = 4;
            for (;;) {
                uint8_t w = p[0];
                uint8_t h = p[1];
                uint8_t tag = p[2];
                uint8_t *pix = p + 3;

                RAWF(int32_t, g_bitmapSets[block], k + 0x380) = tag;
                p = pix;   // not advanced past the pixels
                int wh = w * h;
                void *bmp = ::operator new((size_t)(wh * 2 + 4));
                RAWF(void *, g_bitmapSets[block], k) = bmp;
                RAWF(uint16_t, bmp, 0) = w;
                RAWF(uint16_t, bmp, 2) = h;
                memcpy(static_cast<char *>(bmp) + 4, pix, (size_t)(wh * 2));
                k = k + 4;
                if (k >= 0x384)
                    break;
            }
        }

        i = i + 1;
        if (i >= g_verdataCount)
            break;
    }
}
HOOK_FUNC(0x0043f5e0, LoadVerdata);
