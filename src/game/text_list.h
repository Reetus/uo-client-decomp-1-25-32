// CTextList: the history of text lines shown by the text gump (vtable 0x004a0838, instance at 0x006486b0:
// head 0x006486b4, tail 0x006486b8). Lines are CTextLine nodes (text_gump.h), 0x20 bytes each; fields not named
// there are reached with rawf: +0x08 line height, +0x0c baseline, +0x14 caller argument.
// Not given a sizeof: the object is a static one and the constructor writes up to +0x11.
#pragma once

#include <cstddef>
#include <cstdint>

#include "text_gump.h"

class CTextList {
public:
                            // +0x00  the compiler's vptr (table 0x004a0838)
    CTextLine *head;        // +0x04
    CTextLine *tail;        // +0x08
    int32_t count;          // +0x0c  at most 100 lines
    uint8_t flag10;         // +0x10  set to 1 by the constructor
    uint8_t flag11;         // +0x11

    // 0x00478e80
    CTextList();
    // Frees every line (text, then node). Defined inline in text_list.cpp so the scalar deleting destructor (0x00478ea0)
    // expands it; the atexit thunk 0x0044ab20 jumps to the plain function TextList_Dtor_00478ef0.
    virtual ~CTextList();
    // 0x00478f30: append a narrow line (RET 0xc); at 100 lines the oldest is dropped first.
    void __thiscall AddLine(char *text, int hue, int arg);
    // 0x00479070: append a Unicode line (RET 0xc).
    void __thiscall AddLineUnicode(uint16_t *text, int hue, int arg);
    // 0x00479380 / 0x00479400: AddLineUnicode / AddLine with arg 0, log the text, and refresh every text gump (RET 0xc;
    // the third argument is not read).
    void __thiscall AddLineUnicodeAndLog(uint16_t *text, int hue, int unused);
    void __thiscall AddLineAndLog(char *text, int hue, int unused);
};

static_assert(offsetof(CTextList, head) == 0x04, "CTextList::head");
static_assert(offsetof(CTextList, tail) == 0x08, "CTextList::tail");
static_assert(offsetof(CTextList, count) == 0x0c, "CTextList::count");
static_assert(offsetof(CTextList, flag10) == 0x10, "CTextList::flag10");

// 0x00478ef0: the destructor as the atexit thunk jumps to it (ECX = object).
void __fastcall TextList_Dtor_00478ef0(void *self, void *);
