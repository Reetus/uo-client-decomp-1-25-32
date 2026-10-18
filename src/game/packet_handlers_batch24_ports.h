// Prototypes of the ported procedures in packet_handlers_batch24.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __stdcall Packet_Handle0x82(uint8_t *packet);   // 0x00461290
void __stdcall Packet_Handle0x88(uint8_t *packet);   // 0x00461400
void __stdcall Packet_Handle0x8B(uint8_t *packet);   // 0x004615a0
void __stdcall Packet_Handle0x85(uint8_t *packet);   // 0x00461b20
void __stdcall Packet_Handle0x86(uint8_t *packet);   // 0x00461c10
void __stdcall Packet_Handle0x98(uint8_t *packet);   // 0x00461d70
void __stdcall Packet_Handle0x9A(uint8_t *packet);   // 0x00461e00
void __stdcall Packet_Handle0xA5(uint8_t *packet);   // 0x00461f30
void __stdcall Packet_Handle0xAA(uint8_t *packet);   // 0x00462270
void __stdcall Packet_Handle0xAB(uint8_t *packet);   // 0x004622f0
void __stdcall Packet_Handle0xAF(uint8_t *packet);   // 0x004624c0
void __stdcall Packet_Handle0xB0(uint8_t *packet);   // 0x00462580
