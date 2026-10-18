// Prototypes of the ported procedures in art_update_ops.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __stdcall ArtUpdateRecord(const void *data, int index, int mode, int arg);   // 0x00436b50
void * __stdcall TileArtOrDefault(unsigned id);   // 0x00436c40
