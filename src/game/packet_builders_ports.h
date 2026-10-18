// Prototypes of the ported procedures in packet_builders.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __stdcall Packet_Build0x69(uint8_t *buf, uint8_t mode);   // 0x004645d0
void __stdcall Packet_Build0x71(uint8_t *buf, uint8_t type, void *obj);   // 0x00464d40
void __stdcall Packet_Build0xA4(uint8_t *buf);   // 0x00465b60
