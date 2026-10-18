// DDrawDisplay: the DirectDraw display object (IDirectDraw, primary surface, flip chain)
// plus the DIB-section helper that shares its +0x10314 / +0x1031c fields.
// Layout from decomp/pass2/0x0047e550.cpp, 0x0047e6b0.cpp, 0x0047e2f0.cpp.
//
// IDirectDraw vtable offsets used: 0x08 Release, 0x18 CreateSurface, 0x28 FlipToGDISurface,
// 0x50 SetCooperativeLevel, 0x54 SetDisplayMode. IDirectDrawSurface: 0x08 Release,
// 0x30 GetAttachedSurface, 0x54 GetPixelFormat.
#pragma once

#include <cstddef>
#include <cstdint>

#include <windows.h>

#include "../runtime/ported.h"

class DDrawDisplay {
public:
    // 0x0047e550: DirectDrawCreate, exclusive fullscreen 640x480x16, primary surface (flip chain if enabled).
    void __thiscall Init();
    // 0x0047e6b0: release surfaces and IDirectDraw.
    void __thiscall Shutdown();
    // 0x0047e2f0: top-down 16bpp DIB section (bitfield masks 565/555, or DIB_PAL_COLORS when palettized).
    HBITMAP __thiscall CreateTopDownDib16(LONG width, LONG height, void **ppBits);
    // 0x0047e250: blit from `hbm` (selected into a memory DC) onto `hdc`; src position in args 8 and 9,
    // arg 7 unused. Selects the palette first when palettized. Returns the DeleteDC result.
    int __thiscall BlitBitmapToDC(HDC hdc, int dx, int dy, int w, int h, HGDIOBJ hbm, int unused, int sx, int sy);
    // 0x0047e2d0: DeleteObject(obj); `this` and the second argument are ignored.
    int __thiscall DeleteGdiObject(HGDIOBJ obj, int unused);
    // 0x0047e4e0: ready = 0, windowed-mode flag (0x00758274) = 1. Returns this.
    DDrawDisplay *__thiscall ResetReady();
    // 0x0047e500: if ready, flip to the GDI surface (when flipping), release the primary surface and IDirectDraw.
    void __thiscall Release();

    void *dd;               // +0x00  IDirectDraw *
    void *primary;          // +0x04  IDirectDrawSurface *
    void *backBuffer;       // +0x08  the attached back buffer, or the primary when not flipping
    int ready;              // +0x0c
    int is565;              // +0x10  primary surface red mask == 0xf800
    int flipping;           // +0x14  flip chain in use
    uint8_t pad_018[0x10314 - 0x18];
    int palettized;         // +0x10314
    uint8_t pad_10318[4];
    void *palette;          // +0x1031c  HPALETTE
    // ? the object is larger than this; the real size and whether Init/Shutdown and CreateTopDownDib16
    // ? run on the same object is only inferred from the field offsets
};

static_assert(offsetof(DDrawDisplay, dd) == 0x00, "DDrawDisplay::dd");
static_assert(offsetof(DDrawDisplay, primary) == 0x04, "DDrawDisplay::primary");
static_assert(offsetof(DDrawDisplay, backBuffer) == 0x08, "DDrawDisplay::backBuffer");
static_assert(offsetof(DDrawDisplay, ready) == 0x0c, "DDrawDisplay::ready");
static_assert(offsetof(DDrawDisplay, is565) == 0x10, "DDrawDisplay::is565");
static_assert(offsetof(DDrawDisplay, flipping) == 0x14, "DDrawDisplay::flipping");
static_assert(offsetof(DDrawDisplay, palettized) == 0x10314, "DDrawDisplay::palettized");
static_assert(offsetof(DDrawDisplay, palette) == 0x1031c, "DDrawDisplay::palette");
