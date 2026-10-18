// RLE sprite clipping and drawing into the current 16-bit surface.
#include <cstdint>

#include "../runtime/ported.h"
#include "circle_trans.h"
#include "globals.h"

#define g_surfaceBase GLOBAL_AS(uint8_t *, g_blitDest)       // base address of the destination surface
#define g_spriteBlitter GLOBAL_PTR(void, g_circleTrans_storage)          // object whose 0x00424570 draws one run
// 0x00424570: draw one run. ECX = blitter object; stack: A (the first argument of the sprite
// call), destination pointer, source pointer, pixel count.
#define Blitter_DrawRun(self, a1, a2, a3, a4) (reinterpret_cast<CCircleTrans *>(self)->BlendSpan((uint32_t)(a1), a2, a3, a4))

// 0x004246e0  size 0x1c8
// Clip the RLE sprite to rect {left, top, right, bottom} and draw it at (x, y). ECX = A (passed
// on to the run blitter), EDX = sprite; stack: rect, x, y (callee pops 0xc).
// Sprite layout: u16 width @+4, u16 height @+6, u16 rowOffset[height] @+8, then runs
// {u16 gap/start, u16 len, u16 pixels[len]} terminated by len == 0.
// High words of the start/len temporaries are assumed zero; y/rows are assumed non-negative.  // ?
void __fastcall DrawRleSpriteClipped(int A, uint8_t *sprite, int *rect, int x, int y)
{
    int skipLeft;
    int limitX;
    uint8_t *runBase;
    uint16_t *rowOffPtr;
    int rowsLeft;
    int visW;
    uint16_t *dstRow;
    uint16_t *dstp;
    uint16_t *srcp;
    uint16_t *pix;
    unsigned int start;
    int width, height, skipTop, rows;
    int len;
    int s;
    int cnt;
    uint16_t *callSrc;
    int doCall;

    if (sprite == nullptr)
        return;
    height = *reinterpret_cast<uint16_t *>(sprite + 6);
    width = *reinterpret_cast<uint16_t *>(sprite + 4);
    rowsLeft = height;
    runBase = sprite + height * 2 + 8;

    if (rect[0] > x)
        skipLeft = rect[0] - x;
    else
        skipLeft = 0;
    if (rect[2] < x + width)                  // signed
        limitX = rect[2] - x;
    else
        limitX = width;
    if (skipLeft >= limitX)
        return;

    if (rect[1] > y)
        skipTop = rect[1] - y;
    else
        skipTop = 0;
    if (rect[3] < height + y)
        rows = rect[3] - y;
    else
        rows = height - 1;                    // kept literally, as in the binary
    dstRow = reinterpret_cast<uint16_t *>(g_surfaceBase + ((y + skipTop) * g_blitStride + x) * 2);
    visW = limitX - skipLeft;
    rowOffPtr = reinterpret_cast<uint16_t *>(sprite + skipTop * 2 + 8);
    if (skipTop >= rows)
        return;
    rowsLeft = rows - skipTop;

    do {
        uint16_t *hdr = reinterpret_cast<uint16_t *>(runBase + *rowOffPtr * 2);
        start = hdr[0];
        len = hdr[1];
        pix = hdr + 2;
        while (len != 0) {
            s = static_cast<int>(start & 0xffff);
            doCall = 0;
            cnt = 0;
            callSrc = nullptr;

            if (skipLeft > s) {                // run begins left of the clip rect
                dstp = dstRow + skipLeft;
                srcp = pix + (skipLeft - s);
                len = len & 0xffff;
                if (len + s > limitX) {
                    cnt = visW;
                    callSrc = srcp;
                    doCall = 1;
                } else {
                    cnt = len - skipLeft + s;
                    if (cnt > 0) {
                        callSrc = srcp;
                        doCall = 1;
                    }
                }
            } else {
                dstp = dstRow + s;
                len = len & 0xffff;
                if (len + s > limitX) {
                    cnt = limitX - s;
                    if (cnt > 0) {
                        callSrc = pix;
                        doCall = 1;
                    }
                } else {
                    cnt = len;
                    callSrc = pix;
                    doCall = 1;
                }
            }
            if (doCall)
                Blitter_DrawRun(g_spriteBlitter, A, dstp, callSrc, cnt);        // 0x0042484e

            pix = pix + len;                   // next run header {gap, len}
            {
                int gap = *pix;
                pix = pix + 1;
                start = start + static_cast<unsigned int>(gap + len);   // ? upper bits of EBX assumed zero
                len = *pix;
                pix = pix + 1;
            }
        }
        dstRow = dstRow + g_blitStride;      // next scanline
        rowOffPtr = rowOffPtr + 1;
        rowsLeft = rowsLeft - 1;
    } while (rowsLeft != 0);
}
HOOK_FUNC(0x004246e0, DrawRleSpriteClipped);
