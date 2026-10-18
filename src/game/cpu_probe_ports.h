// Prototypes of the ported procedures in cpu_probe.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

unsigned int __cdecl CpuFamilyProbe(void);   // 0x00429190
unsigned int __cdecl CpuFeatureFlags(void);   // 0x00429370
