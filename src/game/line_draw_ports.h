// Prototypes of the ported procedures in line_draw.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

int __stdcall DrawLine16Shaded(int x0, int y0, int x1, int y1, uint16_t *bmp);   // 0x00432f90
int __stdcall DrawLine16FromSource(int x0, int y0, int x1, int y1, uint16_t *bmp, uint16_t *src);   // 0x004332f0
