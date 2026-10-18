// CContainerGump: an open container window (vtable 0x0049e5c0).
// Layout partial; see decomp/pass2/0x004261d0.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CContainerGump : public CGump {
public:
    virtual int IsTag68() { return 1; }
    virtual int IsTag90() { return 1; }
    // 0x004261d0: dtor body. Deletes the gumps owned by this container, releases the
    // container object, frees frame data, plays the closing sound, base dtor.
    ~CContainerGump();
    void Dtor() { this->CContainerGump::~CContainerGump(); }

    // CGump::unk_50 (+0x50) holds the container art id (word read in the dtor).

    // 0x00425e30: RET 0x20. (x, y, parent (not read), container object, frame count, width, height, art id)
    CContainerGump(int x, int y, CGump *unusedParent, CObj *container, int nFrames, int w, int h, int gumpId);
    // 0x004261a0: vtable slot 0, RET 4.
    // 0x004263c0: RET 8 (x, y).
    virtual void RButtonUp(int x, int y);
    // 0x00426450: RET 8 (x, y).
    virtual void MouseDown(int x, int y);
    // 0x004264d0: RET 8 (x, y not read).
    virtual void MouseUp(int x, int y);
    // 0x00426630: RET 8 (x, y).
    virtual void MouseOver(int x, int y);
    // 0x004267d0: plain RET.
    virtual void MouseLeave();
    // 0x00426890: RET 8 (x, y).
    virtual void MouseMove(int x, int y);
    // 0x004269a0: plain RET.
    virtual void Update();
    // 0x00426a30: plain RET.
    virtual void Tick();
    // 0x00426b70: RET 4, (clip rect {l, t, r, b}).
    void __thiscall RedrawFrame(const int *clip);
    // 0x00426f70: RET 4, (clip rect).
    virtual void Draw(int *clip);
    // 0x00427020: RET 8, non-zero when the frame bitmap pixel at (x, y) is not 0.
    virtual int HitTest(int x, int y);
    // 0x00427080: RET 8, the contained object whose sprite is hit at (x, y), or 0.
    void *__thiscall GetChildObjectAt(int x, int y);
    // 0x00427290: RET 8, drop the held item at (x, y); 0 when the held item does not fit.
    int __thiscall DropHeldItemAt(int x, int y);
    // 0x004274f0: RET 4, redraw the area of one contained object.
    void __thiscall AddToGump(void *obj);

    int32_t closing;        // +0x94  set to 1 by HandleClose when the gump has no parent
    int32_t titleHeight;    // +0x98  0x14 for corpse art 0x91a, else 0
    void *hoverObj;         // +0x9c  contained object under the mouse
    void *unk_a0;           // +0xa0  ? (its +0xb8 is compared with this)
    int32_t dragStarted;    // +0xa4
    int32_t unk_a8;         // +0xa8
    int32_t mouseDown;      // +0xac
    int32_t unk_b0;         // +0xb0
    uint32_t clickTick;     // +0xb4  GetTickCount at the first press
    uint32_t clickState;    // +0xb8  1 after the first press, 2 after the second
    int32_t animCounter;    // +0xbc  idle animation (art id 9 only)
    int32_t animFrame;      // +0xc0
};

static_assert(offsetof(CContainerGump, owner) == 0x34, "CContainerGump::owner (container object)");
static_assert(offsetof(CContainerGump, frameData) == 0x38, "CContainerGump::frameData");
static_assert(offsetof(CContainerGump, closing) == 0x94, "CContainerGump::closing");
static_assert(offsetof(CContainerGump, hoverObj) == 0x9c, "CContainerGump::hoverObj");
static_assert(offsetof(CContainerGump, clickState) == 0xb8, "CContainerGump::clickState");
static_assert(offsetof(CContainerGump, animFrame) == 0xc0, "CContainerGump::animFrame");
static_assert(sizeof(CContainerGump) == 0xc4, "CContainerGump size (operator new(0xc4))");
