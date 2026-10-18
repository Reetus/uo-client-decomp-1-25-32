// Prototypes of the ported procedures in main_window.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __fastcall SetMainWindowTitle(const char *suffix);   // 0x00480d60
int __cdecl IntroMovieFrame(void);   // 0x00482550
