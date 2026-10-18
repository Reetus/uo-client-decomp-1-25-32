#include "ddraw_display.h"

#include <cstring>

#include "../runtime/ported.h"
#include "com_call.h"
#include <ddraw.h>
#include "globals.h"

#define g_mainWindowHandle GLOBAL_AS(HWND, g_mainWindow)

// DirectDrawCreate(NULL, &dd, NULL) thunk

// DDSURFACEDESC is 0x6c bytes (27 dwords); only these dwords are touched.
enum {
    kDescSize = 0,
    kDescFlags = 1,
    kDescBackBufferCount = 5,
    kDescCaps = 26,   // ddsCaps.dwCaps at +0x68
    kDescDwords = 27,
};

// 0x0047e550  size 0x15e
// Init: DirectDrawCreate, SetCooperativeLevel(hwnd, EXCLUSIVE|FULLSCREEN = 0x11), SetDisplayMode(640, 480, 16),
// create the primary surface (flip chain with 1 back buffer if 0x004afb5c, falling back to a plain primary),
// read the pixel format.
void __thiscall DDrawDisplay::Init()
{
    uint32_t desc[kDescDwords];
    uint32_t pf[8];            // DDPIXELFORMAT, dwRBitMask at +0x10
    uint32_t caps;             // DDSCAPS
    bool haveFlipChain;

    if (DirectDrawCreate(nullptr, reinterpret_cast<IDirectDraw **>(&dd), nullptr) != 0)
        return;
    if (com_call(PORT_RET(long), dd, 0x50, g_mainWindowHandle, (DWORD)0x11) != 0)
        return;
    if (com_call(PORT_RET(long), dd, 0x54, (DWORD)0x280, (DWORD)0x1e0, (DWORD)0x10) != 0)
        return;

    memset(desc, 0, sizeof(desc));
    haveFlipChain = false;

    if (g_useFlipChain != 0) {
        desc[kDescSize] = 0x6c;
        desc[kDescFlags] = 0x21;          // DDSD_CAPS | DDSD_BACKBUFFERCOUNT
        desc[kDescCaps] = 0x218;          // PRIMARYSURFACE | COMPLEX | FLIP
        desc[kDescBackBufferCount] = 1;
        if (com_call(PORT_RET(long), dd, 0x18, desc, &primary, (void *)nullptr) == 0) {
            void *prim = primary;
            haveFlipChain = true;
            flipping = 1;
            caps = 4;                     // DDSCAPS_BACKBUFFER
            com_call(PORT_RET(long), prim, 0x30, &caps, &backBuffer);   // result ignored
        }
    }

    if (!haveFlipChain) {
        flipping = 0;
        desc[kDescBackBufferCount] = 0;
        desc[kDescSize] = 0x6c;
        desc[kDescFlags] = 1;             // DDSD_CAPS
        desc[kDescCaps] = 0x200;          // PRIMARYSURFACE
        if (com_call(PORT_RET(long), dd, 0x18, desc, &primary, (void *)nullptr) != 0) {
            MessageBoxA(g_mainWindowHandle, "Failed to create primary surface", "Note:", 0);
            return;
        }
        backBuffer = primary;
    }

    pf[0] = 0x20;
    com_call(PORT_RET(long), primary, 0x54, pf);
    if (pf[4] == 0xf800) {
        is565 = 1;
        ready = 1;
        return;
    }
    is565 = 0;
    ready = 1;
}
HOOK_METHOD(0x0047e550, DDrawDisplay, Init);

// 0x0047e6b0  size 0x48
// Release the surfaces and IDirectDraw. The ready flag is cleared unconditionally.
void __thiscall DDrawDisplay::Shutdown()
{
    if (dd != nullptr) {
        if (flipping != 0)
            com_call(PORT_RET(long), dd, 0x28);   // FlipToGDISurface
        if (primary != nullptr) {
            com_call(PORT_RET(long), primary, 0x08);   // Release
            primary = nullptr;
        }
        backBuffer = nullptr;
        com_call(PORT_RET(long), dd, 0x08);   // Release
        dd = nullptr;
    }
    ready = 0;
}
HOOK_METHOD(0x0047e6b0, DDrawDisplay, Shutdown);

// 0x0047e2f0  size 0x153
// Create a top-down 16bpp DIB section. BI_BITFIELDS with 565/555 masks, or BI_RGB with a DIB_PAL_COLORS
// identity index table when the display is palettized.
HBITMAP __thiscall DDrawDisplay::CreateTopDownDib16(LONG width, LONG height, void **ppBits)
{
    struct {
        BITMAPINFOHEADER hdr;
        union {
            DWORD masks[3];
            WORD indices[256];
        } colors;
    } bmi;
    HDC hdc;
    HBITMAP hbm;
    int i;
    DWORD *masks;

    bmi.hdr.biWidth = width;
    bmi.hdr.biHeight = -height;
    bmi.hdr.biCompression = (palettized != 0) ? 0 : 3;   // BI_RGB when palettized, else BI_BITFIELDS
    bmi.hdr.biXPelsPerMeter = 0x2dcc;
    bmi.hdr.biYPelsPerMeter = 0x2dcc;
    bmi.hdr.biSize = 0x28;
    bmi.hdr.biPlanes = 1;
    bmi.hdr.biBitCount = 0x10;
    bmi.hdr.biSizeImage = 0;
    bmi.hdr.biClrUsed = 0x100;
    bmi.hdr.biClrImportant = 0;

    hdc = GetDC(g_mainWindowHandle);

    if (palettized == 0) {
        masks = (DWORD *)((BYTE *)&bmi + (bmi.hdr.biSize & 0xffff));
        if (g_is565 == 0) {
            masks[0] = 0x7c00;
            masks[1] = 0x3e0;
        } else {
            masks[0] = 0xf800;
            masks[1] = 0x7e0;
        }
        masks[2] = 0x1f;
    } else {
        for (i = 0; i < 0x100; i++)
            bmi.colors.indices[i] = (WORD)i;
        SelectPalette(hdc, (HPALETTE)palette, 0);
    }

    hbm = CreateDIBSection(hdc, (BITMAPINFO *)&bmi, (palettized != 0) ? DIB_PAL_COLORS : DIB_RGB_COLORS, ppBits, nullptr, 0);
    GetLastError();   // result discarded
    ReleaseDC(g_mainWindowHandle, hdc);
    return hbm;
}
HOOK_METHOD(0x0047e2f0, DDrawDisplay, CreateTopDownDib16);
