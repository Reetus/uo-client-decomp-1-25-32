#include <cstdint>

#include "../runtime/ported.h"
#include <cstdio>
#include <cstring>

#include "app.h"
#include "globals.h"
#include "hue_table.h"
#include "crt.h"
#include "raw_field.h"
#include "palette_manager_ports.h"

// 0x0047e700  size 0x10c
// Copy 16-bit sprite pixels, skipping zero (transparent) pixels and remapping
// gray pixels through the hue table at 0x006a3ff8. dst in ECX, src in EDX,
// count and hueIndex on the stack. See decomp/pass2/0x0047e700.cpp.
void __fastcall HueCopyPixels(uint16_t *dst, uint16_t *src, int count, int hueIndex)
{
    const int shiftR = g_redShift16;
    const int shiftG = g_greenShift16;
    const uint16_t *hueTable = GLOBAL_PTR(uint16_t, g_hueTable_storage);
    uint16_t p;

    if (g_is565 != 0) {   // 565 surface
        if (count == 0)
            return;
        do {
            p = *src++;
            if (p != 0) {
                uint16_t a = (p >> (shiftR & 0x1f)) & 0x1f;
                uint16_t b = (p >> (shiftG & 0x1f)) & 0x1f;
                if (a == b && a == (p & 0x1f))
                    p = hueTable[(p >> (shiftR & 0x1f)) + hueIndex * 0x2c];
                *dst = p;
            }
            dst++;
        } while (--count != 0);
        return;
    }

    if (count == 0)                          // 555 surface
        return;
    do {
        p = *src++;
        if (p != 0) {
            if ((uint16_t)((p >> (shiftG & 0x1f)) ^ (p & 0x3ff)) == 0)
                *dst = hueTable[(p >> (shiftR & 0x1f)) + hueIndex * 0x2c];
            else
                *dst = p;
        }
        dst++;
    } while (--count != 0);
}
HOOK_FUNC(0x0047e700, HueCopyPixels);

// ---------------------------------------------------------------------------
// Skin hues (batch 03).
// ---------------------------------------------------------------------------

// The colour converter object at 0x0068dd98.
#define g_colorConv GLOBAL_PTR(void, g_clientState_storage)
// 0x004368b0: 16-bit colour -> 0x00BBGGRR.
static unsigned int color16_to_rgb(int color) { return PORT_THISCALL1(&UnpackRGB16, unsigned int, g_colorConv, int)(color); }
// 0x00436870: (r, g, b) -> 16-bit colour (result in AX).
static uint16_t rgb_to_color16(int r, int g, int b) { return (uint16_t)PORT_THISCALL3(&PackRGB16, int, g_colorConv, int, int, int)(r, g, b); }

// 0x004416e0  size 0x164
// Fill one hue entry: 32 colours interpolated from colour a to colour b, plus
// the name. delta = (end - start) * 0.03125 per channel (double 1/32 at 0x0049f408).
void __thiscall CHueTable::FillEntry(unsigned int idx, int startColor, int endColor, const char *name)
{
    HueEntry *entry = reinterpret_cast<HueEntry *>(this) + (idx & 0xffff);
    uint16_t *out = entry->colors;

    strcpy(entry->name, name);
    entry->startColor = (uint16_t)startColor;
    entry->endColor = (uint16_t)endColor;

    unsigned int c0 = color16_to_rgb(startColor);
    double r = (double)(int)(c0 & 0xff);
    double g = (double)(int)((c0 >> 8) & 0xff);
    double b = (double)(int)((c0 >> 16) & 0xff);

    unsigned int c1 = color16_to_rgb(endColor);
    double dr = ((double)(int)(c1 & 0xff) - r) * 0.03125;
    double dg = ((double)(int)((c1 >> 8) & 0xff) - g) * 0.03125;
    double db = ((double)(int)((c1 >> 16) & 0xff) - b) * 0.03125;

    for (int i = 0x20; i != 0; i--) {
        int ib = (int)b;   // __ftol
        int ig = (int)g;
        int ir = (int)r;
        *out = rgb_to_color16(ir, ig, ib);
        r = r + dr;
        g = g + dg;
        b = b + db;
        out = out + 1;
    }
}
HOOK_METHOD(0x004416e0, CHueTable, FillEntry);

// 0x00441060  size 0x123
// Load skinhue.txt: lines "(%d,%d,%d)\n", build 16-bit skin colours and
// register hues 1001... An all-zero line advances the group counter only (it
// does not advance hueId or the 0x00527c66 cursor).
void LoadSkinHues()
{
    char name[0x50];
    // ? v0..v2 are uninitialised in the original when the first fscanf fails; zero here.
    int v2;
    int v0;
    int v1;
    int hueId = 0x3e9;
    int tblOff = 0x7d2;   // byte offset into the ushort table at 0x00527c66

    FILE *fp = fopen(g_app.ResolveDataPath(const_cast<char *>("skinhue.txt")), "rt");
    if (fp == nullptr)
        return;

    g_skinHueCount = 1;
    g_skinHueTable[0] = 0;

    do {
        fscanf(fp, "(%d,%d,%d)\n", &v0, &v1, &v2);

        if (v0 == 0 && v1 == 0 && v2 == 0) {
            g_skinHueCount = g_skinHueCount + 1;
            *reinterpret_cast<int *>(g_skinHueCount * 4 + GADDR(dat_005283ac)) = hueId - 0x3e9;
        } else {
            RAWF(uint16_t, g_00527c66, tblOff) = rgb_to_color16(v0, v1, v2);

            uint16_t color = reinterpret_cast<uint16_t (*)[64]>(g_shadeTableG)[v1 >> 3][0];
            color |= reinterpret_cast<uint16_t (*)[64]>(g_shadeTableB)[v2 >> 3][0];
            color |= reinterpret_cast<uint16_t (*)[64]>(g_shadeTableR)[v0 >> 3][0];

            sprintf(name, "SkinHue #%d", hueId);
            g_hueTable.FillEntry((unsigned)hueId, 1, color, name);

            hueId = hueId + 1;
            tblOff = tblOff + 2;
        }
    } while ((RAWF(uint8_t, fp, 0xc) & 0x10) == 0);   // !feof (original FILE flag _IOEOF)

    fclose(fp);
}
HOOK_FUNC(0x00441060, LoadSkinHues);
