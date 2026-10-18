// Prototypes of the ported procedures in gump_cascade.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __fastcall GumpCascade_Alloc(int32_t *outX, int32_t *outY);   // 0x0040d1b0
void __fastcall GumpCascade_Free(int32_t x, int32_t y);   // 0x0040d300
