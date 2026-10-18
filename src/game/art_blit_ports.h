// Prototypes of the ported procedures in art_blit.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

typedef void(__fastcall *RowBlitFn)(uint16_t *dst, uint16_t *src, int count);

void __fastcall RowBlend(uint16_t *dst, uint16_t *src, int count);   // 0x00476330
void __fastcall RleArtWalk(RowBlitFn blit, uint8_t *rec, int *clip, int x, int y);   // 0x004767d0
