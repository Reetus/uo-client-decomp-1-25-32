// Prototypes of the ported procedures in uv_player.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

struct UvpPlayer;

void __cdecl UvpPlayerThread(UvpPlayer *player);   // 0x00483c60
