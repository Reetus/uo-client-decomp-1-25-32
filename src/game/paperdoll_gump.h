// CPaperdollGump: the paperdoll window of a mobile (vtable 0x0049fc28, name
// string "paperdoll gump" at 0x004ae380). Layout partial, from
// decomp/pass2/0x00466be0 (constructor), 0x00467280 (destructor),
// 0x00467680 (RedrawFrame) and 0x00467dd0 (single-click handler).
// Allocated with operator new(0xf8) at every site.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CPaperdollGump : public CGump {
public:
    virtual int IsTag68() { return 1; }
    virtual int IsTag78() { return 1; }
    // slot override (gump virtual-slot prep): own copy of the shared body
    virtual int HitTest(int x, int y);
    virtual void RButtonUp(int x, int y);
    // Batch u1 (small_methods_u1.cpp).
    // 0x0040cf70: war / peace button callback: when the owner's slot 0x28 answers non-zero, toggle the war mode.
    void __thiscall OnWarButton();
    // 0x00466be0  size 0x663  constructor, RET 0x24 (nine stack args):
    // (x, y) position, `parent` is not read, `obj` the mobile shown, three unused ints,
    // `name` the title (copied to +0xa4), `warFlag` selects the war/peace button art.
    CPaperdollGump(int px, int py, int parent, void *obj, int a5, int a6, int a7, char *nameText, uint8_t warFlag);
    // 0x00467280  size 0x117  destructor body: deletes the gumps owned by this one, frees the bitmaps.
    ~CPaperdollGump();
    // Second pass, batch 25 (paperdoll_gump_ops.cpp).
    // 0x004673a0  left button down at (x, y), RET 8.
    virtual void MouseDown(int x, int y);
    // 0x00467410  left button up, RET 8 (a double click when the click state is 2).
    virtual void MouseUp(int x, int y);
    // 0x004674a0  mouse move, RET 8: starts a drag of the item under the cursor or moves the gump.
    virtual void MouseMove(int x, int y);
    // 0x004675b0  per-tick update: a pending single click is delivered after the double-click time.
    virtual void Tick();
    // 0x00467c10  draw pass, RET 4: blit the frame bitmap (the second one when unk_60), then the children.
    virtual void Draw(int *clip);
    // 0x00467cf0  equipment layer under (x, y) or -1, RET 0xc.
    int __thiscall HitTestLayer(int mx, int my, int flag);
    // 0x00467f30  double click at (x, y), RET 8.
    void __thiscall OnDoubleClick(int mx, int my);
    // 0x00467470  mouse-over handler (arguments and `this` not read), RET 8.
    virtual void MouseOver(int x, int y);
    // 0x00467680  size 0x55b  redraw the paperdoll frame: backdrop, name, body, equipment layers.
    void __thiscall RedrawFrame();
    // 0x00467dd0  size 0x153  single click at (x, y): pick the item under the cursor or open the guide.
    void __thiscall OnClick(int mx, int my);

    void *nameBMP;          // +0x94  freed with tag "nameBMP"; never set in the constructor (zeroed)
    CGump *warButton;       // +0x98  peace/war toggle button (own paperdoll only)
    int32_t heldLayer;      // +0x9c  equipment layer of the item being dragged, -1 if none
    int32_t isPlayer;       // +0xa0  1 when the paperdoll belongs to the player
    char title[0x3c];       // +0xa4  name text, copied from the constructor argument
    int32_t unk_e0;         // +0xe0
    int32_t unk_e4;         // +0xe4
    int32_t unk_e8;         // +0xe8
    int32_t unk_ec;         // +0xec
    int32_t unk_f0;         // +0xf0
    int32_t unk_f4;         // +0xf4
};

static_assert(offsetof(CPaperdollGump, nameBMP) == 0x94, "CPaperdollGump::nameBMP");
static_assert(offsetof(CPaperdollGump, warButton) == 0x98, "CPaperdollGump::warButton");
static_assert(offsetof(CPaperdollGump, heldLayer) == 0x9c, "CPaperdollGump::heldLayer");
static_assert(offsetof(CPaperdollGump, isPlayer) == 0xa0, "CPaperdollGump::isPlayer");
static_assert(offsetof(CPaperdollGump, title) == 0xa4, "CPaperdollGump::title");
static_assert(offsetof(CPaperdollGump, unk_e0) == 0xe0, "CPaperdollGump::unk_e0");
static_assert(offsetof(CPaperdollGump, unk_f4) == 0xf4, "CPaperdollGump::unk_f4");
static_assert(sizeof(CPaperdollGump) == 0xf8, "CPaperdollGump size (operator new(0xf8))");
