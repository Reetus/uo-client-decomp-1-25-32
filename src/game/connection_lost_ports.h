// Prototypes of the ported procedures in connection_lost.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __stdcall ConnectionLostTick(int unusedArg);   // 0x00476ef0
