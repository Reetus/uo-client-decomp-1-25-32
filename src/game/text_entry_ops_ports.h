// Prototypes of the ported procedures in text_entry_ops.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __cdecl FocusMainWindow(void);   // 0x004429e0
const char * __fastcall CopyFirstWordLower(const char *text, char *dest);   // 0x00442c60
