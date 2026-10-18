// CRegionManager: table of 0x2000 region pointers loaded from regions.mul.
// `this` of the original methods is the table itself. Layout from
// decomp/pass2/0x0046b1d0.cpp and 0x0046ba20.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

// One region record, operator new(0x66).
struct CRegion {
    // 0x0046aff0: zero every field (byte +0x63 is left alone); returns this.
    CRegion *__thiscall Ctor();
    // 0x0046b040  RET 4: copy the name (strncpy 0x28), the six rect shorts, name2 and t1..t5 from another record.
    CRegion *__thiscall CopyFrom(const CRegion *src);
    // 0x0046b0d0  RET 4: same from a 0x6b-byte packed on-disk record (the id word at +0x28 and the music dword are skipped).
    CRegion *__thiscall CopyFromDisk(const void *src);

    char name[0x28];        // +0x00
    int16_t rect[6];        // +0x28  six shorts, read in file order
    char name2[0x28];       // +0x34
    int16_t t1;             // +0x5c
    int16_t t2;             // +0x5e
    int16_t t3;             // +0x60
    uint8_t t4;             // +0x62
    int16_t t5;             // +0x64
};

static_assert(offsetof(CRegion, rect) == 0x28, "CRegion::rect");
static_assert(offsetof(CRegion, name2) == 0x34, "CRegion::name2");
static_assert(offsetof(CRegion, t1) == 0x5c, "CRegion::t1");
static_assert(offsetof(CRegion, t3) == 0x60, "CRegion::t3");
static_assert(offsetof(CRegion, t4) == 0x62, "CRegion::t4");
static_assert(offsetof(CRegion, t5) == 0x64, "CRegion::t5");
static_assert(sizeof(CRegion) == 0x66, "CRegion size");

class CRegionManager {
public:
    // 0x0046b160  size 0x64  constructor (static initialiser 0x0044acd0): clear the table, loadRegions, then find the
    // ".goto" command in the dot-command table at 0x004a8c20 and store its index in gotoIndex (-1 when absent). Returns this.
    CRegionManager *__thiscall Ctor();
    // 0x0046b1d0  read regions.mul (three on-disk formats) into the table
    void __thiscall loadRegions();
    // 0x0046ba20  open regions.mul for writing, stage every record, close (nothing is written; see the .cpp)
    void __thiscall writeRegionsToDisk();
    // 0x0046bb80  RET 8: replace table[id] (id 0..0x2000 accepted) with a copy of `src` ('_' in its name turned into
    // ' ' first; an empty name only clears the slot), then call writeRegionsToDisk.
    void __thiscall SetRegion(int id, CRegion *src);

    CRegion *regions[0x2000];   // +0x00
    int32_t gotoIndex;          // +0x10000  index of "goto" in the dot-command table, -1 when absent (set by Ctor)
};

static_assert(offsetof(CRegionManager, regions) == 0x00, "CRegionManager::regions");
