// Prototypes of the ported procedures in bark_gump_ui.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __fastcall Stub_00401250(void *, void *, int, int);   // 0x00401250
void __fastcall Stub_00401260(void *, void *, int);   // 0x00401260
void __fastcall Stub_00421390(void *, void *, int);   // 0x00421390
void __fastcall CBark_BitmapOutline(uint16_t *bmp, int width, int height, uint32_t color);   // 0x00401a90
