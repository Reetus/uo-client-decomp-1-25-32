// Prototypes of the ported procedures in effect_gump.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __stdcall CreateEffectGump(unsigned int kind, int p2, int p3, unsigned short p4, int p5, unsigned short p6, int p7, unsigned short p8, unsigned char p9, unsigned char p10, unsigned char p11, unsigned char p12, unsigned char p13, unsigned char p14);   // 0x00437190
