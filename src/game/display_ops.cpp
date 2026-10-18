// Back-buffer DIB creation, GDI blit helper, 555 -> 565 pixel conversion, and DirectDraw release.
#include "ddraw_display.h"

#include <cstdint>
#include <cstring>

#include "../runtime/ported.h"
#include "com_call.h"
#include "globals.h"
#include "gump_art_ops_ports.h"
#include "gump_art_ports.h"
#include "art_row_blits_ports.h"

#define dop_display GLOBAL_AS(DDrawDisplay, g_clientState_storage)

// gump art size: ECX = art id, EDX = &width, stack = &height
#define dop_GumpArtSize(a1, a2, a3) (GumpArtSizeImpl(a1, a2, a3))
// clipped gump art draw: ECX = row blit, EDX = art id, stack (clip, x, y, dest, stride, destH)
#define dop_GumpArtDrawClipped PORT_FUNC8(&DrawGumpArtClipped, void, __fastcall, uintptr_t, int, const RECT *, int, int, uint16_t *, int, int)

// 0x0047e1a0  size 0x87
// Create the 640x480 top-down DIB back buffer on the display object, remember its pixels, then paint
// gump art 0x587 into it (row blitter 0x004764c0).
void __cdecl CreateBackBuffer()
{
    RECT rc;
    int w, h;

    dop_backBufferBitmap = dop_display.CreateTopDownDib16(0x280, 0x1e0, reinterpret_cast<void **>(&g_backBuffer));
    dop_backBufferCopy = g_backBuffer;
    rc.left = 0;
    rc.right = 0x280;
    rc.top = 0;
    rc.bottom = 0x1e0;
    dop_GumpArtSize(0x587, &w, &h);
    dop_GumpArtDrawClipped(PORT_ADDR(&RowBlitCopyNonZero), 0x587, &rc, 0, 0, g_backBuffer, 0x280, 0x1e0);
}
HOOK_FUNC(0x0047e1a0, CreateBackBuffer);

// 0x0047e230  size 0x11
void __cdecl DestroyBackBuffer()
{
    if (dop_backBufferBitmap != nullptr)
        DeleteObject(dop_backBufferBitmap);
}
HOOK_FUNC(0x0047e230, DestroyBackBuffer);

// 0x0047e250  size 0x78
int __thiscall DDrawDisplay::BlitBitmapToDC(HDC hdc, int dx, int dy, int w, int h, HGDIOBJ hbm, int unused, int sx, int sy)
{
    (void)unused;
    HDC mem = CreateCompatibleDC(hdc);
    HGDIOBJ old;

    if (palettized != 0)
        SelectPalette(hdc, static_cast<HPALETTE>(palette), 0);
    old = SelectObject(mem, hbm);
    BitBlt(hdc, dx, dy, w, h, mem, sx, sy, SRCCOPY);
    SelectObject(mem, old);
    return DeleteDC(mem);
}
HOOK_METHOD(0x0047e250, DDrawDisplay, BlitBitmapToDC);

// 0x0047e2d0  size 0x11
int __thiscall DDrawDisplay::DeleteGdiObject(HGDIOBJ obj, int unused)
{
    (void)unused;
    return DeleteObject(obj);
}
HOOK_METHOD(0x0047e2d0, DDrawDisplay, DeleteGdiObject);

// 0x0047e450  size 0x56
// ECX = dest, EDX = src, stack = pixel count (RET 4). RGB555 -> RGB565, two pixels at a time, odd tail.
void __fastcall Convert555To565(uint16_t *dest, const uint16_t *src, int count)
{
    int pairs = count >> 1;
    const uint32_t *s = reinterpret_cast<const uint32_t *>(src);
    uint32_t *d = reinterpret_cast<uint32_t *>(dest);

    while (pairs != 0) {
        uint32_t v = *s++;
        *d++ = ((v & 0xffe07fe0u) << 1) | (v & 0x1f001fu);
        pairs--;
    }
    if ((count & 1) != 0) {
        uint32_t v = *reinterpret_cast<const uint16_t *>(s);
        *reinterpret_cast<uint16_t *>(d) = static_cast<uint16_t>(((v & 0xffe0u) << 1) | (v & 0x1fu));
    }
}
HOOK_FUNC(0x0047e450, Convert555To565);

// 0x0047e4b0  size 0x23
// ECX = dest, EDX = src, stack = pixel count and a second argument that is never read (RET 8).
void __fastcall CopyPixels16(uint16_t *dest, const uint16_t *src, int count, int unused)
{
    (void)unused;
    memcpy(dest, src, static_cast<size_t>(count) * 2);
}
HOOK_FUNC(0x0047e4b0, CopyPixels16);

// 0x0047e4e0  size 0x14
DDrawDisplay *__thiscall DDrawDisplay::ResetReady()
{
    ready = 0;
    dop_windowedFlag = 1;
    return this;
}
HOOK_METHOD(0x0047e4e0, DDrawDisplay, ResetReady);

// 0x0047e500  size 0x4f
void __thiscall DDrawDisplay::Release()
{
    if (ready == 0)
        return;
    if (dd != nullptr) {
        if (flipping != 0)
            com_call(PORT_RET(long), dd, 0x28);   // IDirectDraw::FlipToGDISurface
        if (primary != nullptr) {
            com_call(PORT_RET(long), primary, 0x08);
            primary = nullptr;
        }
        backBuffer = nullptr;
        com_call(PORT_RET(long), dd, 0x08);
        dd = nullptr;
    }
    ready = 0;
}
HOOK_METHOD(0x0047e500, DDrawDisplay, Release);
