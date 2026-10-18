// Prototypes of the ported procedures in cpu_speed.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

uint32_t * __fastcall CpuSpeedMeasure(int requestedMHz, int , uint32_t *out);   // 0x00478740
