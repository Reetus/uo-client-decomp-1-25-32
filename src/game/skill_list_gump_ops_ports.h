// Prototypes of the ported procedures in skill_list_gump_ops.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __fastcall SkillList_DrawTiledFrame(int artTop, int middleCount, uint16_t *bmp, int y, int bottomPad);   // 0x00472be0
