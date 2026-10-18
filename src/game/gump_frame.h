// 9-slice frame painter used by the gump classes that draw a bordered background into their frame0.
// 0x0041f540 (decomp/pass2/0x0041f540.cpp).
#pragma once

#include <cstdint>

// Paints art pieces baseArtId+0 .. +8 (TL, top, TR, left, centre, right, BL, bottom, BR) into the bitmap
// (16-bit width and height header, pixels from bmp + 2). When `flag` is 0 it also places random ornament pieces
// baseArtId+0xb .. +0x12 on the four edges.
// ECX = baseArtId, EDX = bmp, one stack argument, RET 4.
void __fastcall PaintGumpFrame(int baseArtId, uint16_t *bmp, int flag);
