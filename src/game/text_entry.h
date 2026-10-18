// CTextEntry: the chat / text-entry object. It collects typed characters in a
// narrow or wide buffer (wide when the global unicode flag 0x004b7234 is set),
// shows them through a CInputGump at +0x11c, and sends the finished line.
// Layout from decomp/pass2/0x004430b0.cpp, 0x00443590.cpp, 0x00442cb0.cpp.
// Object size 0x22c (WinMain allocates it).
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"

class CTextEntry {
public:
    // 0x00442910: format a text line into a 64K stack buffer, send it, reset the entry.
    void __thiscall SendLine(int arg1, int arg2);
    // 0x00442cb0: try the typed line as a dot-command; returns 1 if consumed.
    int __thiscall TryDotCommand();
    // 0x004430b0: handle one typed character.
    void __thiscall HandleChar(int ch);
    // 0x00443590: refresh the on-screen input line from the typed buffer, with a mode prefix.
    void __thiscall RefreshLine();

    // Second pass, text_entry_ops.cpp.
    // 0x00442980  size 0x2f: real constructor, sets the vtable (0x0049f5dc) and clears the fields.
    CTextEntry();
    // 0x004429b0: scalar deleting destructor (vtable slot 0, the compiler's).
    virtual ~CTextEntry();
    // 0x004429f0: RET 8, (virtual key, extra); returns 0 for Alt+F4, Alt+Tab and keys outside the cursor-pad and Ctrl+Q cases, else 1.
    int __thiscall OnKeyDown(int key, int extra);
    // 0x004430a0: RET 4, set the chat mode (+0x114).
    void __thiscall SetMode(int newMode);

    int32_t unk_04;           // +0x04  '+' / '-' decrement it instead of typing while nonzero
    int32_t length;           // +0x08  characters typed
    uint8_t buffer[0x108];    // +0x0c  narrow chars, or wide chars when the unicode flag is set  // ? real size: 0x80 chars + terminator
    int32_t mode;             // +0x114  0 say, 1 broadcast, 2 emote, 5 command, 8 whisper, 9 yell
    int32_t echoCommands;     // +0x118  echo a consumed dot-command as a bark
    void *inputGump;          // +0x11c  CInputGump
    uint8_t lastLine[0x108];  // +0x120  last entered line
    int32_t lastMode;         // +0x228
};

static_assert(offsetof(CTextEntry, unk_04) == 0x04, "CTextEntry::unk_04");
static_assert(offsetof(CTextEntry, length) == 0x08, "CTextEntry::length");
static_assert(offsetof(CTextEntry, buffer) == 0x0c, "CTextEntry::buffer");
static_assert(offsetof(CTextEntry, mode) == 0x114, "CTextEntry::mode");
static_assert(offsetof(CTextEntry, echoCommands) == 0x118, "CTextEntry::echoCommands");
static_assert(offsetof(CTextEntry, inputGump) == 0x11c, "CTextEntry::inputGump");
static_assert(offsetof(CTextEntry, lastLine) == 0x120, "CTextEntry::lastLine");
static_assert(offsetof(CTextEntry, lastMode) == 0x228, "CTextEntry::lastMode");
static_assert(sizeof(CTextEntry) == 0x22c, "CTextEntry size (operator new(0x22c) in WinMain)");
