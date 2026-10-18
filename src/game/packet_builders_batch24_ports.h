// Prototypes of the ported procedures in packet_builders_batch24.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __stdcall Build_Packet_0x9B(uint8_t *buf, uint8_t a, const char *text);   // 0x00462840
void __stdcall Build_Packet_0x22(uint8_t *buf);   // 0x00462940
void __stdcall Build_Packet_0x01(uint8_t *buf);   // 0x00462a10
void __stdcall Build_Packet_0x02(uint8_t *buf, uint8_t a, uint8_t b);   // 0x00462a90
void __stdcall Build_Packet_0x08(uint8_t *buf, uint32_t serial, uint32_t x, uint32_t y, uint8_t z, uint32_t container);   // 0x00462b60
void __stdcall Build_Packet_0x06(uint8_t *buf, uint32_t serial);   // 0x00462d30
void __stdcall Build_Packet_0x03(uint8_t *buf, const char *text, uint32_t len, uint8_t type);   // 0x00462db0
void __stdcall Build_Packet_0x05(uint8_t *buf, uint32_t serial);   // 0x00463010
void __stdcall Build_Packet_0x07(uint8_t *buf, uint32_t serial, uint32_t quantity);   // 0x00463090
void __stdcall Build_Packet_0x12(uint8_t *buf, uint8_t kind, const char *text, uint32_t len);   // 0x00463170
void __stdcall Build_Packet_0x09(uint8_t *buf, uint32_t serial);   // 0x00463260
void __stdcall Build_Packet_0x13(uint8_t *buf, uint32_t serial, uint8_t layer, uint32_t mobile);   // 0x004632e0
void __stdcall Build_Packet_0x00(uint8_t *buf, const void *name, const void *password, uint8_t b14, uint8_t b18, uint8_t b1c, uint8_t b20, uint32_t w24, uint32_t w28, uint32_t u2c, uint32_t u30, uint32_t u34, uint8_t b38, uint8_t b3c, uint8_t b40, uint8_t b44, uint8_t b48, uint8_t b4c, uint8_t u50, uint8_t u54, uint32_t d58);   // 0x00463400
void __stdcall Build_Packet_0x5D(uint8_t *buf, const char *name, const char *password, uint32_t slot);   // 0x004636c0
void __stdcall Build_Packet_0x34(uint8_t *buf, uint8_t kind, uint32_t serial);   // 0x00463910
void __stdcall Build_Packet_0x2C(uint8_t *buf, uint8_t a);   // 0x00463a30
void __stdcall Build_Packet_0x3B(uint8_t *buf, uint32_t vendor, uint8_t flag, uint8_t *list);   // 0x00463ab0
void __stdcall Build_Packet_0x9F(uint8_t *buf, uint32_t vendor, uint8_t *list);   // 0x00463c90
void __stdcall Build_Packet_0x56(uint8_t *buf, uint32_t serial, uint8_t a, uint8_t b, uint32_t x, uint32_t y);   // 0x00463e40
void __stdcall Build_Packet_0x93(uint8_t *buf, uint32_t serial, const void *title, const void *author);   // 0x00464010
