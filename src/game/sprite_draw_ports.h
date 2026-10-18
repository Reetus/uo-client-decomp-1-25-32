// Prototypes of the ported procedures in sprite_draw.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __fastcall DrawRleSpriteClipped(int A, uint8_t *sprite, int *rect, int x, int y);   // 0x004246e0
