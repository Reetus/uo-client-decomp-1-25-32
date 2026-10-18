// Prototypes of the ported procedures in art_record_ops.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void * __fastcall ArtRecord_ReturnThis(void *self);   // 0x00476120
uint8_t * __stdcall ArtRecord_Get(int index);   // 0x00476130
uint16_t __stdcall ArtRecord_PixelAt(const uint8_t *rec, int x, int y);   // 0x00476160
