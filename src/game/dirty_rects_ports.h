// Prototypes of the ported procedures in dirty_rects.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __fastcall DirtyRects_Invalidate(int *r);   // 0x0044f8b0
void __fastcall ScrollBackBuffer(int dx, int dy);   // 0x0044fa50
void __cdecl FlushDirtyRects(void);   // 0x0044fc60
void __cdecl InvalidateAll(void);   // 0x0044fdf0
int __fastcall RectIsTall(const int8_t *r);   // 0x0044fe30
