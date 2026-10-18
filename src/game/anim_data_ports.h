// Prototypes of the ported procedures in anim_data.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __cdecl ObjectManager_UpdateAnimations(void);   // 0x00401110
