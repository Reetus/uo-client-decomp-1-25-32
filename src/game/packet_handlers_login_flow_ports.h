// Prototypes of the ported procedures in packet_handlers_login_flow.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __stdcall Packet_Handle0x7C(uint8_t *packet);   // 0x0045f080
void __stdcall Packet_Handle0x95(uint8_t *packet);   // 0x0045f410
void __stdcall Packet_HandleServerList(uint8_t *packet);   // 0x004603e0
void __stdcall Packet_HandleCharacterList(uint8_t *packet);   // 0x00460b80
