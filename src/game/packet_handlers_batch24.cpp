// Inbound packet handlers (second pass, batch 24): login flow replies and
// gump / dialog openers.
//   0x82 login denied               0x00461290    0x88 open paperdoll      0x00461400
//   0x8B two string dialog (sign)   0x004615a0    0x85 char delete result  0x00461b20
//   0x86 character list update      0x00461c10    0x98 all names           0x00461d70
//   0x9A console prompt             0x00461e00    0xA5 open URL            0x00461f30
//   0xAA attack character           0x00462270    0xAB text entry dialog   0x004622f0
//   0xAF death animation            0x004624c0    0xB0 gump dialog         0x00462580
//
// All are __stdcall (RET 4) with the packet pointer. The first field of a packet is at
// "base" (3 for a variable-length opcode, 1 otherwise), see packet_cursor.h. The SEH frames
// the original wraps around operator new + constructor calls are omitted.
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "book_gump.h"
#include "generic_gump.h"
#include "globals.h"
#include "gump.h"
#include "gump_manager.h"
#include "object_manager.h"
#include "obj.h"
#include "crt.h"
#include "packet_cursor.h"
#include "paperdoll_gump.h"
#include "raw_field.h"
#include "sign_gump.h"
#include "string_query_gump.h"
#include "vcall.h"
#include "world_obj_ops.h"
#include "gump_art_ops_ports.h"
#include "login_gumps.h"
#include "input_control_gump.h"
#include "client_misc_ops_ports.h"
#include "window_helpers_31_ports.h"

#define g24_inputCtl GLOBAL_AS(uint8_t *, g_awaitingTargetControl)          // input control waiting for a target
#define g24_attackTarget GLOBAL_AS(uint32_t, g_attackSerial)
#define g24_consolePromptA GLOBAL_AS(uint32_t, g_promptSerialA)
#define g24_consolePromptB GLOBAL_AS(uint32_t, g_promptSerialB)

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

static void GumpLink(void *gump) { reinterpret_cast<CGump *>(gump)->LinkInto(nullptr, 1); }   // CGump::LinkInto, runs on null too

// 0x00439f60 with ECX = the gump manager (the port ignores ECX).
static inline void *CreateMessageGump(const char *text, CGump *parent, int a, int b)
{
    return PORT_THISCALL4(&CreateGump2ac, void *, &g_gumpMgr, const char *, CGump *, int, int)(text, parent, a, b);
}

// Delete the waiting gump if there is one.
static inline void DeleteWaitingGump()
{
    if (g_waitingGump != nullptr) {
        decomp::vdelete(g_waitingGump);
        g_waitingGump = nullptr;
    }
}

// ---------------------------------------------------------------------------
// 0x00461290  size 0x15c
// Inbound packet 0x82: login denied. Picks the message from the reason byte
// (0..3, anything else is the "problem communicating with Origin" text and
// passes 1 as the last argument), drops the waiting gump and shows the message.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x82(uint8_t *packet)
{
    char text[0x100];
    int fatal = 0;

    switch (packet[pkt::Base(packet)]) {
    case 0:
        strcpy(text, g_004adaa4);
        break;
    case 1:
        strcpy(text, g_004ada7c);   // "Someone is already using this account."
        break;
    case 2:
        strcpy(text, g_004ada00);
        break;
    case 3:
        strcpy(text, g_004ad9a8);   // "Your account credentials are invalid. ..."
        break;
    default:
        strcpy(text, g_004ad948);   // "There is some problem communicating with Origin. ..."
        fatal = 1;
        break;
    }

    DeleteWaitingGump();
    PORT_THISCALL4(&CreateGump2ac, void *, &g_gumpMgr, const char *, CGump *, int, int)(text, g_dlgParent, 1, fatal);   // 0x00439f60
}
HOOK_FUNC(0x00461290, Packet_Handle0x82);

// ---------------------------------------------------------------------------
// 0x00461400  size 0x198
// Inbound packet 0x88: open a paperdoll for a mobile. {u32 serial, char name[60], u8 warFlag}.
// An existing paperdoll of that object is brought to the front instead.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x88(uint8_t *packet)
{
    int artW;
    int artH;

    uint32_t serial = ntohl(rd32(packet, pkt::Base(packet)));
    char *name = reinterpret_cast<char *>(packet) + pkt::Base(packet) + 4;
    uint8_t warFlag = packet[pkt::Base(packet) + 0x40];

    CObj *obj = reinterpret_cast<CObj *>(g_objMgr.FindBySerial(serial));
    if (obj == nullptr)
        return;
    if (decomp::vcall(PORT_RET(int), obj, 0x24) == 0)
        return;

    CGump *found = g_gumpMgr.FindGumpForObject(obj, 1, 0, g_gumpListTail);
    if (found == nullptr) {
        GumpArtSizeImpl((obj != g_player) + 0xa, &artW, &artH);   // GetGumpArtSize
        CPaperdollGump *gump = nullptr;
        if (name != nullptr) {
            gump = new CPaperdollGump(0x280 - artW, 0, 0, obj, 1, artW, artH, name, warFlag);
        } else {
            gump = new CPaperdollGump(0x280 - artW, 0, 0, obj, 1, artW, artH, g_emptyString, warFlag);
        }
        GumpLink(gump);
    } else {
        if (found->unk_60 != 0)
            static_cast<CBookGump *>(found)->Uniconify();
        found->BringToFront(0);
    }
}
HOOK_FUNC(0x00461400, Packet_Handle0x88);

// ---------------------------------------------------------------------------
// 0x004615a0  size 0x1b7
// Inbound packet 0x8B: sign / plaque. {u32 serial, u16 art, u16 len1, char text1[len1],
// u16 len2 (not used), char text2[]}. Opens a CSignGump unless the object already has one.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x8B(uint8_t *packet)
{
    uint32_t serial = ntohl(rd32(packet, pkt::Base(packet)));
    uint32_t art = ntohs(rd16(packet, pkt::Base(packet) + 4));
    uint32_t len1 = ntohs(rd16(packet, pkt::Base(packet) + 6));
    uint32_t off = (len1 & 0xffff) + 8;
    const uint8_t *text1 = packet + pkt::Base(packet) + 8;
    ntohs(rd16(packet, pkt::Base(packet) + off));   // second length: read, not used
    off += 2;
    char *text2 = reinterpret_cast<char *>(packet) + pkt::Base(packet) + off;

    CObj *obj = nullptr;
    if (serial != 0)
        obj = reinterpret_cast<CObj *>(g_objMgr.FindBySerial(serial));

    CGump *g = nullptr;
    if (obj != nullptr) {
        g = g_gumpListTail;
        while (g != nullptr) {
            if (decomp::vcall(PORT_RET(int), g, 0xf8) != 0 && g->owner == obj)
                break;
            g = g->prev;
        }
    }
    if (g != nullptr)
        return;

    CSignGump *gump = new CSignGump(0x64, 0x64, static_cast<int>(art & 0xffff),
                                    static_cast<int>(reinterpret_cast<intptr_t>(text1)), text2, obj);
    GumpLink(gump);
}
HOOK_FUNC(0x004615a0, Packet_Handle0x8B);

// ---------------------------------------------------------------------------
// 0x00461b20  size 0xd7
// Inbound packet 0x85: character delete refused. Reason byte 0..4 selects the text.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x85(uint8_t *packet)
{
    char text[0x100];

    if (g_prevGump == nullptr)
        return;

    switch (packet[pkt::Base(packet)]) {
    case 0:
        sprintf(text, g_004adcd8);   // "That character password is invalid."
        break;
    case 1:
        sprintf(text, g_004adcb8);   // "That character does not exist."
        break;
    case 2:
        sprintf(text, g_004adc8c);   // "That character is being played right now."
        break;
    case 3:
        sprintf(text, g_004adc24);   // "That character is not old enough to delete. ..."
        break;
    case 4:
        sprintf(text, g_004adbdc);   // "... queued for backup and cannot be deleted."
        break;
    default:
        sprintf(text, g_004adbb8);   // "Couldn't carry out your request."
        break;
    }

    DeleteWaitingGump();
    CreateMessageGump(text, g_prevGump, 1, 0);
}
HOOK_FUNC(0x00461b20, Packet_Handle0x85);

// ---------------------------------------------------------------------------
// 0x00461c10  size 0x160
// Inbound packet 0x86: character list update. Clears the five slots, reads the
// count, then five records of {char name[0x1e], char password[0x1e]}; each
// non-empty record is copied to the slot tables and added to the list gump.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x86(uint8_t *packet)
{
    if (g_prevGump == nullptr)
        return;

    int i = 0;
    char *slot = g_slotNeedsPw;
    do {
        reinterpret_cast<CLoginGump *>(g_prevGump)->RemoveEntry(i);   // CLoginGump::RemoveEntry
        slot[-0x96] = 0;
        slot[0] = 0;
        slot += 0x1e;
        i++;
    } while (slot < g_slotNeedsPw + 0x96);

    uint32_t rec = 1;
    g_charSlotCount = packet[pkt::Base(packet)];
    i = 0;
    slot = g_slotNeedsPw;
    do {
        char *name = reinterpret_cast<char *>(packet) + pkt::Base(packet) + rec;
        rec += 0x3c;
        if (strlen(name) != 0) {
            strcpy(slot - 0x96, name);
            strcpy(slot, name + 0x1e);
            reinterpret_cast<CLoginGump *>(g_prevGump)->AddEntry(name, (int)(name + 0x1e), i);   // AddEntry
        }
        slot += 0x1e;
        i++;
    } while (slot < g_slotNeedsPw + 0x96);

    DeleteWaitingGump();
    g_prevGump->visible = 1;
}
HOOK_FUNC(0x00461c10, Packet_Handle0x86);

// ---------------------------------------------------------------------------
// 0x00461d70  size 0x90
// Inbound packet 0x98: all names. Copies the name into the input control that
// asked for a target (+0xec), redraws it and clears the pending request.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x98(uint8_t *packet)
{
    ntohl(rd32(packet, pkt::Base(packet)));   // serial: read, not used
    const char *name = reinterpret_cast<const char *>(packet) + pkt::Base(packet) + 4;
    strcpy(reinterpret_cast<char *>(g24_inputCtl + 0xec), name);
    reinterpret_cast<CInputControlGump *>(g24_inputCtl)->Redraw();   // CInputControlGump::Redraw
    g24_inputCtl = nullptr;
}
HOOK_FUNC(0x00461d70, Packet_Handle0x98);

// ---------------------------------------------------------------------------
// 0x00461e00  size 0x12e
// Inbound packet 0x9A: console prompt. Stores the two serials and sets the
// prompt flag; the text is copied to a heap block and freed again.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x9A(uint8_t *packet)
{
    uint32_t a = ntohl(rd32(packet, pkt::Base(packet)));
    uint32_t b = ntohl(rd32(packet, pkt::Base(packet) + 4));
    ntohl(rd32(packet, pkt::Base(packet) + 8));   // third dword: read, not used

    uint32_t base = pkt::IsVariable(packet) ? 3u : 1u;
    uint32_t len = pkt::Cursor(packet) - 0xc - base;
    char *text = static_cast<char *>(::operator new(len + 1));
    strncpy(text, reinterpret_cast<const char *>(packet) + base + 0xc, len);   // 0x0048b750
    text[len] = 0;
    g_promptReply = 1;
    g24_consolePromptA = a;
    g24_consolePromptB = b;
    operator delete(text);
}
HOOK_FUNC(0x00461e00, Packet_Handle0x9A);

// ---------------------------------------------------------------------------
// 0x00461f30  size 0x9b
// Inbound packet 0xA5: open a URL. The text after the header is copied out and
// handed to LaunchWebBrowser (0x004830a0, ECX = URL); 0x00478ce0 first leaves
// full-screen mode.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0xA5(uint8_t *packet)
{
    uint32_t base = pkt::IsVariable(packet) ? 3u : 1u;
    uint32_t len = pkt::Cursor(packet) - base;
    char *url = static_cast<char *>(::operator new(len + 1));
    strncpy(url, reinterpret_cast<const char *>(packet) + base, len);   // 0x0048b750
    url[len] = 0;
    Misc_ToggleIfFullscreen();   // leave full-screen mode if set
    PORT_THISCALL0(&LaunchWebBrowser, void, url)();     // LaunchWebBrowser
    operator delete(url);
}
HOOK_FUNC(0x00461f30, Packet_Handle0xA5);

// ---------------------------------------------------------------------------
// 0x00462270  size 0x75
// Inbound packet 0xAA: attack target. Stores the new serial and invalidates
// the previous and the new target so they redraw.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0xAA(uint8_t *packet)
{
    uint32_t old = g24_attackTarget;
    g24_attackTarget = ntohl(rd32(packet, pkt::Base(packet)));
    if (old != 0) {
        CWorldObj *o = reinterpret_cast<CWorldObj *>(g_objMgr.FindBySerial(old));
        if (o != nullptr)
            o->Invalidate();
    }
    if (g24_attackTarget != 0) {
        CWorldObj *o = reinterpret_cast<CWorldObj *>(g_objMgr.FindBySerial(g24_attackTarget));
        if (o != nullptr)
            o->Invalidate();
    }
}
HOOK_FUNC(0x00462270, Packet_Handle0xAA);

// ---------------------------------------------------------------------------
// 0x004622f0  size 0x1cf
// Inbound packet 0xAB: text entry dialog. {u32 serial, u16 type, u16 len1, char title[len1],
// u8 flags, u8 kind, u32 arg, u16 len2 (not used), char text[]}.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0xAB(uint8_t *packet)
{
    uint32_t serial = ntohl(rd32(packet, pkt::Base(packet)));
    uint32_t type = ntohs(rd16(packet, pkt::Base(packet) + 4));
    uint32_t len1 = ntohs(rd16(packet, pkt::Base(packet) + 6));
    uint32_t off = (len1 & 0xffff) + 8;
    char *title = reinterpret_cast<char *>(packet) + pkt::Base(packet) + 8;
    uint8_t flags = packet[pkt::Base(packet) + off];
    off++;
    uint8_t kind = packet[pkt::Base(packet) + off];
    off++;
    uint32_t arg = ntohl(rd32(packet, pkt::Base(packet) + off));
    off += 4;
    ntohs(rd16(packet, pkt::Base(packet) + off));   // second length: read, not used
    off += 2;
    char *text = reinterpret_cast<char *>(packet) + pkt::Base(packet) + off;

    CStringQueryGump *gump = new CStringQueryGump(serial, static_cast<uint16_t>(type), title, flags, kind, arg, text);
    GumpLink(gump);
}
HOOK_FUNC(0x004622f0, Packet_Handle0xAB);

// ---------------------------------------------------------------------------
// 0x004624c0  size 0xb7
// Inbound packet 0xAF: death animation. {u32 victim, u32 corpse, u32 flag}. A
// mobile victim starts its status action (alt set when the flag word is zero).
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0xAF(uint8_t *packet)
{
    uint32_t victim = ntohl(rd32(packet, pkt::Base(packet)));
    uint32_t corpse = ntohl(rd32(packet, pkt::Base(packet) + 4));
    uint32_t flag = ntohl(rd32(packet, pkt::Base(packet) + 8));

    CObj *obj = reinterpret_cast<CObj *>(g_objMgr.FindBySerial(victim));
    if (obj != nullptr && decomp::vcall(PORT_RET(int), obj, 0x24) != 0)
        reinterpret_cast<CMobile *>(obj)->StartStatusAction(static_cast<int>(corpse), flag == 0);
}
HOOK_FUNC(0x004624c0, Packet_Handle0xAF);

// ---------------------------------------------------------------------------
// 0x00462580  size 0x2bf
// Inbound packet 0xB0: generic gump dialog. {u32 serial, u32 gumpId, u32 x, u32 y,
// u16 layoutLen, char layout[], u16 lineCount, then lineCount x {u16 len, u16 text[len]}}.
// The line table is built on the heap, handed to CGenericGump and freed again.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0xB0(uint8_t *packet)
{
    uint16_t **lines = nullptr;

    uint32_t serial = ntohl(rd32(packet, pkt::Base(packet)));
    uint32_t gumpId = ntohl(rd32(packet, pkt::Base(packet) + 4));
    uint32_t x = ntohl(rd32(packet, pkt::Base(packet) + 8));
    uint32_t y = ntohl(rd32(packet, pkt::Base(packet) + 0xc));
    uint32_t layoutLen = ntohs(rd16(packet, pkt::Base(packet) + 0x10));
    uint32_t off = (layoutLen & 0xffff) + 0x12;
    char *layout = reinterpret_cast<char *>(packet) + pkt::Base(packet) + 0x12;

    uint16_t lineCount = static_cast<uint16_t>(ntohs(rd16(packet, pkt::Base(packet) + off)));
    off += 2;
    if (lineCount != 0) {
        int n = lineCount;
        lines = static_cast<uint16_t **>(::operator new(static_cast<size_t>(n) * 4));
        if (n > 0) {
            uint16_t **slot = lines;
            int remaining = n;
            do {
                uint32_t len = ntohs(rd16(packet, pkt::Base(packet) + off)) & 0xffff;
                off += 2;
                uint16_t *str = static_cast<uint16_t *>(::operator new(len * 2 + 2));
                *slot = str;
                str[len] = 0;
                for (int j = 0; j < static_cast<int>(len); j++) {
                    str[j] = rd16(packet, pkt::Base(packet) + off);
                    str[j] = static_cast<uint16_t>(ntohs(str[j]));
                    off += 2;
                }
                slot++;
                remaining--;
            } while (remaining != 0);
        }
    }

    new CGenericGump(static_cast<int>(x), static_cast<int>(y), layout, reinterpret_cast<char **>(lines),
                     static_cast<int>(serial), static_cast<int>(gumpId));

    if (lineCount != 0) {
        int n = lineCount;
        if (n > 0) {
            uint16_t **slot = lines;
            do {
                operator delete(*slot);
                slot++;
                n--;
            } while (n != 0);
        }
        operator delete(lines);
    }
}
HOOK_FUNC(0x00462580, Packet_Handle0xB0);
