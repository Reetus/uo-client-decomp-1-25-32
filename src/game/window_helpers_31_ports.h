// Prototypes of the ported procedures in window_helpers_31.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __cdecl PasteClipboardText(void);   // 0x004816e0
void __cdecl SwitchToWindowedMode(void);   // 0x004817a0
void __stdcall SetCursorForRegion(unsigned id);   // 0x00482be0
int __fastcall ShowMessageBoxAndExit(const char *text, const char *caption, int exitFlag);   // 0x00482c70
int __fastcall LaunchWebBrowser(const char *url);   // 0x004830a0
