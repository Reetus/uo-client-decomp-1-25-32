// CObjPickerGump: "objpicker gump", a list of (model id, hue, name) entries the
// player chooses from. vtable 0x004a02b8. Layout from decomp/pass2/0x0046eb80.cpp
// and 0x0046f050.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CObjPickerGump : public CGump {
public:
    // 0x0046eb80  size 0x492  constructor, RET 0x24
    CObjPickerGump(int x_, int y_, char *title_, unsigned int serial_, uint16_t modelId_, int count, uint16_t *ids_,
                   uint16_t *hues_, char **names_);
    // 0x0046f050  sends the selection packet when the socket exists, then frees everything
    ~CObjPickerGump();
    void Dtor() { this->CObjPickerGump::~CObjPickerGump(); }
    // 0x00410bc0  page up: step back one entry
    void __thiscall PrevPage();
    // 0x00410c80  page down: step forward one entry
    void __thiscall NextPage();

    // Second pass, obj_picker_gump_ops.cpp.
    // 0x0046f020  vtable slot 0, RET 4
    // 0x0046f1a0  repaint the item strip into the frame bitmap
    void __thiscall RedrawList();
    // 0x0046f3a0  vtable slot 8, RET 4: blit the frame bitmap, then draw the children
    virtual void Draw(int *clip);
    // 0x0046f460  RET 8 (x, y): mouse up
    virtual void MouseUp(int x, int y);
    // 0x0046f4c0  RET 8 (x, y): mouse move, shows the hovered entry's name in the list control
    virtual void MouseOver(int x, int y);
    // 0x0046f610  plain RET: one step of the slide animation
    virtual void Update();
    // 0x0046f700  RET 8 (x, y): click on the strip selects the entry under the pointer
    void __thiscall ClickStrip(int px, int py);

    int32_t unk_94;             // +0x94
    int32_t unk_98;
    int32_t unk_9c;
    int32_t unk_a0;
    int32_t unk_a4;
    int32_t unk_a8;
    int32_t unk_ac;
    uint32_t serial;            // +0xb0
    int32_t count;              // +0xb4  number of entries
    int32_t unk_b8;             // +0xb8  cleared
    int32_t slideOffset;        // +0xbc  pixel offset of the first visible entry while sliding
    int32_t slideDir;           // +0xc0  slide direction (> 0 forward, else back)
    int32_t unk_c4;             // +0xc4  -1
    int32_t selection;          // +0xc8  1-based selected entry, 0 = none
    uint16_t modelId;           // +0xcc
    uint8_t pad_ce[2];
    uint16_t *ids;              // +0xd0  count ushorts (operator new)
    uint16_t *hues;             // +0xd4  count ushorts (operator new)
    char title[0x128 - 0xd8];   // +0xd8
    char *names[0x100];         // +0x128  count heap-copied names (capacity up to +0x528)
    int32_t pickRect[4];        // +0x528  0x28, 0x2f, 0x101, 0x5b
    uint8_t *listControl;       // +0x538  edit/list control (0x158 bytes)
    uint8_t *okButton;          // +0x53c
    uint8_t *cancelButton;      // +0x540
    virtual int HitTest(int x, int y);   // slot HitTest, copy of CStringPickerGump::HitTest
    virtual void MouseDown(int x, int y);   // slot MouseDown, copy of CFightingGump::BeginDrag
    virtual void RButtonUp(int x, int y);   // slot RButtonUp, copy of CCourseGump::CloseAndDelete
    virtual void MouseMove(int x, int y);   // slot MouseMove, copy of CSpellIconGump::MouseMove
    virtual void Tick();   // slot Tick, copy of CInputControlGump::UpdateClickState
};

static_assert(offsetof(CObjPickerGump, unk_94) == 0x94, "CObjPickerGump::unk_94");
static_assert(offsetof(CObjPickerGump, unk_ac) == 0xac, "CObjPickerGump::unk_ac");
static_assert(offsetof(CObjPickerGump, serial) == 0xb0, "CObjPickerGump::serial");
static_assert(offsetof(CObjPickerGump, count) == 0xb4, "CObjPickerGump::count");
static_assert(offsetof(CObjPickerGump, unk_b8) == 0xb8, "CObjPickerGump::unk_b8");
static_assert(offsetof(CObjPickerGump, slideOffset) == 0xbc, "CObjPickerGump::slideOffset");
static_assert(offsetof(CObjPickerGump, slideDir) == 0xc0, "CObjPickerGump::slideDir");
static_assert(offsetof(CObjPickerGump, unk_c4) == 0xc4, "CObjPickerGump::unk_c4");
static_assert(offsetof(CObjPickerGump, selection) == 0xc8, "CObjPickerGump::selection");
static_assert(offsetof(CObjPickerGump, modelId) == 0xcc, "CObjPickerGump::modelId");
static_assert(offsetof(CObjPickerGump, ids) == 0xd0, "CObjPickerGump::ids");
static_assert(offsetof(CObjPickerGump, hues) == 0xd4, "CObjPickerGump::hues");
static_assert(offsetof(CObjPickerGump, title) == 0xd8, "CObjPickerGump::title");
static_assert(offsetof(CObjPickerGump, names) == 0x128, "CObjPickerGump::names");
static_assert(offsetof(CObjPickerGump, pickRect) == 0x528, "CObjPickerGump::pickRect");
static_assert(offsetof(CObjPickerGump, listControl) == 0x538, "CObjPickerGump::listControl");
static_assert(offsetof(CObjPickerGump, okButton) == 0x53c, "CObjPickerGump::okButton");
static_assert(offsetof(CObjPickerGump, cancelButton) == 0x540, "CObjPickerGump::cancelButton");
static_assert(sizeof(CObjPickerGump) == 0x544, "CObjPickerGump size (operator new(0x544))");
