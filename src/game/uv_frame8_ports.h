// Prototypes of the ported procedures in uv_frame8.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

struct UvPlayer8;

int __fastcall UvDecodeFrameSetup(UvPlayer8 *self, const uint8_t *stream);   // 0x00489f80
int __fastcall UvAllocFrameBuffers(UvPlayer8 *self, const uint8_t *stream);   // 0x0048a3f0
void __fastcall UvBuildColorTable(UvPlayer8 *self);   // 0x0048a550
void __fastcall UvBlitFrame(UvPlayer8 *self, uint8_t *dst, uint32_t);   // 0x0048a610
