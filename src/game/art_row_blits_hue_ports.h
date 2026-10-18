// Prototypes of the ported procedures in art_row_blits_hue.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __fastcall RowBlitCopyAll(uint16_t *dst, uint16_t *src, int count);   // 0x004761e0
void __fastcall RowBlitLookupAll(uint16_t *dst, uint16_t *src, int count);   // 0x00476210
void __fastcall RowBlitKeyedLookupAll(uint16_t *dst, uint16_t *src, int count);   // 0x00476260
void __fastcall RowBlitBlendAll(uint16_t *dst, uint16_t *src, int count);   // 0x004762d0
void __fastcall RowBlitLookupBlendAll(uint16_t *dst, uint16_t *src, int count);   // 0x00476390
