// Gumps of the character select / character creation screens that this batch
// touches. Layouts are partial (only offsets the ported methods use). Names of
// the classes marked "?" are inferred from the pass2 descriptions, not from
// RTTI or strings: the real class names are not established yet.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

// Character select gump ("Change password" button). Slot controls are an array of CGump * at +0xb0
// (each is an edit control whose text is at ctl+0xec); +0x98 is the selected slot index.
// 0x0040f180 ? class name inferred from the pass2 description.
class CCharSelectGump : public CGump {
public:
    // 0x0040f180: open a CChgPWGump for the selected character (SEH frame omitted).
    void __thiscall OnChangePassword();
    // 0x0040ebe0  Play button: ask for the character password (CPlayCharGump) or log in directly
    void __thiscall Play();
    // 0x0040ed80  Back button: delete this gump, send the disconnect packet, return to the main menu
    void __thiscall Disconnect();
    // 0x0040ee50  New Character button (at most 5): open the stat and skill pages
    void __thiscall NewCharacter();
    // 0x0040ef50  Delete button: open a CDeleteCharGump for the selected character
    void __thiscall OpenDeleteDialog();

    uint8_t pad_94[4];
    int32_t selectedSlot;   // +0x98
    uint8_t pad_9c[0xb0 - 0x9c];
    CGump *slots;           // +0xb0  first of the slot controls (array)
};

static_assert(offsetof(CCharSelectGump, selectedSlot) == 0x98, "CCharSelectGump::selectedSlot");
static_assert(offsetof(CCharSelectGump, slots) == 0xb0, "CCharSelectGump::slots");

// Node of a control's item list (control + 0xb8): entry control and next node.
struct CtlListNode {
    CGump *ctl;
    CtlListNode *next;
};
static_assert(sizeof(CtlListNode) == 8, "CtlListNode size");

// Character creation gump: collects the selections into the character-creation globals.
// 0x0040fc30, 0x004118b0 ? class name inferred from the pass2 description.
class CCharCreateGump : public CGump {
public:
    // 0x0040fc30: copy the selected values of the child controls into the globals.
    void __thiscall CollectSelections();
    // 0x004118b0: open the "Emote Sample" gump, or bring the existing one to the front.
    void __thiscall ToggleEmoteSample();
    // 0x004117d0: open the "Speech Sample" gump (hue hue_c0), or bring the existing one to the front.
    void __thiscall ToggleSpeechSample();

    uint8_t pad_94[0xc0 - 0x94];
    uint16_t hue_c0;       // +0xc0
    uint16_t hue_c2;       // +0xc2  (also the hue of the Emote Sample gump)
    uint8_t pad_c4[4];
    CGump *ctl_c8;          // +0xc8  value at ctl+0xb0 (a)
    CGump *ctl_cc;          // +0xcc  value at ctl+0xb0 (b)
    uint8_t pad_d0[4];
    CGump *ctl_d4;          // +0xd4
    CGump *ctl_d8;          // +0xd8
    CGump *ctl_dc;          // +0xdc
    CGump *ctl_e0;          // +0xe0
    uint8_t pad_e4[0xf4 - 0xe4];
    CGump *page_f4;         // +0xf4  gump whose layer byte (+0x54) == 3 selects the skills page
    CGump *ctl_f8;          // +0xf8
    uint8_t pad_fc[0x10c - 0xfc];
    CGump *list_10c;        // +0x10c  list control (count at +0xcc, items at +0xb8)
    CGump *list_110;        // +0x110
    uint8_t pad_114[0x12c - 0x114];
    CGump *list_12c;        // +0x12c
    CGump *list_130;        // +0x130
    CGump *ctl_134;         // +0x134
    uint8_t pad_138[0x164 - 0x138];
    uint8_t *profList;      // +0x164  linked list copied into the globals (node: next at +0xe0)
};

static_assert(offsetof(CCharCreateGump, hue_c0) == 0xc0, "CCharCreateGump::hue_c0");
static_assert(offsetof(CCharCreateGump, hue_c2) == 0xc2, "CCharCreateGump::hue_c2");
static_assert(offsetof(CCharCreateGump, ctl_c8) == 0xc8, "CCharCreateGump::ctl_c8");
static_assert(offsetof(CCharCreateGump, ctl_cc) == 0xcc, "CCharCreateGump::ctl_cc");
static_assert(offsetof(CCharCreateGump, ctl_d4) == 0xd4, "CCharCreateGump::ctl_d4");
static_assert(offsetof(CCharCreateGump, ctl_d8) == 0xd8, "CCharCreateGump::ctl_d8");
static_assert(offsetof(CCharCreateGump, ctl_dc) == 0xdc, "CCharCreateGump::ctl_dc");
static_assert(offsetof(CCharCreateGump, ctl_e0) == 0xe0, "CCharCreateGump::ctl_e0");
static_assert(offsetof(CCharCreateGump, page_f4) == 0xf4, "CCharCreateGump::page_f4");
static_assert(offsetof(CCharCreateGump, ctl_f8) == 0xf8, "CCharCreateGump::ctl_f8");
static_assert(offsetof(CCharCreateGump, list_10c) == 0x10c, "CCharCreateGump::list_10c");
static_assert(offsetof(CCharCreateGump, list_110) == 0x110, "CCharCreateGump::list_110");
static_assert(offsetof(CCharCreateGump, list_12c) == 0x12c, "CCharCreateGump::list_12c");
static_assert(offsetof(CCharCreateGump, list_130) == 0x130, "CCharCreateGump::list_130");
static_assert(offsetof(CCharCreateGump, ctl_134) == 0x134, "CCharCreateGump::ctl_134");
static_assert(offsetof(CCharCreateGump, profList) == 0x164, "CCharCreateGump::profList");

// 0x00410110: gump (class unknown) with a state byte at +0x54 and a reference at +0xf0.
class CGump_00410110 : public CGump {
public:
    // Switch to state 2: release the focus if +0xf0 is the focused gump and redraw frame 0 with art 0x961.
    void __thiscall SetStateTwo();

    uint8_t pad_94[0xf0 - 0x94];
    int32_t ref_f0;         // +0xf0  compared with the focus global 0x006488d0
};
static_assert(offsetof(CGump_00410110, ref_f0) == 0xf0, "CGump_00410110::ref_f0");

// 0x00410820: gump (class unknown) that sends a type-5 packet naming itself, then deletes itself.
class CGump_00410820 : public CGump {
public:
    void __thiscall SendAndDelete();
};

// 0x00410910: gump (class unknown) that sends a type-6 packet naming two serials.
// owner (+0x34) is the object; owner+0x7c points at a second object; serials are at +0x78.
class CGump_00410910 : public CGump {
public:
    void __thiscall SendOwnerPacket();
};
