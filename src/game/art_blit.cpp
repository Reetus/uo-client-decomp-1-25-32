// Row blend and clipped RLE art walk (0x00476330, 0x004767d0).
#include <cstdint>

#include "../runtime/ported.h"
#include "globals.h"

// Blend masks and the destination surface used by the RLE walker.

typedef void(__fastcall *RowBlitFn)(uint16_t *dst, uint16_t *src, int count);

// 0x00476330  size 0x55  row blend: dst = ((((m1 & dst) + (src & m1)) >> 1) + (m0 & dst)) >> 1 | 1   (RET 4)
// ECX = dst, EDX = src, stack = count; arithmetic on 32-bit ints, 16-bit store.
void __fastcall RowBlend(uint16_t *dst, uint16_t *src, int count)
{
    if (count == 0)
        return;
    do {
        int d = *dst;
        int s = *src;
        int t = ((g_blendMaskA & d) + (s & g_blendMaskA)) >> 1;
        src++;
        dst++;
        t = (t + (g_blendMaskB & d)) >> 1;
        t |= 1;
        count--;
        dst[-1] = static_cast<uint16_t>(t);
    } while (count != 0);
}
HOOK_FUNC(0x00476330, RowBlend);

// 0x004767d0  size 0x1b1  walk an RLE art record clipped to a rect, calling a row-blit function per visible run
// ECX = row-blit (called with ECX = dest, EDX = src, one stack arg = pixel count), EDX = art record,
// stack = (clip {l,t,r,b}, x, y), RET 0xc. Record: +4 width, +6 height, +8 ushort rowOffset[height], then run data;
// each row = {ushort startX, then repeat {ushort len; len pixels; ushort gap}} until len == 0.
void __fastcall RleArtWalk(RowBlitFn blit, uint8_t *rec, int *clip, int x, int y)
{
    int width, height, leftClip, visW, firstRow, lastRow, rows, maxVis;
    uint16_t *data, *rowOff, *dstRow, *src, *dstPtr, *srcPtr;
    unsigned int cur;
    unsigned int len;

    if (rec == nullptr)
        return;

    height = *reinterpret_cast<uint16_t *>(rec + 6);
    width = *reinterpret_cast<uint16_t *>(rec + 4);
    data = reinterpret_cast<uint16_t *>(rec + height * 2 + 8);

    if (clip[0] > x)
        leftClip = clip[0] - x;
    else
        leftClip = 0;

    if (clip[2] < x + width)
        visW = clip[2] - x;
    else
        visW = width;

    if (leftClip >= visW)
        return;

    if (clip[1] > y)
        firstRow = clip[1] - y;
    else
        firstRow = 0;

    if (clip[3] < height + y)
        lastRow = clip[3] - y;
    else
        lastRow = height - 1;

    dstRow = g_blitDest + ((y + firstRow) * g_blitStride + x);
    rowOff = reinterpret_cast<uint16_t *>(rec + firstRow * 2 + 8);
    maxVis = visW - leftClip;
    if (firstRow >= lastRow)
        return;
    rows = lastRow - firstRow;

    for (;;) {
        uint16_t *p = data + *rowOff;
        cur = p[0];
        len = p[1];
        src = p + 2;
        if (len != 0) {
            do {
                cur &= 0xffff;
                if (leftClip > static_cast<int>(cur)) {
                    // run starts left of the clip rect
                    dstPtr = dstRow + leftClip;
                    srcPtr = src + (leftClip - cur);
                    len &= 0xffff;
                    if (static_cast<int>(len + cur) > visW) {
                        blit(dstPtr, srcPtr, maxVis);
                    } else {
                        int n = static_cast<int>(len) - leftClip + static_cast<int>(cur);
                        if (n > 0)
                            blit(dstPtr, srcPtr, n);
                    }
                } else {
                    dstPtr = dstRow + cur;
                    len &= 0xffff;
                    if (static_cast<int>(len + cur) > visW) {
                        int n = visW - static_cast<int>(cur);
                        if (n > 0)
                            blit(dstPtr, src, n);
                    } else {
                        blit(dstPtr, src, static_cast<int>(len));
                    }
                }
                src += len;
                cur += *src + len;   // gap + len
                src++;
                len = *src;
                src++;
            } while (static_cast<uint16_t>(len) != 0);
        }
        dstRow += g_blitStride;
        rowOff++;
        rows--;
        if (rows == 0)
            break;
    }
}
HOOK_FUNC(0x004767d0, RleArtWalk);
