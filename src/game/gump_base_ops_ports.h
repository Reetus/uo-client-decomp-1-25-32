// Prototypes of the ported procedures in gump_base_ops.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __stdcall GumpRectUnion(int *dst, const int *src);   // 0x0043a990
void __stdcall GetContainerGumpRect(int gumpId, int *rc);   // 0x0043adf0
int __stdcall GumpRectIntersects(const int *a, const int *b);   // 0x0043b540
void __fastcall GumpRectClip(const int *in, const int *clip, int *out);   // 0x0043b590
void __fastcall GumpBlitRows(int rows, int width, uint16_t *dst, uint16_t *src, int dstStride, int srcStride);   // 0x0043b760
