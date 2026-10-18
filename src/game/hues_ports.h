// Prototypes of the ported procedures in hues.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __cdecl LoadSkinHues(void);   // 0x00441060
void __fastcall HueCopyPixels(uint16_t *dst, uint16_t *src, int count, int hueIndex);   // 0x0047e700
