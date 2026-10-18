// CTutorialGump: the character creation tutorial pages (vtable 0x0049e4a8, name "tutorial gump").
// Layout from decomp/merged/chunk_07_00422a10.cpp (constructor 0x00423f10, not ported here) and the
// page handlers 0x00412490 / 0x004125e0. Full screen (0x280 x 0x1e0); +0x98 is the page index into the
// file name table at 0x004a6eb0.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CTutorialGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual void Draw(int *clip);
    virtual int HitTest(int x, int y);
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    // 0x00412490: go to the next page that loads; delete the gump when none is left.
    void __thiscall NextPage();
    // 0x004125e0: go to the previous page that loads; delete the gump when none is left and re-show the city gump.
    void __thiscall PrevPage();

    // Added by porting pass 2 (batch 08).
    // 0x00423f10  size 0x2d3  constructor, RET 4 (pageIndex) (allocation 0x9c).
    CTutorialGump(int page);
    // 0x00424220: destructor (frees the frame bitmap and table, then the base).
    ~CTutorialGump();
    void Dtor() { this->CTutorialGump::~CTutorialGump(); }

    int32_t unk_94;
    int32_t pageIndex;     // +0x98  index into the table at 0x004a6eb0
};

static_assert(offsetof(CTutorialGump, pageIndex) == 0x98, "CTutorialGump::pageIndex");
static_assert(sizeof(CTutorialGump) == 0x9c, "CTutorialGump size (operator new(0x9c))");
