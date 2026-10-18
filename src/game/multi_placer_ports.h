// Prototypes of the ported procedures in multi_placer.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __cdecl MultiObjects_DeleteAll(void);   // 0x0040cf30
