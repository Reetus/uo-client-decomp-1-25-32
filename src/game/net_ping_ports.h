// Prototypes of the ported procedures in net_ping.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __cdecl Net_PingServerList(void);   // 0x004667b0
