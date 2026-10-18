// CBBLineGump: one message line in the bulletin board list. ctor 0x00403c50,
// vtable 0x0049c460. Derived fields start at +0x94.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CBBLineGump : public CGump {
public:
    virtual int IsTage0() { return 1; }
    // 0x00403fc0: left button released on the line. Sends a "request message"
    // packet (type 3) on the second click, otherwise toggles the replies at x < 0x16.
    virtual void MouseUp(int x, int y);
    // 0x004045c0: nearest sibling in the +0x40 direction that is a BB line (virtual
    // slot 0xe0) and is visible (no parent line, or parent's +0x88 object has +0x98 == 1).
    CBBLineGump *__thiscall PrevVisibleLine();
    // 0x00404380: redraw the expander icon (art 0x1519 area cleared, then art 0x37 / 0x38) into frame 0.
    void __thiscall DrawIcon();
    // 0x00404430 (virtual slot 0x08): blit frame 0 to the back buffer when the clip intersects, then draw the children.
    virtual void Draw(int *clip);
    // 0x00404500 (virtual slot 0x0c): visible and (x, y) inside the gump.
    virtual int HitTest(int x, int y);
    // 0x00404540: 1 for a top-level line, else whether the parent thread's object is expanded (+0x98 == 1).
    int __thiscall IsVisibleByParent();
    // 0x00404570: nearest sibling in the +0x3c direction that is a BB line (virtual slot 0xe0) and visible.
    CBBLineGump *__thiscall NextVisibleLine();
    // Batch 00 second pass, bb_gump.cpp.
    // 0x00403c50: RET 0x20. (x, y, width, owner = the post this line shows, parentPost = `post` field, and the three
    // strings of the "%s - %s [%s]" caption).
    CBBLineGump(int x, int y, int width, CObj *owner, CObj *parentPost, const char *s1, const char *s2, const char *s3);
    ~CBBLineGump();                                   // 0x00403ec0
    void Dtor() { this->CBBLineGump::~CBBLineGump(); }
    virtual void MouseDown(int x, int y);   // 0x00403f60  RET 8
    void __thiscall ToggleReplies(int mouseX, int mouseY);     // 0x004040b0  RET 8: expand / collapse when the click is on the icon
    virtual void MouseMove(int x, int y);   // 0x004042e0  RET 8: drag the parent board gump
    virtual void Tick();                       // 0x00404320  double-click window expired

    CObj *post;          // +0x94  the message object this line shows (serial at obj+0x78), may be null
    int32_t iconState;   // +0x98  0 = collapsed icon (art 0x37), 1 = expanded icon (art 0x38)
    int32_t unk_9c;      // +0x9c  set while a drag/press is pending
    uint8_t pad_a0[4];
    int32_t unk_a4;      // +0xa4  cleared on mouse up
    int32_t unk_a8;      // +0xa8  cleared on mouse up
    int32_t clickTime;   // +0xac  GetTickCount() of the first click
    int32_t clickState;  // +0xb0  2 = second click pending
    virtual void RButtonUp(int x, int y);   // slot RButtonUp, copy of CMacroCmdGump::ForwardToParent
};

static_assert(offsetof(CBBLineGump, post) == 0x94, "CBBLineGump::post");
static_assert(offsetof(CBBLineGump, iconState) == 0x98, "CBBLineGump::iconState");
static_assert(offsetof(CBBLineGump, unk_9c) == 0x9c, "CBBLineGump::unk_9c");
static_assert(offsetof(CBBLineGump, unk_a4) == 0xa4, "CBBLineGump::unk_a4");
static_assert(offsetof(CBBLineGump, unk_a8) == 0xa8, "CBBLineGump::unk_a8");
static_assert(offsetof(CBBLineGump, clickState) == 0xb0, "CBBLineGump::clickState");
static_assert(offsetof(CBBLineGump, clickTime) == 0xac, "CBBLineGump::clickTime");
static_assert(sizeof(CBBLineGump) == 0xb4, "CBBLineGump size (operator new(0xb4))");
