// Prototypes of the ported procedures in mobile_cached_draw.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

class CMobile;

int __fastcall Mobile_FindMountEntry(CMobile *self);   // 0x00471040
void __fastcall Mobile_ComputeBoundsCached(CMobile *self);   // 0x00471190
void __fastcall Mobile_DrawCached(CMobile *self, int, int *clipArg);   // 0x00471570
void __fastcall Mobile_ComputeBoundsPreview(CMobile *self);   // 0x00471e20
void __fastcall Mobile_DrawPreviewCached(CMobile *self, int, int *clipArg);   // 0x004721f0
