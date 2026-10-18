// Prototypes of the ported procedures in unicode_text.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __stdcall HandleUnicodeSpeechPacket(void *packet);   // 0x0047c1f0
void __fastcall MeasureWrappedUnicode(uint16_t *str, int maxWidth, int *outWidth, int *outHeight, int *outZero);   // 0x0047cd70
void __fastcall DrawWrappedUnicode(int x, int y, int colorIndex, uint16_t *str, int maxWidth, uint16_t *dest, int pitch, int *clip);   // 0x0047d0b0
