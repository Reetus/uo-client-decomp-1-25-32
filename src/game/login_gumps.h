// Login-flow gumps: CLoginGump, CAcctLoginGump, CChgPWGump, CDeleteCharGump.
// Layouts partial; see decomp/pass2/0x004212d0.cpp .. 0x00422910.cpp.
// Child controls (input fields, buttons, check boxes) are classes not ported
// yet, so they are held as opaque pointers.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CInputControlGump;

// "Login" gump (vtable 0x0049dbe8).
class CLoginGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    // Gump prep P5: slot target formerly a free function (vtable_stubs_u2.cpp).
    virtual void OnOk();
    virtual void OnCancel();
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual void Draw(int *clip);
    virtual int HitTest(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void RButtonUp(int x, int y);
    virtual void OnChildChanged(int child);
    virtual void OnTab();
    virtual void Slot64(int a);
    // 0x00420b90: constructor (batch 01): background art, five input fields, five buttons.
    CLoginGump();
    // 0x004212d0: destructor: delete the five child controls, free frame data (the base destructor follows).
    ~CLoginGump();
    void Dtor() { this->CLoginGump::~CLoginGump(); }

    // Added by porting pass 2 (batch 07). unk_94 is the number of filled entries, state[0..4]
    // are the five entry values (-1 = free) and children[i] the input control of entry i.
    // 0x00421390: RET 4, empty virtual override.
    // 0x004213c0: RET 4. Moves the focus flag to the field `ctl`.
    virtual void OnFocus(int field);
    // 0x00421440: RET 4. Calls vtable slot 0x54(arg), then 0x0040ee50 when the focused field is empty, else 0x0040ebe0.
    virtual void OnEnter(int a);
    // 0x00421490: RET 0xc (text, unused, value). Puts `text` into the next free field.
    void __thiscall AddEntry(const char *text, int unused, int value);
    // 0x00421540: RET 4. Removes the entry whose value matches, shifting the later ones down.
    void __thiscall RemoveEntry(int value);

    int32_t unk_94;         // +0x94  set to 0 by the constructor
    int32_t focusIndex;     // +0x98  index into children[] of the initially focused field (from DAT_0069f9e8)
    int32_t state[5];       // +0x9c  entry values, -1 after field i+1 is built (+0x9c, +0xa0, +0xa4, +0xa8, +0xac)
    CInputControlGump *children[5];   // +0xb0  input controls, deleted through vtable slot 0
};

static_assert(offsetof(CLoginGump, focusIndex) == 0x98, "CLoginGump::focusIndex");
static_assert(offsetof(CLoginGump, state) == 0x9c, "CLoginGump::state");
static_assert(offsetof(CLoginGump, children) == 0xb0, "CLoginGump::children");
static_assert(sizeof(CLoginGump) == 0xc4, "CLoginGump size (operator new(0xc4))");

// "AcctLogin gump" (vtable 0x0049dd00): account name, password, check box.
class CAcctLoginGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    // Gump prep P5: slot target formerly a free function (vtable_stubs_u2.cpp).
    virtual void OnTab();
    virtual void OnOk();
    virtual void OnCancel();
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual void Draw(int *clip);
    virtual int HitTest(int x, int y);
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void RButtonUp(int x, int y);
    virtual void OnChildChanged(int child);
    virtual void Slot64(int a);
    // 0x00421660: constructor
    CAcctLoginGump();
    // 0x00421ab0: destructor (the base destructor follows)
    ~CAcctLoginGump();
    void Dtor() { this->CAcctLoginGump::~CAcctLoginGump(); }
    // 0x0040f3b0  Connect: copy the account fields, connect if needed, send the account login, save uo.cfg settings
    void __thiscall Connect();
    // 0x0040f600  Cancel: delete this gump, show the main menu, close the connection
    void __thiscall Cancel();

    // Added by porting pass 2 (batch 07).
    // 0x00421c50: RET 4. Moves the focus flag between the two input controls.
    virtual void OnFocus(int field);

    void *nameInput;        // +0x94
    void *passwordInput;    // +0x98
    void *checkBox;         // +0x9c
};

static_assert(offsetof(CAcctLoginGump, nameInput) == 0x94, "CAcctLoginGump::nameInput");
static_assert(offsetof(CAcctLoginGump, passwordInput) == 0x98, "CAcctLoginGump::passwordInput");
static_assert(offsetof(CAcctLoginGump, checkBox) == 0x9c, "CAcctLoginGump::checkBox");
static_assert(sizeof(CAcctLoginGump) == 0xa0, "CAcctLoginGump size (operator new(0xa0))");

// "ChgPW gump" (vtable 0x0049de18): change password dialog.
class CChgPWGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    // Gump prep P5: slot target formerly a free function (vtable_stubs_u2.cpp).
    virtual void OnTab();
    virtual void OnOk();
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual int HitTest(int x, int y);
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void RButtonUp(int x, int y);
    virtual void MouseMove(int x, int y);
    virtual void OnCancel();
    // 0x00421ce0: constructor (unused, account), both char *; the original is not told them apart by anything else.
    CChgPWGump(char *unused, char *account);
    // 0x004221e0: destructor (the base destructor follows)
    ~CChgPWGump();
    void Dtor() { this->CChgPWGump::~CChgPWGump(); }
    // 0x0040f250  OK: compare the two new passwords, send the change-password packet, show the waiting gump
    void __thiscall Confirm();

    // Added by porting pass 2 (batch 07).
    // 0x004222b0: RET 4. Draw handler (vtable slot 0x08): blits frame0 clipped to `clip`, then the children.
    virtual void Draw(int *clip);
    // 0x00422470: RET 4. Moves the focus flag between the three password controls.
    virtual void OnFocus(int field);

    int32_t unk_94;         // +0x94  set to 0
    char account[0x20];     // +0x98  strcpy target; size is a guess up to the first control  // ?
    void *oldPassword;      // +0xb8
    void *newPassword;      // +0xbc
    void *verifyPassword;   // +0xc0
};

static_assert(offsetof(CChgPWGump, account) == 0x98, "CChgPWGump::account");
static_assert(offsetof(CChgPWGump, oldPassword) == 0xb8, "CChgPWGump::oldPassword");
static_assert(offsetof(CChgPWGump, newPassword) == 0xbc, "CChgPWGump::newPassword");
static_assert(offsetof(CChgPWGump, verifyPassword) == 0xc0, "CChgPWGump::verifyPassword");
static_assert(sizeof(CChgPWGump) == 0xc4, "CChgPWGump size (operator new(0xc4))");

// "DeleteChar gump" (vtable 0x0049df30): confirm deleting a character.
class CDeleteCharGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    // Gump prep P5: slot target formerly a free function (vtable_stubs_u2.cpp).
    virtual void OnOk();
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual void Draw(int *clip);
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void RButtonUp(int x, int y);
    virtual void MouseMove(int x, int y);
    virtual void OnCancel();
    // 0x00422590: constructor (charName, unused), both char *
    CDeleteCharGump(char *charName, char *unused);
    // 0x00422910: destructor (the base destructor follows)
    ~CDeleteCharGump();
    void Dtor() { this->CDeleteCharGump::~CDeleteCharGump(); }
    // 0x0040f020  OK: send the delete-character packet for the selected slot and show the waiting gump
    void __thiscall DelChar();

    // Added by porting pass 2 (batch 07).
    // 0x004229c0: RET 8 (px, py). Non-zero when the point is inside frame0.
    virtual int HitTest(int x, int y);

    int32_t unk_94;         // +0x94  set to 0
    void *password;         // +0x98  password input control
};

static_assert(offsetof(CDeleteCharGump, unk_94) == 0x94, "CDeleteCharGump::unk_94");
static_assert(offsetof(CDeleteCharGump, password) == 0x98, "CDeleteCharGump::password");
static_assert(sizeof(CDeleteCharGump) == 0x9c, "CDeleteCharGump size (operator new(0x9c))");
