// Prototypes of the ported procedures in config_file.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __stdcall LoadConfigFile(int which);   // 0x004249b0
void __fastcall WriteConfigSetting(uint32_t id, const char *text);   // 0x00424ac0
void __fastcall ResolveHostPort(const char *spec, int *a, int *b, int *c, int *d, int *port);   // 0x00424e80
