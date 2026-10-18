// Prototypes of the ported procedures in obj_pool_alloc.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

class CDynamicObj;
class CObj;
class CTerrainTile;
struct TileCoords;

CDynamicObj * __fastcall DynamicObj_PoolNew(uint32_t serial, uint32_t graphic, uint32_t param);   // 0x004516a0
CTerrainTile * __fastcall TerrainTile_PoolNew(int tileId, TileCoords *coords);   // 0x00451860
