// Prototypes of the ported procedures in inbound_data_updates.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __stdcall Packet_Handle0x44(uint8_t *packet);   // 0x0045b560
void __stdcall Packet_HandleRegionUpdate_0045b660(const uint8_t *packet);   // 0x0045b660
void __stdcall Packet_Handle0x43(const uint8_t *packet);   // 0x0045b980
void __stdcall Packet_Handle0x3F(uint8_t *packet);   // 0x0045ba30
void __stdcall Packet_Handle0x92(const uint8_t *packet);   // 0x0045bb00
void __stdcall Packet_Handle0x94(const uint8_t *packet);   // 0x0045bc20
void __stdcall Packet_Handle0x40(uint8_t *packet);   // 0x0045bd20
void __stdcall Packet_Handle0x41(uint8_t *packet);   // 0x0045bdb0
void __stdcall Packet_Handle0x42(const uint8_t *packet);   // 0x0045bee0
void __stdcall Packet_Handle0x3E(const uint8_t *packet);   // 0x0045bfc0
void __stdcall Packet_Handle0x45(const uint8_t *packet);   // 0x0045c180
