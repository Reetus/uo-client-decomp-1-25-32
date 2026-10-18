// Prototypes of the ported procedures in mobile_draw_ops.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

class CMobile;
class CObj;

void __fastcall Mobile_GetAnimFrame(uint32_t bodyReg, uint32_t facingReg, int tick, int specific, int unusedArg, int act, int run, int *outIdx, int *outFrame, CMobile *mob);   // 0x0042caf0
void __fastcall Mobile_GetStandAnimIndex(int body, uint32_t dir, int *outIdx, int *outCount);   // 0x0042cff0
int __fastcall Obj_IsTileType0b10(CObj *obj);   // 0x0042db90
int __fastcall Mobile_NameHue(CMobile *self);   // 0x0042dbc0
