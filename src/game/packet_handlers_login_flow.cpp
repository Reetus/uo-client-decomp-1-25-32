// Inbound packet handlers (second pass, batch 23): menus, dye window and the
// login flow server / character lists.
//   0x7C menu                          0x0045f080
//   0x95 dye window                    0x0045f410
//   server list (0xA8)                 0x004603e0
//   character / start location list   0x00460b80
//
// All are __stdcall (RET 4) with the packet pointer. The first field of a packet
// is at "base" (3 for a variable-length opcode, 1 otherwise), see packet_cursor.h.
// The stack probe and the SEH frames the original wraps around operator new +
// constructor calls are omitted.
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"
#include "gump.h"
#include "hue_gump.h"
#include "login_gumps.h"
#include "obj_picker_gump.h"
#include "crt.h"
#include "packet_cursor.h"
#include "raw_field.h"
#include "scroll_choice_gump.h"
#include "sys_info.h"
#include "vcall.h"
#include "app_support_ports.h"
#include "string_query_gump.h"
#include "packet_builders_ports.h"
#include "config_file_ports.h"
#include "waiting_gump.h"
#include "client_socket.h"
#include "main_window_ports.h"

#define g_serverIds GLOBAL_AS(uint8_t *, g_serverValues)          // 2 bytes per server
#define g_serverIp GLOBAL_AS(uint8_t *, g_pingServerIps)           // 4 bytes per server

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

static void SetTitleSuffix(char *text) { PORT_THISCALL0(&SetMainWindowTitle, void, text)(); }          // ECX = text
static void GumpLink(void *gump) { reinterpret_cast<CGump *>(gump)->LinkInto(nullptr, 1); }   // CGump::LinkInto, runs on null too
static void LoginGump_AddEntry(void *gump, const char *city, const char *area, int index)
{
    reinterpret_cast<CLoginGump *>(gump)->AddEntry(city, (int)(area), index);
}
static int RandomMod(int n) { return App_RandomMod(n); }

// ---------------------------------------------------------------------------
// 0x0045f080  size 0x390
// Inbound packet 0x7C: menu. Reads the title, then `count` entries of
// {u16 id, u16 hue, text}. A first id of 0 opens a text-only string picker
// gump (also stored in 0x006488d4), anything else the object picker.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x7C(uint8_t *packet)
{
    char *texts[0x100];

    uint32_t serial = ntohl(rd32(packet, pkt::Base(packet)));
    uint32_t menuId = ntohs(rd16(packet, pkt::Base(packet) + 4));
    uint32_t titleLen = packet[pkt::Base(packet) + 6];
    uint32_t off = titleLen + 7;
    char *title = static_cast<char *>(::operator new(titleLen + 1));
    memcpy(title, packet + pkt::Base(packet) + 7, titleLen);
    title[titleLen] = 0;

    uint32_t count = packet[pkt::Base(packet) + off];
    off++;
    uint16_t *ids = static_cast<uint16_t *>(::operator new(count * 2));
    uint16_t *hues = static_cast<uint16_t *>(::operator new(count * 2));
    int n = static_cast<int>(count);
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            ids[i] = ntohs(rd16(packet, pkt::Base(packet) + off));
            off += 2;
            hues[i] = ntohs(rd16(packet, pkt::Base(packet) + off));
            off += 2;
            uint32_t len = packet[pkt::Base(packet) + off];
            off++;
            if (len != 0) {
                texts[i] = static_cast<char *>(::operator new(len + 1));
                const uint8_t *src = packet + pkt::Base(packet) + off;
                off += len;
                memcpy(texts[i], src, len);
                texts[i][len] = 0;
            } else {
                texts[i] = nullptr;
            }
        }
    }

    if (ids[0] == 0) {
        CStringPickerGump *gump = new CStringPickerGump(title, serial, menuId & 0xffff, n, ids, texts);       // CStringPickerGump ctor, RET 0x18
        GumpLink(gump);
        g_scrollChoiceGump = static_cast<CGump *>(gump);
    } else {
        CObjPickerGump *gump = new CObjPickerGump(0, 0, title, serial, static_cast<uint16_t>(menuId), n, ids, hues, texts);
        GumpLink(gump);
    }

    operator delete(title);
    operator delete(ids);
    operator delete(hues);
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            if (texts[i] != nullptr)
                operator delete(texts[i]);
        }
    }
}
HOOK_FUNC(0x0045f080, Packet_Handle0x7C);

// ---------------------------------------------------------------------------
// 0x0045f410  size 0xe5
// Inbound packet 0x95: dye window (CHueGump centered).
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x95(uint8_t *packet)
{
    uint32_t serial = ntohl(rd32(packet, pkt::Base(packet)));
    uint32_t a = ntohs(rd16(packet, pkt::Base(packet) + 4));
    uint32_t b = ntohs(rd16(packet, pkt::Base(packet) + 6));
    CHueGump *gump = new CHueGump(static_cast<int>(serial), static_cast<uint16_t>(a), static_cast<uint16_t>(b));
    GumpLink(gump);
}
HOOK_FUNC(0x0045f410, Packet_Handle0x95);

// ---------------------------------------------------------------------------
// 0x004603e0  size 0x79b
// Server list reply: sends the system information record when it differs from
// the cached one, sets the window title, stores the server table (id, load,
// time zone, ip, name), sorts the servers by time zone distance and opens the
// server choice gump. A count of 0 shows the "servers are down" message.
// ---------------------------------------------------------------------------
void __stdcall Packet_HandleServerList(uint8_t *packet)
{
    char text[0x400];
    CSysInfo cached;
    uint8_t sysRec[0xa0];

    Packet_Build0xA4(sysRec);                      // Packet_Build0xA4
    if (cached.ReadRegistry() == 0 || cached.Equals(reinterpret_cast<const CSysInfo *>(sysRec)) == 0) {
        pkt::Send(sysRec);
        reinterpret_cast<CSysInfo *>(sysRec)->WriteRegistry();
    }

    strcpy(text, g_acctName);
    strcat(text, g_titlePart1);
    strcat(text, g_titlePart2);
    if (g_inGame7211ac != 0) {
        strcat(text, g_titlePartA);
        strcat(text, g_titlePartB);
        strcat(text, g_titlePartC);
    }
    SetTitleSuffix(text);

    uint8_t flags = packet[pkt::Base(packet)];
    sprintf(text, g_fmtNumber, static_cast<uint32_t>(flags));
    g_charFlags = flags;
    WriteConfigSetting(0x1d, text);    // WriteConfigSetting

    uint8_t al = static_cast<uint8_t>(ntohs(rd16(packet, pkt::Base(packet) + 1)));
    uint32_t off = 3;
    if (al == 0) {
        if (g_waitingGump != nullptr) {
            decomp::vdelete(g_waitingGump);
            g_waitingGump = nullptr;
        }
        if (g_dlgParent == nullptr)
            return;
        RAWF(int32_t, g_dlgParent, 0x68) = 0;
        void *gump = new CGump_0043de30(-1, -1, g_strNoServersMsg, (int)(g_dlgParent), 0, 1, 0);          // CWaitingGump ctor
        g_waitingGump = static_cast<CGump *>(gump);
        GumpLink(gump);
        g_flag6f493c = 0;
        reinterpret_cast<CSocketBase *>(g_pktSender)->Close();                            // CSocketBase::Close
        return;
    }

    int count = static_cast<signed char>(al);
    g_serverCount = count;
    if (g_serverNames != nullptr)
        operator delete(g_serverNames);
    uint32_t n1 = static_cast<uint32_t>(count) + 1;
    uint32_t n4 = n1 * 4;
    uint32_t n2 = n1 * 2;
    g_serverNames = static_cast<char **>(::operator new(n4));
    memset(g_serverNames, 0, n4);
    if (g_serverIds != nullptr)
        operator delete(g_serverIds);
    g_serverIds = static_cast<uint8_t *>(::operator new(n2));
    if (g_serverKeyA != nullptr)
        operator delete(g_serverKeyA);
    g_serverKeyA = static_cast<uint8_t *>(::operator new(n1));
    if (g_serverFlags != nullptr)
        operator delete(g_serverFlags);
    g_serverFlags = static_cast<uint8_t *>(::operator new(n1));
    if (g_serverSortKeys != nullptr)
        operator delete(g_serverSortKeys);
    g_serverSortKeys = static_cast<uint8_t *>(::operator new(n1));
    if (GLOBAL_AS(uint8_t *, g_serverPing) != nullptr)
        operator delete(GLOBAL_AS(uint8_t *, g_serverPing));
    GLOBAL_AS(uint8_t *, g_serverPing) = static_cast<uint8_t *>(::operator new(n2));
    if (g_serverIp != nullptr)
        operator delete(g_serverIp);
    g_serverIp = static_cast<uint8_t *>(::operator new(n4));

    for (int i = 0; i < count; i++) {
        uint32_t id = ntohs(rd16(packet, pkt::Base(packet) + off));
        off += 2;
        char *name = reinterpret_cast<char *>(packet) + pkt::Base(packet) + off;
        off += 0x20;
        uint8_t load = packet[pkt::Base(packet) + off];
        off++;
        uint8_t zone = packet[pkt::Base(packet) + off];
        off++;
        uint32_t ip = ntohl(rd32(packet, pkt::Base(packet) + off));
        off += 4;
        if (strlen(name) != 0) {
            reinterpret_cast<uint16_t *>(g_serverIds)[i] = static_cast<uint16_t>(id);
            g_serverKeyA[i] = load;
            g_serverFlags[i] = zone;
            g_serverSortKeys[i] = 0;
            reinterpret_cast<uint16_t *>(GLOBAL_AS(uint8_t *, g_serverPing))[i] = 0;
            reinterpret_cast<uint32_t *>(g_serverIp)[i] = ip;
            name[0xf] = 0;
            g_serverNames[i] = static_cast<char *>(::operator new(strlen(name) + 1));
            strcpy(g_serverNames[i], name);
        }
    }

    if (g_waitingGump != nullptr) {
        decomp::vdelete(g_waitingGump);
        g_waitingGump = nullptr;
    }
    if (g_prevGump != nullptr) {
        decomp::vdelete(g_prevGump);
        g_prevGump = nullptr;
    }
    if (g_dlgParent != nullptr)
        RAWF(int32_t, g_dlgParent, 0x68) = 0;

    uint8_t *order = static_cast<uint8_t *>(::operator new(static_cast<size_t>(g_serverCount)));
    for (int i = 0; i < g_serverCount; i++)
        order[i] = static_cast<uint8_t>(i);

    for (int i = 0; i < g_serverCount; i++) {
        int tz = g_serverFlags[i];
        int d = tz - g_localTimezone;
        d = (d ^ (d >> 31)) - (d >> 31);
        d &= 0xff;
        int wrap = tz + 0xc;
        int other = 0xc - tz;
        if (other < wrap)
            wrap = g_localTimezone + other + 0xc;
        else
            wrap += 0xc - g_localTimezone;
        if (d < wrap)
            g_serverSortKeys[i] = static_cast<uint8_t>(d);
        else
            g_serverSortKeys[i] = static_cast<uint8_t>(wrap);
    }

    for (int a = 0; a < g_serverCount - 1; a++) {
        int best = -1;
        for (int b = a; b < g_serverCount; b++) {
            if (g_serverFlags[order[b]] != 0x32) {
                if (best == -1 || g_serverSortKeys[order[b]] < g_serverSortKeys[order[best]])
                    best = b;
            }
        }
        if (best != a && best != -1) {
            uint8_t t = order[best];
            uint8_t u = order[a];
            order[a] = t;
            order[best] = u;
        }
    }

    CScrollChoiceGump *gump = new CScrollChoiceGump(g_pMainMenu, 0x4cc);   // 0x0041cd50, size 0x22cc
    for (int i = 0; i < g_serverCount; i++) {
        uint32_t idx = order[i];
        char *name = g_serverNames[idx];
        if (name != nullptr) {
            int8_t lo = static_cast<int8_t>(g_serverIds[idx * 2]);
            if (lo < 0)
                gump->AddEntry(static_cast<int>(static_cast<uint8_t>(-lo)), name, 0x3b2);
            else
                gump->AddEntry(static_cast<int>(reinterpret_cast<uint16_t *>(g_serverIds)[idx]), name, 0);
        } else {
            gump->AddEntry(static_cast<int>(reinterpret_cast<uint16_t *>(g_serverIds)[idx]), g_strEmptyServer, 0);
        }
    }
    operator delete(order);
    gump->Redraw();
    GumpLink(gump);
    g_scrollChoiceGump = gump;
    if (g_loginGumpHook != 0 && g_prevGump != nullptr)
        decomp::vcall_void(g_prevGump, 0x4c);
}
HOOK_FUNC(0x004603e0, Packet_HandleServerList);

// ---------------------------------------------------------------------------
// 0x00460b80  size 0x70f
// Character list reply (five name / area slots) followed by the start location
// list, whose location groups are put in a random order. Replaces the
// character select gump with a new login gump holding the entries.
// ---------------------------------------------------------------------------
void __stdcall Packet_HandleCharacterList(uint8_t *packet)
{
    char text[0x400];
    char slotName[0x20];
    char slotArea[0x20];
    char locName[0x20];
    char locArea[0x20];
    char prevName[0x20];
    int groupStarts[0x104];
    int perm[0x104];
    int order[0x104];
    int nGroups;

    strcpy(text, g_acctName);
    strcat(text, g_titlePart1);
    strcat(text, g_titlePart2);
    if (g_inGame7211ac != 0) {
        strcat(text, g_titlePartA);
        strcat(text, g_titlePartB);
        strcat(text, g_titlePartC);
    }
    SetTitleSuffix(text);

    CLoginGump *gump = new CLoginGump();

    uint32_t off = 1;
    int i = 0;
    char *slotPtr = g_slotNeedsPw;
    g_charSlotCount = packet[pkt::Base(packet)];
    do {
        strncpy(slotName, reinterpret_cast<const char *>(packet) + pkt::Base(packet) + off, 0x1e);
        off += 0x1e;
        strncpy(slotArea, reinterpret_cast<const char *>(packet) + pkt::Base(packet) + off, 0x1e);
        off += 0x1e;
        slotName[0x1d] = 0;
        slotArea[0x1d] = 0;
        strcpy(slotPtr - 0x96, slotName);
        strcpy(slotPtr, slotArea);
        if (strlen(slotName) != 0)
            LoginGump_AddEntry(gump, slotName, slotArea, i);
        i++;
        slotPtr += 0x1e;
    } while (reinterpret_cast<uintptr_t>(slotPtr) < GEND(g_slotNeedsPw));

    nGroups = 0;
    uint8_t nLocs = packet[pkt::Base(packet) + off];
    off++;
    if (nLocs != 0) {
        int n = nLocs;
        g_locCount = n;
        uint32_t sz = static_cast<uint32_t>(n) * 4 + 4;
        g_locNames = static_cast<char **>(::operator new(sz));
        memset(g_locNames, 0, sz);
        g_locIds = static_cast<int32_t *>(::operator new(sz));
        groupStarts[0] = 0;
        int *groupPtr = &groupStarts[0];
        for (i = 0; i < n; i++) {
            int8_t id = static_cast<int8_t>(packet[pkt::Base(packet) + off]);
            off++;
            strncpy(locName, reinterpret_cast<const char *>(packet) + pkt::Base(packet) + off, 0x1f);
            off += 0x1f;
            locName[0x1e] = 0;
            strncpy(locArea, reinterpret_cast<const char *>(packet) + pkt::Base(packet) + off, 0x1f);
            off += 0x1f;
            locArea[0x1e] = 0;
            if (strlen(locName) != 0) {
                g_locNames[id] = static_cast<char *>(::operator new(strlen(locName) + strlen(locArea) + 3));
                sprintf(g_locNames[id], g_fmtLocation, locName, locArea);
                g_locIds[id] = id;
            }
            if (i > 0) {
                if (strcmp(locName, prevName) != 0) {
                    nGroups++;
                    groupPtr++;
                    *groupPtr = i;
                }
            }
            strcpy(prevName, locName);
        }
    }

    if (nLocs != 0) {
        int n = nLocs;
        uint32_t sz = static_cast<uint32_t>(n) * 4 + 4;
        char **newList = static_cast<char **>(::operator new(sz));
        memset(newList, 0, sz);
        int G = nGroups + 1;
        groupStarts[G] = n;
        int k = 0;
        do {
            perm[k] = k;                                   // runs once even for G < 0
            k++;
        } while (k <= G);
        if (G > 0) {
            int left = G;
            int *op = &order[0];
            int bytesLeft = G * 4;
            int cnt = G;
            do {
                int r = RandomMod(left);                   // random in [0, left)
                *op = perm[r];
                if (r < left - 1)
                    perm[r] = *reinterpret_cast<int *>(reinterpret_cast<char *>(perm) + bytesLeft - 4);
                op++;
                bytesLeft -= 4;
                left--;
                cnt--;
            } while (cnt != 0);
        }
        order[G] = G;
        int m = 0;
        if (G > 0) {
            int *op = &order[0];
            int cnt = G;
            do {
                int gi = *op;
                int b = groupStarts[gi];
                if (b < n) {
                    int end = groupStarts[gi + 1];
                    while (b < end) {
                        char *src = g_locNames[b];
                        char *cp = static_cast<char *>(::operator new(strlen(src) + 1));
                        newList[m] = cp;
                        strcpy(cp, g_locNames[b]);
                        g_locIds[m] = b;
                        m++;
                        b++;
                        if (!(b < n))
                            break;
                    }
                }
                op++;
                cnt--;
            } while (cnt != 0);
        }
        if (n > 0) {
            for (int j = 0; j < n; j++)
                operator delete(g_locNames[j]);
        }
        operator delete(g_locNames);
        g_locNames = newList;
    }

    if (g_waitingGump != nullptr) {
        decomp::vdelete(g_waitingGump);
        g_waitingGump = nullptr;
    }
    if (g_prevGump != nullptr)
        decomp::vdelete(g_prevGump);
    if (g_dlgParent != nullptr)
        RAWF(int32_t, g_dlgParent, 0x68) = 0;
    g_prevGump = gump;
    GumpLink(gump);
}
HOOK_FUNC(0x00460b80, Packet_HandleCharacterList);
