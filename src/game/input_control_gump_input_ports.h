// Prototypes of the ported procedures in input_control_gump_input.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __fastcall InputControl_OnTargetSelected(void *obj);   // 0x0041ed20
void __fastcall InputControl_DrawSizedArt(int artId, uint16_t *bmp);   // 0x0041fdf0
