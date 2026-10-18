// CInputControlGump: single/multi-line text entry and drop-down caption control.
// operator new(0x158). Vtable 0x0049d9b8, name "InputControl gump".
// Layout from decomp/pass2/0x0041db90.cpp (ctor), 0x0041e050.cpp (dtor) and 0x0041e0f0.cpp (Redraw),
// plus the fields callers set after construction (login, client config, character creation).
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CInputControlGump : public CGump {
public:
    virtual int IsTagc8() { return 1; }
    // Gump prep P5: slot target formerly a free function (vtable_stubs_u2.cpp).
    virtual void MouseLeave();
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual void Draw(int *clip);
    virtual int HitTest(int x, int y);
    // 0x0041db90  size 0x25c  variant A, sized from art (x, y, parent (not read), art id, drop-down art, mode, max length,
    // font, hue, a10, a11, width override). RET 0x30.
    CInputControlGump(int x_, int y_, void *parentUnused, int artId, int a5, uint8_t mode_, int a7, int16_t font_,
                      int16_t hue_, int a10, int a11, int widthOverride);
    // 0x0041de20  size 0x226  variant B, explicit width and height (16-bit parameters: the original keeps the low half of
    // the pushed dword), transparent (x, y, parent (not read), w, h, mode, max length, font, hue, a10, a11, sized flag).
    // RET 0x30.
    CInputControlGump(int x_, int y_, void *parentUnused, uint16_t w, uint16_t h, uint8_t mode_, int maxLen, int16_t font_,
                      int16_t hue_, int a10, int a11, int sizedFlag);
    // 0x0041e050: destructor body (the vtable slot 0 scalar deleting destructor is 0x0041ddf0).
    ~CInputControlGump();
    // 0x0041e0f0: repaint the control into its frame0 bitmap.
    void __thiscall Redraw();

    // Added by porting pass 2 (batch 07).
    // 0x0041e9f0: RET 8 (px, py); clears g_pDragGump and forwards to the parent's vtable slot 0x1c.
    virtual void RButtonUp(int x, int y);
    // 0x0041ea20: RET 8 (px, py).
    virtual void MouseDown(int x, int y);
    // 0x0041ec10: RET 8 (px, py).
    virtual void MouseUp(int x, int y);
    // 0x0041ecb0: RET 8 (px, py); sets the cursor and the d4 flag.
    virtual void MouseOver(int x, int y);
    // 0x0041ee00: plain RET. Double-click timeout.
    virtual void Tick();
    // 0x0041ee50: plain RET, always returns 1. Scrolls so the caret stays visible.
    int __thiscall ScrollToCaret();
    // 0x0041ef60: RET 4. Key press (ASCII or 0xe0xx extended code).
    virtual void OnKey(int key);

    int32_t unk_94;
    int32_t unk_98;
    int32_t unk_9c;
    int32_t unk_a0;
    int32_t unk_a4;
    int32_t unk_a8;
    int32_t unk_ac;        // +0xac  a10
    int32_t password;      // +0xb0  non-zero: text is shown as '*'
    int32_t dropDown;      // +0xb4  non-zero: caption / drop-down style; also selects the +0x140 art when set
    int32_t centered;      // +0xb8  a11: caption is centered
    int32_t unk_bc;
    int32_t sizedByArg;    // +0xc0  (widthOverride != 0): background drawn through 0x0041fdf0
    int32_t transparent;   // +0xc4  non-zero: frame0 is cleared instead of drawn from art
    int32_t multiline;     // +0xc8  non-zero: wrapped multi-line text
    int32_t unk_cc;
    int32_t unk_d0;
    int32_t flag_d4;       // +0xd4  ? tested together with artDe
    int32_t focused;       // +0xd8  non-zero: use artFocused (+0xe0)
    uint16_t artEmpty;     // +0xdc  background art while the text is empty
    uint16_t artDe;         // +0xde  background art used when flag_d4 is set
    uint16_t artFocused;   // +0xe0
    uint8_t marginX;       // +0xe2  text x margin (default 3)
    uint8_t marginY;       // +0xe3  text y margin (default 2)
    uint8_t mode;          // +0xe4  0 / 2 text, anything else numeric
    uint8_t pad_e5[3];
    int32_t value;         // +0xe8  numeric value / selected index
    char text[0x54];       // +0xec
    int32_t artDropDown;   // +0x140  a5: background art used when dropDown is set
    int32_t maxLength;     // +0x144  a7
    int32_t caret;         // +0x148
    int32_t scroll;        // +0x14c  first visible character
    uint16_t font;         // +0x150
    uint16_t hue;          // +0x152
    int32_t unk_154;
};

static_assert(offsetof(CInputControlGump, unk_ac) == 0xac, "CInputControlGump::unk_ac");
static_assert(offsetof(CInputControlGump, password) == 0xb0, "CInputControlGump::password");
static_assert(offsetof(CInputControlGump, dropDown) == 0xb4, "CInputControlGump::dropDown");
static_assert(offsetof(CInputControlGump, centered) == 0xb8, "CInputControlGump::centered");
static_assert(offsetof(CInputControlGump, sizedByArg) == 0xc0, "CInputControlGump::sizedByArg");
static_assert(offsetof(CInputControlGump, transparent) == 0xc4, "CInputControlGump::transparent");
static_assert(offsetof(CInputControlGump, multiline) == 0xc8, "CInputControlGump::multiline");
static_assert(offsetof(CInputControlGump, flag_d4) == 0xd4, "CInputControlGump::flag_d4");
static_assert(offsetof(CInputControlGump, focused) == 0xd8, "CInputControlGump::focused");
static_assert(offsetof(CInputControlGump, artEmpty) == 0xdc, "CInputControlGump::artEmpty");
static_assert(offsetof(CInputControlGump, artDe) == 0xde, "CInputControlGump::artDe");
static_assert(offsetof(CInputControlGump, artFocused) == 0xe0, "CInputControlGump::artFocused");
static_assert(offsetof(CInputControlGump, marginX) == 0xe2, "CInputControlGump::marginX");
static_assert(offsetof(CInputControlGump, marginY) == 0xe3, "CInputControlGump::marginY");
static_assert(offsetof(CInputControlGump, mode) == 0xe4, "CInputControlGump::mode");
static_assert(offsetof(CInputControlGump, value) == 0xe8, "CInputControlGump::value");
static_assert(offsetof(CInputControlGump, text) == 0xec, "CInputControlGump::text");
static_assert(offsetof(CInputControlGump, artDropDown) == 0x140, "CInputControlGump::artDropDown");
static_assert(offsetof(CInputControlGump, maxLength) == 0x144, "CInputControlGump::maxLength");
static_assert(offsetof(CInputControlGump, caret) == 0x148, "CInputControlGump::caret");
static_assert(offsetof(CInputControlGump, scroll) == 0x14c, "CInputControlGump::scroll");
static_assert(offsetof(CInputControlGump, font) == 0x150, "CInputControlGump::font");
static_assert(offsetof(CInputControlGump, hue) == 0x152, "CInputControlGump::hue");
static_assert(sizeof(CInputControlGump) == 0x158, "CInputControlGump size (operator new(0x158))");
