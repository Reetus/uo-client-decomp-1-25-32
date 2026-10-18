// CMultiPlacer (placeholder name): the object that previews a multi (house / boat component list)
// at a map position. The one instance registers itself in the global at 0x004b7180; the loaded multi
// table is the separate heap block pointed to by 0x004b7184. Layout from 0x0040cb00 .. 0x0040ccd0;
// the object is embedded in a larger one (fields reach +0x14c) and its allocation size is not known.
#pragma once

#include <cstddef>
#include <cstdint>

// One component of a loaded multi, 0x10 bytes. Built by CMultiPlacer::LoadMulti from the 12-byte
// records of the multi file (id, x, y, z, flags) with the placement offsets subtracted.
struct MultiEntry {
    int16_t dx;        // +0x00
    int16_t dy;        // +0x02
    int16_t dz;        // +0x04
    uint16_t graphic;  // +0x06
    uint16_t pad_08;   // +0x08  zeroed
    uint8_t pad_0a[2];
    uint8_t pad_0c[4]; // +0x0c  zeroed
    MultiEntry();      // 0x0040cf20, a lone ret
    ~MultiEntry();     // 0x0040caf0, a lone ret
};
static_assert(sizeof(MultiEntry) == 0x10, "MultiEntry size");

// The loaded multi (operator new(0x108)): name "multi", then the entry array (operator new of
// 4 + count * 0x10 bytes; the count is stored in the dword before the first entry).
struct MultiTable {
    char name[0x100];       // +0x00  "multi"
    MultiEntry *entries;    // +0x100
    int32_t count;          // +0x104
    MultiTable() { entries = nullptr; }
};
static_assert(offsetof(MultiTable, entries) == 0x100, "MultiTable::entries");
static_assert(offsetof(MultiTable, count) == 0x104, "MultiTable::count");
static_assert(sizeof(MultiTable) == 0x108, "MultiTable size (operator new(0x108))");

class CMultiPlacer {
public:
    // 0x0040cb00  size 0x39: real constructor, registers in 0x004b7180 and clears the placement state.
    CMultiPlacer();
    // 0x0040cb40: RET 8. Create one object per component of the loaded multi around tile (x, y).
    void __thiscall Place(int x, int y);
    // 0x0040cc70: free the loaded multi table and clear the base height fields.
    void __thiscall FreeTable();
    // 0x0040ccd0: RET 0x10. Load multi `multiId` from the multi files (0xd / 0xe) into a new table, with
    // (offX, offY, offZ) subtracted from every component. Returns 1, or 0 when the multi is missing.
    int __thiscall LoadMulti(int multiId, int16_t offX, int16_t offY, int16_t offZ);

    char unk_000;          // +0x000  cleared by Ctor
    uint8_t pad_001[0xff];
    int32_t unk_100;       // +0x100  cleared by Ctor
    int32_t tileX;         // +0x104  -1 until placed
    int32_t tileY;         // +0x108
    int32_t baseZ;         // +0x10c  ground height at the tile plus +0x144
    int32_t unk_110;       // +0x110  -1 in Ctor
    uint8_t pad_114[0x144 - 0x114];
    int32_t zOffset;       // +0x144
    int32_t curZ;          // +0x148  height of the component being created
    int32_t loaded;        // +0x14c  set to 1 by LoadMulti
};

static_assert(offsetof(CMultiPlacer, unk_100) == 0x100, "CMultiPlacer::unk_100");
static_assert(offsetof(CMultiPlacer, tileX) == 0x104, "CMultiPlacer::tileX");
static_assert(offsetof(CMultiPlacer, baseZ) == 0x10c, "CMultiPlacer::baseZ");
static_assert(offsetof(CMultiPlacer, unk_110) == 0x110, "CMultiPlacer::unk_110");
static_assert(offsetof(CMultiPlacer, zOffset) == 0x144, "CMultiPlacer::zOffset");
static_assert(offsetof(CMultiPlacer, loaded) == 0x14c, "CMultiPlacer::loaded");
static_assert(sizeof(CMultiPlacer) == 0x150, "CMultiPlacer size (operator new(0x150) in WinMain)");
