// Packet buffer cursor and byte-order helpers shared by the packet builders
// and handlers.
//
// Every outbound packet is built in a local buffer: byte 0 is the opcode;
// for a variable-length opcode bytes 1..2 hold the current write offset and
// the length, for a fixed-length opcode the write offset lives in the global
// word at 0x00747978. Whether an opcode is variable length is bit 0x8000 of
// the word at 0x004add84 + opcode * 8 (8-byte table entries).
//
// Inbound handlers get the packet pointer and read fields at "base + n",
// where base is 3 for a variable-length opcode and 1 otherwise.
#pragma once

#include <cstdint>
#include <cstring>

#include <winsock.h>

#include "../runtime/ported.h"
#include "packet_buffer_ops_ports.h"
#include "inbound_status_misc_ports.h"
#include "client_socket.h"
#include "globals.h"

namespace pkt {

// Packet length table (8-byte entries, flags word first). The flags are a signed
// 16-bit field: that is what keeps the original's "and eax, 0x8000" in the header size.
struct PktInfo { int16_t flags; int16_t unk2; int32_t unk4; };
inline const PktInfo &Info(uint8_t opcode) { return reinterpret_cast<const PktInfo *>(GADDR(g_packetTable))[opcode]; }
inline uint16_t Flags(uint8_t opcode) { return *reinterpret_cast<const uint16_t *>(GADDR(g_packetTable) + static_cast<uint32_t>(opcode) * 8u); }
inline bool IsVariable(const uint8_t *buf) { return (*reinterpret_cast<const uint8_t *>(GADDR(g_packetTable) + static_cast<uint32_t>(buf[0]) * 8u + 1) & 0x80) != 0; }

// Fixed-length packets keep their write offset here.

// Base offset of the first field of an inbound packet.
inline uint16_t Base(const uint8_t *packet)
{
    uint16_t h;
    if (static_cast<int16_t>(Info(packet[0]).flags) & 0x8000)
        h = 3;
    else
        h = 1;
    return h;
}

// Word form of the variable-length test (the writers inlined in the builders test the
// 16-bit flags with a register mask; IsVariable above is the byte form).
inline int IsVariableW(const uint8_t *buf) { return static_cast<int16_t>(Info(buf[0]).flags) & 0x8000; }

// Inline cursor accessors (the original expands these at every field).
inline uint32_t Cursor(const uint8_t *buf)
{
    uint16_t off;
    if (IsVariableW(buf))
        off = *reinterpret_cast<const uint16_t *>(buf + 1);
    else
        off = g_pktFixedCursor;
    return off;
}
inline void SetCursor(uint8_t *buf, uint32_t off)
{
    if (IsVariableW(buf))
        *reinterpret_cast<uint16_t *>(buf + 1) = static_cast<uint16_t>(off);
    else
        g_pktFixedCursor = static_cast<uint16_t>(off);
}

// Unaligned stores into the buffer.
inline void Put16(uint8_t *buf, uint32_t off, uint16_t v) { std::memcpy(buf + off, &v, sizeof(v)); }
inline void Put32(uint8_t *buf, uint32_t off, uint32_t v) { std::memcpy(buf + off, &v, sizeof(v)); }

// Out-of-line cursor and append helpers of the original image (ECX = buffer).
inline void OrigSetCursor(uint8_t *buf, uint32_t off) { PacketBuf_SetCursor(buf, off); }
inline uint32_t OrigGetCursor(uint8_t *buf) { return Packet_GetWriteOffset(buf); }   // result in AX
inline void OrigAppendU8(uint8_t *buf, uint8_t v) { PacketBuf_WriteU8(buf, v); }
inline void OrigAppendU16(uint8_t *buf, uint16_t v) { PacketBuf_WriteU16(buf, v); }
inline void OrigAppendU32(uint8_t *buf, uint32_t v) { PacketBuf_WriteU32(buf, v); }
// 0x00458ca0: ECX = inbound packet, returns the base offset in AX.
inline uint32_t OrigBase(const uint8_t *packet) { return PacketBuf_HeaderSize(packet); }

// Packet sender object (global at 0x0069e0ec, the connection) and builder (0x0069e0e0).
inline void Send(const uint8_t *buf) { reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket(buf); }

} // namespace pkt
