// Prototypes of the ported procedures in paperdoll_gump_ops.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

int __fastcall Paperdoll_LayerIsCovered(int layer, uint8_t *equip);   // 0x00466b60
