// Prototypes of the ported procedures in inbound_status_misc.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __stdcall Packet_Handle0x30(const uint8_t *packet);   // 0x0045aa70
void __stdcall Packet_Handle0x31(const uint8_t *packet);   // 0x0045aac0
void __stdcall Packet_Handle0x11(const uint8_t *packet);   // 0x0045aae0
void __stdcall Packet_Handle0x15(const uint8_t *packet);   // 0x0045af10
void __stdcall Packet_Handle0x3A(const uint8_t *packet);   // 0x0045b010
void __stdcall Packet_Handle0x3B(const uint8_t *packet);   // 0x0045b390
void __stdcall Packet_Handle0x38(const uint8_t *packet);   // 0x0045b4c0
void __stdcall Packet_Handle0x4E(const uint8_t *packet);   // 0x0045c1d0
void __stdcall Packet_Handle0x5B(const uint8_t *packet);   // 0x0045c240
void __stdcall Packet_Handle0x53(const uint8_t *packet);   // 0x0045c2d0
void __stdcall Packet_Handle0x55(const uint8_t *packet);   // 0x0045c550
void __stdcall Packet_Handle0x56(const uint8_t *packet);   // 0x0045c5b0
void __stdcall Packet_Handle0x54(const uint8_t *packet);   // 0x0045c790
void __stdcall Packet_Handle0x65(const uint8_t *packet);   // 0x0045c970
uint32_t __fastcall Packet_GetWriteOffset(const uint8_t *buf);   // 0x0045d610
void __stdcall Packet_Handle0x6C(const uint8_t *packet);   // 0x0045d900
