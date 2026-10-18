// Prototypes of the ported procedures in present_backbuffer.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __cdecl Present_BackBuffer(void);   // 0x00482ca0
