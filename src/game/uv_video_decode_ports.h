// Prototypes of the ported procedures in uv_video_decode.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

struct UvChunk;
struct UvVideo;

void __fastcall UV_DecodeChunk(UvVideo *o, UvChunk *chunk);   // 0x00486850
