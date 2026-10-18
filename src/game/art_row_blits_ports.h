// Prototypes of the ported procedures in art_row_blits.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

typedef void(__fastcall *RowBlitFn)(uint16_t *dst, uint16_t *src, int count);

void __fastcall RowBlitKeyedBlend(uint16_t *dst, uint16_t *src, int count);   // 0x00476410
void __fastcall RowBlitCopyNonZero(uint16_t *dst, uint16_t *src, int count);   // 0x004764c0
void __fastcall RowBlitBlendNonZero(uint16_t *dst, uint16_t *src, int count);   // 0x004764f0
void __fastcall RowBlitHalfMaskNonZero(uint16_t *dst, uint16_t *src, int count);   // 0x00476550
void __fastcall RowBlitLookupNonZero(uint16_t *dst, uint16_t *src, int count);   // 0x004765a0
void __fastcall RowBlitKeyedLookupNonZero(uint16_t *dst, uint16_t *src, int count);   // 0x00476600
void __fastcall RowBlitLookupBlendNonZero(uint16_t *dst, uint16_t *src, int count);   // 0x00476680
void __fastcall RowBlitKeyedLookupBlendNonZero(uint16_t *dst, uint16_t *src, int count);   // 0x00476710
void __fastcall SetBlitSurface(uint16_t *dest, int stride);   // 0x004767c0
void __fastcall RleArtWalkUnclipped(RowBlitFn blit, uint8_t *rec, int x, int y);   // 0x00476990
