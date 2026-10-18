// Prototypes of the ported procedures in object_manager_ops.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

class CDynamicObj;

void __cdecl Combat_FaceTarget(void);   // 0x00456080
uint32_t __fastcall Graphic_IsInRanges_0x00456840(uint32_t graphic);   // 0x00456840
void __stdcall Object_Use(CDynamicObj *obj);   // 0x00456890
void __stdcall Object_ShowName(CDynamicObj *obj);   // 0x00456a10
uint32_t __stdcall ObjectManager_Always1(uint32_t tileX, uint32_t tileY);   // 0x00457000
uint32_t __stdcall Graphic_Is_0x28e_Or_0xeed(uint32_t graphic);   // 0x00457050
void __fastcall Profanity_Apply(char *text);   // 0x00457460
