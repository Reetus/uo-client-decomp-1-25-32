// Prototypes of the ported procedures in packet_buffer_ops.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __fastcall PacketBuf_Begin(uint8_t *buf, uint8_t opcode);   // 0x004574e0
void __fastcall PacketBuf_WriteU8(uint8_t *buf, uint8_t value);   // 0x00457520
void __fastcall PacketBuf_WriteU16(uint8_t *buf, uint16_t value);   // 0x00457570
void __fastcall PacketBuf_WriteU32(uint8_t *buf, uint32_t value);   // 0x004575f0
void __fastcall PacketBuf_ReadU8(const uint8_t *packet, uint32_t *cursor, uint8_t *out);   // 0x00457660
void __fastcall PacketBuf_ReadU16(const uint8_t *packet, uint32_t *cursor, uint16_t *out);   // 0x004576a0
void __fastcall PacketBuf_ReadU32(const uint8_t *packet, uint32_t *cursor, uint32_t *out);   // 0x004576f0
void __fastcall PacketBuf_ReadPtr(const uint8_t *packet, uint32_t *cursor, const uint8_t **out, int length);   // 0x00457740
void __fastcall PacketBuf_SetCursor(uint8_t *buf, uint32_t offset);   // 0x00458060
uint32_t __fastcall PacketBuf_HeaderSize(const uint8_t *packet);   // 0x00458ca0
