// Handlers of the character creation pages and the server / skill / profession pickers whose receiving class is
// not identified yet. Each is a __thiscall on the gump the button reports to; they are grouped in one stand-in
// class (as CGumpCallbacks does) only so that they can be members. Fields are reached by offset.
// ? Replace CGumpHandlers04 by the real classes when they are known. The names below are from behaviour.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CGumpHandlers04 : public CGump {
public:
    // 0x004113a0: skill button: hides the gump at +0x98 and opens the skill list over the field at +0xa4.
    void __thiscall OpenSkillPicker();
    // 0x00411990: sends packet 0x2C with 2, then deletes this gump.
    void __thiscall SendPacket2cAndDelete();
    // 0x004119d0: sends packet 0x2C with 1.
    void __thiscall SendPacket2cOne();
    // 0x00411a00: profession button: hides the gump at +0xc8 and opens the profession list over the field at +0xc4.
    void __thiscall OpenProfessionPicker();
    // 0x00411b70 / 0x00411bb0: previous / next entry of a scroll choice gump (current +0xbc, count +0xc0,
    // thumb +0x22c0).
    void __thiscall ScrollChoicePrev();
    void __thiscall ScrollChoiceNext();
    // 0x004120f0: colour picker OK: stores the two thumb positions as percentages in 0x0069f86c / 0x0069f870 and
    // the frame index of +0xb8 in 0x0069f868, sends the packet, deletes this gump.
    void __thiscall HuePickerOk();
    // 0x00412280 / 0x004123e0: add a resize-gump backdrop child (heights 0xdc / 0x104) below this gump.
    void __thiscall AddBackdropChildDc();
    void __thiscall AddBackdropChild104();
    // 0x00412740: menu cancel: sends packet 0x7d with no choice, deletes this gump (serial +0x2ac, model +0x2b0).
    void __thiscall MenuCancel();
    // 0x00412790: menu OK: sends the choice of the checked radio child, or asks for one.
    void __thiscall MenuOk();
    // 0x00412850: sends packet 0x05 with the word at 0x005285fc, then deletes this gump.
    void __thiscall SendPacket05AndDelete();
    // 0x00412b30 / 0x00412c50 / 0x00412d70: refill a scroll choice gump with the server list sorted by one of
    // three keys (the last pings the servers first).
    void __thiscall SortServersByKeyA();
    void __thiscall SortServersByKeyB();
    void __thiscall SortServersByPing();
    // 0x00412e90 / 0x00412f20: text entry dialog reply (packet 0xAC) with flag 1 / 0, then deletes this gump.
    void __thiscall TextEntryOk();
    void __thiscall TextEntryCancel();
};

static_assert(sizeof(CGumpHandlers04) == 0x94, "CGumpHandlers04 adds no fields");
