// Prototypes of the ported procedures in raster_pal.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __fastcall DrawTexturedTrianglePal(void *dest, const uint16_t *tex, int sizeShift, const int *pv0, const int *pv1, const int *pv2, int light0, int light1, int light2, const int *clip);   // 0x00450c60
