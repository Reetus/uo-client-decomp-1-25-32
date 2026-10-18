// Prototypes of the ported procedures in map_gump_ops.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

int __stdcall IsOnScreen640x480(int sx, int sy);   // 0x004442d0
void __cdecl Pathfind_NextStep(void);   // 0x004448e0
void __cdecl PathReverseLinks(void);   // 0x00445080
int __stdcall MapDistancePoints(const int16_t *a, const int16_t *b);   // 0x00445280
