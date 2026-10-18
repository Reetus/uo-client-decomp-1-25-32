// Outbound packet builders, second pass (batch 25):
// 0x66 (two forms), 0x6F, 0x71 (#1), 0x72, 0x73, 0x75, 0x7D, 0x95, 0x80, 0x83,
// 0x84, 0x98, 0x9A, 0xA0, 0xA7, 0xAC and the 0x00465b30 walk thunk.
// 0x004641e0, 0x00464360, 0x00464900, 0x00464ac0, 0x00464be0, 0x00465030,
// 0x004650b0, 0x00465190, 0x00465360, 0x00465490, 0x004655c0, 0x00465700,
// 0x00465890, 0x00465910, 0x00465ab0, 0x00465b30, 0x00465de0, 0x00465eb0.
//
// Cursor scheme as in packet_cursor.h. Every builder expands the cursor
// accessor inline (no call to 0x00458060 / 0x0045d610 / 0x00457520...), so
// the helpers below use pkt::Cursor / pkt::SetCursor only. The htons / htonl
// calls (0x00498658 / 0x0049865e) are made through pkt:: before the cursor is
// read, as in the original.
#include <cstring>

#include "../runtime/ported.h"
#include "packet_cursor.h"
#include "raw_field.h"
#include "walk_queue_ports.h"
#include "globals.h"

#define g_netConn GLOBAL_AS(uint8_t *, g_pktSender)   // connection object; +0x20028 is the login count

// Starts a packet: opcode, then cursor 3 (variable length) or 1 (fixed).
static void b25_begin(uint8_t *buf, uint8_t opcode)
{
    buf[0] = opcode;
    if (pkt::IsVariable(buf))
        *reinterpret_cast<uint16_t *>(buf + 1) = 3;
    else
        g_pktFixedCursor = 1;
}

static void b25_u8(uint8_t *buf, uint8_t v)
{
    uint32_t off = pkt::Cursor(buf);
    buf[off] = v;
    pkt::SetCursor(buf, off + 1);
}

static void b25_u16(uint8_t *buf, uint16_t v)
{
    uint32_t off = pkt::Cursor(buf);
    pkt::Put16(buf, off, v);
    pkt::SetCursor(buf, off + 2);
}

static void b25_u32(uint8_t *buf, uint32_t v)
{
    uint32_t off = pkt::Cursor(buf);
    pkt::Put32(buf, off, v);
    pkt::SetCursor(buf, off + 4);
}

// 30 bytes: seven dwords and a word.
static void b25_blob30(uint8_t *buf, const void *src)
{
    uint32_t off = pkt::Cursor(buf);
    std::memcpy(buf + off, src, 0x1e);
    pkt::SetCursor(buf, off + 0x1e);
}

// ---------------------------------------------------------------------------
// 0x004641e0  size 0x17a
// Packet 0x66, "request page" form: serial, page count 1, page, line count 0xffff. RET 0xc.
// ---------------------------------------------------------------------------
void __stdcall Pkt_Build_0x66_BookPageRequest(uint8_t *buf, uint32_t serial, uint32_t page)
{
    b25_begin(buf, 0x66);
    b25_u32(buf, htonl(serial));
    b25_u16(buf, htons(1));
    b25_u16(buf, htons(page));
    b25_u16(buf, htons(0xffff));
}
HOOK_FUNC(0x004641e0, Pkt_Build_0x66_BookPageRequest);

// ---------------------------------------------------------------------------
// 0x00464360  size 0x267
// Packet 0x66, "send page text" form. `book` (null: only the header is written) holds the
// serial at book->+0x34->+0x78 and the page array at +0x9c (0x648 bytes per page: u16 line
// count, then lines of 0x50 bytes). RET 0xc.
// ---------------------------------------------------------------------------
void __stdcall Pkt_Build_0x66_BookPageSend(uint8_t *buf, void *book, uint32_t page)
{
    b25_begin(buf, 0x66);
    if (book == nullptr)
        return;

    b25_u32(buf, htonl(RAWF(uint32_t, RAWF(void *, book, 0x34), 0x78)));
    b25_u16(buf, htons(1));
    b25_u16(buf, htons(page));

    uint32_t pageOff = ((page - 1) & 0xffffu) * 0x648u;
    uint16_t hostCount = RAWF(uint16_t, RAWF(uint8_t *, book, 0x9c) + pageOff, 0);
    b25_u16(buf, htons(hostCount));
    uint32_t n = hostCount;
    if (n == 0)
        return;
    uint32_t lineOff = 0;
    do {
        const char *line = reinterpret_cast<const char *>(RAWF(uint8_t *, book, 0x9c) + lineOff + pageOff + 2);
        uint32_t len = static_cast<uint32_t>(strlen(line)) + 1;
        uint32_t off = pkt::Cursor(buf);
        std::memcpy(buf + off, line, len);
        pkt::SetCursor(buf, len + off);
        lineOff += 0x50;
        n--;
    } while (n != 0);
}
HOOK_FUNC(0x00464360, Pkt_Build_0x66_BookPageSend);

// ---------------------------------------------------------------------------
// 0x00464900  size 0x1b4
// Packet 0x6F (secure trade): action byte, three serials, a zero byte. RET 0x14.
// ---------------------------------------------------------------------------
void __stdcall Pkt_Build_0x6F_SecureTrade(uint8_t *buf, uint8_t action, uint32_t id1, uint32_t id2, uint32_t id3)
{
    b25_begin(buf, 0x6f);
    b25_u8(buf, action);
    b25_u32(buf, htonl(id1));
    b25_u32(buf, htonl(id2));
    b25_u32(buf, htonl(id3));
    b25_u8(buf, 0);
}
HOOK_FUNC(0x00464900, Pkt_Build_0x6F_SecureTrade);

// ---------------------------------------------------------------------------
// 0x00464ac0  size 0x11e
// Packet 0x71 (bulletin board), simple form: type byte, board serial, message serial. RET 0x10.
// ---------------------------------------------------------------------------
void __stdcall Pkt_Build_0x71_BBoardSimple(uint8_t *buf, uint8_t type, uint32_t boardSerial, uint32_t msgSerial)
{
    b25_begin(buf, 0x71);
    b25_u8(buf, type);
    b25_u32(buf, htonl(boardSerial));
    b25_u32(buf, htonl(msgSerial));
}
HOOK_FUNC(0x00464ac0, Pkt_Build_0x71_BBoardSimple);

// ---------------------------------------------------------------------------
// 0x00464be0  size 0x15a
// Packet 0x72 (war mode): four bytes. RET 0x14.
// ---------------------------------------------------------------------------
void __stdcall Pkt_Build_0x72_WarMode(uint8_t *buf, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
{
    b25_begin(buf, 0x72);
    b25_u8(buf, b1);
    b25_u8(buf, b2);
    b25_u8(buf, b3);
    b25_u8(buf, b4);
}
HOOK_FUNC(0x00464be0, Pkt_Build_0x72_WarMode);

// ---------------------------------------------------------------------------
// 0x00465030  size 0x75
// Packet 0x73 (ping): one byte. RET 8.
// ---------------------------------------------------------------------------
void __stdcall Pkt_Build_0x73_Ping(uint8_t *buf, uint8_t seq)
{
    b25_begin(buf, 0x73);
    b25_u8(buf, seq);
}
HOOK_FUNC(0x00465030, Pkt_Build_0x73_Ping);

// ---------------------------------------------------------------------------
// 0x004650b0  size 0xda
// Packet 0x75 (rename character): serial, 30-byte name. RET 0xc.
// ---------------------------------------------------------------------------
void __stdcall Pkt_Build_0x75_RenameCharacter(uint8_t *buf, uint32_t serial, const void *name30)
{
    b25_begin(buf, 0x75);
    b25_u32(buf, htonl(serial));
    b25_blob30(buf, name30);
}
HOOK_FUNC(0x004650b0, Pkt_Build_0x75_RenameCharacter);

// ---------------------------------------------------------------------------
// 0x00465190  size 0x1cd
// Packet 0x7D (menu response): dialog id, then four words. RET 0x18.
// ---------------------------------------------------------------------------
void __stdcall Pkt_Build_0x7D_MenuResponse(uint8_t *buf, uint32_t dialogId, uint32_t w1, uint32_t w2, uint32_t w3, uint32_t w4)
{
    b25_begin(buf, 0x7d);
    b25_u32(buf, htonl(dialogId));
    b25_u16(buf, htons(w1));
    b25_u16(buf, htons(w2));
    b25_u16(buf, htons(w3));
    b25_u16(buf, htons(w4));
}
HOOK_FUNC(0x00465190, Pkt_Build_0x7D_MenuResponse);

// ---------------------------------------------------------------------------
// 0x00465360  size 0x129
// Packet 0x95 (dye window reply): serial, two words. RET 0x10.
// ---------------------------------------------------------------------------
void __stdcall Pkt_Build_0x95_DyeWindowReply(uint8_t *buf, uint32_t serial, uint32_t w1, uint32_t w2)
{
    b25_begin(buf, 0x95);
    b25_u32(buf, htonl(serial));
    b25_u16(buf, htons(w1));
    b25_u16(buf, htons(w2));
}
HOOK_FUNC(0x00465360, Pkt_Build_0x95_DyeWindowReply);

// ---------------------------------------------------------------------------
// 0x00465490  size 0x130
// Packet 0x80 (account login): two 30-byte blobs, one byte. RET 0x10.
// ---------------------------------------------------------------------------
void __stdcall Pkt_Build_0x80_AccountLogin(uint8_t *buf, const void *account30, const void *password30, uint8_t key)
{
    b25_begin(buf, 0x80);
    b25_blob30(buf, account30);
    b25_blob30(buf, password30);
    b25_u8(buf, key);
}
HOOK_FUNC(0x00465490, Pkt_Build_0x80_AccountLogin);

// ---------------------------------------------------------------------------
// 0x004655c0  size 0x13a
// Packet 0x83 (delete character): 30-byte blob, slot, login count from the connection. RET 0xc.
// ---------------------------------------------------------------------------
void __stdcall Pkt_Build_0x83_DeleteCharacter(uint8_t *buf, uint32_t slot, const void *password30)
{
    b25_begin(buf, 0x83);
    b25_blob30(buf, password30);
    b25_u32(buf, htonl(slot));
    b25_u32(buf, htonl(RAWF(uint32_t, g_netConn, 0x20028)));
}
HOOK_FUNC(0x004655c0, Pkt_Build_0x83_DeleteCharacter);

// ---------------------------------------------------------------------------
// 0x00465700  size 0x18d
// Packet 0x84: two 30-byte blobs, slot, login count from the connection. RET 0x10.
// ---------------------------------------------------------------------------
void __stdcall Pkt_Build_0x84(uint8_t *buf, uint32_t slot, const void *blobA30, const void *blobB30)
{
    b25_begin(buf, 0x84);
    b25_blob30(buf, blobA30);
    b25_blob30(buf, blobB30);
    b25_u32(buf, htonl(slot));
    b25_u32(buf, htonl(RAWF(uint32_t, g_netConn, 0x20028)));
}
HOOK_FUNC(0x00465700, Pkt_Build_0x84);

// ---------------------------------------------------------------------------
// 0x00465890  size 0x7e
// Packet 0x98 (all names query): serial. RET 8.
// ---------------------------------------------------------------------------
void __stdcall Pkt_Build_0x98_AllNamesQuery(uint8_t *buf, uint32_t serial)
{
    b25_begin(buf, 0x98);
    b25_u32(buf, htonl(serial));
}
HOOK_FUNC(0x00465890, Pkt_Build_0x98_AllNamesQuery);

// ---------------------------------------------------------------------------
// 0x00465910  size 0x199
// Packet 0x9A (prompt reply): three serials, then the NUL-terminated text. RET 0x14.
// ---------------------------------------------------------------------------
void __stdcall Pkt_Build_0x9A_PromptReply(uint8_t *buf, uint32_t s1, uint32_t s2, uint32_t s3, const char *text)
{
    b25_begin(buf, 0x9a);
    b25_u32(buf, htonl(s1));
    b25_u32(buf, htonl(s2));
    b25_u32(buf, htonl(s3));
    uint32_t len = static_cast<uint32_t>(strlen(text)) + 1;
    uint32_t off = pkt::Cursor(buf);
    std::memcpy(buf + off, text, len);
    pkt::SetCursor(buf, len + off);
}
HOOK_FUNC(0x00465910, Pkt_Build_0x9A_PromptReply);

// ---------------------------------------------------------------------------
// 0x00465ab0  size 0x7f
// Packet 0xA0 (select server): one word. RET 8.
// ---------------------------------------------------------------------------
void __stdcall Pkt_Build_0xA0_SelectServer(uint8_t *buf, uint32_t index)
{
    b25_begin(buf, 0xa0);
    b25_u16(buf, htons(index));
}
HOOK_FUNC(0x00465ab0, Pkt_Build_0xA0_SelectServer);

// ---------------------------------------------------------------------------
// 0x00465b30  size 0x2e
// Takes a received packet (RET 4), reads its first field byte (offset 3 for a variable-length
// opcode, else 1) and calls 0x0044a5f0 (forced walk) with it in ECX; only CL is used there.
// ---------------------------------------------------------------------------
void __stdcall Walk_ForcedFromPacket(const uint8_t *packet)
{
    uint32_t base = ((pkt::Flags(packet[0]) & 0x8000u) | 0x4000u) >> 14;
    uint8_t dir = packet[base];
    Walk_ForcedThunk(dir);
}
HOOK_FUNC(0x00465b30, Walk_ForcedFromPacket);

// ---------------------------------------------------------------------------
// 0x00465de0  size 0xcd
// Packet 0xA7 (request tip): word, byte. RET 0xc.
// ---------------------------------------------------------------------------
void __stdcall Pkt_Build_0xA7_RequestTip(uint8_t *buf, uint32_t tipIndex, uint8_t type)
{
    b25_begin(buf, 0xa7);
    b25_u16(buf, htons(tipIndex));
    b25_u8(buf, type);
}
HOOK_FUNC(0x00465de0, Pkt_Build_0xA7_RequestTip);

// ---------------------------------------------------------------------------
// 0x00465eb0  size 0x1f5
// Packet 0xAC (gump text entry reply): serial, word, button byte, u16 strlen+1, text with NUL. RET 0x14.
// ---------------------------------------------------------------------------
void __stdcall Pkt_Build_0xAC_GumpTextEntryReply(uint8_t *buf, uint32_t serial, uint32_t gumpType, uint8_t button, const char *text)
{
    b25_begin(buf, 0xac);
    b25_u32(buf, htonl(serial));
    b25_u16(buf, htons(gumpType));
    b25_u8(buf, button);
    b25_u16(buf, htons(static_cast<uint32_t>(strlen(text)) + 1));
    uint32_t len = static_cast<uint32_t>(strlen(text)) + 1;
    uint32_t off = pkt::Cursor(buf);
    std::memcpy(buf + off, text, len);
    pkt::SetCursor(buf, len + off);
}
HOOK_FUNC(0x00465eb0, Pkt_Build_0xAC_GumpTextEntryReply);
