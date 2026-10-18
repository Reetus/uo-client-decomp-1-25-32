// Gumps of the login / character select / character creation flow and a few list gumps whose class is not
// established. Names marked "?" are placeholders named from behaviour or address; the hook addresses are what
// matters. Layouts are partial: only the offsets the ported methods read are named, the rest is read raw.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CInputControlGump;

// CPlayCharGump: the password prompt shown by the character select gump before logging in
// (ctor 0x00422a10, operator new(0x9c)). +0x98 is the password input control (text at +0xec).
class CPlayCharGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    // Gump prep P5: slot target formerly a free function (vtable_stubs_u2.cpp).
    virtual void OnOk();
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual void Draw(int *clip);
    virtual int HitTest(int x, int y);
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void RButtonUp(int x, int y);
    virtual void MouseMove(int x, int y);
    virtual void OnCancel();
    // 0x0040eab0  OK: log in with the selected character and the typed password, then delete this gump
    void __thiscall Ok();

    // Added by porting pass 2 (batch 07).
    // 0x00422a10: RET 8 (charName, unused); frame0 from art 0x474, a "Password" label, the "Play as %s?" text,
    // the password input control (+0x98) and two buttons (OK 0x0040eab0, 0x0040cf60).
    CPlayCharGump(const char *charName, int unused);
    // 0x00422d90: destructor (re-shows the gump in 0x006488e8, deletes the input control, frees frame0; the base destructor follows).
    ~CPlayCharGump();
    void Dtor() { this->CPlayCharGump::~CPlayCharGump(); }

    uint8_t pad_94[4];
    CInputControlGump *password;   // +0x98
};
static_assert(offsetof(CPlayCharGump, password) == 0x98, "CPlayCharGump::password");
static_assert(sizeof(CPlayCharGump) == 0x9c, "CPlayCharGump size (operator new(0x9c))");

// ? Dialog of the skill group list ("Rename group" / "Delete group"): +0x98 group number, +0x9c edit control.
class CSkillGroupDlg : public CGump {
public:
    // 0x0040f6b0  OK of the rename dialog: store the new group name, redraw the skills window, save, delete this
    void __thiscall RenameGroupOK();
    // 0x0040f740  OK of the delete dialog: remove the group (renumber skills and names), redraw, delete this, save
    void __thiscall DeleteGroup();

    uint8_t pad_94[4];
    int32_t group;          // +0x98
    void *nameEdit;         // +0x9c  edit control, text at +0xec
};
static_assert(offsetof(CSkillGroupDlg, group) == 0x98, "CSkillGroupDlg::group");
static_assert(offsetof(CSkillGroupDlg, nameEdit) == 0x9c, "CSkillGroupDlg::nameEdit");

// ? Scrolling list gump (children are line gumps in a list at +0xb8, current at +0xc0, index +0xd4, count +0xcc).
class CGump_00410510 : public CGump {
public:
    // 0x00410510  scroll one entry back
    void __thiscall ScrollBack();
    // 0x004105d0  scroll one entry forward
    void __thiscall ScrollForward();
};

// ? Gump whose children (virtual slot 0xd8 non-zero) are laid out in rows of 0x1a pixels.
class CGump_004106a0 : public CGump {
public:
    // 0x004106a0  if some child row is above the top (y < 0): move all rows down by 0x1a
    void __thiscall ScrollRowsDown();
    // 0x00410750  if at least two rows are below the top (y > 0): move all rows up by 0x1a
    void __thiscall ScrollRowsUp();
};

// ? Hue picker gump ("CHueGump::Redraw" is 0x00442080): value at +0xd8, thumb position at +0xda, thumb control at +0x94.
class CHueGump_00410d30 : public CGump {
public:
    // 0x00410d30  step the position down
    void __thiscall StepDown();
    // 0x00410d70  step the position up (at most 4)
    void __thiscall StepUp();
};

// ? 0x00411010: deletes itself, plays the intro movie and shows a fresh main menu gump.
class CGump_00411010 : public CGump {
public:
    void __thiscall Run();
};

// ? 0x004111c0: handler of the 0x2ac-byte waiting gump (+0x2a0 child, +0x9c flag).
class CGump_004111c0 : public CGump {
public:
    void __thiscall Close();
};
