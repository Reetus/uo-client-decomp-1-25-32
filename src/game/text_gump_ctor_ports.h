// Prototypes of the ported procedures in text_gump_ctor.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

class CBarkGump;

CBarkGump * __stdcall CreateBarkGump(int hue, int font, char *text);   // 0x00479480
