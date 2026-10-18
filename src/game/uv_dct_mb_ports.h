// Prototypes of the ported procedures in uv_dct_mb.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void Thunk_UvDecodeBlocksS32(void);   // 0x00487792
void Thunk_UvDecodeBlocksS16(void);   // 0x00487f71
void Thunk_UvOutputRowS32(void);   // 0x00488750
void Thunk_UvDecodeBlocksPacked(void);   // 0x00488892
void Thunk_UvDecodeBlocksLut32(void);   // 0x004889f2
void Thunk_UvReadTwoNibbles(void);   // 0x00488b52
void Thunk_UvOutputRowS16(void);   // 0x00489193
