#include "toggle_control.h"

#include "../runtime/ported.h"
#include "globals.h"
#include "window_helpers_31_ports.h"


// 0x00416640  size 0x2f
// Mouse down: brings the parent to the front, remembers the press position.
void __thiscall CToggleControl::MouseDown(int mx, int my)
{
    if (parent != nullptr)
        CGump_BringToFront(parent, 0);
    unk_a8 = 1;
    unk_6c = mx;
    unk_70 = my;
}
HOOK_VMETHOD(0x00416640, CToggleControl, MouseDown);

// 0x00416670  size 0x50
// Mouse up: releases; an enabled control flips its checked state, redraws and tells the parent (slot 0x44).
void __thiscall CToggleControl::MouseUp(int, int)
{
    unk_a8 = 0;
    g_dragGump = nullptr;
    if (disabled == 0) {
        checked = (checked == 0);
        Redraw();
        if (parent != nullptr)
            PORT_VSLOT1(parent, 0x44, void, CGump *)(this);
    }
}
HOOK_VMETHOD(0x00416670, CToggleControl, MouseUp);

// 0x004166c0  size 0x44
// Mouse enter: sets the hover flag (redrawing when a hover art exists) and selects cursor 0 while enabled.
void __thiscall CToggleControl::MouseOver(int, int)
{
    if (hovering == 0) {
        hovering = 1;
        if (artHover != 0)
            Redraw();
    }
    if (disabled == 0)
        PORT_THISCALL1(&SetCursorForRegion, void, GLOBAL_PTR(void, g_app_storage), unsigned)(0);   // cursor 0; ECX = the app object
}
HOOK_VMETHOD(0x004166c0, CToggleControl, MouseOver);
