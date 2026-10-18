// Prototypes of the ported procedures in missile_draw.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __fastcall MissileDrawTileArt(uint16_t artId, uint8_t angle, int *rect, int *clip);   // 0x00437e60
