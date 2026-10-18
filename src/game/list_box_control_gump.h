// CListBoxControlGump: drop-down list used by the client config gump. operator new(0x1244)
// (allocation site 0x004180bb). Vtable 0x0049dad0, name "ListBoxControl gump".
// Layout from decomp/pass2/0x00420090.cpp and 0x00420200.cpp; the row storage between +0xac and +0x1240 is
// not described by this batch.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CListBoxControlGump : public CGump {
public:
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual void Draw(int *clip);
    virtual int HitTest(int x, int y);
    // 0x00420090: real constructor, RET 0x20 (eight stack args). `unused4` / `unused5` are not read by the constructor.
    CListBoxControlGump(int x, int y, unsigned int artId, int unused4, int unused5, int16_t font, int16_t hue,
                        unsigned int rows);
    // 0x00420200: destructor body (vtable 0x0049dad0 is written first).
    ~CListBoxControlGump();
    void Dtor() { this->CListBoxControlGump::~CListBoxControlGump(); }

    // Added by porting pass 2 (batch 07). In the fields below unk_94 is the linked CInputControlGump
    // (32-bit pointer), unk_9c the item count and unk_a8 the first visible row.
    // 0x00420290: RET 4. row == -1 repaints everything, otherwise only that item's row.
    void __thiscall Redraw(int row);
    // 0x00420860: RET 8 (px, py). Left button up: copies the selected item to the linked input control, then deletes the list.
    virtual void MouseUp(int x, int y);
    // 0x00420910: RET 8 (px, py). Mouse move: updates the selected row.
    virtual void MouseMove(int x, int y);
    // 0x004209e0: plain RET. Scrolls by one row when the mouse is above / below the list.
    virtual void Tick();
    // 0x00420a70: RET 8 (value, name). Appends an item and grows / moves the gump.
    void __thiscall AddItem(int itemValue, const char *itemName);

    int32_t unk_94;
    int32_t hasScroll;     // +0x98  1 when rows != 0
    int32_t unk_9c;
    int32_t selected;      // +0xa0  -1 initially
    int32_t rows;          // +0xa4  rows & 0xff
    int32_t unk_a8;
    int32_t values[100];        // +0x0ac  item values
    char names[100][0x29];      // +0x23c  item texts
    uint16_t font;         // +0x1240
    uint16_t hue;          // +0x1242
};

static_assert(offsetof(CListBoxControlGump, hasScroll) == 0x98, "CListBoxControlGump::hasScroll");
static_assert(offsetof(CListBoxControlGump, selected) == 0xa0, "CListBoxControlGump::selected");
static_assert(offsetof(CListBoxControlGump, rows) == 0xa4, "CListBoxControlGump::rows");
static_assert(offsetof(CListBoxControlGump, font) == 0x1240, "CListBoxControlGump::font");
static_assert(offsetof(CListBoxControlGump, hue) == 0x1242, "CListBoxControlGump::hue");
static_assert(sizeof(CListBoxControlGump) == 0x1244, "CListBoxControlGump size (operator new(0x1244))");
static_assert(offsetof(CListBoxControlGump, values) == 0xac, "CListBoxControlGump::values");
static_assert(offsetof(CListBoxControlGump, names) == 0x23c, "CListBoxControlGump::names");
