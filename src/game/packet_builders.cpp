// Outbound packet builders: 0x6C (target cursor response, two entry points),
// 0x69, 0x71 (#2) and 0xA4 (system information).
// 0x0045d3d0, 0x0045d630, 0x004645d0, 0x00464d40, 0x00465b60.
//
// Packets are built in a caller-supplied buffer with the cursor scheme
// described in packet_cursor.h. The original expands the cursor accessor at
// every field; the port uses pkt::Cursor / pkt::SetCursor and calls the
// out-of-line helpers (0x00458060, 0x0045d610, 0x00457520/70/f0) wherever
// the original does.
#include <cstring>

#include "../runtime/ported.h"
#include "packet_cursor.h"
#include "raw_field.h"
#include "target_cursor.h"
#include "vcall.h"
#include "app_support_ports.h"
#include "sys_info.h"
#include "globals.h"

// Globals (single-file use).
#define g_targetPending GLOBAL_AS(int32_t, g_awaitingTargetControl)       // cleared when the request is finished
#define g_values69 GLOBAL_PTR(uint32_t, g_ccVals76c)           // mode 1: one dword per name
#define g_values69b GLOBAL_PTR(uint32_t, g_ccVals71c)          // mode 2: one dword per name

static void TargetCursor_Done() { App_SetCursor11(); }
static inline void SysInfo_Collect(uint8_t *info) { reinterpret_cast<CSysInfo *>(info)->Collect(); }   // fills the 0x98-byte record

// Starts a packet: opcode, then the initial cursor (3 for a variable-length opcode, 1 for a fixed one).
static inline void begin_packet(uint8_t *buf, uint8_t opcode)
{
    buf[0] = opcode;
    if ((*reinterpret_cast<const uint32_t *>(GADDR(g_packetTable) + static_cast<uint32_t>(buf[0]) * 8u) & 0x8000) != 0)
        *reinterpret_cast<uint16_t *>(buf + 1) = 3;
    else
        g_pktFixedCursor = 1;
}

// Starts a packet through the out-of-line setter (0x00458060), as 0x0045d630 does.
static void begin_packet_orig(uint8_t *buf)
{
    buf[0] = 0x6c;
    if (pkt::IsVariable(buf))
        pkt::OrigSetCursor(buf, 3);
    else
        g_pktFixedCursor = 1;
}

// ---------------------------------------------------------------------------
// 0x0045d3d0  size 0x236
// Send the "cancel" variant of packet 0x6C and clear the targeting state.
// ---------------------------------------------------------------------------
void __cdecl TargetCursor_SendCancel()
{
    uint8_t buf[0x1c];
    uint32_t off;
    uint32_t v;

    begin_packet(buf, 0x6c);

    // type byte 0
    off = pkt::Cursor(buf);
    buf[off] = 0;
    off = off + 1;
    pkt::SetCursor(buf, off);

    // htonl(cursor id)
    v = htonl(g_targetCursorId);
    off = pkt::Cursor(buf);
    pkt::Put32(buf, off, v);
    pkt::OrigSetCursor(buf, off + 4);

    // flags byte
    off = pkt::Cursor(buf);
    buf[off] = g_targetFlags;
    pkt::OrigSetCursor(buf, off + 1);

    // htonl(0)
    v = htonl(0);
    off = pkt::Cursor(buf);
    pkt::Put32(buf, off, v);
    pkt::OrigSetCursor(buf, off + 4);

    // htons(0xffff) twice
    v = htons(0xffff);
    off = pkt::Cursor(buf);
    pkt::Put16(buf, off, static_cast<uint16_t>(v));
    pkt::OrigSetCursor(buf, off + 2);

    v = htons(0xffff);
    off = pkt::Cursor(buf);
    pkt::Put16(buf, off, static_cast<uint16_t>(v));
    pkt::OrigSetCursor(buf, off + 2);

    // htons(0) twice, cursor read through 0x0045d610
    v = htons(0);
    off = pkt::OrigGetCursor(buf) & 0xffff;
    pkt::Put16(buf, off, static_cast<uint16_t>(v));
    pkt::OrigSetCursor(buf, off + 2);

    v = htons(0);
    off = pkt::OrigGetCursor(buf) & 0xffff;
    pkt::Put16(buf, off, static_cast<uint16_t>(v));
    pkt::OrigSetCursor(buf, off + 2);

    pkt::Send(buf);
    TargetCursor_Done();
    g_targetFlags = 0;
    g_targetPending = 0;
}
HOOK_FUNC(0x0045d3d0, TargetCursor_SendCancel);

// ---------------------------------------------------------------------------
// 0x0045d630  size 0x2c9
// Target cursor completion: send packet 0x6C for the clicked object.
// ECX = the object; null is ignored.
// ---------------------------------------------------------------------------
void __thiscall CTargetObj::SendTargetResponse()
{
    uint8_t buf[0x28];
    uint32_t off;
    uint32_t idn;

    if (this == nullptr)
        return;

    if (decomp::vcall(PORT_RET(int), this, 0x1c) != 0) {
        uint16_t tx = RAWF(uint16_t, this, 0x20);
        uint16_t ty = RAWF(uint16_t, this, 0x22);
        uint16_t tz = RAWF(uint16_t, this, 0x24);
        begin_packet_orig(buf);
        off = pkt::OrigGetCursor(buf) & 0xffff;
        buf[off] = 1;
        pkt::OrigSetCursor(buf, off + 1);
        idn = htonl(g_targetCursorId);
        off = pkt::OrigGetCursor(buf) & 0xffff;
        pkt::Put32(buf, off, idn);
        pkt::OrigSetCursor(buf, off + 4);
        uint8_t flags = g_targetFlags;
        off = pkt::OrigGetCursor(buf) & 0xffff;
        buf[off] = flags;
        pkt::OrigSetCursor(buf, off + 1);
        idn = htonl(0);
        off = pkt::OrigGetCursor(buf) & 0xffff;
        pkt::Put32(buf, off, idn);
        pkt::OrigSetCursor(buf, off + 4);
        pkt::OrigAppendU16(buf, tx);
        pkt::OrigAppendU16(buf, ty);
        pkt::OrigAppendU16(buf, tz);
        pkt::OrigAppendU16(buf, 0);
    } else if (decomp::vcall(PORT_RET(int), this, 0x2c) != 0) {
        uint16_t ty = RAWF(uint16_t, this, 0x22);
        uint16_t gfx = RAWF(uint16_t, this, 0x38);
        uint16_t tz = RAWF(uint16_t, this, 0x24);
        uint16_t tx = RAWF(uint16_t, this, 0x20);
        uint32_t serial = RAWF(uint32_t, this, 0x78);
        begin_packet_orig(buf);
        off = pkt::OrigGetCursor(buf) & 0xffff;
        buf[off] = 0;
        pkt::OrigSetCursor(buf, off + 1);
        idn = htonl(g_targetCursorId);
        off = pkt::OrigGetCursor(buf) & 0xffff;
        pkt::Put32(buf, off, idn);
        pkt::OrigSetCursor(buf, off + 4);
        uint8_t flags = g_targetFlags;
        off = pkt::OrigGetCursor(buf) & 0xffff;
        buf[off] = flags;
        pkt::OrigSetCursor(buf, off + 1);
        pkt::OrigAppendU32(buf, serial);
        pkt::OrigAppendU16(buf, tx);
        pkt::OrigAppendU16(buf, ty);
        pkt::OrigAppendU16(buf, tz);
        pkt::OrigAppendU16(buf, gfx);
    } else {
        uint16_t ty = RAWF(uint16_t, this, 0x22);
        uint16_t gfx = RAWF(uint16_t, this, 0x38);
        uint16_t tz = RAWF(uint16_t, this, 0x24);
        uint16_t tx = RAWF(uint16_t, this, 0x20);
        begin_packet_orig(buf);
        off = pkt::OrigGetCursor(buf) & 0xffff;
        buf[off] = 1;
        pkt::OrigSetCursor(buf, off + 1);
        pkt::OrigAppendU32(buf, g_targetCursorId);
        pkt::OrigAppendU8(buf, g_targetFlags);
        pkt::OrigAppendU32(buf, 0);
        pkt::OrigAppendU16(buf, tx);
        pkt::OrigAppendU16(buf, ty);
        pkt::OrigAppendU16(buf, tz);
        pkt::OrigAppendU16(buf, gfx);
    }
    pkt::Send(buf);
    TargetCursor_Done();
    g_targetFlags = 0;
}
HOOK_METHOD(0x0045d630, CTargetObj, SendTargetResponse);

// ---------------------------------------------------------------------------
// 0x004645d0  size 0x32c
// Outbound packet 0x69 builder. RET 8: buf, mode.
//   mode 0: one zero byte; mode 1: count byte + (name, htonl(value)) pairs
//   from the list at 0x0069f4c4; mode 2: the same from the list at 0x0069f26c;
//   mode > 2: only the mode byte.
// ---------------------------------------------------------------------------
void __stdcall Packet_Build0x69(uint8_t *buf, uint8_t mode)
{
    uint32_t off;
    uint32_t len;
    uint32_t v;
    int i;

    begin_packet(buf, 0x69);

    off = pkt::Cursor(buf);
    buf[off] = mode;
    off = off + 1;
    pkt::SetCursor(buf, off);

    if (mode == 0) {
        off = pkt::Cursor(buf);
        buf[off] = 0;
        off = off + 1;
        pkt::SetCursor(buf, off);
        return;
    }

    if (mode == 1) {
        uint8_t cnt = static_cast<uint8_t>(g_ccSel264);
        off = pkt::Cursor(buf);
        buf[off] = cnt;
        off = off + 1;
        pkt::SetCursor(buf, off);

        i = 0;
        if (g_ccSel264 > 0) {
            const char *name = g_ccNames4c4;
            const uint32_t *vals = g_values69;
            do {
                len = static_cast<uint32_t>(strlen(name)) + 1;
                off = pkt::Cursor(buf);
                memcpy(buf + off, name, len);
                pkt::OrigSetCursor(buf, off + len);
                v = htonl(*vals);
                off = pkt::Cursor(buf);
                pkt::Put32(buf, off, v);
                pkt::OrigSetCursor(buf, off + 4);
                i++;
                name += 0x1e;
                vals++;
            } while (i < g_ccSel264);
        }
        return;
    }

    if (mode == 2) {
        uint8_t cnt = static_cast<uint8_t>(g_ccSel260);
        off = pkt::Cursor(buf);
        buf[off] = cnt;
        pkt::OrigSetCursor(buf, off + 1);

        i = 0;
        if (g_ccSel260 > 0) {
            const char *name = g_ccNames26c;
            const uint32_t *vals = g_values69b;
            do {
                len = static_cast<uint32_t>(strlen(name)) + 1;
                off = pkt::Cursor(buf);
                memcpy(buf + off, name, len);
                pkt::OrigSetCursor(buf, len + off);
                v = htonl(*vals);
                off = pkt::OrigGetCursor(buf) & 0xffff;
                pkt::Put32(buf, off, v);
                pkt::OrigSetCursor(buf, off + 4);
                i++;
                name += 0x1e;
                vals++;
            } while (i < g_ccSel260);
        }
    }
}
HOOK_FUNC(0x004645d0, Packet_Build0x69);

// ---------------------------------------------------------------------------
// 0x00464d40  size 0x2e1
// Outbound packet 0x71 builder #2 (variable length). RET 0xc: buf, type, board/message object.
//   obj+0x34 parent object (serial at +0x78), obj+0xbc serial, obj+0x94 -> subject string at +0xec,
//   obj+0x158 line count, obj+0x174 array of char * body lines.
// ---------------------------------------------------------------------------
void __stdcall Packet_Build0x71(uint8_t *buf, uint8_t type, void *obj)
{
    uint32_t off;
    uint32_t v;
    uint8_t len;
    uint8_t cnt;
    int i;

    begin_packet(buf, 0x71);

    off = pkt::Cursor(buf);
    buf[off] = type;
    off = off + 1;
    pkt::SetCursor(buf, off);

    void *parent = RAWF(void *, obj, 0x34);
    if (parent != nullptr) {
        v = htonl(RAWF(uint32_t, parent, 0x78));
        off = pkt::Cursor(buf);
    } else {
        v = htonl(0);
        off = pkt::Cursor(buf);
    }
    pkt::Put32(buf, off, v);
    pkt::OrigSetCursor(buf, off + 4);

    v = htonl(RAWF(uint32_t, obj, 0xbc));
    off = pkt::Cursor(buf);
    pkt::Put32(buf, off, v);
    pkt::OrigSetCursor(buf, off + 4);

    const char *str = reinterpret_cast<const char *>(RAWF(uintptr_t, obj, 0x94) + 0xec);
    len = static_cast<uint8_t>(strlen(str) + 1);
    off = pkt::Cursor(buf);
    buf[off] = len;
    pkt::OrigSetCursor(buf, off + 1);

    str = reinterpret_cast<const char *>(RAWF(uintptr_t, obj, 0x94) + 0xec);
    off = pkt::Cursor(buf);
    memcpy(buf + off, str, len);
    pkt::OrigSetCursor(buf, len + off);

    cnt = RAWF(uint8_t, obj, 0x158);
    off = pkt::Cursor(buf);
    buf[off] = cnt;
    pkt::OrigSetCursor(buf, off + 1);

    i = 0;
    if (RAWF(int32_t, obj, 0x158) > 0) {
        char **linePtr = reinterpret_cast<char **>(static_cast<uint8_t *>(obj) + 0x174);
        do {
            len = static_cast<uint8_t>(strlen(*linePtr) + 1);
            off = pkt::Cursor(buf);
            buf[off] = len;
            pkt::OrigSetCursor(buf, off + 1);
            str = *linePtr;
            off = pkt::OrigGetCursor(buf) & 0xffff;
            memcpy(buf + off, str, len);
            pkt::OrigSetCursor(buf, len + off);
            linePtr++;
            i++;
        } while (i < RAWF(int32_t, obj, 0x158));
    }
}
HOOK_FUNC(0x00464d40, Packet_Build0x71);

// ---------------------------------------------------------------------------
// 0x00465b60  size 0x276
// Outbound packet 0xA4 builder (system information). RET 4: buf.
// The 0x98-byte record from 0x0043ef00: +1 u8, +2 u16 (read as dword), +4 u8,
// four 32-byte strings at +5/+0x25/+0x45/+0x65, +0x86 u16, +0x88 u16, +0x8c u32,
// +0x90 u32, +0x94 u32 (appended raw through 0x004575f0).
// ---------------------------------------------------------------------------
void __stdcall Packet_Build0xA4(uint8_t *buf)
{
    uint8_t info[0x98];
    uint32_t off;
    uint32_t v;

    begin_packet(buf, 0xa4);

    SysInfo_Collect(info);

    off = pkt::Cursor(buf);
    buf[off] = info[1];
    pkt::OrigSetCursor(buf, off + 1);

    uint32_t d;
    memcpy(&d, info + 2, sizeof(d));
    v = htons(d);
    off = pkt::Cursor(buf);
    pkt::Put16(buf, off, static_cast<uint16_t>(v));
    pkt::OrigSetCursor(buf, off + 2);

    off = pkt::Cursor(buf);
    buf[off] = info[4];
    pkt::OrigSetCursor(buf, off + 1);

    off = pkt::Cursor(buf);
    memcpy(buf + off, info + 5, 0x20);
    pkt::OrigSetCursor(buf, off + 0x20);

    off = pkt::Cursor(buf);
    memcpy(buf + off, info + 0x25, 0x20);
    pkt::OrigSetCursor(buf, off + 0x20);

    off = pkt::Cursor(buf);
    memcpy(buf + off, info + 0x45, 0x20);
    pkt::OrigSetCursor(buf, off + 0x20);

    off = pkt::OrigGetCursor(buf) & 0xffff;
    memcpy(buf + off, info + 0x65, 0x20);
    pkt::OrigSetCursor(buf, off + 0x20);

    memcpy(&d, info + 0x86, sizeof(d));
    v = htons(d);
    off = pkt::OrigGetCursor(buf) & 0xffff;
    pkt::Put16(buf, off, static_cast<uint16_t>(v));
    pkt::OrigSetCursor(buf, off + 2);

    memcpy(&d, info + 0x88, sizeof(d));
    v = htons(d);
    off = pkt::OrigGetCursor(buf) & 0xffff;
    pkt::Put16(buf, off, static_cast<uint16_t>(v));
    pkt::OrigSetCursor(buf, off + 2);

    memcpy(&d, info + 0x8c, sizeof(d));
    v = htonl(d);
    off = pkt::OrigGetCursor(buf) & 0xffff;
    pkt::Put32(buf, off, v);
    pkt::OrigSetCursor(buf, off + 4);

    memcpy(&d, info + 0x90, sizeof(d));
    v = htonl(d);
    off = pkt::OrigGetCursor(buf) & 0xffff;
    pkt::Put32(buf, off, v);
    pkt::OrigSetCursor(buf, off + 4);

    memcpy(&d, info + 0x94, sizeof(d));
    pkt::OrigAppendU32(buf, d);
}
HOOK_FUNC(0x00465b60, Packet_Build0xA4);
