// Inbound packet handlers, second pass (batch 22), part 2: the "versioned data" update packets that patch the
// client's data files in memory (art, terrain, statics, hues, animation, regions, records) and the tile data.
//
//   0x0045b560 0x44  hue table (8 blocks of 0x20 colours)     0x0045b660 (region update)
//   0x0045b980 0x43  hue record                                0x0045ba30 0x3F  statics block
//   0x0045bb00 0x92  record 0xd/0xe (12-byte records)          0x0045bc20 0x94  record 0xf/0x10
//   0x0045bd20 0x40  terrain tile data                         0x0045bdb0 0x41  tile data (terrain / items)
//   0x0045bee0 0x42  art record                                0x0045bfc0 0x3E  eight dwords
//   0x0045c180 0x45  version OK
//
// All are __stdcall (RET 4) with the packet pointer and work on it in place (the byte-swapped fields are
// written back into the packet buffer). Every one of them only acts when the "versioned data" flag
// 0x006f4768 is set. The first field is at base = 3 / 1 (packet_cursor.h); the original expands that test
// inline at every field read (or calls 0x00458ca0), the port reads at base + offset. Two of the callees are
// empty in the image (0x004242b0, and the virtual override 0x00421390 of the login gump at 0x00528698); they are
// still called, as in the original. The SEH frames of the original are omitted.
#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "file_manager.h"
#include "globals.h"
#include "gump.h"
#include "obj.h"
#include "object_manager.h"
#include "crt.h"
#include "packet_cursor.h"
#include "raw_field.h"
#include "region_manager.h"
#include "circle_trans_ports.h"
#include "login_gumps.h"
#include "bark_gump_ui_ports.h"
#include "part_list_ops_ports.h"
#include "fighting_gump.h"
#include "art_update_ops_ports.h"

#define g_versioned GLOBAL_AS(uint32_t, g_loginByAddr)

// ---------------------------------------------------------------------------
// Original code not ported yet.
// ---------------------------------------------------------------------------
static void Nullsub(int a, int b) { CircleTrans_Nullsub(a, b); }
static void LoginStub(int v) { PORT_THISCALL1(&Stub_00421390, void, GLOBAL_PTR(void, g_regionMusic_storage), int)(v); }
static void *ByteSwapRecords12(const void *data, int len) { return ByteSwapRecords12(data, len); }

static uint32_t rd32(const uint8_t *p, uint32_t off)
{
    uint32_t v;
    memcpy(&v, p + off, sizeof(v));
    return v;
}
static uint16_t rd16(const uint8_t *p, uint32_t off)
{
    uint16_t v;
    memcpy(&v, p + off, sizeof(v));
    return v;
}
static void wr32(uint8_t *p, uint32_t off, uint32_t v) { memcpy(p + off, &v, sizeof(v)); }
static void wr16(uint8_t *p, uint32_t off, uint16_t v) { memcpy(p + off, &v, sizeof(v)); }
static int ptrint(const void *p) { return static_cast<int>(reinterpret_cast<uintptr_t>(p)); }

// ---------------------------------------------------------------------------
// 0x0045b560  size 0xf8
// Inbound packet 0x44: two dwords (serial, extra) then 8 blocks of 0x20 words + 2 words (0x58 bytes each),
// every word byte-swapped in place.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x44(uint8_t *packet)
{
    uint8_t *d = packet + pkt::Base(packet);
    uint32_t serial = ntohl(rd32(d, 0));
    uint32_t extra = ntohl(rd32(d, 4));
    uint8_t *p = d + 8;
    for (int block = 8; block != 0; block--) {
        for (int i = 0x20; i != 0; i--) {
            wr16(p, 0, ntohs(rd16(p, 0)));
            p += 2;
        }
        wr16(p, 0, ntohs(rd16(p, 0)));
        uint16_t next = ntohs(rd16(p, 2));
        p += 2;
        wr16(p, 0, next);
        p += 0x16;
    }
    if (g_versioned != 0) {
        int base = g_00758210;
        Nullsub(static_cast<int>(serial) + base, static_cast<int>(extra));          // 0x004242b0
        LoginStub(static_cast<int>(serial) + g_00758210);        // 0x00421390
    }
}
HOOK_FUNC(0x0045b560, Packet_Handle0x44);

// ---------------------------------------------------------------------------
// 0x0045b660  size 0x31a
// Inbound region update (opcode not established): builds a CRegion from the packet (two 0x28-byte names, six
// rectangle shorts, four trailing fields) and installs it as region number `index` in the region table 0x006e4738.
// ---------------------------------------------------------------------------
void __stdcall Packet_HandleRegionUpdate_0045b660(const uint8_t *packet)
{
    CRegion region;
    (&region)->CRegion::Ctor();                                       // CRegion::CRegion
    const uint8_t *d = packet + pkt::Base(packet);                                  // 0x00458ca0 (called 8x in the original: 0x00458ca0 0x00458ca0 0x00458ca0 0x00458ca0 0x00458ca0 0x00458ca0 0x00458ca0)
    strncpy(region.name, reinterpret_cast<const char *>(d), 0x28);             // 0x0048b750
    uint32_t index = ntohl(rd32(d, 0x28));
    region.rect[0] = static_cast<int16_t>(ntohs(rd16(d, 0x2c)));
    region.rect[1] = static_cast<int16_t>(ntohs(rd16(d, 0x2e)));
    region.rect[2] = static_cast<int16_t>(ntohs(rd16(d, 0x30)));
    region.rect[3] = static_cast<int16_t>(ntohs(rd16(d, 0x32)));
    region.rect[4] = static_cast<int16_t>(ntohs(rd16(d, 0x34)));
    region.rect[5] = static_cast<int16_t>(ntohs(rd16(d, 0x36)));
    uint32_t extra = ntohl(rd32(d, 0x38));
    strncpy(region.name2, reinterpret_cast<const char *>(d + 0x3c), 0x28);     // 0x0048b750
    region.t1 = static_cast<int16_t>(ntohs(rd16(d, 0x64)));
    region.t2 = static_cast<int16_t>(ntohs(rd16(d, 0x66)));
    region.t3 = static_cast<int16_t>(ntohs(rd16(d, 0x68)));
    region.t4 = d[0x6a];
    region.t5 = static_cast<int16_t>(ntohs(rd16(d, 0x6b)));                    // 0x004576a0 (ReadU16)
    if (g_versioned != 0) {
        Nullsub(static_cast<int>(index) + g_regionIdBase, static_cast<int>(extra));   // 0x004242b0
        reinterpret_cast<CRegionManager *>(GLOBAL_PTR(void, g_006e4738_storage))->SetRegion(static_cast<int>(index), &region);   // SetRegion
        LoginStub(static_cast<int>(index) + g_regionIdBase);         // 0x00421390
    }
}
HOOK_FUNC(0x0045b660, Packet_HandleRegionUpdate_0045b660);

// ---------------------------------------------------------------------------
// 0x0045b980  size 0xa7
// Inbound packet 0x43: hue record (two dwords then data). Only hands the three values to 0x00431ce0
// (an empty method) and the login stub.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x43(const uint8_t *packet)
{
    const uint8_t *d = packet + pkt::Base(packet);
    uint32_t first = ntohl(rd32(d, 0));
    uint32_t second = ntohl(rd32(d, 4));
    const uint8_t *data = d + 8;
    if (g_versioned != 0) {
        PORT_THISCALL3(&Nop_00431ce0, void, GLOBAL_PTR(void, g_animData_storage), int, int, int)(static_cast<int>(first), static_cast<int>(second), ptrint(data));
        LoginStub(static_cast<int>(first) + g_loadProgressArg);         // 0x00421390
    }
}
HOOK_FUNC(0x0045b980, Packet_Handle0x43);

// ---------------------------------------------------------------------------
// 0x0045ba30  size 0xc4
// Inbound packet 0x3F: statics block. serial, then {count, dword, count * 7-byte records}; the count, the dword
// and the two words of each record (at +0 and +5) are byte-swapped in place.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x3F(uint8_t *packet)
{
    uint8_t *d = packet + pkt::Base(packet);
    uint32_t serial = ntohl(rd32(d, 0));
    uint8_t *block = d + 4;
    uint32_t count = ntohl(rd32(block, 0));
    wr32(block, 0, count);
    uint8_t *p = block + 4;
    wr32(p, 0, ntohl(rd32(p, 0)));
    p += 4;
    int32_t n = static_cast<int32_t>(count);
    if (n > 0) {
        do {
            uint16_t a = ntohs(rd16(p, 0));
            uint16_t b = ntohs(rd16(p, 5));
            wr16(p, 0, a);
            wr16(p, 5, b);
            p += 7;
            n--;
        } while (n != 0);
    }
    if (g_versioned != 0) {
        Nullsub(static_cast<int>(serial), ptrint(block));                           // 0x004242b0
        LoginStub(static_cast<int>(serial) + g_00758258);        // 0x00421390
    }
}
HOOK_FUNC(0x0045ba30, Packet_Handle0x3F);

// ---------------------------------------------------------------------------
// 0x0045bb00  size 0x11c
// Inbound packet 0x92: record update for file pair 0xd / 0xe. serial, extra dword, then the records (12 bytes
// each, byte-swapped into a fresh buffer by 0x0044ee40). Variable length: the data length is
// packet length - base - 8.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x92(const uint8_t *packet)
{
    uint32_t cursor = pkt::Cursor(packet);
    uint32_t base = pkt::Base(packet);
    const uint8_t *d = packet + base;
    uint32_t serial = ntohl(rd32(d, 0));
    uint32_t extra = ntohl(rd32(d, 4));
    int len = static_cast<int>(cursor - base) - 8;
    const uint8_t *data = d + 8;
    if (g_versioned != 0) {
        void *swapped = ByteSwapRecords12(data, len);                               // 0x0044ee40
        g_fileManager.ReplaceRecord(0xd, 0xe, static_cast<int>(serial), static_cast<int>(extra), swapped, len);   // 0x00440210
        operator delete(swapped);                                                         // 0x0048ad60
        Nullsub(static_cast<int>(serial) + g_00758244, static_cast<int>(extra));   // 0x004242b0
        LoginStub(static_cast<int>(serial) + g_00758244);        // 0x00421390
    }
}
HOOK_FUNC(0x0045bb00, Packet_Handle0x92);

// ---------------------------------------------------------------------------
// 0x0045bc20  size 0xfc
// Inbound packet 0x94: record update for file pair 0xf / 0x10: a byte (record), a dword, then the raw data
// (packet length - base - 5 bytes).
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x94(const uint8_t *packet)
{
    uint32_t cursor = pkt::Cursor(packet);
    uint32_t base = pkt::Base(packet);
    const uint8_t *d = packet + base;
    uint8_t record = d[0];
    uint32_t value = ntohl(rd32(d, 1));
    int len = static_cast<int>(cursor - base) - 5;
    const uint8_t *data = d + 5;
    if (g_versioned != 0) {
        uint32_t rec = record;
        g_fileManager.ReplaceRecord(0xf, 0x10, static_cast<int>(rec), static_cast<int>(value), const_cast<uint8_t *>(data), len);   // 0x00440210
        Nullsub(g_00758224 + static_cast<int>(rec), static_cast<int>(value));   // 0x004242b0
        LoginStub(g_00758224 + static_cast<int>(rec));           // 0x00421390
    }
}
HOOK_FUNC(0x0045bc20, Packet_Handle0x94);

// ---------------------------------------------------------------------------
// 0x0045bd20  size 0x90
// Inbound packet 0x40: terrain tile data. A dword (tile number), 0x40 {word, byte} entries whose words are
// byte-swapped in place, and a trailing dword at +0xc4, swapped in place.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x40(uint8_t *packet)
{
    uint8_t *d = packet + pkt::Base(packet);
    wr32(d, 0, ntohl(rd32(d, 0)));
    uint8_t *p = d + 4;
    for (int i = 0x40; i != 0; i--) {
        wr16(p, 0, ntohs(rd16(p, 0)));
        p += 3;
    }
    uint32_t tail = ntohl(rd32(d, 0xc4));
    uint32_t tile = rd32(d, 0);
    wr32(d, 0xc4, tail);
    if (g_versioned != 0) {
        Nullsub(static_cast<int>(tile), ptrint(d));                                 // 0x004242b0
        LoginStub(static_cast<int>(tile) + g_0075825c);          // 0x00421390
    }
}
HOOK_FUNC(0x0045bd20, Packet_Handle0x40);

// ---------------------------------------------------------------------------
// 0x0045bdb0  size 0x127
// Inbound packet 0x41: tile data for tile number `serial`: below 0x200 the terrain record layout (32 entries,
// handed to CObjectManager::SetTerrTileData), else the item layout (SetItemTileData). Fields are swapped in place.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x41(uint8_t *packet)
{
    uint8_t *packetBase = packet + pkt::Base(packet);
    uint32_t serial = ntohl(rd32(packetBase, 0));
    if (g_versioned == 0)
        return;
    uint8_t *block = packetBase + 4;
    wr32(block, 0, ntohl(rd32(block, 0)));
    uint8_t *p = block + 4;
    if (static_cast<int32_t>(serial) < 0x200) {
        for (int i = 0x20; i != 0; i--) {
            wr32(p, 0, ntohl(rd32(p, 0)));
            uint16_t w = ntohs(rd16(p, 4));
            p += 4;
            wr16(p, 0, w);
            p += 0x16;
        }
        (&g_objMgr)->CObjectManager::SetTerrTileData(serial, block);   // CObjectManager::SetTerrTileData
    } else {
        for (int i = 0x20; i != 0; i--) {
            wr32(p, 0, ntohl(rd32(p, 0)));
            uint32_t a = ntohl(rd32(p, 6));
            p += 6;
            wr32(p, 0, a);
            uint16_t b = ntohs(rd16(p, 4));
            p += 4;
            wr16(p, 0, b);
            uint16_t c = ntohs(rd16(p, 2));
            p += 2;
            wr16(p, 0, c);
            uint16_t e = ntohs(rd16(p, 2));
            p += 2;
            wr16(p, 0, e);
            p += 0x17;
        }
        (&g_objMgr)->CObjectManager::SetItemTileData(serial, block);   // CObjectManager::SetItemTileData
    }
    LoginStub(static_cast<int>(serial) + g_tileGroupBase);            // 0x00421390
}
HOOK_FUNC(0x0045bdb0, Packet_Handle0x41);

// ---------------------------------------------------------------------------
// 0x0045bee0  size 0xdc
// Inbound packet 0x42: art record. index, arg, mode dword (-1 = empty record), then the record data (0x0045bee0
// passes a null data pointer for an empty record) handed to ArtUpdateRecord (0x00436b50).
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x42(const uint8_t *packet)
{
    const uint8_t *d = packet + pkt::Base(packet);
    uint32_t index = ntohl(rd32(d, 0));
    uint32_t arg = ntohl(rd32(d, 4));
    uint32_t mode = ntohl(rd32(d, 8));
    const uint8_t *data = nullptr;
    if (mode != 0xffffffffu)
        data = d + 0xc;
    if (g_versioned != 0) {
        ArtUpdateRecord(data, static_cast<int>(index), static_cast<int>(mode), static_cast<int>(arg));   // ArtUpdateRecord
        LoginStub(static_cast<int>(index) + AUO_unk75821c);         // 0x00421390
    }
}
HOOK_FUNC(0x0045bee0, Packet_Handle0x42);

// ---------------------------------------------------------------------------
// 0x0045bfc0  size 0x1b2
// Inbound packet 0x3E: a dword and eight more dwords (kept on the stack, handed to the empty 0x004242b0).
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x3E(const uint8_t *packet)
{
    const uint8_t *d = packet + pkt::Base(packet);
    uint32_t first = ntohl(rd32(d, 0));
    uint32_t values[8];
    values[0] = ntohl(rd32(d, 4));
    values[1] = ntohl(rd32(d, 8));
    values[2] = ntohl(rd32(d, 0xc));
    values[3] = ntohl(rd32(d, 0x10));
    values[4] = ntohl(rd32(d, 0x14));
    values[5] = ntohl(rd32(d, 0x18));
    values[6] = ntohl(rd32(d, 0x1c));
    values[7] = ntohl(rd32(d, 0x20));
    if (g_versioned != 0)
        Nullsub(static_cast<int>(first), ptrint(values));                           // 0x004242b0
}
HOOK_FUNC(0x0045bfc0, Packet_Handle0x3E);

// ---------------------------------------------------------------------------
// 0x0045c180  size 0x44
// Inbound packet 0x45: version OK. One dword handed to the login stub.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x45(const uint8_t *packet)
{
    uint32_t version = ntohl(rd32(packet, pkt::Base(packet)));
    if (g_versioned != 0)
        LoginStub(static_cast<int>(version));                                       // 0x00421390
}
HOOK_FUNC(0x0045c180, Packet_Handle0x45);
