// Prototypes of the ported procedures in display_ops.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __cdecl CreateBackBuffer(void);   // 0x0047e1a0
void __cdecl DestroyBackBuffer(void);   // 0x0047e230
void __fastcall Convert555To565(uint16_t *dest, const uint16_t *src, int count);   // 0x0047e450
void __fastcall CopyPixels16(uint16_t *dest, const uint16_t *src, int count, int unused);   // 0x0047e4b0
