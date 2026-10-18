// Prototypes of the ported procedures in wnd_proc.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

LRESULT __stdcall WndProc_Main(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);   // 0x004818a0
