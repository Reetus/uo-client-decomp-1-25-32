// Prototypes of the ported procedures in walk_manager_ops.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

struct WalkProbe;

int __fastcall Walk_GetZVal(const int8_t *z, int flag, int mode, int dir);   // 0x00448600
int __cdecl CompareColumnEntries(const void *a, const void *b);   // 0x004487e0
int __fastcall Walk_CollectCell(WalkProbe *out, int x, int y, int mode);   // 0x004488d0
