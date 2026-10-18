// Prototypes of the ported procedures in uv_audio.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

struct UvpState;

DWORD __cdecl UvpAudioThread(UvpState *s);   // 0x00484970
