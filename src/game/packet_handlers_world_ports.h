// Prototypes of the ported procedures in packet_handlers_world.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __stdcall Packet_Handle0x1B(uint8_t *packet);   // 0x00457b90
void __stdcall Packet_Handle0x78(uint8_t *packet);   // 0x00458930
void __stdcall Packet_Handle0x24(uint8_t *packet);   // 0x00459020
void __stdcall Packet_Handle0x90(uint8_t *packet);   // 0x00459200
void __stdcall Packet_Handle0x23(uint8_t *packet);   // 0x004594e0
void __stdcall Packet_Handle0x27(uint8_t *packet);   // 0x00459ca0
