// Hue table loading: hues.mul, hairhue.txt and the colour group lists. (Porting pass 2, batch 15.)
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "hue_table.h"
#include "crt.h"
#include "hues_ports.h"
#include "globals.h"
#include "raw_field.h"

// The colour converter's component tables (see LoadSkinHues in hues.cpp): index (component >> 3) << 7.

static uint16_t HueTableWord(uint8_t *table, unsigned offset)
{
    uint16_t v;
    memcpy(&v, table + offset, sizeof(v));
    return v;
}

// 0x00441190  size 0xf8
// Load hairhue.txt: lines "(%d,%d,%d)\n" become hue entries 0x44d.. named "HairHue #%d". An all-zero line only
// advances the group counter at 0x005284b0 (group starts are kept in the array at 0x005284b4).
// ? v0..v2 are uninitialised in the original when the first fscanf fails; zero here.
static void LoadHairHues()
{
    char name[0x50];
    int v0 = 0;
    int v1 = 0;
    int v2 = 0;
    int hueId = 0x44d;

    FILE *fp = fopen(g_app.ResolveDataPath(const_cast<char *>("hairhue.txt")), "rt");
    if (fp == nullptr)
        return;

    g_005284b0 = 1;
    g_005284b8 = 0;

    do {
        fscanf(fp, "(%d,%d,%d)\n", &v0, &v1, &v2);

        if (v0 == 0 && v1 == 0 && v2 == 0) {
            g_005284b0 = g_005284b0 + 1;
            *reinterpret_cast<int *>(g_005284b0 * 4 + GADDR(g_skinHueCount)) = hueId - 0x44d;
        } else {
            uint16_t color = HueTableWord(g_shadeTableR, static_cast<unsigned>((v0 >> 3) << 7));
            color |= HueTableWord(g_shadeTableG, static_cast<unsigned>((v1 >> 3) << 7));
            color |= HueTableWord(g_shadeTableB, static_cast<unsigned>((v2 >> 3) << 7));
            sprintf(name, "HairHue #%d", hueId);
            g_hueTable.FillEntry(static_cast<unsigned>(hueId), 1, color, name);
            hueId = hueId + 1;
        }
    } while ((reinterpret_cast<uint8_t *>(fp)[0xc] & 0x10) == 0);   // !feof (original FILE flag _IOEOF)

    fclose(fp);
}
HOOK_FUNC(0x00441190, LoadHairHues);

// 0x00441290  size 0xd3
// ECX = file name, EDX = first hue id (16 bits), stack = label, RET 4. Every line "(%d,%d,%d)\n" of the file
// becomes the next hue entry, named "<label> #<n>" with n counted from 0. Unlike LoadHairHues there is no
// all-zero special case.
// ? v0..v2 are uninitialised in the original when the first fscanf fails; zero here.
static void __fastcall LoadHueGroupFile(const char *fileName, uint16_t firstId, const char *label)
{
    char name[0x50];
    int v0;
    int v1;
    int v2;
    int hueId = firstId;

    FILE *fp = fopen(g_app.ResolveDataPath(const_cast<char *>(fileName)), "rt");
    if (fp == nullptr)
        return;

    do {
        fscanf(fp, "(%d,%d,%d)\n", &v0, &v1, &v2);

        int r = v0 >> 3;
        int b = v2 >> 3;
        int g = v1 >> 3;
        uint16_t color = RAWF(uint16_t, g_shadeTableR, r << 7);
        color |= RAWF(uint16_t, g_shadeTableB, b << 7);
        color |= RAWF(uint16_t, g_shadeTableG, g << 7);
        sprintf(name, "%s #%d", label, hueId - firstId);
        g_hueTable.FillEntry(static_cast<unsigned>(hueId), 1, color, name);
        hueId = hueId + 1;
    } while ((reinterpret_cast<uint8_t *>(fp)[0xc] & 0x10) == 0);

    fclose(fp);
}
HOOK_FUNC(0x00441290, LoadHueGroupFile);

// 0x00441370  size 0x36d
// Load hues.mul: 3000 entries, a 4-byte header before every eighth, each entry 32 colours, start, end and a 0x14-byte
// name. A colour of 0 in an entry that has a start or end colour becomes 1. When the file cannot be opened for
// reading, it is created blank ("Blank Color" entries); when that fails too the table is blanked in memory.
// Then skinhue.txt (0x00441060), hairhue.txt (0x00441190) and the group lists, and finally every colour outside
// 1..0x7fff is set to 1.
void __thiscall CHueTable::LoadHues()
{
    HueEntry *entry = reinterpret_cast<HueEntry *>(this);
    FILE *fp = fopen(g_app.ResolveDataPath(const_cast<char *>("hues.mul")), "rb");

    if (fp != nullptr) {
        for (int i = 0; i < 3000; i++) {
            HueEntry *e = &entry[i];
            if ((i & 7) == 0)
                fseek(fp, 4, 1);
            for (int j = 0; j < 0x20; j++)
                fread(&e->colors[j], 2, 1, fp);
            fread(&e->startColor, 2, 1, fp);
            fread(&e->endColor, 2, 1, fp);
            fread(e->name, 0x14, 1, fp);
            for (int j = 0; j < 0x20; j++) {
                if ((e->startColor != 0 || e->endColor != 0) && e->colors[j] == 0)
                    e->colors[j] = 1;
            }
        }
        fclose(fp);
    } else {
        fp = fopen(g_app.ResolveDataPath(const_cast<char *>("hues.mul")), "wb");
        if (fp != nullptr) {
            for (int i = 0; i < 3000; i++) {
                HueEntry *e = &entry[i];
                if ((i & 7) == 0) {
                    int32_t header = 0;
                    fwrite(&header, 4, 1, fp);
                }
                strcpy(e->name, "Blank Color");
                e->startColor = 0;
                e->endColor = 0;
                for (int j = 0; j < 0x20; j++) {
                    e->colors[j] = 0;
                    fwrite(&e->colors[j], 2, 1, fp);
                }
                fwrite(&e->startColor, 2, 1, fp);
                fwrite(&e->endColor, 2, 1, fp);
                fwrite(e->name, 0x14, 1, fp);
            }
            fclose(fp);
        } else {
            for (int i = 0; i < 3000; i++) {
                HueEntry *e = &entry[i];
                strcpy(e->name, "Blank Color");
                e->startColor = 0;
                e->endColor = 0;
                memset(e->colors, 0, sizeof(e->colors));
            }
        }
    }

    LoadSkinHues();   // LoadSkinHues (hues.cpp)
    LoadHairHues();
    LoadHueGroupFile("pinkpur.txt", 0x4b0, "Pinks");
    LoadHueGroupFile("blues.txt", 0x514, "Blues");
    LoadHueGroupFile("greens.txt", 0x578, "Greens");
    LoadHueGroupFile("oranges.txt", 0x5dc, "Oranges");
    LoadHueGroupFile("reds.txt", 0x640, "Reds");
    LoadHueGroupFile("yellows.txt", 0x6a4, "Yellows");
    LoadHueGroupFile("neutrals.txt", 0x708, "Neutrals");
    LoadHueGroupFile("snakes.txt", 0x7d0, "Snakes");
    LoadHueGroupFile("birds.txt", 0x834, "Birds");
    LoadHueGroupFile("slimes.txt", 0x898, "Slimes");
    LoadHueGroupFile("animals.txt", 0x8fc, "Animals");
    LoadHueGroupFile("metals.txt", 0x960, "Metals");

    for (int i = 0; i < 3000; i++) {
        for (int j = 0; j < 0x20; j++) {
            uint16_t c = entry[i].colors[j];
            if (c == 0 || c > 0x7fff)
                entry[i].colors[j] = 1;
        }
    }
}
HOOK_METHOD(0x00441370, CHueTable, LoadHues);
