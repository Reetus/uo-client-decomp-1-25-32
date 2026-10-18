// Prototypes of the ported procedures in map_world_ops.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

struct Pos3;

int __stdcall Map_LineOfSightClear(Pos3 from, Pos3 to, uint32_t mask);   // 0x00446190
void * __stdcall Map_Alloc(size_t size, const char *name);   // 0x004462f0
void __stdcall Map_Free(void *p, int unused);   // 0x00446310
