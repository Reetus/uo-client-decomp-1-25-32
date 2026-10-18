// Prototypes of the ported procedures in map_world_cells.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __fastcall Map_RebaseCells(int dx, int dy);   // 0x00445db0
