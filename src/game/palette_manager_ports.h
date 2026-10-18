// Prototypes of the ported procedures in palette_manager.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

int __stdcall PackRGB16(int r, int g, int b);   // 0x00436870
unsigned __stdcall UnpackRGB16(unsigned pixel);   // 0x004368b0
