// Prototypes of the ported procedures in char_flow_gumps.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __cdecl CF_ResetToMainMenu(void);   // 0x00410dc0
void __cdecl CF_ShowAcctLogin(void);   // 0x00410ef0
void __cdecl CF_LaunchWelcomeSplash(void);   // 0x004110e0
