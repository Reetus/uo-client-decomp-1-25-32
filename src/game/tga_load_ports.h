// Prototypes of the ported procedures in tga_load.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

int __fastcall LoadTGA16(const char *fileName, uint16_t *bitmap);   // 0x0043ebe0
