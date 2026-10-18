// Outbound packet builders (second pass, batch 24).
//   0x9B help request     0x00462840    0x22 resync ack         0x00462940
//   0x01 disconnect       0x00462a10    0x02 move request       0x00462a90
//   0x08 drop item        0x00462b60    0x06 double click       0x00462d30
//   0x03 speech           0x00462db0    0x05 attack request     0x00463010
//   0x07 pick up item     0x00463090    0x12 text command       0x00463170
//   0x09 single click     0x00463260    0x13 equip item         0x004632e0
//   0x00 create character 0x00463400    0x5D login character    0x004636c0
//   0x34 get player status 0x00463910   0x2C (one byte)         0x00463a30
//   0x3B buy items        0x00463ab0    0x9F sell items         0x00463c90
//   0x56 map pin          0x00463e40    0x93 book header        0x00464010
//
// All are __stdcall: the packet buffer is the first stack argument and the callee pops
// everything. Callers reach them with ECX = the packet builder object (0x0069e0e0), which
// none of them reads. The buffer is filled with the cursor scheme of packet_cursor.h; the
// original expands the cursor accessor at every field, so put8 / put16 / put32 below stand
// for those inline sequences. 0x00463400 calls the out-of-line cursor and append helpers
// (0x00458060, 0x0045d610, 0x00457520, 0x00457570, 0x004575f0) exactly as the original does.
#include <cstdint>
#include <cstring>

#include "../runtime/ported.h"
#include "crt.h"
#include "packet_cursor.h"
#include "raw_field.h"
#include "globals.h"

#define g24b_netConnection GLOBAL_AS(uint8_t *, g_pktSender)   // connection object; the dword at +0x20028 goes out in 0x00 and 0x5D

// Opcode byte, then the initial cursor: 3 for a variable-length opcode, 1 for a fixed one.
static void begin_packet(uint8_t *buf, uint8_t opcode)
{
    buf[0] = opcode;
    if (pkt::IsVariable(buf))
        *reinterpret_cast<uint16_t *>(buf + 1) = 3;
    else
        g_pktFixedCursor = 1;
}

static void put8(uint8_t *buf, uint32_t v)
{
    uint32_t off = pkt::Cursor(buf);
    buf[off] = static_cast<uint8_t>(v);
    pkt::SetCursor(buf, off + 1);
}
static void put16(uint8_t *buf, uint32_t v)
{
    uint32_t off = pkt::Cursor(buf);
    pkt::Put16(buf, off, static_cast<uint16_t>(v));
    pkt::SetCursor(buf, off + 2);
}
static void put32(uint8_t *buf, uint32_t v)
{
    uint32_t off = pkt::Cursor(buf);
    pkt::Put32(buf, off, v);
    pkt::SetCursor(buf, off + 4);
}
static void putBytes(uint8_t *buf, const void *src, uint32_t n)
{
    uint32_t off = pkt::Cursor(buf);
    memcpy(buf + off, src, n);
    pkt::SetCursor(buf, off + n);
}

// ---------------------------------------------------------------------------
// 0x00462840  size 0xf7  RET 0xc
// Packet 0x9B: help request. {u8 a, char text[0x100]}; the text is strncpy'd into a
// local 0x100-byte buffer first (0x0048b750), then copied whole.
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x9B(uint8_t *buf, uint8_t a, const char *text)
{
    char local[0x100];

    begin_packet(buf, 0x9b);
    put8(buf, a);
    strncpy(local, text, 0x100);   // 0x0048b750
    putBytes(buf, local, 0x100);
}
HOOK_FUNC(0x00462840, Build_Packet_0x9B);

// ---------------------------------------------------------------------------
// 0x00462940  size 0xc2  RET 4
// Packet 0x22: two zero bytes.
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x22(uint8_t *buf)
{
    begin_packet(buf, 0x22);
    put8(buf, 0);
    put8(buf, 0);
}
HOOK_FUNC(0x00462940, Build_Packet_0x22);

// ---------------------------------------------------------------------------
// 0x00462a10  size 0x7c  RET 4
// Packet 0x01: htonl(-1).
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x01(uint8_t *buf)
{
    begin_packet(buf, 0x01);
    uint32_t v = htonl(0xffffffffu);
    put32(buf, v);
}
HOOK_FUNC(0x00462a10, Build_Packet_0x01);

// ---------------------------------------------------------------------------
// 0x00462a90  size 0xc6  RET 0xc
// Packet 0x02: two bytes (direction, sequence).
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x02(uint8_t *buf, uint8_t a, uint8_t b)
{
    begin_packet(buf, 0x02);
    put8(buf, a);
    put8(buf, b);
}
HOOK_FUNC(0x00462a90, Build_Packet_0x02);

// ---------------------------------------------------------------------------
// 0x00462b60  size 0x1c1  RET 0x18
// Packet 0x08: drop item. {u32 serial, u16 x, u16 y, u8 z, u32 container}.
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x08(uint8_t *buf, uint32_t serial, uint32_t x, uint32_t y, uint8_t z, uint32_t container)
{
    begin_packet(buf, 0x08);
    uint32_t v = htonl(serial);
    put32(buf, v);
    v = htons(x);
    put16(buf, v);
    v = htons(y);
    put16(buf, v);
    put8(buf, z);
    v = htonl(container);
    put32(buf, v);
}
HOOK_FUNC(0x00462b60, Build_Packet_0x08);

// ---------------------------------------------------------------------------
// 0x00462d30  size 0x7e  RET 8
// Packet 0x06: double click. {u32 serial}.
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x06(uint8_t *buf, uint32_t serial)
{
    begin_packet(buf, 0x06);
    uint32_t v = htonl(serial);
    put32(buf, v);
}
HOOK_FUNC(0x00462d30, Build_Packet_0x06);

// ---------------------------------------------------------------------------
// 0x00462db0  size 0x259  RET 0x10
// Packet 0x03: speech. {u8 type, u16 hue, u16 font, text[len + 1]}. Type 2 takes the
// hue / font from the words at 0x0069e4d4 / 0x0069e4d6, every other type from 0x0069e4d0 /
// 0x0069e4d2; both are sent through htons.
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x03(uint8_t *buf, const char *text, uint32_t len, uint8_t type)
{
    begin_packet(buf, 0x03);
    put8(buf, type);
    if (type == 2) {
        uint32_t v = htons(GLOBAL_AS(uint32_t, g_hue1));
        put16(buf, v);
        v = htons(g_colorEmoteB);
        put16(buf, v);
    } else {
        uint32_t v = htons(GLOBAL_AS(uint32_t, g_colorSay));
        put16(buf, v);
        v = htons(g_colorSayFont);
        put16(buf, v);
    }
    putBytes(buf, text, len + 1);
}
HOOK_FUNC(0x00462db0, Build_Packet_0x03);

// ---------------------------------------------------------------------------
// 0x00463010  size 0x7e  RET 8
// Packet 0x05: attack request. {u32 serial}.
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x05(uint8_t *buf, uint32_t serial)
{
    begin_packet(buf, 0x05);
    uint32_t v = htonl(serial);
    put32(buf, v);
}
HOOK_FUNC(0x00463010, Build_Packet_0x05);

// ---------------------------------------------------------------------------
// 0x00463090  size 0xd7  RET 0xc
// Packet 0x07: pick up item. {u32 serial, u16 quantity}.
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x07(uint8_t *buf, uint32_t serial, uint32_t quantity)
{
    begin_packet(buf, 0x07);
    uint32_t v = htonl(serial);
    put32(buf, v);
    v = htons(quantity);
    put16(buf, v);
}
HOOK_FUNC(0x00463090, Build_Packet_0x07);

// ---------------------------------------------------------------------------
// 0x00463170  size 0xeb  RET 0x10
// Packet 0x12: text command. {u8 kind, text[len + 1]}.
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x12(uint8_t *buf, uint8_t kind, const char *text, uint32_t len)
{
    begin_packet(buf, 0x12);
    put8(buf, kind);
    putBytes(buf, text, len + 1);
}
HOOK_FUNC(0x00463170, Build_Packet_0x12);

// ---------------------------------------------------------------------------
// 0x00463260  size 0x7e  RET 8
// Packet 0x09: single click. {u32 serial}.
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x09(uint8_t *buf, uint32_t serial)
{
    begin_packet(buf, 0x09);
    uint32_t v = htonl(serial);
    put32(buf, v);
}
HOOK_FUNC(0x00463260, Build_Packet_0x09);

// ---------------------------------------------------------------------------
// 0x004632e0  size 0x11d  RET 0x10
// Packet 0x13: equip item. {u32 serial, u8 layer, u32 mobile}.
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x13(uint8_t *buf, uint32_t serial, uint8_t layer, uint32_t mobile)
{
    begin_packet(buf, 0x13);
    uint32_t v = htonl(serial);
    put32(buf, v);
    put8(buf, layer);
    v = htonl(mobile);
    put32(buf, v);
}
HOOK_FUNC(0x004632e0, Build_Packet_0x13);

// ---------------------------------------------------------------------------
// 0x00463400  size 0x2ba  RET 0x54 (21 stack args)
// Packet 0x00: create character. Written through the out-of-line cursor helpers:
// htonl(0xedededed), htons(0xffff) x2, u8 0, name[0x1e], password[0x1e], four bytes
// (b14..b20), six bytes (b38..b4c), two htons words (w24, w28), three u16 (u2c..u34), two
// u8, a u32, and the dword at connection + 0x20028. The stream order is not the argument
// order: scripts/exacttest.py found the port writing the words before the six bytes.
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x00(uint8_t *buf, const void *name, const void *password, uint8_t b14, uint8_t b18,
                                 uint8_t b1c, uint8_t b20, uint32_t w24, uint32_t w28, uint32_t u2c, uint32_t u30,
                                 uint32_t u34, uint8_t b38, uint8_t b3c, uint8_t b40, uint8_t b44, uint8_t b48,
                                 uint8_t b4c, uint8_t u50, uint8_t u54, uint32_t d58)
{
    uint32_t off;
    uint32_t v;
    uint16_t w;

    buf[0] = 0;
    if (pkt::Flags(buf[0]) & 0x8000)
        pkt::OrigSetCursor(buf, 3);
    else
        g_pktFixedCursor = 1;

    v = htonl(0xededededu);
    off = pkt::OrigGetCursor(buf) & 0xffff;
    pkt::Put32(buf, off, v);
    pkt::OrigSetCursor(buf, off + 4);

    w = htons(0xffff);
    off = pkt::OrigGetCursor(buf) & 0xffff;
    pkt::Put16(buf, off, w);
    pkt::OrigSetCursor(buf, off + 2);

    w = htons(0xffff);
    off = pkt::OrigGetCursor(buf) & 0xffff;
    pkt::Put16(buf, off, w);
    pkt::OrigSetCursor(buf, off + 2);

    off = pkt::OrigGetCursor(buf) & 0xffff;
    buf[off] = 0;
    pkt::OrigSetCursor(buf, off + 1);

    off = pkt::OrigGetCursor(buf) & 0xffff;
    memcpy(buf + off, name, 0x1e);
    pkt::OrigSetCursor(buf, off + 0x1e);

    off = pkt::OrigGetCursor(buf) & 0xffff;
    memcpy(buf + off, password, 0x1e);
    pkt::OrigSetCursor(buf, off + 0x1e);

    off = pkt::OrigGetCursor(buf) & 0xffff;
    buf[off] = b14;
    pkt::OrigSetCursor(buf, off + 1);
    off = pkt::OrigGetCursor(buf) & 0xffff;
    buf[off] = b18;
    pkt::OrigSetCursor(buf, off + 1);
    off = pkt::OrigGetCursor(buf) & 0xffff;
    buf[off] = b1c;
    pkt::OrigSetCursor(buf, off + 1);
    off = pkt::OrigGetCursor(buf) & 0xffff;
    buf[off] = b20;
    pkt::OrigSetCursor(buf, off + 1);

    off = pkt::OrigGetCursor(buf) & 0xffff;
    buf[off] = b38;
    pkt::OrigSetCursor(buf, off + 1);
    off = pkt::OrigGetCursor(buf) & 0xffff;
    buf[off] = b3c;
    pkt::OrigSetCursor(buf, off + 1);
    off = pkt::OrigGetCursor(buf) & 0xffff;
    buf[off] = b40;
    pkt::OrigSetCursor(buf, off + 1);
    off = pkt::OrigGetCursor(buf) & 0xffff;
    buf[off] = b44;
    pkt::OrigSetCursor(buf, off + 1);
    off = pkt::OrigGetCursor(buf) & 0xffff;
    buf[off] = b48;
    pkt::OrigSetCursor(buf, off + 1);
    off = pkt::OrigGetCursor(buf) & 0xffff;
    buf[off] = b4c;
    pkt::OrigSetCursor(buf, off + 1);

    // The five words follow the six bytes b38..b4c in the stream, although they come first on the stack.
    w = htons(w24);
    off = pkt::OrigGetCursor(buf) & 0xffff;
    pkt::Put16(buf, off, w);
    pkt::OrigSetCursor(buf, off + 2);

    w = htons(w28);
    off = pkt::OrigGetCursor(buf) & 0xffff;
    pkt::Put16(buf, off, w);
    pkt::OrigSetCursor(buf, off + 2);

    pkt::OrigAppendU16(buf, u2c);
    pkt::OrigAppendU16(buf, u30);
    pkt::OrigAppendU16(buf, u34);

    pkt::OrigAppendU8(buf, u50);
    pkt::OrigAppendU8(buf, u54);
    pkt::OrigAppendU32(buf, d58);
    pkt::OrigAppendU32(buf, *reinterpret_cast<uint32_t *>(g24b_netConnection + 0x20028));
}
HOOK_FUNC(0x00463400, Build_Packet_0x00);

// ---------------------------------------------------------------------------
// 0x004636c0  size 0x24f  RET 0x10
// Packet 0x5D: login character. {u32 0xedededed, char name[0x1e], char password[0x1e],
// u32 slot, u32 connection+0x20028}. Each string is strcpy'd into a zeroed 0x1e-byte local
// first (an over-long string overruns it, as in the original).
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x5D(uint8_t *buf, const char *name, const char *password, uint32_t slot)
{
    char local[0x1e];

    begin_packet(buf, 0x5d);
    uint32_t v = htonl(0xededededu);
    put32(buf, v);

    memset(local, 0, sizeof(local));
    strcpy(local, name);
    putBytes(buf, local, 0x1e);

    memset(local, 0, sizeof(local));
    strcpy(local, password);
    putBytes(buf, local, 0x1e);

    v = htonl(slot);
    put32(buf, v);
    v = htonl(*reinterpret_cast<uint32_t *>(g24b_netConnection + 0x20028));
    put32(buf, v);
}
HOOK_FUNC(0x004636c0, Build_Packet_0x5D);

// ---------------------------------------------------------------------------
// 0x00463910  size 0x11e  RET 0xc
// Packet 0x34: get player status. {u32 0xedededed, u8 kind, u32 serial}.
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x34(uint8_t *buf, uint8_t kind, uint32_t serial)
{
    begin_packet(buf, 0x34);
    uint32_t v = htonl(0xededededu);
    put32(buf, v);
    put8(buf, kind);
    v = htonl(serial);
    put32(buf, v);
}
HOOK_FUNC(0x00463910, Build_Packet_0x34);

// ---------------------------------------------------------------------------
// 0x00463a30  size 0x75  RET 8
// Packet 0x2C: one byte.
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x2C(uint8_t *buf, uint8_t a)
{
    begin_packet(buf, 0x2c);
    put8(buf, a);
}
HOOK_FUNC(0x00463a30, Build_Packet_0x2C);

// ---------------------------------------------------------------------------
// 0x00463ab0  size 0x1d6  RET 0x10
// Packet 0x3B: buy items. {u32 vendor, u8 flag, then per list node {u8 +0, u32 +4, u16 +0xa}}.
// The list is linked through the dword at +0xc.
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x3B(uint8_t *buf, uint32_t vendor, uint8_t flag, uint8_t *list)
{
    begin_packet(buf, 0x3b);
    uint32_t v = htonl(vendor);
    put32(buf, v);
    put8(buf, flag);
    for (uint8_t *node = list; node != nullptr; node = RAWF(uint8_t *, node, 0xc)) {
        put8(buf, node[0]);
        v = htonl(RAWF(uint32_t, node, 4));
        put32(buf, v);
        v = htons(RAWF(uint16_t, node, 0xa));
        put16(buf, v);
    }
}
HOOK_FUNC(0x00463ab0, Build_Packet_0x3B);

// ---------------------------------------------------------------------------
// 0x00463c90  size 0x1a2  RET 0xc
// Packet 0x9F: sell items. {u32 vendor, u16 count, then per list node {u32 +4, u16 +0xa}}.
// The list is linked through the dword at +0xc.
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x9F(uint8_t *buf, uint32_t vendor, uint8_t *list)
{
    begin_packet(buf, 0x9f);
    uint32_t v = htonl(vendor);
    put32(buf, v);

    uint32_t count = 0;
    for (uint8_t *node = list; node != nullptr; node = RAWF(uint8_t *, node, 0xc))
        count++;
    v = htons(count);
    put16(buf, v);

    for (uint8_t *node = list; node != nullptr; node = RAWF(uint8_t *, node, 0xc)) {
        v = htonl(RAWF(uint32_t, node, 4));
        put32(buf, v);
        v = htons(RAWF(uint16_t, node, 0xa));
        put16(buf, v);
    }
}
HOOK_FUNC(0x00463c90, Build_Packet_0x9F);

// ---------------------------------------------------------------------------
// 0x00463e40  size 0x1c4  RET 0x18
// Packet 0x56: map pin. {u32 serial, u8 a, u8 b, u16 x - 0x18, u16 y + 0xffe1}.
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x56(uint8_t *buf, uint32_t serial, uint8_t a, uint8_t b, uint32_t x, uint32_t y)
{
    begin_packet(buf, 0x56);
    uint32_t v = htonl(serial);
    put32(buf, v);
    put8(buf, a);
    put8(buf, b);
    uint32_t px = x - 0x18;
    uint32_t py = y + 0xffe1;
    v = htons(px);
    put16(buf, v);
    v = htons(py);
    put16(buf, v);
}
HOOK_FUNC(0x00463e40, Build_Packet_0x56);

// ---------------------------------------------------------------------------
// 0x00464010  size 0x1c8  RET 0x10
// Packet 0x93: book header. {u32 serial, u8 0, u16 0, 0x3c bytes (title), 0x1e bytes (author)}.
// ---------------------------------------------------------------------------
void __stdcall Build_Packet_0x93(uint8_t *buf, uint32_t serial, const void *title, const void *author)
{
    begin_packet(buf, 0x93);
    uint32_t v = htonl(serial);
    put32(buf, v);
    put8(buf, 0);
    v = htons(0);
    put16(buf, v);
    putBytes(buf, title, 0x3c);
    putBytes(buf, author, 0x1e);
}
HOOK_FUNC(0x00464010, Build_Packet_0x93);
