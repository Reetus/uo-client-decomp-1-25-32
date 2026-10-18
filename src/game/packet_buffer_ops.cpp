// Packet buffer helpers of the original image (0x004574e0 .. 0x00458060, 0x00458ca0):
//   outbound: Begin / WriteU8 / WriteU16 / WriteU32 / SetCursor
//   inbound:  ReadU8 / ReadU16 / ReadU32 / ReadPtr / HeaderSize
//
// Outbound: ECX = packet buffer, EDX = value, plain RET. The write cursor is the
// word at buf+1 for a variable-length opcode and the global at 0x00747978
// otherwise (see packet_cursor.h). Inbound: ECX = packet, EDX = pointer to a
// dword read cursor (relative to the field base 1 or 3), destination pointer(s)
// on the stack, callee pops them. Multi-byte fields go through the Winsock
// byte-order thunks. The original returns a leftover register value from the
// readers; no caller looks at it, so these return void.
#include <cstdint>

#include "../runtime/ported.h"
#include "packet_cursor.h"

// ---------------------------------------------------------------------------
// 0x004574e0  size 0x32
// Start a packet: opcode byte, then the write cursor at 3 (variable length) or 1 (fixed).
// ---------------------------------------------------------------------------
void __fastcall PacketBuf_Begin(uint8_t *buf, uint8_t opcode)
{
    buf[0] = opcode;
    if (pkt::IsVariable(buf))
        pkt::Put16(buf, 1, 3);
    else
        g_pktFixedCursor = 1;
}
HOOK_FUNC(0x004574e0, PacketBuf_Begin);

// ---------------------------------------------------------------------------
// 0x00457520  size 0x4e
// Append one byte at the write cursor and advance it.
// ---------------------------------------------------------------------------
void __fastcall PacketBuf_WriteU8(uint8_t *buf, uint8_t value)
{
    uint16_t off;
    if (pkt::IsVariable(buf))
        off = *reinterpret_cast<uint16_t *>(buf + 1);
    else
        off = g_pktFixedCursor;
    uint32_t at = off;
    buf[at] = value;
    uint32_t next = at + 1;
    if (pkt::IsVariable(buf))
        *reinterpret_cast<uint16_t *>(buf + 1) = static_cast<uint16_t>(next);
    else
        g_pktFixedCursor = static_cast<uint16_t>(next);
}
HOOK_FUNC(0x00457520, PacketBuf_WriteU8);

// ---------------------------------------------------------------------------
// 0x00457570  size 0x73
// Append a 16-bit value in network order (htons, 0x00498658) and advance the cursor.
// ---------------------------------------------------------------------------
void __fastcall PacketBuf_WriteU16(uint8_t *buf, uint16_t value)
{
    value = htons(value);
    uint16_t off;
    if (pkt::Flags(buf[0]) & 0x8000)
        off = *reinterpret_cast<uint16_t *>(buf + 1);
    else
        off = g_pktFixedCursor;
    uint32_t at = off;
    pkt::Put16(buf, at, value);
    uint32_t next = at + 2;
    if (pkt::Flags(buf[0]) & 0x8000)
        *reinterpret_cast<uint16_t *>(buf + 1) = static_cast<uint16_t>(next);
    else
        g_pktFixedCursor = static_cast<uint16_t>(next);
}
HOOK_FUNC(0x00457570, PacketBuf_WriteU16);

// ---------------------------------------------------------------------------
// 0x004575f0  size 0x62
// Append a 32-bit value in network order (htonl, 0x0049865e) and advance the cursor.
// ---------------------------------------------------------------------------
void __fastcall PacketBuf_WriteU32(uint8_t *buf, uint32_t value)
{
    uint32_t net = htonl(value);
    uint16_t off;
    if (pkt::Flags(buf[0]) & 0x8000)
        off = *reinterpret_cast<uint16_t *>(buf + 1);
    else
        off = g_pktFixedCursor;
    uint32_t at = off;
    pkt::Put32(buf, at, net);
    uint32_t next = at + 4;
    if (pkt::Flags(buf[0]) & 0x8000)
        *reinterpret_cast<uint16_t *>(buf + 1) = static_cast<uint16_t>(next);
    else
        g_pktFixedCursor = static_cast<uint16_t>(next);
}
HOOK_FUNC(0x004575f0, PacketBuf_WriteU32);

// ---------------------------------------------------------------------------
// 0x00457660  size 0x36   (RET 4)
// Read one byte at base + *cursor, advance the cursor by 1.
// ---------------------------------------------------------------------------
void __fastcall PacketBuf_ReadU8(const uint8_t *packet, uint32_t *cursor, uint8_t *out)
{
    *out = packet[pkt::Base(packet) + *cursor];
    *cursor += 1;
}
HOOK_FUNC(0x00457660, PacketBuf_ReadU8);

// ---------------------------------------------------------------------------
// 0x004576a0  size 0x47   (RET 4)
// Read a word at base + *cursor, convert with ntohs (0x0049862e), advance the cursor by 2.
// ---------------------------------------------------------------------------
void __fastcall PacketBuf_ReadU16(const uint8_t *packet, uint32_t *cursor, uint16_t *out)
{
    uint16_t raw;
    std::memcpy(&raw, packet + pkt::Base(packet) + *cursor, sizeof(raw));
    *out = raw;                      // the original stores the raw word first
    *out = ntohs(raw);
    *cursor += 2;
}
HOOK_FUNC(0x004576a0, PacketBuf_ReadU16);

// ---------------------------------------------------------------------------
// 0x004576f0  size 0x44   (RET 4)
// Read a dword at base + *cursor, convert with ntohl (0x00498616), advance the cursor by 4.
// ---------------------------------------------------------------------------
void __fastcall PacketBuf_ReadU32(const uint8_t *packet, uint32_t *cursor, uint32_t *out)
{
    uint32_t raw;
    std::memcpy(&raw, packet + pkt::Base(packet) + *cursor, sizeof(raw));
    *out = raw;                      // the original stores the raw dword first
    *out = ntohl(raw);
    *cursor += 4;
}
HOOK_FUNC(0x004576f0, PacketBuf_ReadU32);

// ---------------------------------------------------------------------------
// 0x00457740  size 0x35   (RET 8)
// Hand out a pointer to the bytes at base + *cursor and skip `length` of them.
// ---------------------------------------------------------------------------
void __fastcall PacketBuf_ReadPtr(const uint8_t *packet, uint32_t *cursor, const uint8_t **out, int length)
{
    *out = packet + pkt::Base(packet) + *cursor;
    *cursor += static_cast<uint32_t>(length);
}
HOOK_FUNC(0x00457740, PacketBuf_ReadPtr);

// ---------------------------------------------------------------------------
// 0x00458060  size 0x1d
// Set the write cursor (DX) of the packet in ECX.
// ---------------------------------------------------------------------------
void __fastcall PacketBuf_SetCursor(uint8_t *buf, uint32_t offset)
{
    if (pkt::IsVariable(buf))
        pkt::Put16(buf, 1, static_cast<uint16_t>(offset));
    else
        g_pktFixedCursor = static_cast<uint16_t>(offset);
}
HOOK_FUNC(0x00458060, PacketBuf_SetCursor);

// ---------------------------------------------------------------------------
// 0x00458ca0  size 0x16
// Offset of the first field of the inbound packet in ECX: 3 for a variable-length
// opcode, 1 otherwise. Result in EAX.
// ---------------------------------------------------------------------------
uint32_t __fastcall PacketBuf_HeaderSize(const uint8_t *packet)
{
    uint16_t size = (static_cast<int16_t>(pkt::Info(packet[0]).flags) & 0x8000) ? 3 : 1;
    return size;
}
HOOK_FUNC(0x00458ca0, PacketBuf_HeaderSize);
