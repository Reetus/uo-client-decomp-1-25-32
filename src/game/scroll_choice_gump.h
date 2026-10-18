// CScrollChoiceGump: scrolling list of named choices drawn into frame0 (hover highlight,
// three selection marks). Fields are from the assembly of 0x0041d620 .. 0x0041d830 only;
// the constructor and the other methods are not in this port.
// The class name is the first-pass proposal (decomp/merged chunk_05), not from a string.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"
#include "input_control_gump.h"

class CScrollChoiceGump : public CGump {
public:
    // Gump prep P5: own copy of a slot body the original shares between gumps.
    virtual void Draw(int *clip);
    virtual void MouseUp(int x, int y);
    virtual void MouseMove(int x, int y);
    // Gump prep P5: slot target formerly a free function (vtable_stubs_u2.cpp).
    virtual void MouseLeave();
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual int HitTest(int x, int y);
    // 0x0041d620: RET 8, (x, y) relative mouse position; updates the hover row.
    virtual void MouseOver(int x, int y);
    // 0x0041d700: plain RET. Double-click timer / click counter.
    virtual void Tick();
    // 0x0041d780: RET 0xc, (pos, range, unused) scrollbar callback.
    virtual void OnScroll(int a, int b, int c);
    // 0x0041d7b0: RET 0xc, (id, name, color).
    void __thiscall AddEntry(int id, const char *entryName, uint16_t color);
    // 0x0041d830: repaint frame0.
    void __thiscall Redraw();

    int32_t unk_94;         // +0x94  set to 1 by Tick_0041d700
    int32_t unk_98;
    int32_t pendingClick;   // +0x9c  tested by Tick_0041d700  // ?
    int32_t unk_a0;
    uint32_t tickStart;     // +0xa4  GetTickCount of the first click
    int32_t waitingDouble;  // +0xa8  1 while waiting for a double click
    int32_t listLeft;       // +0xac  list rectangle: left, top, right, bottom
    int32_t listTop;        // +0xb0
    int32_t listRight;      // +0xb4
    int32_t listBottom;     // +0xb8
    int32_t firstRow;       // +0xbc  first visible entry
    int32_t count;          // +0xc0  number of entries
    int32_t clickCount;     // +0xc4
    int32_t mark[3];        // +0xc8  entry ids that get a mark sprite at x = 0x1a / 0x28 / 0x36
    int32_t ids[100];       // +0xd4
    uint16_t colors[100];   // +0x264  text hue per entry, 0 = default 0x386
    char names[100][0x50];  // +0x32c
    uint8_t pad_226c[0x50]; // +0x226c  title string (the constructors copy a string here; ends at +0x22bc)
    CGump *owner_22bc;      // +0x22bc  ctor argument 1: object notified of the choice (vtable slot 0x44) and closed (slot 0x60)
    CGump *thumb_22c0;      // +0x22c0  scroll thumb (only for art 0x4ca / 0x4cc)
    CInputControlGump *choiceBox_22c4;  // +0x22c4  ctor 0x41c930 argument 2: edit box that receives the choice; 0 = server list
    int32_t serverList_22c8; // +0x22c8  1 for the two-argument constructor
    // 0x0041c930  size 0x3ed  constructor (owner, target edit box, art id). RET 0xc.
    CScrollChoiceGump(CGump *ownerArg, CInputControlGump *choiceBoxArg, int artId);
    // 0x0041d290  destructor (the out-of-line body; the base destructor follows implicitly).
    ~CScrollChoiceGump();
    // 0x0041cd50: RET 8, (owner, artId). Size 0x22cc.
    CScrollChoiceGump(CGump *ownerArg, int artId);
    // 0x0041d290: the destructor body.
    void Dtor() { this->CScrollChoiceGump::~CScrollChoiceGump(); }
    // 0x0041cd20: vtable slot 0, RET 4.
    // 0x0041d440: vtable slot 0x10 (mouse down), RET 8, (x, y).
    virtual void MouseDown(int x, int y);
    // 0x0041d410: vtable slot 0x1c, RET 8, two ignored arguments: close the gump.
    virtual void RButtonUp(int x, int y);
    // 0x0041d4a0: RET 8, (x, y) local position: choose the entry under the point.
    void __thiscall OnLeftClick(int px, int py);
};

static_assert(offsetof(CScrollChoiceGump, pendingClick) == 0x9c, "CScrollChoiceGump::pendingClick");
static_assert(offsetof(CScrollChoiceGump, tickStart) == 0xa4, "CScrollChoiceGump::tickStart");
static_assert(offsetof(CScrollChoiceGump, waitingDouble) == 0xa8, "CScrollChoiceGump::waitingDouble");
static_assert(offsetof(CScrollChoiceGump, listLeft) == 0xac, "CScrollChoiceGump::listLeft");
static_assert(offsetof(CScrollChoiceGump, firstRow) == 0xbc, "CScrollChoiceGump::firstRow");
static_assert(offsetof(CScrollChoiceGump, count) == 0xc0, "CScrollChoiceGump::count");
static_assert(offsetof(CScrollChoiceGump, clickCount) == 0xc4, "CScrollChoiceGump::clickCount");
static_assert(offsetof(CScrollChoiceGump, mark) == 0xc8, "CScrollChoiceGump::mark");
static_assert(offsetof(CScrollChoiceGump, ids) == 0xd4, "CScrollChoiceGump::ids");
static_assert(offsetof(CScrollChoiceGump, colors) == 0x264, "CScrollChoiceGump::colors");
static_assert(offsetof(CScrollChoiceGump, names) == 0x32c, "CScrollChoiceGump::names");
static_assert(offsetof(CScrollChoiceGump, pad_226c) == 0x226c, "CScrollChoiceGump title");
static_assert(offsetof(CScrollChoiceGump, owner_22bc) == 0x22bc, "CScrollChoiceGump::owner_22bc");
static_assert(offsetof(CScrollChoiceGump, thumb_22c0) == 0x22c0, "CScrollChoiceGump::thumb_22c0");
static_assert(offsetof(CScrollChoiceGump, choiceBox_22c4) == 0x22c4, "CScrollChoiceGump::choiceBox_22c4");
static_assert(offsetof(CScrollChoiceGump, serverList_22c8) == 0x22c8, "CScrollChoiceGump::serverList_22c8");
static_assert(sizeof(CScrollChoiceGump) == 0x22cc, "CScrollChoiceGump size (operator new(0x22cc) at the callers)");
