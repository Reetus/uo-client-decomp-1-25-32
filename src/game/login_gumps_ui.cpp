// Login-flow gumps: scalar deleting destructors, focus handling and the field list of CLoginGump,
// the draw handler of CChgPWGump and the frame hit test of CDeleteCharGump. (Porting pass 2, batch 07.)
#include "login_gumps.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "globals.h"
#include "input_control_gump.h"
#include "crt.h"
#include "vcall.h"
#include "gump_base_ops_ports.h"
#include "char_ui_gumps.h"


static inline CInputControlGump *ctl_at(void *p)
{
    return static_cast<CInputControlGump *>(p);
}

HOOK_VTABLE(0x0049dbe8, CLoginGump);
HOOK_SDD(0x004212a0, CLoginGump);

// 0x00421390  Stub_00421390 (empty, one stack argument) moved to bark_gump_ui.cpp next to Stub_00401260.

// 0x004213c0  size 0x7f
// Finds `ctl` among the five fields; when that changes the focused field, clears the focus flag (+0xd8)
// of the old one and sets it on the new one, redrawing both.
void __thiscall CLoginGump::OnFocus(int ctlArg)
{
    void * ctl = reinterpret_cast<void *>(ctlArg);
    int oldIndex = focusIndex;
    for (int i = 0; i < 5; i++) {
        if (ctl == children[i])
            focusIndex = i;
    }
    if (oldIndex != focusIndex) {
        children[oldIndex]->focused = 0;
        children[oldIndex]->Redraw();
        children[focusIndex]->focused = 1;
        children[focusIndex]->Redraw();
    }
}
HOOK_VMETHOD(0x004213c0, CLoginGump, OnFocus);

// 0x00421440  size 0x49
// Enter key: virtual slot 0x54 first, then NewCharacter (0x0040ee50) when the focused field is empty, else 0x0040ebe0.
void __thiscall CLoginGump::OnEnter(int arg)
{
    PORT_VSLOT1(this, 0x54, void, int)(arg);
    const char *s = children[focusIndex]->text;
    if (strlen(s) == 0)
        decomp::thiscall_at_void(PORT_ADDR(&CCharSelectGump::NewCharacter), this);
    else
        decomp::thiscall_at_void(PORT_ADDR(&CCharSelectGump::Play), this);
}
HOOK_VMETHOD(0x00421440, CLoginGump, OnEnter);

// 0x00421490  size 0xab
// Puts `text` into field number unk_94 (the next free one), redraws it, moves its caret to the end when it
// holds the keyboard focus, records `value` for the entry and counts it.
void __thiscall CLoginGump::AddEntry(const char *text, int unused, int value)
{
    (void)unused;
    if (strlen(text) == 0)
        return;
    CInputControlGump *ctl = children[unk_94];
    strcpy(ctl->text, text);
    children[unk_94]->Redraw();
    if (children[unk_94] == g_focusedInput)
        PORT_VSLOT1(children[unk_94], 0x3c, void, int)(0xe023);   // HandleKey(End)
    state[unk_94] = value;
    unk_94 = unk_94 + 1;
}
HOOK_METHOD(0x00421490, CLoginGump, AddEntry);

// 0x00421540  size 0x120
// Removes the entry whose value is `value`: later entries' texts and values move down one slot, the last
// field is blanked, its value set to -1, the count drops and the freed field gets +0xb4 = 1.
void __thiscall CLoginGump::RemoveEntry(int value)
{
    int i = 0;
    int count = unk_94;

    for (; i < count; i++) {
        if (state[i] == value)
            break;
    }
    if (i == count)
        return;
    if (i < count - 1) {
        do {
            strcpy(children[i]->text, children[i + 1]->text);
            children[i]->Redraw();
            state[i] = state[i + 1];
            i++;
        } while (i < unk_94 - 1);
    }
    strcpy(children[unk_94 - 1]->text, g_emptyString);
    children[unk_94 - 1]->Redraw();
    state[unk_94 - 1] = -1;
    unk_94 = unk_94 - 1;
    children[unk_94]->dropDown = 1;   // +0xb4
}
HOOK_METHOD(0x00421540, CLoginGump, RemoveEntry);

HOOK_VTABLE(0x0049dd00, CAcctLoginGump);
HOOK_SDD(0x00421a80, CAcctLoginGump);

// 0x00421c50  size 0x87
// Focus moved to `ctl` (when it is not focused yet): focus flag (+0xd8) set on the clicked one of the two
// input controls and cleared on the other, both redrawn.
void __thiscall CAcctLoginGump::OnFocus(int ctlArg)
{
    void * ctl = reinterpret_cast<void *>(ctlArg);
    if (ctl_at(ctl)->focused != 0)
        return;
    if (ctl == nameInput) {
        ctl_at(nameInput)->focused = 1;
        ctl_at(nameInput)->Redraw();
        ctl_at(passwordInput)->focused = 0;
        ctl_at(passwordInput)->Redraw();
        return;
    }
    ctl_at(nameInput)->focused = 0;
    ctl_at(nameInput)->Redraw();
    ctl_at(passwordInput)->focused = 1;
    ctl_at(passwordInput)->Redraw();
}
HOOK_VMETHOD(0x00421c50, CAcctLoginGump, OnFocus);

HOOK_VTABLE(0x0049de18, CChgPWGump);
HOOK_SDD(0x004221b0, CChgPWGump);
// CChgPWGump::Draw (0x004222b0) lives in gump_draw_copies.cpp with the other copies of its body.


// 0x00422470  size 0x107
// Focus moved to `ctl` (when it is not focused yet): the focus flag (+0xd8) is set on it and cleared on the
// other two of the three password controls (old, new, verify), all redrawn.
void __thiscall CChgPWGump::OnFocus(int ctlArg)
{
    void * ctl = reinterpret_cast<void *>(ctlArg);
    if (ctl_at(ctl)->focused != 0)
        return;
    if (ctl == oldPassword) {
        ctl_at(oldPassword)->focused = 1;
        ctl_at(oldPassword)->Redraw();
        ctl_at(newPassword)->focused = 0;
        ctl_at(newPassword)->Redraw();
        ctl_at(verifyPassword)->focused = 0;
        ctl_at(verifyPassword)->Redraw();
        return;
    }
    if (ctl == newPassword) {
        ctl_at(newPassword)->focused = 1;
        ctl_at(newPassword)->Redraw();
        ctl_at(oldPassword)->focused = 0;
        ctl_at(oldPassword)->Redraw();
        ctl_at(verifyPassword)->focused = 0;
        ctl_at(verifyPassword)->Redraw();
        return;
    }
    ctl_at(newPassword)->focused = 0;
    ctl_at(newPassword)->Redraw();
    ctl_at(oldPassword)->focused = 0;
    ctl_at(oldPassword)->Redraw();
    ctl_at(verifyPassword)->focused = 1;
    ctl_at(verifyPassword)->Redraw();
}
HOOK_VMETHOD(0x00422470, CChgPWGump, OnFocus);

HOOK_VTABLE(0x0049df30, CDeleteCharGump);
HOOK_SDD(0x004228e0, CDeleteCharGump);

// 0x004229c0  size 0x3c
// Hit test: 1 when (px, py) lies inside the frame0 bitmap, else 0.
int __thiscall CDeleteCharGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}
HOOK_VMETHOD(0x004229c0, CDeleteCharGump, HitTest);
