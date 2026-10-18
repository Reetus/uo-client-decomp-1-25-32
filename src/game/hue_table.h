// CHueTable: the table of 32-colour hue ramps at 0x006a3ff8, 0x58 bytes per
// entry (HueCopyPixels in hues.cpp indexes it as ushort [hue * 0x2c + gray]).
// Layout from decomp/pass2/0x004416e0.cpp. Table size unknown.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"
#include "globals.h"

struct HueEntry {
    uint16_t colors[0x20];   // +0x00  16-bit colours, interpolated start -> end
    uint16_t startColor;     // +0x40
    uint16_t endColor;       // +0x42
    char name[0x14];         // +0x44  ? length of the name buffer is inferred from the entry size
};
static_assert(sizeof(HueEntry) == 0x58, "HueEntry size");
static_assert(offsetof(HueEntry, startColor) == 0x40, "HueEntry::startColor");
static_assert(offsetof(HueEntry, endColor) == 0x42, "HueEntry::endColor");
static_assert(offsetof(HueEntry, name) == 0x44, "HueEntry::name");

class CHueTable {
public:
    // 0x004416e0: fill one entry: 32 colours interpolated from startColor to endColor, plus name.
    void __thiscall FillEntry(unsigned int idx, int startColor, int endColor, const char *name);
    // 0x00441370: load hues.mul (3000 entries; written blank when it is missing), then skinhue.txt, hairhue.txt and
    // the twelve colour-group lists, and clamp every colour to 1..0x7fff.
    void __thiscall LoadHues();
};

#define g_hueTable GLOBAL_AS(CHueTable, g_hueTable_storage)
