// Inbound packet handlers (second pass, batch 23): targeting, shops, trades,
// effects, bulletin board, war mode, sequencing.
//   0x99 boat / multi placement target   0x0045d9a0
//   0x6D play music                      0x0045db90
//   0x74 open buy window                 0x0045dbd0
//   0x9E sell list                       0x0045ddc0
//   0x6E mobile animation                0x0045e100
//   0x6F secure trading                  0x0045e250
//   0x70 graphical effect                0x0045e5c0
//   0x71 bulletin board                  0x0045e830
//   0x72 war mode                        0x0045ee70
//   0x7B sequencing                      0x0045ef30
//
// All are __stdcall (RET 4) with the packet pointer. The first field of a packet
// is at "base" (3 for a variable-length opcode, 1 otherwise), see packet_cursor.h.
// The SEH frames the original wraps around operator new + constructor calls are
// omitted.
#include <cstdint>
#include <cstring>

#include "../runtime/ported.h"
#include "pos3.h"
#include "bb_gump.h"
#include "bb_msg_gump.h"
#include "effect_sequence.h"
#include "globals.h"
#include "gump.h"
#include "music_manager.h"
#include "multi_placer.h"
#include "object_manager.h"
#include "crt.h"
#include "packet_cursor.h"
#include "raw_field.h"
#include "shop_gump.h"
#include "toggle_control.h"
#include "vcall.h"
#include "app_support_ports.h"
#include "gump_manager.h"
#include "obj.h"
#include "trade_gump.h"
#include "effect_gump_ports.h"
#include "app_session_ops_ports.h"
#include "target_cursor.h"

#define g_warModeB GLOBAL_AS(uint32_t, g_fightMode)
#define g_warModeC GLOBAL_AS(uint32_t, g_fightValueA)
#define g_warModeD GLOBAL_AS(uint32_t, g_fightValueB)

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

static void *FindObject(uint32_t serial) { return (&g_objMgr)->CObjectManager::FindBySerial(serial); }
static void SetTargetCursorBusy() { App_SetCursor13(); }   // sets 0x006f4774 = 1, mouse cursor 0x13
static void ShowAlert(const char *text) { PORT_THISCALL4(&CreateGump2ac, void, g_gumpManager, const char *, int, int, int)(text, 0, 0, 0); }
static void GumpLink(void *gump) { reinterpret_cast<CGump *>(gump)->LinkInto(nullptr, 1); }   // 0x0043bcc0 (CGump::LinkInto), runs on null too

// ---------------------------------------------------------------------------
// 0x0045d9a0  size 0x1e5
// Inbound packet 0x99: set the target cursor to "place multi" mode and, when the
// multi placer exists, load the multi from the packet.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x99(uint8_t *packet)
{
    uint8_t type = packet[pkt::Base(packet)];
    uint32_t id = ntohl(rd32(packet, pkt::Base(packet) + 1));
    SetTargetCursorBusy();
    g_targetCursorId = id;
    GLOBAL_AS(uint32_t, g_targetCursorType) = type;
    g_targetCallback = PORT_ADDR(&CTargetObj::SendTargetResponse);                       // CTargetObj::SendTargetResponse, stored as a code address

    ntohl(rd32(packet, pkt::Base(packet) + 5));           // discarded
    ntohs(rd16(packet, pkt::Base(packet) + 9));           // discarded
    ntohs(rd16(packet, pkt::Base(packet) + 0xb));         // discarded
    ntohs(rd16(packet, pkt::Base(packet) + 0xd));         // discarded
    ntohs(rd16(packet, pkt::Base(packet) + 0xf));         // discarded
    uint32_t a = ntohs(rd16(packet, pkt::Base(packet) + 0x11));
    uint32_t b = ntohs(rd16(packet, pkt::Base(packet) + 0x13));
    uint32_t c = ntohs(rd16(packet, (pkt::OrigBase(packet) & 0xffff) + 0x15));
    uint32_t d = ntohs(rd16(packet, (pkt::OrigBase(packet) & 0xffff) + 0x17));
    if (g_multiPlacer != nullptr)
        g_multiPlacer->LoadMulti(static_cast<int>(a & 0xffff), static_cast<int16_t>(b), static_cast<int16_t>(c),
                                    static_cast<int16_t>(d));
}
HOOK_FUNC(0x0045d9a0, Packet_Handle0x99);

// ---------------------------------------------------------------------------
// 0x0045db90  size 0x3f
// Inbound packet 0x6D: play a music track.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x6D(uint8_t *packet)
{
    uint32_t track = ntohs(rd16(packet, pkt::Base(packet)));
    g_musicManager.PlayTrack(static_cast<uint16_t>(track), 1, 0);
}
HOOK_FUNC(0x0045db90, Packet_Handle0x6D);

// ---------------------------------------------------------------------------
// 0x0045dbd0  size 0x1e8
// Inbound packet 0x74: open buy window. Stores the price and a malloc'd copy of
// the name in each item of the vendor's buy container (first item at obj+0xa8,
// next at +0x40, name at +0xa0, price at +0xa4).
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x74(uint8_t *packet)
{
    uint32_t serial = ntohl(rd32(packet, pkt::Base(packet)));
    uint32_t off = 5;
    uint8_t count = packet[pkt::Base(packet) + 4];

    if (serial == 0)
        return;
    void *vendor = FindObject(serial);
    if (vendor == nullptr)
        return;

    for (uint8_t *item = RAWF(uint8_t *, vendor, 0xa8); item != nullptr; item = RAWF(uint8_t *, item, 0x40)) {
        RAWF(uint32_t, item, 0xa4) = 0;
        if (RAWF(void *, item, 0xa0) != nullptr)
            operator delete(RAWF(void *, item, 0xa0));
        RAWF(void *, item, 0xa0) = nullptr;
    }

    uint8_t *item = RAWF(uint8_t *, vendor, 0xa8);
    int n = 0;
    int limit = count;
    if (limit <= 0)
        return;
    for (;;) {
        if (item == nullptr)
            return;
        uint32_t price = ntohl(rd32(packet, pkt::Base(packet) + off));
        off += 4;
        uint8_t len = packet[pkt::Base(packet) + off];
        off++;
        char *name = reinterpret_cast<char *>(packet) + pkt::Base(packet) + off;
        off += len;
        if (name != nullptr && len > 0 && name[len - 1] != 0)
            name[len - 1] = 0;
        RAWF(uint32_t, item, 0xa4) = price;
        if (name != nullptr && len > 0 && strlen(name) != 0) {
            char *copy = static_cast<char *>(::operator new(strlen(name) + 1));
            RAWF(char *, item, 0xa0) = copy;
            strcpy(copy, name);
        }
        item = RAWF(uint8_t *, item, 0x40);
        n++;
        if (n >= limit)
            break;
    }
}
HOOK_FUNC(0x0045dbd0, Packet_Handle0x74);

// ---------------------------------------------------------------------------
// 0x0045ddc0  size 0x33f
// Inbound packet 0x9E: sell list. Builds an array of 0x10-byte records
// {serial, graphic, hue, amount, price, name copy}, closes any open shop gump
// of the vendor, opens a sell shop gump and frees the temporary array.
// ---------------------------------------------------------------------------
struct SellRecord {
    uint32_t serial;     // +0x00
    uint16_t graphic;    // +0x04
    uint16_t hue;        // +0x06
    uint16_t amount;     // +0x08
    uint16_t price;      // +0x0a
    char *name;          // +0x0c
};
static_assert(sizeof(SellRecord) == 0x10, "SellRecord size");

void __stdcall Packet_Handle0x9E(uint8_t *packet)
{
    uint32_t serial = ntohl(rd32(packet, pkt::Base(packet)));
    uint32_t count = ntohs(rd16(packet, pkt::Base(packet) + 4));
    uint32_t off = 6;

    if (serial == 0 || static_cast<uint16_t>(count) == 0)
        return;

    int n = static_cast<int>(count & 0xffff);
    SellRecord *items = static_cast<SellRecord *>(::operator new(static_cast<size_t>(n) << 4));
    if (n > 0) {
        SellRecord *rec = items;
        int left = n;
        for (;;) {
            rec->serial = ntohl(rd32(packet, pkt::Base(packet) + off));
            off += 4;
            rec->graphic = ntohs(rd16(packet, pkt::Base(packet) + off));
            off += 2;
            rec->hue = ntohs(rd16(packet, pkt::Base(packet) + off));
            off += 2;
            rec->amount = ntohs(rd16(packet, pkt::Base(packet) + off));
            off += 2;
            rec->price = ntohs(rd16(packet, pkt::Base(packet) + off));
            off += 2;
            uint32_t len = ntohs(rd16(packet, pkt::Base(packet) + off)) & 0xffffu;
            off += 2;
            const uint8_t *src = packet + pkt::Base(packet) + off;
            off += len;
            rec->name = static_cast<char *>(::operator new(len + 1));
            memcpy(rec->name, src, len);
            rec->name[len] = 0;
            rec++;
            if (--left == 0)
                break;
        }
    }

    void *vendor = FindObject(serial);
    if (vendor != nullptr && decomp::vcall(PORT_RET(int), vendor, 0x24) != 0) {      // slot 0x24: is mobile
        for (CGump *g = g_gumpListTail; g != nullptr; g = g->prev) {   // +0x3c
            if (decomp::vcall(PORT_RET(int), g, 0xa0) != 0 && g->owner == vendor) {   // slot 0xa0: is shop gump
                decomp::vdelete(g);
                break;
            }
        }
        CShopGump *gump = new CShopGump(0x64, 0, vendor, static_cast<uint16_t>(count),
                                        reinterpret_cast<ShopSellItem *>(items));
        GumpLink(gump);
        RAWF(CGump *, gump, 0xd8)->BringToFront(0);
    }

    if (n > 0) {
        for (int i = 0; i < n; i++)
            operator delete(items[i].name);
    }
    operator delete(items);
}
HOOK_FUNC(0x0045ddc0, Packet_Handle0x9E);

// ---------------------------------------------------------------------------
// 0x0045e100  size 0x14e
// Inbound packet 0x6E: mobile animation, forwarded to CMobile::StartAction.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x6E(uint8_t *packet)
{
    uint32_t serial = ntohl(rd32(packet, pkt::Base(packet)));
    uint32_t a = ntohs(rd16(packet, pkt::Base(packet) + 4));
    uint32_t b = ntohs(rd16(packet, pkt::Base(packet) + 6));
    uint32_t c = ntohs(rd16(packet, pkt::Base(packet) + 8));
    uint8_t d = packet[pkt::Base(packet) + 0xa];
    uint8_t e = packet[pkt::Base(packet) + 0xb];
    uint8_t f = packet[pkt::Base(packet) + 0xc];

    void *mob = FindObject(serial);
    if (mob != nullptr && decomp::vcall(PORT_RET(int), mob, 0x24) != 0)             // slot 0x24: is mobile
        reinterpret_cast<CMobile *>(mob)->StartAction(a, b, f, c, d, e);                                    // CMobile::StartAction, RET 0x18
}
HOOK_FUNC(0x0045e100, Packet_Handle0x6E);

// ---------------------------------------------------------------------------
// 0x0045e250  size 0x36b
// Inbound packet 0x6F: secure trading. Action byte 0 = open (or, for a serial
// found by 0x0044c000, build a reply packet that the original never sends),
// 1 = close the trade gump, 2 = update the two accept check boxes.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x6F(uint8_t *packet)
{
    uint8_t out[0x30];
    const char *name = nullptr;      // the original leaves this uninitialised when there is no name

    uint8_t action = packet[pkt::Base(packet)];
    uint32_t id1 = ntohl(rd32(packet, pkt::Base(packet) + 1));
    uint32_t id2 = ntohl(rd32(packet, pkt::Base(packet) + 5));
    uint32_t id3 = ntohl(rd32(packet, pkt::Base(packet) + 9));
    if (packet[pkt::Base(packet) + 0xd] != 0)
        name = reinterpret_cast<const char *>(packet) + pkt::Base(packet) + 0xe;

    void *obj = FindObject(id1);
    switch (action) {
    case 0: {
        if (obj == nullptr || decomp::vcall(PORT_RET(int), obj, 0x24) == 0)           // slot 0x24: is mobile
            return;
        if (App_IsInList_0069e8b0(id1) != 0) {
            // Builds packet 0x6F {1, htonl(id2), 0, 0, 0}; no send follows in the original.
            out[0] = 0x6f;
            if (pkt::IsVariable(out))
                *reinterpret_cast<uint16_t *>(out + 1) = 3;
            else
                g_pktFixedCursor = 1;
            uint32_t cur = pkt::Cursor(out);
            out[cur] = 1;
            pkt::OrigSetCursor(out, cur + 1);
            uint32_t v = htonl(id2);
            cur = pkt::OrigGetCursor(out) & 0xffffu;
            pkt::Put32(out, cur, v);
            pkt::OrigSetCursor(out, cur + 4);
            v = htonl(0);
            cur = pkt::OrigGetCursor(out) & 0xffffu;
            pkt::Put32(out, cur, v);
            pkt::OrigSetCursor(out, cur + 4);
            v = htonl(0);
            cur = pkt::OrigGetCursor(out) & 0xffffu;
            pkt::Put32(out, cur, v);
            pkt::OrigSetCursor(out, cur + 4);
            pkt::OrigAppendU8(out, 0);
            return;
        }
        CTradeGump *gump = new CTradeGump((CObj *)(obj), id2, id3, name);   // CTradeGump ctor, RET 0x10
        GumpLink(gump);
        return;
    }
    case 1: {
        if (obj == nullptr || decomp::vcall(PORT_RET(int), obj, 0x30) == 0)           // slot 0x30: is container
            return;
        uint8_t *board = RAWF(uint8_t *, obj, 0xac);
        if (board == nullptr)
            return;
        void *trade = RAWF(void *, board, 0xa0);
        if (trade == nullptr)
            return;
        decomp::vdelete(trade);
        return;
    }
    case 2: {
        if (obj == nullptr || decomp::vcall(PORT_RET(int), obj, 0x30) == 0)           // slot 0x30: is container
            return;
        if (RAWF(uint8_t *, obj, 0xac) == nullptr)
            return;
        if (RAWF(uint8_t *, RAWF(uint8_t *, obj, 0xac), 0xa0) == nullptr)
            return;
        RAWF(CToggleControl *, RAWF(uint8_t *, RAWF(uint8_t *, obj, 0xac), 0xa0), 0xc0)->checked = (id2 != 0);
        RAWF(CToggleControl *, RAWF(uint8_t *, RAWF(uint8_t *, obj, 0xac), 0xa0), 0xc0)->Redraw();
        RAWF(CToggleControl *, RAWF(uint8_t *, RAWF(uint8_t *, obj, 0xac), 0xa0), 0xc4)->checked = (id3 != 0);
        RAWF(CToggleControl *, RAWF(uint8_t *, RAWF(uint8_t *, obj, 0xac), 0xa0), 0xc4)->Redraw();
        return;
    }
    default:
        return;
    }
}
HOOK_FUNC(0x0045e250, Packet_Handle0x6F);

// ---------------------------------------------------------------------------
// 0x0045e5c0  size 0x263
// Inbound packet 0x70: graphical effect. Resolves the two serials to mobiles
// (null when not found or not a mobile) and calls the effect-gump factory
// 0x00437190 with ECX = gump manager.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x70(uint8_t *packet)
{
    void *src = nullptr;
    void *dst = nullptr;
    Pos3 from;
    Pos3 to;
    uint8_t type = packet[pkt::Base(packet)];
    uint32_t srcSerial = ntohl(*reinterpret_cast<const uint32_t *>(packet + pkt::Base(packet) + 1));
    uint32_t dstSerial = ntohl(*reinterpret_cast<const uint32_t *>(packet + (pkt::OrigBase(packet) & 0xffff) + 5));
    uint32_t gfx = ntohs(*reinterpret_cast<const uint16_t *>(packet + (pkt::OrigBase(packet) & 0xffff) + 9));
    uint32_t sx = ntohs(*reinterpret_cast<const uint16_t *>(packet + (pkt::OrigBase(packet) & 0xffff) + 0xb));
    from.x = sx;
    uint32_t sy = ntohs(*reinterpret_cast<const uint16_t *>(packet + (pkt::OrigBase(packet) & 0xffff) + 0xd));
    from.y = sy;
    from.z = static_cast<int8_t>(packet[(pkt::OrigBase(packet) & 0xffff) + 0xf]);
    uint32_t dx = ntohs(*reinterpret_cast<const uint16_t *>(packet + (pkt::OrigBase(packet) & 0xffff) + 0x10));
    to.x = dx;
    uint32_t dy = ntohs(*reinterpret_cast<const uint16_t *>(packet + (pkt::OrigBase(packet) & 0xffff) + 0x12));
    to.y = dy;
    to.z = static_cast<int8_t>(packet[(pkt::OrigBase(packet) & 0xffff) + 0x14]);
    uint8_t b15 = packet[(pkt::OrigBase(packet) & 0xffff) + 0x15];
    uint8_t b16 = packet[(pkt::OrigBase(packet) & 0xffff) + 0x16];
    uint8_t b17 = packet[(pkt::OrigBase(packet) & 0xffff) + 0x17];
    uint8_t b18 = packet[(pkt::OrigBase(packet) & 0xffff) + 0x18];
    uint8_t b19 = packet[(pkt::OrigBase(packet) & 0xffff) + 0x19];
    uint8_t b1a = packet[(pkt::OrigBase(packet) & 0xffff) + 0x1a];

    src = (&g_objMgr)->CObjectManager::FindBySerial(srcSerial);
    if (src == nullptr || decomp::vcall(PORT_RET(int), src, 0x24) == 0)               // slot 0x24: is mobile
        src = nullptr;
    dst = (&g_objMgr)->CObjectManager::FindBySerial(dstSerial);
    if (dst == nullptr || decomp::vcall(PORT_RET(int), dst, 0x24) == 0)
        dst = nullptr;

    PORT_THISCALL12(&CreateEffectGump, void, g_gumpManager, uint8_t, void *, void *, uint32_t, Pos3, Pos3, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t)(type, src, dst, gfx, from, to, b15, b16, b17, b18, b19, b1a);          // CreateEffectGump, RET 0x38
}
HOOK_FUNC(0x0045e5c0, Packet_Handle0x70);

// ---------------------------------------------------------------------------
// 0x0045e830  size 0x63c
// Inbound packet 0x71: bulletin board. Sub-command 0 opens the board gump,
// 1 adds a summary line, 2 opens a message viewer.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x71(uint8_t *packet)
{
    // Locals of the original: three 0x50-0x60-byte string buffers and up to 255 {u16, u16} pairs.
    char author[0x50];
    char subject[0x50];
    char timeText[0x60];
    uint16_t equip[0x200];
    const int base = pkt::Base(packet);
    uint8_t sub = packet[base];

    if (sub == 0) {
        uint32_t serial = ntohl(rd32(packet, pkt::Base(packet) + 1));
        const char *title = reinterpret_cast<const char *>(packet) + pkt::Base(packet) + 5;
        void *board = FindObject(serial);
        if (board == nullptr)
            return;
        CGump *found = nullptr;
        for (CGump *g = g_gumpListTail; g != nullptr; g = g->prev) {   // +0x3c
            if (decomp::vcall(PORT_RET(int), g, 0xdc) != 0 && g->owner == board) {    // slot 0xdc: is bulletin board gump
                g->BringToFront(0);
                found = g;
            }
        }
        if (found != nullptr)
            return;
        CBBGump *gump = new CBBGump(static_cast<CObj *>(board), title);
        gump->LinkInto(nullptr, 1);
        return;
    }
    if (sub == 1) {
        uint32_t boardSerial = ntohl(rd32(packet, pkt::Base(packet) + 1));
        uint32_t parentSerial = ntohl(rd32(packet, pkt::Base(packet) + 5));
        uint32_t replySerial = ntohl(rd32(packet, pkt::Base(packet) + 9));
        uint8_t len = packet[pkt::Base(packet) + 0xd];
        uint32_t off = static_cast<uint32_t>(len) + 0xe;
        strcpy(author, reinterpret_cast<const char *>(packet) + pkt::Base(packet) + 0xe);
        len = packet[pkt::Base(packet) + off];
        off++;
        const char *src = reinterpret_cast<const char *>(packet) + pkt::Base(packet) + off;
        off += len;
        strcpy(subject, src);
        off++;                                                             // skips a length byte that is not read
        strcpy(timeText, reinterpret_cast<const char *>(packet) + pkt::Base(packet) + off);

        void *post = FindObject(boardSerial);
        void *parent = FindObject(parentSerial);
        if (post == nullptr || decomp::vcall(PORT_RET(int), post, 0x30) == 0 || parent == nullptr)   // slot 0x30: is container
            return;
        void *reply = FindObject(replySerial);
        CBBGump *bb = RAWF(CBBGump *, post, 0xac);
        if (bb == nullptr)
            return;
        bb->AddLine(static_cast<CObj *>(parent), static_cast<CObj *>(reply), author, subject, timeText);
        return;
    }
    if (sub != 2)
        return;

    ntohl(rd32(packet, pkt::Base(packet) + 1));                        // board serial, discarded
    uint32_t postSerial = ntohl(rd32(packet, pkt::Base(packet) + 5));
    uint8_t len = packet[(pkt::OrigBase(packet) & 0xffff) + 9];
    uint32_t off = static_cast<uint32_t>(len) + 0xa;
    strcpy(author, reinterpret_cast<const char *>(packet) + (pkt::OrigBase(packet) & 0xffff) + 0xa);
    len = packet[(pkt::OrigBase(packet) & 0xffff) + off];
    off++;
    {
        const char *src = reinterpret_cast<const char *>(packet) + (pkt::OrigBase(packet) & 0xffff) + off;
        off += len;
        strcpy(subject, src);
    }
    len = packet[(pkt::OrigBase(packet) & 0xffff) + off];
    off++;
    {
        const char *src = reinterpret_cast<const char *>(packet) + (pkt::OrigBase(packet) & 0xffff) + off;
        off += len;
        strcpy(timeText, src);
    }
    uint32_t bodyId = ntohs(rd16(packet, (pkt::OrigBase(packet) & 0xffff) + off));
    off += 2;
    uint32_t hue = ntohs(rd16(packet, (pkt::OrigBase(packet) & 0xffff) + off));
    off += 2;
    uint8_t nEquip = packet[(pkt::OrigBase(packet) & 0xffff) + off];
    off++;
    for (int i = 0; i < static_cast<int>(nEquip); i++) {
        equip[i * 2] = ntohs(rd16(packet, (pkt::OrigBase(packet) & 0xffff) + off));
        off += 2;
        equip[i * 2 + 1] = ntohs(rd16(packet, (pkt::OrigBase(packet) & 0xffff) + off));
        off += 2;
    }
    uint8_t nLines = packet[(pkt::OrigBase(packet) & 0xffff) + off];
    off++;

    void *post = FindObject(postSerial);
    if (post == nullptr)
        return;
    CBBMsgGump *gump = new CBBMsgGump(static_cast<CObj *>(post), static_cast<uint32_t>(g_bbRequestSerial),
                                      author, subject, timeText, static_cast<uint16_t>(bodyId),
                                      static_cast<uint16_t>(hue), nEquip, equip, 0);
    g_bbRequestPending = 0;
    int lineCount = nLines;
    for (int i = 0; i < lineCount; i++) {
        uint8_t l = packet[(pkt::OrigBase(packet) & 0xffff) + off];
        off++;
        const char *text = reinterpret_cast<const char *>(packet) + (pkt::OrigBase(packet) & 0xffff) + off;
        off += l;
        if (i < 0x50)
            gump->AddLine(text);
    }
    gump->RedrawText();
    gump->LinkInto(nullptr, 1);
}
HOOK_FUNC(0x0045e830, Packet_Handle0x71);

// ---------------------------------------------------------------------------
// 0x0045ee70  size 0xb7
// Inbound packet 0x72: war mode. Four bytes; stores three in globals and
// passes the first to 0x0044de60 (ECX).
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x72(uint8_t *packet)
{
    uint8_t a = packet[pkt::Base(packet)];
    uint8_t b = packet[pkt::Base(packet) + 1];
    uint8_t c = packet[pkt::Base(packet) + 2];
    uint8_t d = packet[pkt::Base(packet) + 3];
    g_warModeC = c;
    g_warModeB = b;
    g_warModeD = d;
    ApplyWarMode(a);
}
HOOK_FUNC(0x0045ee70, Packet_Handle0x72);

// ---------------------------------------------------------------------------
// 0x0045ef30  size 0x14f
// Inbound packet 0x7B: sequencing. Byte 0 starts building an effect sequence;
// non-zero ends it (stores the repeat count and runs it).
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x7B(uint8_t *packet)
{
    uint8_t flag = packet[pkt::Base(packet)];
    if (flag == 0) {
        if (g_sequencing != 0) {
            ShowAlert("already sequencing!");
            return;
        }
        g_sequencing = 1;
        g_effectSequence = new CEffectSequence;
        return;
    }
    if (g_sequencing == 0) {
        ShowAlert("not sequencing!");
        return;
    }
    g_sequencing = 0;
    g_effectSequence->repeat = flag;
    g_effectSequence->current = g_effectSequence->head;
    SeqNode *node = g_effectSequence->current;
    if (node != nullptr) {
        g_effectSequence->Step(node);
        return;
    }
    if (g_effectSequence != nullptr)
        decomp::vdelete(g_effectSequence);
}
HOOK_FUNC(0x0045ef30, Packet_Handle0x7B);
