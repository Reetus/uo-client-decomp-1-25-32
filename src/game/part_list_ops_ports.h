// Prototypes of the ported procedures in part_list_ops.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

class CDynamicObj;

void __fastcall SetObjectPartList(void *unusedEcx, void *unusedEdx, CDynamicObj *obj, int16_t *pos, int id);   // 0x0044ecd0
void * __stdcall ByteSwapRecords12(const uint8_t *src, int size);   // 0x0044ee40
