// Prototypes of the ported procedures in walk_queue.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

int __fastcall Walk_FindStandZ(int x, int y, int z, int dir, int *outZ);   // 0x00448c90
void __fastcall Walk_ScrollOffset(unsigned dir, int n);   // 0x00449350
void __fastcall Walk_PathfindTo(uint16_t x16, uint16_t y16, uint32_t z32);   // 0x00449b10
void __fastcall Walk_ForcedThunk(uint8_t dir);   // 0x0044a5f0
