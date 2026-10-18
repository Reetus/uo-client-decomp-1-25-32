// CMacroCmdGump ("macro cmd gump", vtable 0x0049d008) and CMacroListGump ("macro list gump", vtable 0x0049d120):
// the macro editor rows of the client config gump (tab 2). operator new(0xac) / operator new(0xd0).
// Layout from the constructors, destructors and callers in 0x00417890..0x00418b10. Class names are the
// strings the constructors store; the field names are guesses from use.
#pragma once

#include <cstddef>
#include <cstdint>

#include "button_gump.h"
#include "gump.h"
#include "input_control_gump.h"

class CMacroListGump;

class CMacroCmdGump : public CGump {
public:
    virtual int IsTagd8() { return 1; }
    // Gump prep P5: own copy of a slot body the original shares between gumps.
    virtual void Draw(int *clip);
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual int HitTest(int x, int y);
    // 0x00417890: RET 0x24, (x, y, w, h, cmdType, cmdArg, text, val1, val2).
    CMacroCmdGump(int x, int y, int w, int h, int cmdType, int cmdArg, const char *text, int val1, int val2);
    // 0x00417c10: vtable slot 0, RET 4.
    // 0x00417c40: destructor body.
    ~CMacroCmdGump();
    void Dtor() { this->CMacroCmdGump::~CMacroCmdGump(); }
    // 0x00417cd0: vtable slot 0x44, RET 4.
    virtual void OnChildChanged(int child);
    // 0x00417d30: vtable slot 0x1c, RET 8; forwards (0, 0) to the same slot of the parent.
    virtual void RButtonUp(int x, int y);
    // 0x00417d50 / 0x00417d70: vtable slots 0x10 / 0x14, RET 8.
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    // 0x00417d90: delete and recreate the argument widgets for the selected command type.
    void __thiscall RebuildArgWidgets();
    // 0x00418070: RET 4, open the drop-down list under `anchor` (the type box or the argument box).
    virtual void Slot64(int a);

    int32_t pressed;                // +0x94  mouse button is down on the row
    CInputControlGump *typeBox;     // +0x98  command type; value = type 0..0x16, 0x63 = none (delete row)
    CInputControlGump *argList;     // +0x9c  argument picked from a list (types 5, 8-11, 13, 15)
    CInputControlGump *argText;     // +0xa0  free text argument (types 1-4, 22)
    CInputControlGump *val1Box;     // +0xa4  never created by the code in this batch (stays 0)
    CInputControlGump *val2Box;     // +0xa8  never created by the code in this batch (stays 0)
};

static_assert(offsetof(CMacroCmdGump, pressed) == 0x94, "CMacroCmdGump::pressed");
static_assert(offsetof(CMacroCmdGump, typeBox) == 0x98, "CMacroCmdGump::typeBox");
static_assert(offsetof(CMacroCmdGump, argList) == 0x9c, "CMacroCmdGump::argList");
static_assert(offsetof(CMacroCmdGump, argText) == 0xa0, "CMacroCmdGump::argText");
static_assert(offsetof(CMacroCmdGump, val1Box) == 0xa4, "CMacroCmdGump::val1Box");
static_assert(offsetof(CMacroCmdGump, val2Box) == 0xa8, "CMacroCmdGump::val2Box");
static_assert(sizeof(CMacroCmdGump) == 0xac, "CMacroCmdGump size (operator new(0xac))");

// Node of the (unused) row list at +0xb4: operator delete'd with its row by the destructor.
struct CMacroListNode {
    CGump *row;                 // +0x00
    CMacroListNode *next;       // +0x04
};

class CMacroListGump : public CGump {
public:
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual int HitTest(int x, int y);
    virtual void RButtonUp(int x, int y);
    virtual void OnTab();
    // 0x004184e0: RET 0x18, (x, y, w, h, artId, arg6).
    CMacroListGump(int x, int y, int w, int h, int artId, int arg6);
    // 0x00418730: vtable slot 0, RET 4.
    // 0x00418760: destructor body.
    ~CMacroListGump();
    void Dtor() { this->CMacroListGump::~CMacroListGump(); }
    // 0x00418830: RET 0x14, append a CMacroCmdGump row below the lowest row; val2 doubles as the y of the first row.
    void __thiscall AddRow(int cmdType, int cmdArg, const char *text, int val1, int val2);
    // 0x00418930: RET 4, delete a row and move the rows below it up.
    void __thiscall RemoveRow(CMacroCmdGump *row);
    // 0x00418a10: vtable slot 0x44, RET 4.
    virtual void OnChildChanged(int child);
    // 0x00418ab0: vtable slot 0x10, RET 8.
    virtual void MouseDown(int x, int y);
    // 0x00418b10: vtable slot 0x08, RET 4.
    virtual void Draw(int *clip);

    CButtonGump *child_94;          // +0x94  scroll up button (art 0x983), hidden after construction
    CButtonGump *child_98;          // +0x98  scroll down button (art 0x985), shown when a second row exists
    int32_t unk_9c;
    int32_t unk_a0;
    int32_t pressed;                // +0xa4  button is down
    int32_t unk_a8;
    int32_t pressTick;              // +0xac  GetTickCount() of the first press
    int32_t clickState;             // +0xb0  0 none, 1 first press seen, 2 second press
    CMacroListNode *head;           // +0xb4  row list torn down by the destructor
    int32_t unk_b8;
    int32_t unk_bc;
    int32_t unk_c0;
    int32_t unk_c4;
    int32_t arg_c8;                 // +0xc8  arg6
    int32_t unk_cc;
    // Gump prep P5: own copies of slot bodies the original shares between gumps.
    virtual void MouseUp(int x, int y);
    virtual void Tick();
    virtual void MouseMove(int x, int y);
};

static_assert(offsetof(CMacroListGump, child_94) == 0x94, "CMacroListGump::child_94");
static_assert(offsetof(CMacroListGump, child_98) == 0x98, "CMacroListGump::child_98");
static_assert(offsetof(CMacroListGump, pressed) == 0xa4, "CMacroListGump::pressed");
static_assert(offsetof(CMacroListGump, pressTick) == 0xac, "CMacroListGump::pressTick");
static_assert(offsetof(CMacroListGump, clickState) == 0xb0, "CMacroListGump::clickState");
static_assert(offsetof(CMacroListGump, head) == 0xb4, "CMacroListGump::head");
static_assert(offsetof(CMacroListGump, arg_c8) == 0xc8, "CMacroListGump::arg_c8");
static_assert(sizeof(CMacroListGump) == 0xd0, "CMacroListGump size (operator new(0xd0))");
