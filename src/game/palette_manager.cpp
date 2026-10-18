// CPaletteManager (palette.mul, gray table), the 16-bit colour pack / unpack helpers and
// animinfo.mul loading. gumps.cxx, 0x00436870..0x00436f70.
//
// The ctor/dtor have no SEH frame in the original.
#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "batch02_helpers.h"
#include "globals.h"
#include "crt.h"
#include "palette_manager.h"
#include "vtables.h"

// Pixel format shifts of the display: red at redShift, green at greenShift, blue at 0.
// 0x004bab38: 1000 two-byte records of animinfo.mul (default 4, 2).
#define PM_animInfoEnd GEND(PM_animInfo)

// 0x00436870  size 0x3e
// Pack 8-bit r, g, b into a 16-bit pixel of the display format.
int __stdcall PackRGB16(int r, int g, int b)
{
    int greenPart = ((g & 0xff) >> 3) << (g_greenShift16 & 31);
    int redPart = ((r & 0xff) >> 3) << (g_redShift16 & 31);
    int bluePart = (b & 0xff) >> 3;
    return greenPart | redPart | bluePart;
}
HOOK_FUNC(0x00436870, PackRGB16);

// 0x004368b0  size 0x3d
// Expand a 16-bit pixel to 0x00BBGGRR (each channel (x & 0x1f) << 3).
unsigned __stdcall UnpackRGB16(unsigned pixel)
{
    unsigned dx = (pixel & 0xffff) >> (g_greenShift16 & 31);
    uint8_t green = static_cast<uint8_t>(static_cast<uint8_t>(dx) << 3);
    uint8_t blue = static_cast<uint8_t>(static_cast<uint8_t>(pixel) << 3);
    unsigned ax = (pixel & 0xffff) >> (g_redShift16 & 31);
    uint8_t red = static_cast<uint8_t>(static_cast<uint8_t>(ax) << 3);
    unsigned bx = static_cast<unsigned>(green) | (static_cast<unsigned>(blue) << 8);
    return static_cast<unsigned>(red) | (bx << 8);
}
HOOK_FUNC(0x004368b0, UnpackRGB16);

// 0x00436ef0  size 0x79
// Read animinfo.mul (two bytes per record, 1000 records) into 0x004bab38; without the file every
// record is {4, 2}.
static void LoadAnimInfoMul()
{
    FILE *f = fopen(g_app.ResolveDataPath(const_cast<char *>("animinfo.mul")), g_fopenReadMode);   // 0x0048ad40
    if (f == nullptr) {
        uint8_t *p = PM_animInfo;
        do {
            p[-1] = 4;
            p[0] = 2;
            p += 2;
        } while (reinterpret_cast<intptr_t>(p) < static_cast<intptr_t>(PM_animInfoEnd));
        return;
    }
    uint8_t *p = PM_animInfo;
    do {
        fread(p - 1, 1, 1, f);      // 0x0048ab80
        fread(p, 1, 1, f);          // 0x0048ab80
        p += 2;
    } while (reinterpret_cast<intptr_t>(p) < static_cast<intptr_t>(PM_animInfoEnd));
    fclose(f);                      // 0x0048aad0
}
HOOK_FUNC(0x00436ef0, LoadAnimInfoMul);

// 0x004368f0  size 0x1e9
// Reads palette.mul, notes whether the desktop is 8 bits per pixel (then builds a GDI palette
// from it), fills the gray table and loads animinfo.mul. Returns this.
CPaletteManager::CPaletteManager()
{
    HWND desktop;
    HDC dc;
    int bits, planes;

    FILE *f = fopen(g_app.ResolveDataPath(const_cast<char *>("palette.mul")), g_fopenReadMode);   // 0x0048ad40, 0x0044c340
    if (f != nullptr) {
        fread(palette, 0x300, 1, f);        // 0x0048ab80
        fclose(f);                          // 0x0048aad0
    }

    desktop = GetDesktopWindow();
    dc = GetDC(desktop);
    bits = GetDeviceCaps(dc, BITSPIXEL);
    planes = GetDeviceCaps(dc, PLANES);
    if (bits * planes == 8)
        paletteDisplay = 1;
    else
        paletteDisplay = 0;
    ReleaseDC(desktop, dc);
    hPalette = 0;

    if (paletteDisplay != 0) {
        uint8_t *lp = reinterpret_cast<uint8_t *>(FrameData_Alloc(g_frameDataAllocator, 0x404, "Logical Palette"));   // 0x004462f0
        logPalette = lp;
        *reinterpret_cast<uint16_t *>(lp) = 0x300;
        *reinterpret_cast<uint16_t *>(lp + 2) = 0x100;
        const uint8_t *src = palette;
        for (int e = 0; e < 0x400; e += 4) {
            lp[e + 4] = src[0];
            lp[e + 5] = src[1];
            lp[e + 6] = src[2];
            lp[e + 7] = 0;
            src += 3;
        }
        hPalette = static_cast<int32_t>(reinterpret_cast<uintptr_t>(CreatePalette(static_cast<LOGPALETTE *>(logPalette))));
    }

    unk_04 = 0;
    for (int i = 1; i < 0x8000; i++) {
        int r = i >> g_redShift16;
        int g = (i >> g_greenShift16) & 0x1f;
        int b = i & 0x1f;
        int total = (r * 67) * 8 + (g * 13) * 8 + (b * 5) * 32;
        int hi = static_cast<int>((static_cast<int64_t>(total) * 0x51eb851f) >> 32);   // divide by 100
        int v = hi >> 5;
        v += static_cast<int>(static_cast<unsigned>(v) >> 31);
        v >>= 3;
        int pixel = (v << (g_redShift16 & 31)) | (v << (g_greenShift16 & 31)) | v;
        grayTable[i - 1] = static_cast<uint16_t>(pixel);
    }
    unk_10008 = 0;
    unk_10320 = 0;
    LoadAnimInfoMul();                           // 0x00436ef0
}
HOOK_CTOR(0x004368f0, CPaletteManager, ());

inline CPaletteManager::~CPaletteManager()
{
    if (hPalette != 0) {
        DeleteObject(reinterpret_cast<HGDIOBJ>(static_cast<uintptr_t>(static_cast<uint32_t>(hPalette))));
        hPalette = 0;
    }
}
HOOK_VTABLE(0x0049ebc4, CPaletteManager);
HOOK_SDD(0x00436ae0, CPaletteManager);   // slot 0

// 0x00436b20  size 0x26
// The destructor as the atexit thunk 0x0044aa60 jumps to it (ECX = the palette manager): deletes the GDI palette. A plain
// function that expands the inline destructor.
void __fastcall PaletteManager_Dtor_00436b20(void *self, void *)
{
    reinterpret_cast<CPaletteManager *>(self)->CPaletteManager::~CPaletteManager();
}
HOOK_FUNC(0x00436b20, PaletteManager_Dtor_00436b20);
