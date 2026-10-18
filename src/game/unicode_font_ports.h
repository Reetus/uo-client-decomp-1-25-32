// Prototypes of the ported procedures in unicode_font.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __cdecl LoadUnifont(void);   // 0x0047c920
const uint8_t * __fastcall UnifontGetGlyph(uint16_t code, int *outX, int *outY, int *outW, int *outH);   // 0x0047c9d0
int __fastcall UnifontHasGlyph(uint16_t code);   // 0x0047ca80
void __fastcall DrawUnicodeString(uint16_t *dest, int pitch, const uint16_t *text, int x, int y, uint16_t color, const int *clip);   // 0x0047cae0
void __fastcall MeasureUnicodeString(const uint16_t *text, int *outWidth, int *outHeight, int *outYOff);   // 0x0047cc80
