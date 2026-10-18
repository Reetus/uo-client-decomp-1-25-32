// Keyboard macro records (macros.txt), built by CApp::LoadKeynamesAndMacros and
// run by CApp::RunMacro. decomp/pass2/0x0044d780.cpp, 0x0044c8e0.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

struct MacroAction {        // 0x14 bytes, parsed by 0x0044d140
    int32_t type;           // +0x00  1..21 (RunMacro switch)
    int32_t arg;            // +0x04
    char *text;             // +0x08  malloc'd text (types 1-4)
    int32_t unk_0c;         // +0x0c
    int32_t unk_10;         // +0x10
};
static_assert(sizeof(MacroAction) == 0x14, "MacroAction size");
static_assert(offsetof(MacroAction, arg) == 0x04, "MacroAction::arg");
static_assert(offsetof(MacroAction, text) == 0x08, "MacroAction::text");

struct MacroRecord {        // 0xe4 bytes (operator new in LoadKeynamesAndMacros)
    int32_t vkey;           // +0x00  MapVirtualKeyA(scan, 1)
    int32_t scan;           // +0x04  scan << 16 (lParam style)
    int32_t shift;          // +0x08
    int32_t alt;            // +0x0c
    int32_t ctrl;           // +0x10
    int32_t count;          // +0x14  actions in use
    MacroAction actions[10];   // +0x18
    MacroRecord *next;      // +0xe0
};
static_assert(sizeof(MacroRecord) == 0xe4, "MacroRecord size");
static_assert(offsetof(MacroRecord, shift) == 0x08, "MacroRecord::shift");
static_assert(offsetof(MacroRecord, alt) == 0x0c, "MacroRecord::alt");
static_assert(offsetof(MacroRecord, ctrl) == 0x10, "MacroRecord::ctrl");
static_assert(offsetof(MacroRecord, count) == 0x14, "MacroRecord::count");
static_assert(offsetof(MacroRecord, actions) == 0x18, "MacroRecord::actions");
static_assert(offsetof(MacroRecord, next) == 0xe0, "MacroRecord::next");
