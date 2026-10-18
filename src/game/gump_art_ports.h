// Prototypes of the ported procedures in gump_art.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

typedef void(__fastcall *RowBlitFn)(uint16_t *dst, uint16_t *src, int count);

void __fastcall DrawGumpArtClipped(RowBlitFn rowBlit, int artId, const RECT *clip, int x, int y, uint16_t *dest, int stride, int destH);   // 0x00434060
