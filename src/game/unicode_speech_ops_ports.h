// Prototypes of the ported procedures in unicode_speech_ops.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

class CUnicodeBarkGump;

void WcsLenThunk(void);   // 0x0047c060
void __fastcall WcsCpy(uint16_t *dst, const uint16_t *src);   // 0x0047c080
void __fastcall WidenStrCpy(uint16_t *dst, const char *src);   // 0x0047c0a0
void __fastcall WcsCat(uint16_t *dst, const uint16_t *src);   // 0x0047c0c0
void __fastcall WidenStrCat(uint16_t *dst, const char *src);   // 0x0047c0f0
void __fastcall NarrowStrCpy(char *dst, const uint16_t *src);   // 0x0047c120
void __fastcall FilterUnicodeText(uint16_t *text);   // 0x0047c150
CUnicodeBarkGump * __stdcall CreateUnicodeBarkGump(int hue, int font, uint16_t *text, int extra);   // 0x0047c4d0
