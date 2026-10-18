// CEditListGump: scrolling list of text rows used by the client config gump (tabs 3 and 4).
// operator new(0xdc); built by 0x004169f0 (x, y, 0, 0, art, rows) or 0x00416740 (x, y, 0, 0, art, a6, a7);
// rows are added with 0x00416d40 (name, value). The class name is a guess
// (decomp/pass2/0x00413c70.cpp calls them "edit lists"). Layout is partial.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"
#include "button_gump.h"
#include "input_control_gump.h"

// Node of the row list: operator new(0xc); `row` is the text entry gump, whose +0x154 points back to its node.
struct CEditListNode {
    CInputControlGump *row;   // +0x00
    CEditListNode *next;      // +0x04
    CEditListNode *prev;      // +0x08
};
static_assert(sizeof(CEditListNode) == 0xc, "CEditListNode size (operator new(0xc))");

class CEditListGump : public CGump {
public:
    virtual int IsTag100() { return 1; }
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual int HitTest(int x, int y);
    virtual void RButtonUp(int x, int y);
    // 0x00416740: RET 0x1c, (x, y, a3, a4, artId, maxCount, font); a3 / a4 are not read. Not editable (+0xb4 = 0).
    CEditListGump(int x, int y, int a3, int a4, int artId, int maxCount, int font);
    // 0x004169f0: RET 0x18, (x, y, a3, a4, artId, maxCount); font 0x1d, editable (+0xb4 = 1).
    CEditListGump(int x, int y, int a3, int a4, int artId, int maxCount);
    // 0x004169c0: vtable slot 0, RET 4.
    // 0x00416c70: destructor body.
    ~CEditListGump();
    void Dtor() { this->CEditListGump::~CEditListGump(); }
    // 0x00416d40: RET 8, append a row.
    void __thiscall AddItem(const char *text, int value);
    // 0x00417100: RET 4, remove a row.
    void __thiscall RemoveItem(CEditListNode *node);
    // 0x00417290: restore the selected row's hue and drop the selection.
    void __thiscall ClearSelection();
    // 0x004172d0: vtable slot 0x44, RET 4, a row's text changed.
    virtual void OnChildChanged(int child);
    // 0x00417440: RET 4, select the row and focus it (vtable slot 0x54).
    virtual void OnFocus(int field);
    // 0x004174e0: RET 4, key handler (vtable slot 0x3c).
    virtual void OnKey(int key);
    // 0x00417520: vtable slot 0x48, focus the row after the focused one.
    virtual void OnTab();
    // 0x004175b0 / 0x00417610 / 0x00417660: vtable slots 0x10 / 0x14 / 0x28, RET 8.
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void MouseMove(int x, int y);
    // 0x004176f0: vtable slot 0x2c, double-click timeout.
    virtual void Tick();
    // 0x00417740: repaint frame0 from the art.
    void __thiscall RedrawBackground();
    // 0x004177d0: vtable slot 0x08, RET 4.
    virtual void Draw(int *clip);

    CButtonGump *child_94;           // +0x94  scroll up button (art 0x983), hidden by default; the config gump shifts its x by -6
    CButtonGump *child_98;           // +0x98  scroll down button (art 0x985); the config gump shifts its x by -6
    int32_t dragging;                // +0x9c  press moved more than 4 pixels
    int32_t unk_a0;
    int32_t pressed;                 // +0xa4  button is down
    int32_t unk_a8;
    int32_t pressTick;               // +0xac  GetTickCount() of the first press
    int32_t clickState;              // +0xb0  0 none, 1 first press seen, 2 second press
    int32_t editable;                // +0xb4  1: trailing "(new)" row, Delete removes rows
    CEditListNode *head;             // +0xb8  first row node
    CInputControlGump **rows_bc;     // +0xbc  tail row node (a CEditListNode *; its first member is the row, so *rows_bc is the last row)
    CEditListNode *cur_c0;           // +0xc0  node searched by SelectRow
    CEditListNode *selected;         // +0xc4  selected node or 0
    int16_t savedHue;                // +0xc8  hue of the selected row before it was highlighted
    int16_t count_ca;                // +0xca  x of the rows (10 at construction; set to 4 / 0xc by the config gump)
    int32_t count;                   // +0xcc  rows added
    int32_t maxCount;                // +0xd0  a6
    int32_t visibleRows;             // +0xd4  0 at construction
    int32_t font;                    // +0xd8  font of the rows: ctor argument, 0x1d for the editable form

    CEditListNode *tail() const { return reinterpret_cast<CEditListNode *>(rows_bc); }
};

static_assert(offsetof(CEditListGump, child_94) == 0x94, "CEditListGump::child_94");
static_assert(offsetof(CEditListGump, child_98) == 0x98, "CEditListGump::child_98");
static_assert(offsetof(CEditListGump, dragging) == 0x9c, "CEditListGump::dragging");
static_assert(offsetof(CEditListGump, pressed) == 0xa4, "CEditListGump::pressed");
static_assert(offsetof(CEditListGump, pressTick) == 0xac, "CEditListGump::pressTick");
static_assert(offsetof(CEditListGump, clickState) == 0xb0, "CEditListGump::clickState");
static_assert(offsetof(CEditListGump, editable) == 0xb4, "CEditListGump::editable");
static_assert(offsetof(CEditListGump, head) == 0xb8, "CEditListGump::head");
static_assert(offsetof(CEditListGump, rows_bc) == 0xbc, "CEditListGump::rows_bc");
static_assert(offsetof(CEditListGump, cur_c0) == 0xc0, "CEditListGump::cur_c0");
static_assert(offsetof(CEditListGump, selected) == 0xc4, "CEditListGump::selected");
static_assert(offsetof(CEditListGump, savedHue) == 0xc8, "CEditListGump::savedHue");
static_assert(offsetof(CEditListGump, count_ca) == 0xca, "CEditListGump::count_ca");
static_assert(offsetof(CEditListGump, count) == 0xcc, "CEditListGump::count");
static_assert(offsetof(CEditListGump, maxCount) == 0xd0, "CEditListGump::maxCount");
static_assert(offsetof(CEditListGump, visibleRows) == 0xd4, "CEditListGump::visibleRows");
static_assert(offsetof(CEditListGump, font) == 0xd8, "CEditListGump::font");
static_assert(sizeof(CEditListGump) == 0xdc, "CEditListGump size (operator new(0xdc))");
