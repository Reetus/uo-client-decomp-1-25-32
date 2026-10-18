// Prototypes of the ported procedures in bb_msg_gump.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __fastcall BBMsg_TileArt(int x, int y, int artId, void *bmp, int *rect);   // 0x004069f0
const char * __fastcall StripParenSuffix(const char *text);   // 0x00406a80
