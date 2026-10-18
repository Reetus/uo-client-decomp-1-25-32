// Prototypes of the ported procedures in uv_video_tables.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

struct UvVideo;

void __fastcall UV_BuildTables(UvVideo *o);   // 0x00485da0
