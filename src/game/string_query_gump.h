// CStringQueryGump ("Stringquery gump", vtable 0x004a04e8) and CStringPickerGump.
// Layout from decomp/pass2/0x0046fe90.cpp and 0x0046fc90.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CStringQueryGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    // Gump prep P5: own copy of a slot body the original shares between gumps.
    virtual void Draw(int *clip);
    // 0x0046fe90  RET 0x1c. Builds the prompt: title, body text, an edit control (kind 1 or 2) and OK / Cancel buttons.
    CStringQueryGump(unsigned int serial_, uint16_t type_, char *title, uint8_t flags_, uint8_t kind_,
                     unsigned int arg6, char *text);
    // 0x004702a0: vtable slot 0, RET 4.
    // 0x004702d0: destructor body.
    ~CStringQueryGump();
    void Dtor() { this->CStringQueryGump::~CStringQueryGump(); }
    // 0x00470380  size 0x5  vtable slot 0x4c: OK; tail jump to CGumpHandlers04::TextEntryOk (0x00412e90) on this gump.
    virtual void OnOk();
    // 0x00470390  size 0xf  vtable slot 0x50: Cancel; runs TextEntryCancel (0x00412f20) when flags bit 0 is set, else nothing.
    virtual void OnCancel();

    int32_t unk_94;             // +0x94  cleared
    uint32_t serial;            // +0x98
    uint16_t type;              // +0x9c
    uint8_t kind;               // +0x9e  1 / 2 = with edit control
    uint8_t flags;              // +0x9f  bit 0 = OK and Cancel, else OK only
    uint8_t *editControl;       // +0xa0
    virtual int HitTest(int x, int y);   // slot HitTest, copy of CDeleteCharGump::HitTestFrame
    virtual void MouseDown(int x, int y);   // slot MouseDown, copy of CSignGump::MouseDown
    virtual void MouseUp(int x, int y);   // slot MouseUp, copy of CMacroCmdGump::OnMouseUp
    virtual void RButtonUp(int x, int y);   // slot RButtonUp, copy of CDragReleaseGump::ClearDragCapture
};

static_assert(offsetof(CStringQueryGump, unk_94) == 0x94, "CStringQueryGump::unk_94");
static_assert(offsetof(CStringQueryGump, serial) == 0x98, "CStringQueryGump::serial");
static_assert(offsetof(CStringQueryGump, type) == 0x9c, "CStringQueryGump::type");
static_assert(offsetof(CStringQueryGump, kind) == 0x9e, "CStringQueryGump::kind");
static_assert(offsetof(CStringQueryGump, flags) == 0x9f, "CStringQueryGump::flags");
static_assert(offsetof(CStringQueryGump, editControl) == 0xa0, "CStringQueryGump::editControl");
static_assert(sizeof(CStringQueryGump) == 0xa4, "CStringQueryGump size (operator new(0xa4))");

// "stringpicker gump" (vtable 0x004a03d0, art 0x13ec): a title, one radio toggle per string and OK / Cancel
// buttons. Layout from the constructor at 0x0046f7e0.
class CStringPickerGump : public CGump {
public:
    // 0x0046f7e0  RET 0x18. (title, serial, type, count, ids, strings)
    CStringPickerGump(char *title, unsigned int serial_, uint16_t type_, int count, uint16_t *ids_, char **strings);
    // 0x0046fbc0  vtable slot 0, RET 4
    // 0x0046fbf0  destructor body
    ~CStringPickerGump();
    void Dtor() { this->CStringPickerGump::~CStringPickerGump(); }
    // 0x0046fc90  vtable slot 8: draw(clip)
    virtual void Draw(int *clip);
    // 0x0046fd50  vtable slot 0x0c, RET 8: frame bitmap pixel at (x, y) is non-zero
    virtual int HitTest(int x, int y);
    // 0x0046fda0  RET 8 (arguments not read): runs the cancel handler 0x00412740 on this, clears the drag gump
    virtual void RButtonUp(int x, int y);
    // 0x0046fdc0  RET 8 (arguments not read): mouse up
    virtual void MouseUp(int x, int y);
    // 0x0046fe10  RET 4: a radio toggle was clicked; checks it and unchecks the others
    virtual void OnChildChanged(int child);

    int32_t unk_94;             // +0x94
    int32_t unk_98;
    int32_t unk_9c;
    int32_t unk_a0;
    int32_t unk_a4;
    int32_t unk_a8;
    uint16_t ids[0x100];        // +0xac  one id per string (capacity up to +0x2ac)
    uint32_t serial;            // +0x2ac
    uint16_t type;              // +0x2b0
    virtual void MouseDown(int x, int y);   // slot MouseDown, copy of CFightingGump::BeginDrag
    virtual void Tick();   // slot Tick, copy of CInputControlGump::UpdateClickState
};

static_assert(offsetof(CStringPickerGump, unk_94) == 0x94, "CStringPickerGump::unk_94");
static_assert(offsetof(CStringPickerGump, unk_a8) == 0xa8, "CStringPickerGump::unk_a8");
static_assert(offsetof(CStringPickerGump, ids) == 0xac, "CStringPickerGump::ids");
static_assert(offsetof(CStringPickerGump, serial) == 0x2ac, "CStringPickerGump::serial");
static_assert(offsetof(CStringPickerGump, type) == 0x2b0, "CStringPickerGump::type");
static_assert(sizeof(CStringPickerGump) == 0x2b4, "CStringPickerGump size (operator new(0x2b4))");
