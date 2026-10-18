// CResizeGump: the rubber-band outline used while a gump is being resized: four one-colour edge strips
// (CDumbGump children, tile ids 1 top, 2 right, 3 bottom, 4 left) that follow a size clamped to [min, max].
// vtable 0x004a0a70, operator new(0xb8) at the three callers. Constructor 0x0047b230.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CResizeGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    // 0x0047b230  size 0x3f4  constructor, RET 0x24, (x, y, width, height, minWidth, minHeight, maxWidth, maxHeight, target).
    // Sets the drag gump global (0x0068d2f4) to this and sizes the outline to the target's +0x6c / +0x70.
    CResizeGump(int x_, int y_, int curW_, int curH_, int minW_, int minH_, int maxW_, int maxH_, CGump *target_);
    // 0x0047b660: destructor: sets the vtable, then the base destructor.
    ~CResizeGump();
    void Dtor() { this->CResizeGump::~CResizeGump(); }
    // 0x0047b670: RET 8 (arguments not read): clear the drag gump, tell the target the final size (its vtable
    // slot 0x38 with width and height), delete this gump.
    virtual void MouseUp(int x, int y);
    // 0x0047b6b0: RET 8 (width, height): clamp to the limits, force an even width, redraw the four edges.
    virtual void MouseMove(int x, int y);
    // 0x0047b890: vtable slot 0x04, RET 4: forward to slot 0x04 of every child (newest first).
    virtual void PreDraw(int *clip);
    // 0x0047b8c0: vtable slot 0x08, RET 4: draw every child (newest first).
    virtual void Draw(int *clip);

    int32_t curW;       // +0x94
    int32_t curH;       // +0x98
    int32_t prevW;      // +0x9c
    int32_t prevH;      // +0xa0
    int32_t minW;       // +0xa4  forced even
    int32_t minH;       // +0xa8
    int32_t maxW;       // +0xac  forced even
    int32_t maxH;       // +0xb0
    CGump *target;      // +0xb4
    virtual int HitTest(int x, int y);   // slot HitTest, copy of CMissileGump::HitTestNever
    virtual void MouseOver(int x, int y);   // slot MouseOver, empty (as CircleTrans_Nullsub (0x004242b0))
};

static_assert(offsetof(CResizeGump, curW) == 0x94, "CResizeGump::curW");
static_assert(offsetof(CResizeGump, minW) == 0xa4, "CResizeGump::minW");
static_assert(offsetof(CResizeGump, maxH) == 0xb0, "CResizeGump::maxH");
static_assert(offsetof(CResizeGump, target) == 0xb4, "CResizeGump::target");
static_assert(sizeof(CResizeGump) == 0xb8, "CResizeGump size (operator new(0xb8))");
