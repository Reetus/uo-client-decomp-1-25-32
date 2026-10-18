// uV intro video player, DirectDraw surface layer (porting pass 3, batch v2):
// frame blit 0x00485210, buffer swap 0x00485d70, surface setup 0x00486590 and
// release 0x00486770, and the bit-stream reset 0x00486e60.
// The player object is ECX in all but the last (see uv_video.h).
#include <cstdint>
#include <cstring>

#include "../runtime/ported.h"
#include "com_call.h"
#include "crt.h"
#include "uv_video.h"
#include "uv_video_tables_ports.h"
#include "globals.h"

#define uv_bitptr GLOBAL_AS(const uint8_t *, UVD_BitPtr)
#define UV_BuildTables PORT_FUNC1(&UV_BuildTables, void, __fastcall, UvVideo *)

// IDirectDrawSurface::Blt of the offscreen surface onto the primary (DDBLT_WAIT).
static void uv_blt(UvSurf *p, const int32_t *dest, const int32_t *src)
{
    com_call(PORT_RET(long), p->primary, 0x14, dest, p->surf0, src, 0x01000000, 0);
}

// 0x00485210  size 0x24d  Blits the offscreen frame to the primary surface (placement flags, line doubling).
void __fastcall UV_BlitFrame(UvVideo *o)
{
    UvSurf *p = o->surf;
    int32_t px = 0;
    int32_t py = 0;
    int32_t w0 = static_cast<int32_t>(p->width);   // [esp+0x10]
    int32_t w1 = w0;                               // [esp+0x20]
    int32_t h0 = static_cast<int32_t>(p->height);  // [esp+0x14]
    int32_t h1 = h0;                               // [esp+0x18]
    if (p->flags & 0x800)
        return;
    if (p->bpp == 8) {
        uint8_t *entries = p->palEntries;
        for (int i = 0; i < 256; i++) {
            entries[i * 4 + 0] = o->palette[i * 3 + 0];
            entries[i * 4 + 1] = o->palette[i * 3 + 1];
            entries[i * 4 + 2] = o->palette[i * 3 + 2];
            entries[i * 4 + 3] = 0;
        }
        com_call(PORT_RET(long), o->surf->dd, 0x58, 1, 0);   // IDirectDraw::WaitForVerticalBlank
        com_call(PORT_RET(long), o->surf->palette, 0x18, 0, 0, 0x100, entries);   // IDirectDrawPalette::SetEntries
        px = 0;
    }
    p = o->surf;
    uint32_t fl = p->flags;
    if (fl & 0x80)
        w0 = static_cast<int32_t>(p->sizeW);
    if (fl & 0x100)
        h1 = static_cast<int32_t>(p->sizeH);
    if (fl & 0x40) {
        w1 = w1 * 2;
        w0 = w0 * 2;
    }
    int32_t edx = h0;
    int32_t ebp;
    if (fl & 0x30) {
        ebp = h1;
        edx = edx * 2;
        h0 = edx;
        ebp = ebp * 2;
    } else {
        ebp = h1;
    }
    if (fl & 4)
        px = (o->winW - w0) >> 1;
    if (fl & 8)
        py = (o->winH - ebp) >> 1;
    if (fl & 1)
        px += p->offX;
    if (fl & 2)
        py += p->offY;
    ebp = ebp + py;
    int32_t dst[4];   // [esp+0x24]
    int32_t src[4];   // [esp+0x34]
    src[0] = 0;
    src[1] = 0;
    src[3] = edx;
    src[2] = w1;
    dst[0] = px;
    dst[1] = py;
    dst[2] = w0 + px;
    dst[3] = ebp;
    if (p->flags & 0x20) {
        int32_t i = 0;
        if (edx <= 0)
            return;
        do {
            int32_t eax = i >> 1;
            src[1] = eax;
            src[3] = eax + 1;
            dst[1] = i + py;
            dst[3] = i + py + 1;
            uv_blt(o->surf, dst, src);
            i += 2;
        } while (i < h0);
        return;
    }
    if (p->flags & 0x10) {
        if (edx <= 0)
            return;
        int32_t back = -1 - py;
        int32_t i = py + 1;
        do {
            int32_t eax = (back + i) >> 1;
            src[1] = eax;
            src[3] = eax + 1;
            dst[1] = i - 1;
            dst[3] = i;
            uv_blt(o->surf, dst, src);
            dst[1] = i;
            dst[3] = i + 1;
            uv_blt(o->surf, dst, src);
            i += 2;
        } while (back + i < h0);
        return;
    }
    uv_blt(p, dst, src);
}
HOOK_FUNC(0x00485210, UV_BlitFrame);

// 0x00485d70  size 0x2a  Swaps the two offscreen surfaces (and the software buffers when flag 0x800).
void __fastcall UV_SwapFrameBuffers(UvVideo *o)
{
    UvSurf *p = o->surf;
    void *a = p->surf1;
    void *b = p->surf0;
    p->surf0 = a;
    p = o->surf;
    p->surf1 = b;
    p = o->surf;
    if (p->flags & 0x800) {
        uint8_t *c = p->buf0;
        uint8_t *d = p->buf1;
        p->buf1 = c;
        p = o->surf;
        p->buf0 = d;
    }
}
HOOK_FUNC(0x00485d70, UV_SwapFrameBuffers);

// Index of the lowest set bit of a mask, by arithmetic shifts (a zero mask never ends, as in the original).
static int32_t uv_low_bit(int32_t mask)
{
    int32_t n = 0;
    if (!(mask & 1)) {
        do {
            mask >>= 1;
            n++;
        } while (!(mask & 1));
    }
    return n;
}

// Number of set bits among bits 0..30.
static int32_t uv_bit_count(uint32_t mask)
{
    int32_t n = 0;
    for (int32_t i = 0; i < 0x1f; i++) {
        if (mask & (1u << i))
            n++;
    }
    return n;
}

// 0x00486590  size 0x1d7  Reads the display's pixel format, creates the palette for 8-bit output, builds the colour tables.
void __fastcall UV_InitVideoSurface(UvVideo *o)
{
    UvSurf *p = o->surf;
    if (p->disabled != 0)
        return;
    p->surf0 = nullptr;
    o->surf->surf1 = nullptr;
    p = o->surf;
    if (!(p->flags & 0x800)) {
        uint32_t desc[27];   // DDSURFACEDESC
        memset(desc, 0, sizeof(desc));
        desc[0] = 0x6c;
        if (com_call(PORT_RET(long), p->primary, 0x58, desc) == 0) {   // GetSurfaceDesc
            o->winW = static_cast<int32_t>(desc[3]);
            o->winH = static_cast<int32_t>(desc[2]);
        }
        UvSurf *s = o->surf;
        if (s->primary != nullptr) {
            uint32_t pf[8];   // DDPIXELFORMAT
            memset(pf, 0, sizeof(pf));
            pf[0] = 0x20;
            com_call(PORT_RET(long), s->primary, 0x54, pf);   // GetPixelFormat
            if (static_cast<int32_t>(pf[3]) == 8) {
                s->bpp = 8;
            } else {
                int32_t rmask = static_cast<int32_t>(pf[4]);
                int32_t gmask = static_cast<int32_t>(pf[5]);
                int32_t bmask = static_cast<int32_t>(pf[6]);
                int32_t rBits = uv_bit_count(static_cast<uint32_t>(rmask));
                s->rBits = rBits;
                int32_t gBits = uv_bit_count(static_cast<uint32_t>(gmask));
                s->gBits = gBits;
                int32_t bBits = uv_bit_count(static_cast<uint32_t>(bmask));
                s->bBits = bBits;
                s->rShift = uv_low_bit(rmask);
                s->gShift = uv_low_bit(gmask);
                int32_t bShift = uv_low_bit(bmask);
                s->bShift = bShift;
                s->bpp = bBits + gBits + rBits;
                s->maskAll = static_cast<uint32_t>(rmask | gmask | bmask);
            }
        }
        if (o->surf->bpp == 8) {
            UvSurf *q = o->surf;
            if (!(q->flags & 0x4000000)) {
                void *mem = malloc(0x400);
                o->surf->palEntries = static_cast<uint8_t *>(mem);
                q = o->surf;
                com_call(PORT_RET(long), q->dd, 0x14, 0x44, q->palEntries, &q->palette, static_cast<void *>(nullptr));   // CreatePalette
                q = o->surf;
                memset(q->palEntries, 0, 0x400);
            }
            q = o->surf;
            com_call(PORT_RET(long), q->primary, 0x7c, q->palette);   // SetPalette
        }
    }
    p = o->surf;
    if (!(p->flags & 0x100000))
        p->brightness = 0x64;
    UV_BuildTables(o);
    o->inited = 1;
}
HOOK_FUNC(0x00486590, UV_InitVideoSurface);

// 0x00486770  size 0xdf  Releases the surfaces, the palette and the codec buffers.
void __fastcall UV_ReleaseVideoSurface(UvVideo *o)
{
    if (o->inited == 0)
        return;
    UvSurf *p = o->surf;
    void *obj = p->surf0;
    if (obj != nullptr && !(p->flags & 0x800))
        com_call(PORT_RET(unsigned long), obj, 0x08);
    p = o->surf;
    obj = p->surf1;
    if (obj != nullptr && !(p->flags & 0x800))
        com_call(PORT_RET(unsigned long), obj, 0x08);
    p = o->surf;
    obj = p->palette;
    if (obj != nullptr && !(p->flags & 0x4000000))
        com_call(PORT_RET(unsigned long), obj, 0x08);
    if (o->mem420 != nullptr)
        free(o->mem420);
    if (o->mem424 != nullptr)
        free(o->mem424);
    if (o->mem428 != nullptr)
        free(o->mem428);
    if (o->mem410 != nullptr)
        free(o->mem410);
    if (o->mem414 != nullptr)
        free(o->mem414);
    if (o->mem41c != nullptr)
        free(o->mem41c);
    if (o->mem418 != nullptr)
        free(o->mem418);
    o->inited = 0;
}
HOOK_FUNC(0x00486770, UV_ReleaseVideoSurface);

// 0x00486e60  size 0x17  Points the bit reader at a stream (cdecl, one stack argument; preserves ESI).
void __cdecl UV_SetBitStream(const uint8_t *stream)
{
    uv_bitptr = stream;
    UVD_BitPos = 0;
}
HOOK_FUNC(0x00486e60, UV_SetBitStream);
