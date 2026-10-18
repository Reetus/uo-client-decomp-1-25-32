// Login-phase inbound packet handlers:
//   0xA8 server list with characters   0x0045f690
//   0x8C server relay (connect to the game server, send 0x91)   0x00461760
//   0xA6 tip of the day                0x00461fd0
//
// All are __stdcall (RET 4) with the packet pointer; field offsets are
// relative to the packet base (see packet_cursor.h). The stack probe and the
// SEH frames of the original are omitted.
#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"
#include "gump.h"
#include "crt.h"
#include "packet_cursor.h"
#include "raw_field.h"
#include "vcall.h"
#include "client_socket.h"
#include "waiting_gump.h"
#include "gump_manager.h"
#include "login_gumps.h"
#include "scroll_choice_gump.h"
#include "packet_builders_ports.h"
#include "sys_info.h"
#include "config_file_ports.h"
#include "app_support_ports.h"
#include "font_manager.h"
#include "tips_gump.h"
#include "main_window_ports.h"

// ---------------------------------------------------------------------------
// Globals (single-file use).
// ---------------------------------------------------------------------------
#define g_dlgA GLOBAL_AS(void *, g_waitingGump)                 // modal message gump
#define g_dlgB GLOBAL_AS(void *, g_prevGump)                 // character list gump
#define g_dlgServers GLOBAL_AS(void *, g_pMainMenu)           // parent for the server list gump
#define g_serverListGump GLOBAL_AS(void *, g_scrollChoiceGump)
// 0xA8 tables
#define g_serverIds GLOBAL_AS(uint8_t *, g_serverValues)          // 2 bytes per server (read as short and as signed byte)

// ---------------------------------------------------------------------------
// Original code not ported yet.
// ---------------------------------------------------------------------------
static void SetTitleSuffix(char *text) { PORT_THISCALL0(&SetMainWindowTitle, void, text)(); }          // ECX = text
static void Net_Send(const uint8_t *buf) { pkt::Send(buf); }
static void Net_Disconnect(void *conn) { reinterpret_cast<CSocketBase *>(conn)->Close(); }
static int Net_Connect(void *conn, uint32_t ip, uint32_t port) { return reinterpret_cast<CClientSocket *>(conn)->Connect(ip, port); }
static void Gump_Show(void *gump) { CGump_AddChild(static_cast<CGump *>(gump), nullptr, 1); }
static void ShowMessage(const char *text, void *parent, int a, int b)
{
    PORT_THISCALL4(&CreateGump2ac, void, g_gumpManager, const char *, void *, int, int)(text, parent, a, b);
}
static void LoginGump_AddEntry(void *gump, const char *city, const char *area, int index)
{
    reinterpret_cast<CLoginGump *>(gump)->AddEntry(city, (int)(area), index);
}
static void *ServerListGump_New(void *parent, int art) { return (void *)new CScrollChoiceGump((CGump *)(parent), art); }   // 0x0041cd50, size 0x22cc
static void ServerListGump_Add(void *gump, uint32_t id, const char *name, uint32_t hue)
{
    reinterpret_cast<CScrollChoiceGump *>(gump)->AddEntry((int)(id), name, (uint16_t)(hue));
}
static void ServerListGump_Finish(void *gump) { reinterpret_cast<CScrollChoiceGump *>(gump)->Redraw(); }
static void SysInfo_Build(uint8_t *buf) { Packet_Build0xA4(buf); }
static int SysInfo_Probe(void *obj) { return reinterpret_cast<CSysInfo *>(obj)->ReadRegistry(); }
static int SysInfo_Check(void *obj, uint8_t *sysRec) { return reinterpret_cast<CSysInfo *>(obj)->Equals((const CSysInfo *)(sysRec)); }
static void SysInfo_Release(uint8_t *sysRec) { reinterpret_cast<CSysInfo *>(sysRec)->WriteRegistry(); }
static void Config_SetString(uint32_t id, const char *text) { WriteConfigSetting(id, text); }
static int RandomMod(int n) { return App_RandomMod(n); }
static int FontWrapIndex(int font, char *text, int width)
{
    return reinterpret_cast<CFontManager *>(GLOBAL_PTR(void, g_fontMgr_storage))->FindWrapBreak(font, text, width);
}

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

static void delete_dlgA()
{
    if (g_dlgA != nullptr) {
        decomp::vdelete(g_dlgA);
        g_dlgA = nullptr;
    }
}

// ---------------------------------------------------------------------------
// 0x0045f690  size 0xd4e
// 0xA8-family login reply: system information, the five character slots, the
// server list, the location list (randomly regrouped), then the gump that
// shows them.
// ---------------------------------------------------------------------------
void __stdcall Packet_HandleServerListWithChars(uint8_t *packet)
{
    char hdrText[0x400];
    uint8_t obj248[0xe8];             // object passed as ECX to 0x0043f240 / 0x0045f500 (never initialised here) ?
    uint8_t sysRec[0xe0];             // filled by the 0xA4 builder
    uint8_t nServers;
    uint8_t nLocs;
    uint8_t idByte;
    char cityName[0x20];
    char areaName[0x20];
    char locName[0x20];
    char locArea[0x20];
    char prevName[0x20];
    int groupStarts[0x25];
    int perm[0x25];
    int order[0x25];
    int *groupPtr;
    int nGroups;
    int i;
    int base;
    int off;
    void *newGump = nullptr;

    if (g_inGame7211ac == 0) {
        SysInfo_Build(sysRec);
        if (SysInfo_Probe(obj248) == 0 || SysInfo_Check(obj248, sysRec) == 0) {
            Net_Send(sysRec);
            SysInfo_Release(sysRec);
        }
    }

    strcpy(hdrText, g_acctName);
    strcat(hdrText, g_titlePart1);
    strcat(hdrText, g_titlePart2);
    if (g_inGame7211ac != 0) {
        strcat(hdrText, g_titlePartA);
        strcat(hdrText, g_titlePartB);
        strcat(hdrText, g_titlePartC);
    }
    SetTitleSuffix(hdrText);

    if (g_loginByAddr != 0 || g_inGame7211ac != 0) {
        newGump = new CLoginGump();
    }

    base = pkt::Base(packet);
    uint8_t slotCount = packet[base];
    if (g_loginByAddr != 0 || g_inGame7211ac != 0)
        g_charSlotCount = slotCount;

    off = 2;
    uint8_t flags = packet[base + off - 1];
    if (g_inGame7211ac == 0) {
        sprintf(hdrText, g_fmtNumber, flags);
        g_charFlags = flags;
        Config_SetString(0x1d, hdrText);
    }

    i = 0;
    char *slotPtr = g_slotNeedsPw;
    do {
        strncpy(cityName, reinterpret_cast<const char *>(packet) + base + off, 0x1e);
        off += 0x1e;
        strncpy(areaName, reinterpret_cast<const char *>(packet) + base + off, 0x1e);
        off += 0x1e;
        areaName[0x1d] = 0;
        cityName[0x1d] = 0;
        if (g_loginByAddr != 0 || g_inGame7211ac != 0) {
            strcpy(slotPtr - 0x96, cityName);
            strcpy(slotPtr, areaName);
            if (strlen(cityName) != 0)
                LoginGump_AddEntry(newGump, cityName, areaName, i);
        }
        i++;
        slotPtr += 0x1e;
    } while (reinterpret_cast<uintptr_t>(slotPtr) < GEND(g_slotNeedsPw));

    nServers = packet[base + off];
    off++;
    if (g_loginByAddr == 0 && g_inGame7211ac == 0 && nServers == 0) {
        delete_dlgA();
        if (GLOBAL_AS(void *, g_dlgParent) == nullptr)
            return;
        RAWF(int32_t, GLOBAL_AS(void *, g_dlgParent), 0x68) = 0;
        void *p1 = new CGump_0043de30(-1, -1, g_strNoServersMsg, (int)(GLOBAL_AS(void *, g_dlgParent)), 0, 1, 0);
        g_dlgA = p1;
        Gump_Show(p1);
        g_flag6f493c = 0;
        Net_Disconnect(g_pktSender);
        return;
    }

    if (g_inGame7211ac == 0) {
        g_serverCount = static_cast<signed char>(nServers);
        void *t = ::operator new(static_cast<signed char>(nServers) * 4 + 4);
        g_serverNames = static_cast<char **>(t);
        memset(t, 0, static_cast<signed char>(nServers) * 4 + 4);
        g_serverIds = static_cast<uint8_t *>(::operator new(static_cast<signed char>(nServers) * 2 + 2));
    }

    for (i = 0; i < static_cast<signed char>(nServers); i++) {
        base = pkt::Base(packet);
        idByte = packet[base + off];
        off++;
        base = pkt::Base(packet);
        char *name = reinterpret_cast<char *>(packet) + base + off;
        off += 0x10;
        if (g_inGame7211ac == 0 && strlen(name) != 0) {
            reinterpret_cast<int16_t *>(g_serverIds)[i] = static_cast<signed char>(idByte);
            name[0xf] = 0;
            g_serverNames[i] = static_cast<char *>(::operator new(strlen(name) + 1));
            strcpy(g_serverNames[i], name);
        }
    }

    // ---- locations ----
    nGroups = 0;
    base = pkt::Base(packet);
    nLocs = packet[base + off];
    off++;
    if (nLocs != 0) {
        if (g_inGame7211ac == 0) {
            g_locCount = static_cast<signed char>(nLocs);
            void *t2 = ::operator new(static_cast<signed char>(nLocs) * 4 + 4);
            g_locNames = static_cast<char **>(t2);
            memset(t2, 0, static_cast<signed char>(nLocs) * 4 + 4);
            g_locIds = static_cast<int32_t *>(::operator new(static_cast<signed char>(nLocs) * 4 + 4));
        }
        groupStarts[0] = 0;
        groupPtr = &groupStarts[0];
        for (i = 0; i < static_cast<signed char>(nLocs); i++) {
            base = pkt::Base(packet);
            idByte = packet[base + off];
            off++;
            base = pkt::Base(packet);
            strncpy(locName, reinterpret_cast<const char *>(packet) + base + off, 0x1f);
            off += 0x1f;
            locName[0x1e] = 0;
            base = pkt::Base(packet);
            strncpy(locArea, reinterpret_cast<const char *>(packet) + base + off, 0x1f);
            off += 0x1f;
            locArea[0x1e] = 0;
            if (g_inGame7211ac == 0 && strlen(locName) != 0) {
                g_locNames[static_cast<signed char>(idByte)] =
                    static_cast<char *>(::operator new(strlen(locName) + strlen(locArea) + 3));
                sprintf(g_locNames[static_cast<signed char>(idByte)], g_fmtLocation, locName, locArea);
                g_locIds[static_cast<signed char>(idByte)] = static_cast<signed char>(idByte);
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

    // ---- random permutation of the location groups ----
    if (g_inGame7211ac == 0 && nLocs != 0) {
        int n = static_cast<signed char>(nLocs);
        int G;
        int kk;
        int *op;
        int cnt;
        int bytesLeft;
        int j;
        char **newList = static_cast<char **>(::operator new(n * 4 + 4));
        memset(newList, 0, n * 4 + 4);
        G = nGroups + 1;
        groupStarts[G] = n;
        for (j = 0; j <= G; j++)
            perm[j] = j;                                   // do-while in the original: runs once even for G == 0
        if (G > 0) {
            int left = G;
            op = &order[0];
            bytesLeft = G * 4;
            cnt = G;
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
        kk = 0;
        if (G > 0) {
            op = &order[0];
            cnt = G;
            do {
                int g = *op;
                int b = groupStarts[g];
                if (b < static_cast<signed char>(nLocs)) {
                    int end = groupStarts[g + 1];
                    if (b < end) {
                        do {
                            char *src = g_locNames[b];
                            char *cp = static_cast<char *>(::operator new(strlen(src) + 1));
                            newList[kk] = cp;
                            strcpy(cp, g_locNames[b]);
                            g_locIds[kk] = b;
                            b++;
                            kk++;
                        } while (b < static_cast<signed char>(nLocs) && b < end);
                    }
                }
                op++;
                cnt--;
            } while (cnt != 0);
        }
        for (j = 0; j < static_cast<signed char>(nLocs); j++)
            operator delete(g_locNames[j]);
        operator delete(g_locNames);
        g_locNames = newList;
    }

    delete_dlgA();
    if (g_dlgB != nullptr) {
        decomp::vdelete(g_dlgB);
        g_dlgB = nullptr;
    }
    if (GLOBAL_AS(void *, g_dlgParent) != nullptr)
        RAWF(int32_t, GLOBAL_AS(void *, g_dlgParent), 0x68) = 0;

    if (g_loginByAddr == 0 && g_inGame7211ac == 0) {
        if (GLOBAL_AS(void *, g_serverSortKeys) != nullptr)
            operator delete(GLOBAL_AS(void *, g_serverSortKeys));
        g_serverSortKeys = static_cast<uint8_t *>(::operator new(g_serverCount + 1));
        uint8_t *sortOrder = static_cast<uint8_t *>(::operator new(g_serverCount));
        for (int k = 0; k < g_serverCount; k++)
            sortOrder[k] = static_cast<uint8_t>(k);

        char *name = g_serverNames[0];
        size_t len = strlen(name);
        if (name[len - 1] >= 0x30 && name[len - 1] <= 0x39) {
            for (int k = 0; k < g_serverCount; k++) {
                name = g_serverNames[k];
                int p = static_cast<int>(strlen(name)) - 2;
                if (name[p] != ' ') {
                    unsigned char c;
                    do {
                        c = static_cast<unsigned char>(name[p - 1]);
                        p--;
                    } while (c != ' ');
                }
                name[p] = 0;
                int tz = atoi(name + p + 1);
                int d = tz - g_localTimezone;
                d = (d ^ (d >> 31)) - (d >> 31);
                d &= 0xff;
                int wrap = tz + 0xc;
                if (0xc - tz < wrap)
                    wrap = g_localTimezone + (0xc - tz) + 0xc;
                else
                    wrap += 0xc - g_localTimezone;
                if (d < wrap)
                    g_serverSortKeys[k] = static_cast<uint8_t>(d);
                else
                    g_serverSortKeys[k] = static_cast<uint8_t>(wrap);
            }
            for (int a = 0; a < g_serverCount - 1; a++) {
                int best = -1;
                for (int b = a; b < g_serverCount; b++) {
                    if (GLOBAL_AS(char *, g_serverFlags)[sortOrder[b]] == '2')
                        continue;
                    if (best == -1 || g_serverSortKeys[sortOrder[b]] < g_serverSortKeys[sortOrder[best]])
                        best = b;
                }
                if (best != a && best != -1) {
                    unsigned char t0 = sortOrder[best];
                    unsigned char t1 = sortOrder[a];
                    sortOrder[a] = t0;
                    sortOrder[best] = t1;
                }
            }
        }

        void *gump = ServerListGump_New(g_dlgServers, 0x4ca);
        // (not null-checked in the original)
        strcpy(static_cast<char *>(gump) + 0x226c, g_strServerListTitle);
        RAWF(int32_t, gump, 0xb8) = 0xcd;
        for (i = 0; i < g_serverCount; i++) {
            int idx = sortOrder[i];
            char *nm = g_serverNames[idx];
            if (nm == nullptr) {
                ServerListGump_Add(gump, reinterpret_cast<uint16_t *>(g_serverIds)[idx], g_strEmptyServer, 0);
            } else if (static_cast<signed char>(g_serverIds[idx * 2]) < 0) {
                ServerListGump_Add(gump, static_cast<uint8_t>(-static_cast<signed char>(g_serverIds[idx * 2])), nm, 0x3b2);
            } else {
                ServerListGump_Add(gump, reinterpret_cast<uint16_t *>(g_serverIds)[idx], nm, 0);
            }
        }
        operator delete(sortOrder);
        ServerListGump_Finish(gump);
        Gump_Show(gump);
        g_serverListGump = gump;
    } else {
        g_dlgB = newGump;
        Gump_Show(newGump);
    }
    if (g_loginGumpHook != 0 && g_dlgB != nullptr)
        decomp::vcall_void(g_dlgB, 0x4c);
}
HOOK_FUNC(0x0045f690, Packet_HandleServerListWithChars);

// ---------------------------------------------------------------------------
// 0x00461760  size 0x3b9
// 0x8C ServerRelay: connect to the game server, then send 0x91 GameServerLogin.
// ---------------------------------------------------------------------------
void __stdcall Packet_HandleServerRelay(uint8_t *packet)
{
    uint8_t buf[0x45];
    int tries = 0;
    uint32_t off;
    int base;

    base = pkt::Base(packet);
    uint32_t ip = ntohl(rd32(packet, base));
    base = pkt::Base(packet);
    uint32_t port = ntohs(rd16(packet, base + 4));
    base = pkt::Base(packet);
    g_authKey = rd32(packet, base + 6);                       // raw store first
    g_authKey = ntohl(rd32(packet, base + 6));

    if (ip == 0) {
        // server full
        delete_dlgA();
        void *newConn = new CGump_0043de30(-1, -1, g_strServerFullMsg, (int)(g_dlgServers), 1, 1, 0);
        g_dlgA = newConn;
        Gump_Show(newConn);
        g_flag6f493c = 0;
        g_inGame7211ac = 0;
        Net_Disconnect(g_pktSender);
        return;
    }

    SetTitleSuffix(g_strConnectingTitle);

    if (ip != RAWF(uint32_t, g_pktSender, 0x20028)) {
        void *newConn = new CClientSocket();
        do {
            if (Net_Connect(newConn, ip, port) != 0)
                break;
            tries++;
        } while (tries < 3);
        if (tries == 3) {
            delete_dlgA();
            ShowMessage(g_strCannotConnectMsg, GLOBAL_AS(void *, g_dlgParent), 1, 1);
            if (g_pktSender != nullptr)
                decomp::vdelete(g_pktSender);
            void *newObj = new CClientSocket();
            g_pktSender = newObj;
            if (newConn != nullptr)
                decomp::vdelete(newConn);
            g_flag6f493c = 0;
            g_loginFlag648928 = 1;
            g_inGame7211ac = 0;
            return;
        }
        g_oldConn = g_pktSender;
        Net_Disconnect(g_pktSender);
        g_pktSender = newConn;
    }

    // ---- build 0x91 ----
    buf[0] = 0x91;
    if (pkt::IsVariable(buf))
        *reinterpret_cast<uint16_t *>(buf + 1) = 3;
    else
        g_pktFixedCursor = 1;
    uint32_t key = htonl(g_authKey);
    off = pkt::Cursor(buf);
    pkt::Put32(buf, off, key);
    off += 4;
    pkt::SetCursor(buf, off);
    off = pkt::Cursor(buf);
    memcpy(buf + off, g_acctName, 0x1e);
    off += 0x1e;
    pkt::SetCursor(buf, off);
    off = pkt::Cursor(buf);
    memcpy(buf + off, g_accountPassword, 0x1e);
    off += 0x1e;
    pkt::SetCursor(buf, off);
    Net_Send(buf);
    g_inGame7211ac = 1;
    g_useHuffman = 1;
}
HOOK_FUNC(0x00461760, Packet_HandleServerRelay);

// ---------------------------------------------------------------------------
// 0x00461fd0  size 0x2a0
// 0xA6 tip of the day: word-wrap the text into up to 100 lines of 0x50 bytes,
// open the tip window and remember the tip number.
// ---------------------------------------------------------------------------
void __stdcall Packet_HandleTipOfTheDay(uint8_t *packet)
{
    char lines[100][0x50];
    char numBuf[0x20];
    int base;

    base = pkt::Base(packet);
    uint8_t flag = packet[base];
    base = pkt::Base(packet);
    uint32_t tipNo = ntohl(rd32(packet, base + 1));
    base = pkt::Base(packet);
    uint32_t textLen = ntohs(rd16(packet, base + 5)) & 0xffffu;
    base = pkt::Base(packet);
    char *text = static_cast<char *>(::operator new(textLen + 1));
    memcpy(text, packet + base + 7, textLen);
    text[textLen] = 0;
    if (static_cast<int>(textLen) > 0) {
        for (int i = 0; i < static_cast<int>(textLen); i++)
            if (text[i] == 0xd)
                text[i] = 0;
    }

    int pos = 0;
    int lineCount = 0;
    char *p = text;
    if (static_cast<int>(textLen) > 1) {
        char *linePtr = &lines[0][0];
        do {
            int idx = FontWrapIndex(6, p, 0xd6);
            if (idx != -1) {
                p[idx + 1] = 0;
                strcpy(linePtr, p);
                p = p + idx + 2;
                pos = pos + idx + 2;
            } else {
                strcpy(linePtr, p);
                pos = pos + static_cast<int>(strlen(p)) + 1;
                p = p + strlen(p) + 1;
            }
            lineCount++;
            linePtr += 0x50;
        } while (pos + 1 < static_cast<int>(textLen));
    }

    if (flag != 1 || static_cast<int>(tipNo) > g_tipLast) {
        Gump_Show(new CTipsGump(0x64, 0x64, lineCount, &lines[0][0], flag == 0));
    }

    if (flag == 0) {
        g_tipNumber = tipNo;
        sprintf(numBuf, g_fmtNumber, tipNo);
        Config_SetString(0x2b, numBuf);
    } else if (flag == 1) {
        g_tipLast = static_cast<int32_t>(tipNo);
        sprintf(numBuf, g_fmtNumber, tipNo);
        Config_SetString(0x2c, numBuf);
    }
    operator delete(text);
}
HOOK_FUNC(0x00461fd0, Packet_HandleTipOfTheDay);
