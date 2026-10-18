// Prototypes of the ported procedures in packet_handlers_dialogs.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __stdcall Packet_Handle0x99(uint8_t *packet);   // 0x0045d9a0
void __stdcall Packet_Handle0x6D(uint8_t *packet);   // 0x0045db90
void __stdcall Packet_Handle0x74(uint8_t *packet);   // 0x0045dbd0
void __stdcall Packet_Handle0x9E(uint8_t *packet);   // 0x0045ddc0
void __stdcall Packet_Handle0x6E(uint8_t *packet);   // 0x0045e100
void __stdcall Packet_Handle0x6F(uint8_t *packet);   // 0x0045e250
void __stdcall Packet_Handle0x70(uint8_t *packet);   // 0x0045e5c0
void __stdcall Packet_Handle0x71(uint8_t *packet);   // 0x0045e830
void __stdcall Packet_Handle0x72(uint8_t *packet);   // 0x0045ee70
void __stdcall Packet_Handle0x7B(uint8_t *packet);   // 0x0045ef30
