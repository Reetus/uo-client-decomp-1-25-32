// Prototypes of the ported procedures in gump_art_ops.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

typedef void(__fastcall *RowBlitFn)(uint16_t *dst, uint16_t *src, int count);

void __fastcall GumpArtSizeImpl(int artId, int *outW, int *outH);   // 0x00433e70
void __fastcall GumpBlitBitmap(RowBlitFn rowBlit, uint16_t *src, int w, int h, int x, int y, uint16_t *dest, int stride, int destH);   // 0x00434260
void __fastcall GumpBlitBitmapClipped(RowBlitFn rowBlit, uint16_t *src, const RECT *clip, int w, int h, int x, int y, uint16_t *dest, int stride, int destH);   // 0x004342e0
uint16_t __fastcall GumpArtPixel(int artId, int x, int y);   // 0x00434410
