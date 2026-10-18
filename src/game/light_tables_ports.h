// Prototypes of the ported procedures in light_tables.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

class CObj;

void * __fastcall BuildLightCircleTables(void *self);   // 0x0044ef30
void * __fastcall BuildColourTables(void *self);   // 0x0044f050
void __fastcall InvalidateLightRect(CObj *self);   // 0x0044f140
void __fastcall DrawLightFootprint(unsigned id, int x, int y, int *clip);   // 0x0044f3c0
void __fastcall DrawLightFootprintRemap(unsigned id, int x, int y, int *clip);   // 0x0044f510
