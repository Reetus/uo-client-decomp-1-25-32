// Prototypes of the ported procedures in inbound_book_cfg.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __stdcall Packet_Handle0x93(const uint8_t *packet);   // 0x0045c9f0
void __stdcall Packet_Handle0x66(const uint8_t *packet);   // 0x0045cbe0
void __stdcall Packet_Handle0x69(const uint8_t *packet);   // 0x0045cec0
void __stdcall Packet_Handle0x6A(const uint8_t *packet);   // 0x0045d2c0
