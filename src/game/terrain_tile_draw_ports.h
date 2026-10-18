// Prototypes of the ported procedures in terrain_tile_draw.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

class CTerrainTile;
struct TileCoords;
struct Pos3;

// A position by value: three 16-bit coordinates, passed in 8 bytes of stack.

void __fastcall TerrainTile_SetTileAndPos(CTerrainTile *self, int, int tileId, TileCoords *coords);   // 0x0042ee90
void __fastcall Pos_SetXYZ(int16_t *self, int, Pos3 pos);   // 0x0042eec0
