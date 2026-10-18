// Prototypes of the ported procedures in client_misc_ops.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

struct CSpeechSenderRaw;

int __fastcall StrContainsTest(const char *text);   // 0x00478cb0
int __cdecl Misc_Return0(void);   // 0x00478cd0
int __cdecl Misc_ToggleIfFullscreen(void);   // 0x00478ce0
void __fastcall Misc_SetFullscreen(int wanted);   // 0x00478cf0
void __fastcall SendSpeechText(CSpeechSenderRaw *self, int /*edx*/, char *text, int /*unused*/, int type);   // 0x00478d90
