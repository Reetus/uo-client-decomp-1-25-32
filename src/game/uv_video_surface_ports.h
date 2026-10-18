// Prototypes of the ported procedures in uv_video_surface.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

struct UvVideo;

void __fastcall UV_BlitFrame(UvVideo *o);   // 0x00485210
void __fastcall UV_SwapFrameBuffers(UvVideo *o);   // 0x00485d70
void __fastcall UV_InitVideoSurface(UvVideo *o);   // 0x00486590
void __fastcall UV_ReleaseVideoSurface(UvVideo *o);   // 0x00486770
