// CScrollThumbGump: slider thumb used by the character creation stat/skill pages and the
// client config gump (FINDINGS.md: "scroll thumb" FUN_00413740, "scrollbar update" FUN_00413c00).
// operator new(0xb4). Layout is partial: only the field the ported code writes is named.
// The class name is a guess from FINDINGS.md.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"
#include "../runtime/ported.h"

class CScrollThumbGump : public CGump {
public:
    virtual int IsTag98() { return 1; }
    // Batch u1 (small_methods_u1.cpp).
    // 0x00413960: vtable slot 0x24: clear the hover flag.
    virtual void MouseLeave();
    // 0x00413740  size 0x113  constructor (x, y, vertical, limit, parent (not read), art id). RET 0x18.
    CScrollThumbGump(int x_, int y_, int vertical_, int limit, int parent_, int artId);
    // 0x00413890: destructor (sets the vtable, then the CGump destructor).
    ~CScrollThumbGump();
    // 0x00413860: vtable slot 0, RET 4.
    void Dtor() { this->CScrollThumbGump::~CScrollThumbGump(); }
    // 0x004138a0 / 0x004138c0 / 0x00413930 / 0x00413970 / 0x00413a40: vtable slots 0x10 / 0x14 / 0x20 / 0x28 / 0x08.
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void MouseOver(int x, int y);
    virtual void MouseMove(int x, int y);
    virtual void Draw(int *clip);
    // 0x00413b20: place the thumb from the parent's text list (cursor node at parent+0xc8, offset at parent+0xcc).
    void __thiscall PlaceFromTextList();
    // 0x00413c00: RET 8, (value, range): move the thumb to value/range.
    void __thiscall SetPosition(int value, int range);
    // 0x0041aac0: RET 8, (x, y) screen position: try to grab the thumb; non-zero when it did. Ported in
    // scroll_thumb_gump.cpp (reads +0x9c / +0xa0 / +0xa4 through rawf, meaning not settled).
    int __thiscall TryGrab(int sx, int sy);

    int32_t vertical;     // +0x94  non-zero: the thumb slides along y between minY and maxY, else along x
    int32_t hot;          // +0x98  set by the hover handler (draws artAlt)
    int32_t minX;         // +0x9c
    int32_t maxX;         // +0xa0
    int32_t minY;         // +0xa4
    int32_t maxY;         // +0xa8
    uint16_t artAlt;      // +0xac  second art id, written by callers after construction
    uint8_t pad_ae[2];
    int32_t unk_b0;       // +0xb0  non-zero while the thumb is being dragged
    virtual void RButtonDown(int x, int y);   // slot RButtonDown, copy of CButtonGump::RButtonDown
    virtual void RButtonUp(int x, int y);   // slot RButtonUp, copy of CButtonGump::RButtonUp
    virtual int HitTest(int x, int y);   // slot HitTest, copy of CGump::ContainsLocalPoint
};

static_assert(offsetof(CScrollThumbGump, vertical) == 0x94, "CScrollThumbGump::vertical");
static_assert(offsetof(CScrollThumbGump, maxY) == 0xa8, "CScrollThumbGump::maxY");
static_assert(offsetof(CScrollThumbGump, artAlt) == 0xac, "CScrollThumbGump::artAlt");
static_assert(sizeof(CScrollThumbGump) == 0xb4, "CScrollThumbGump size (operator new(0xb4))");

// 0x0041c2f0 is vtable slot 0x1c of CCharStatGump and CInitSkillGump; it reads nothing from the object.
struct CDragReleaseGump : CGump {
    void __thiscall ClearDragCapture(int mx, int my);
};

namespace gfx {
inline void ScrollThumbSet(CScrollThumbGump *thumb, int value, int range)
{
    thumb->CScrollThumbGump::SetPosition(value, range);
}
inline int ScrollThumbTryGrab(CScrollThumbGump *thumb, int x, int y)
{
    return thumb->CScrollThumbGump::TryGrab(x, y);
}
} // namespace gfx
