// Prototypes of the ported procedures in sound_ops.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

int __stdcall PlaySound3D(int soundId, const int16_t *pos, float loudness, int loopFlag, int freqJitter, int allowDuplicate);   // 0x00480010
void __fastcall PlaySoundEffectIfEnabled(int soundId, int pan, long vol);   // 0x00480ae0
void __cdecl LockSoundBuffers(void);   // 0x00480bc0
void __cdecl UnlockSoundBuffers(void);   // 0x00480c10
LRESULT __cdecl PostMenuCommand9c99(void);   // 0x00480e20
