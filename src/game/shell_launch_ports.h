// Prototypes of the ported procedures in shell_launch.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

HANDLE __fastcall OpenUrlWithBrowser(int ecxArg, char *url, char *workDir);   // 0x0047e020
