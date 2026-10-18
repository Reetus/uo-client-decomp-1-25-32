// Prototypes of the ported procedures in login_packets.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __stdcall Packet_HandleServerListWithChars(uint8_t *packet);   // 0x0045f690
void __stdcall Packet_HandleServerRelay(uint8_t *packet);   // 0x00461760
void __stdcall Packet_HandleTipOfTheDay(uint8_t *packet);   // 0x00461fd0
