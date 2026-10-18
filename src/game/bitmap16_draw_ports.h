// Prototypes of the ported procedures in bitmap16_draw.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

int __stdcall Bitmap16_FillRect(int x0, int y0, int x1, int y1, int color, uint16_t *bmp);   // 0x00432c70
int __stdcall DrawLine16(int x0, int y0, int x1, int y1, int color, uint16_t *bmp);   // 0x00432cf0
