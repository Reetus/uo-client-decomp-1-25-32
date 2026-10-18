// Prototypes of the ported procedures in cpu_speed_qpc.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

uint32_t __fastcall GetCpuSpeedMHz(int requestedMHz);   // 0x00478670
uint32_t * __fastcall MeasureCpuSpeedQPC(uint32_t guess, int /*edx*/, uint32_t *out);   // 0x00478bc0
