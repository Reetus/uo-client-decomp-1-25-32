// Session-level client procedures between the macro code and the part-list code (second pass, batch 19):
// chardata.mul, war mode, text / target command packets, the "waiting" dialogs that precede a login /
// character packet, the single-click sweep and the login server connection search.
// The SEH frames of the original are omitted.
#include <windows.h>

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "globals.h"
#include "gump.h"
#include "input_gump.h"
#include "obj.h"
#include "crt.h"
#include "raw_field.h"
#include "status_gump.h"
#include "vcall.h"
#include "waiting_gump.h"
#include "packet_builders_batch24_ports.h"
#include "client_socket.h"
#include "mouse_manager.h"
#include "config_file_ports.h"
#include "packet_builders_b25_ports.h"
#include "circle_trans.h"
#include "world_obj_ops.h"
#include "app_session_ops_ports.h"
#include "window_helpers_31_ports.h"
#include "object_manager.h"
#include "map_world.h"
#include "walk.h"

#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)              // this for the packet builders 0x004631xx..0x004654xx
#define g_playerWarMode(p) RAWF(int32_t, p, 0x150)
#define g_loginParent GLOBAL_AS(void *, g_dlgParent)
#define g_charListParent GLOBAL_AS(void *, g_prevGump)
#define g_createParent GLOBAL_AS(void *, g_cityChoiceGump)
#define g_dynamicListHead GLOBAL_AS(CObj *, g_dynListHead)
#undef g_mouse   // the class header's typed form; this file addresses the object by offset
#define g_mouse GLOBAL_PTR(void, g_mouse_storage)

// Login server state (0x0044e250 / 0x0044e750).


// Packet helpers of the original (ECX = the packet builder object / the connection).
#define Pkt_BuildText(o, a1, a2, a3, a4) PORT_THISCALL4(&Build_Packet_0x12, void, (o), char *, int, const char *, size_t)(a1, a2, a3, a4)   // (pkt, id, text, length), RET 0x10
#define Pkt_Send(self, a1) (reinterpret_cast<CClientSocket *>(self)->SendPacket((const uint8_t *)(a1)))                                    // RET 4
#define SetMouseCursor(self, a1) (reinterpret_cast<CMouseManager *>(self)->SetMouseCursor(a1))                                 // RET 4
#define ResolveHostPort PORT_FUNC6(&ResolveHostPort, void, __fastcall, const char *, int *, int *, int *, int *, int *)

// 0x0044d690  size 0xef
// Write chardata.mul under the data path: the two blocks of five words at +0x3f0 and +0x3fa.
// The path comes from the global app object's data path at 0x0069e2d0, not from `this`.
void __thiscall CApp::SaveCharData()
{
    size_t len;
    FILE *f;

    strcpy(g_resolvedPath, g_appDataPath);
    if (g_resolvedPath[0] != 0) {
        len = strlen(g_resolvedPath);
        if (g_resolvedPath[len - 1] != '\\')
            strcat(g_resolvedPath, g_pathSeparator);
    }
    strcat(g_resolvedPath, "chardata.mul");
    f = fopen(g_resolvedPath, "wb");
    fwrite(reinterpret_cast<char *>(this) + 0x3f0, 2, 5, f);
    fwrite(reinterpret_cast<char *>(this) + 0x3fa, 2, 5, f);
    fclose(f);
}
HOOK_METHOD(0x0044d690, CApp, SaveCharData);

// 0x0044dd70  size 0x2d
// Redraw the typed-text strip from the text entry's buffer when the fps flag (0x004b6b38) is set.
void __cdecl RedrawInputLine()
{
    if (g_showFps != 0)
        RAWF(CInputGump *, g_textEntry, 0x11c)->Redraw(reinterpret_cast<char *>(g_textEntry) + 0x0c, g_colorSay, g_colorSayFont);
}
HOOK_FUNC(0x0044dd70, RedrawInputLine);

// 0x0044dda0  size 0xbb
// Set the war mode (mode in ECX; -1 toggles from the player's current flag) and tell the server.
void __fastcall SetWarMode(int mode)
{
    uint8_t pkt2[2];   // packet 0x2c
    uint8_t pkt6[6];   // packet 0x72
    int value;

    if (decomp::vcall(PORT_RET(int), g_player, 0x8c) != 0) {
        PORT_THISCALL2(&Build_Packet_0x2C, void, g_packetBuilder, uint8_t *, int)(pkt2, 0);   // Build_Packet_0x2C, RET 8
        reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket(pkt2);
    }
    if (mode != -1)
        value = mode;
    else
        value = (g_playerWarMode(g_player) == 0) ? 1 : 0;
    g_pendingWarMode = value;
    // Pkt_Build_0x72_WarMode: (pkt, mode, b0x0069f868, b0x0069f86c, b0x0069f870), RET 0x14
    PORT_THISCALL5(&Pkt_Build_0x72_WarMode, void, g_packetBuilder, uint8_t *, int, uint8_t, uint8_t, uint8_t)(pkt6, value, GLOBAL_AS(uint8_t, g_fightMode), GLOBAL_AS(uint8_t, g_fightValueA), GLOBAL_AS(uint8_t, g_fightValueB));
    reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket(pkt6);
    PORT_THISCALL0(&CObjList::Clear, void, GLOBAL_PTR(void, g_tickIndexHead_storage))();   // CObjList::Clear
    CGump *screen = g_screenGump;
    void *sprite = RAWF(void *, screen, 0xd4);
    if (sprite != nullptr) {
        RAWF(void *, screen, 0xd4) = nullptr;
        reinterpret_cast<CWorldObj *>(sprite)->Invalidate();   // CWorldObj::Invalidate
    }
}
HOOK_FUNC(0x0044dda0, SetWarMode);

// 0x0044de60  size 0x10c
// Store the war mode on the player (relinking it when it is in the world) and switch the war button art of
// every status gump that belongs to the player.
void __fastcall ApplyWarMode(int mode)
{
    CObj *player;
    int inWorld;
    CGump *g;

    if (mode == g_pendingWarMode)
        g_pendingWarMode = -1;
    player = g_player;
    inWorld = RAWF(int32_t, player, 0x1c);
    if (inWorld != 0) {
        decomp::vcall_void(player, 0x14);
        player = g_player;
    }
    g_playerWarMode(player) = mode;
    if (inWorld != 0)
        decomp::vcall_void(g_player, 0x10);
    for (g = g_gumpListTail; g != nullptr; g = g->prev) {
        if (decomp::vcall(PORT_RET(int), g, 0xac) != 0 && g->owner == g_player)
            static_cast<CStatusGump *>(g)->RedrawFrame(1, 1, 1, 1);
        if (decomp::vcall(PORT_RET(int), g, 0x78) != 0 && g->owner == g_player) {
            if (mode != 0) {
                RAWF(int32_t, RAWF(void *, g, 0x98), 0x50) = 0x7e8;
                RAWF(int32_t, RAWF(void *, g, 0x98), 0xa0) = 0x7e9;
                RAWF(int32_t, RAWF(void *, g, 0x98), 0xa4) = 0x7ea;
            } else {
                RAWF(int32_t, RAWF(void *, g, 0x98), 0x50) = 0x7e5;
                RAWF(int32_t, RAWF(void *, g, 0x98), 0xa0) = 0x7e6;
                RAWF(int32_t, RAWF(void *, g, 0x98), 0xa4) = 0x7e7;
            }
        }
    }
}
HOOK_FUNC(0x0044de60, ApplyWarMode);

// 0x0044df70  size 0x5e
// Send a text packet (builder 0x00463170) of type ECX with the text in EDX.
// The original first formats the text with "%s" into a local buffer (0x0048b1d0) and never reads it.
void __fastcall SendTextPacket(int type, const char *text)
{
    char unused[0x200];
    char pkt[0xf8];

    sprintf(unused, "%s", text);   // 0x0048b1d0; result unused
    Pkt_BuildText(g_packetBuilder, pkt, type, text, strlen(text));
    Pkt_Send(g_pktSender, pkt);
}
HOOK_FUNC(0x0044df70, SendTextPacket);

// 0x0044dfd0  size 0x1cd
// Target callback (ECX = the clicked object, 0 for none): send the pending command with its target as
// packet 0x27 ("<text> ...") or, when a number is set, packet 0x2f ("<number> ..."), then drop the
// target cursor. Kind 0: text only; kind 1: x y z of the object; kind 2: the serial of an item.
void __fastcall SendTargetCommand(CObj *target)
{
    char buf[0x200];
    char pkt[0xf8];

    g_targetCallback = 0;
    if (g_targetCursorType == 0) {
        if (g_cmdNumber != 0) {
            sprintf(buf, "%d", g_cmdNumber);
            goto send2f;
        }
        Pkt_BuildText(g_packetBuilder, pkt, 0x27, g_cmdPrefix, strlen(g_cmdPrefix));
        Pkt_Send(g_pktSender, pkt);
    } else if (g_targetCursorType == 1) {
        if (target == nullptr)
            goto done;
        if (g_cmdNumber != 0) {
            sprintf(buf, "%d %d %d %d", g_cmdNumber, static_cast<int>(target->x), static_cast<int>(target->y), static_cast<int>(target->z));
            goto send2f;
        }
        sprintf(buf, "%s %d %d %d", g_cmdPrefix, static_cast<int>(target->x), static_cast<int>(target->y), static_cast<int>(target->z));
        Pkt_BuildText(g_packetBuilder, pkt, 0x27, buf, strlen(buf));
        Pkt_Send(g_pktSender, pkt);
    } else if (g_targetCursorType == 2) {
        if (target == nullptr)
            goto done;
        if (decomp::vcall(PORT_RET(int), target, 0x24) == 0)
            goto done;
        if (g_cmdNumber != 0) {
            sprintf(buf, "%d %d", g_cmdNumber, RAWF(int32_t, target, 0x78));
            goto send2f;
        }
        sprintf(buf, "%s %d", g_cmdPrefix, RAWF(int32_t, target, 0x78));
        Pkt_BuildText(g_packetBuilder, pkt, 0x27, buf, strlen(buf));
        Pkt_Send(g_pktSender, pkt);
    }
    goto done;
send2f:
    Pkt_BuildText(g_packetBuilder, pkt, 0x2f, buf, strlen(buf));
    Pkt_Send(g_pktSender, pkt);
done:
    g_targetCallbackActive = 0;
    SetMouseCursor(g_mouse, 0x11);
}
HOOK_FUNC(0x0044dfd0, SendTargetCommand);

// 0x0044e1a0  size 0x5f
// Begin a command that needs a target: ECX = command text, EDX = kind (0 send now, 1 / 2 ask for a target),
// stack = number. RET 4.
void __fastcall BeginTargetCommand(const char *prefix, int kind, int number)
{
    g_cmdPrefix = prefix;
    g_targetCursorType = kind;
    g_cmdNumber = number;
    if (kind == 0) {
        SendTargetCommand(nullptr);   // SendTargetCommand
        return;
    }
    if (kind == 1 || kind == 2) {
        PORT_THISCALL1(&SetCursorForRegion, void, &g_app, int)(0x27);   // does not read ECX
        g_targetCallbackActive = 1;
        SetMouseCursor(g_mouse, 0x13);
        g_targetCallback = PORT_ADDR(&SendTargetCommand);
    }
}
HOOK_FUNC(0x0044e1a0, BeginTargetCommand);

// 0x0044e200  size 0x49
// Reset the light levels and scroll offsets, delete every world object and reset the map and walk state.
void __cdecl ResetWorldState()
{
    CObj *player = g_player;

    g_lightAmbient = 0;
    g_playerLight = 0;
    if (player != nullptr)
        RAWF(int32_t, player, 0x9c) = 0;
    g_scrollOffsetY = 0;
    g_scrollOffsetX = 0;
    g_scrollOffset = 0;
    reinterpret_cast<CObjectManager *>(GLOBAL_PTR(void, g_objMgr_storage))->DeleteAllObjects();   // CObjectManager::DeleteAllObjects
    reinterpret_cast<CMap *>(GLOBAL_PTR(void, g_map_storage))->Reset();   // map object reset
    reinterpret_cast<CWalkManager *>(GLOBAL_PTR(void, g_walk_storage))->ClearCounters();   // walk manager reset (tail jump in the original)
}
HOOK_FUNC(0x0044e200, ResetWorldState);

// 0x0044e250  size 0x10c
// Pick the address to connect to for the current login server (kind 0) or backup server (kind 1; ECX).
// A fixed address (0x006f4768) wins; otherwise the cached ip of the entry, else "host,port" resolved by
// 0x00424e80. A negative index sets the failure flag 0x006f4794.
void __fastcall SelectLoginAddress(int kind)
{
    int a, b, c, d, port;
    int idx;
    uint32_t ip;

    g_flag6f479c = 0;
    g_useHuffman = 0;
    if (GLOBAL_AS(uint32_t, g_loginByAddr) != 0) {
        g_hostIp = GLOBAL_AS(uint32_t, g_loginByAddr);
        g_hostPort = g_loginByAddrPort;
        return;
    }
    if (kind == 0) {
        idx = g_loginServerIndex;
        if (idx < 0) {
            g_suppressFlag = 1;
            return;
        }
        ip = g_loginServerIp[idx];
        if (ip != 0) {
            g_hostIp = ip;
            g_hostPort = g_loginServerPort[idx];
            return;
        }
        ResolveHostPort(g_loginServerSpec[idx], &a, &b, &c, &d, &port);
    } else {
        idx = g_backupServerIndex;
        if (idx < 0) {
            g_suppressFlag = 1;
            return;
        }
        ip = g_backupServerIp[idx];
        if (ip != 0) {
            g_hostIp = ip;
            g_hostPort = g_backupServerPort[idx];
            return;
        }
        ResolveHostPort(g_backupServerSpec[idx], &a, &b, &c, &d, &port);
    }
    g_hostPort = static_cast<uint16_t>(port);
    g_hostIp = (((static_cast<uint32_t>(a) << 8 | static_cast<uint32_t>(b)) << 8 | static_cast<uint32_t>(c)) << 8) | static_cast<uint32_t>(d);
}
HOOK_FUNC(0x0044e250, SelectLoginAddress);

// 0x0044e360  size 0xc7
// Show the "Entering Britannia... " dialog and send the play-character packet 0x5d (ECX, EDX, stack arg
// forwarded to the builder 0x004636c0). RET 4.
void __fastcall ShowEnteringBritannia(int a, int b, int c)
{
    uint8_t pkt[0x4c];
    void *mem;
    CGump *gump;

    if (g_waitingGump != nullptr)
        decomp::vdelete(g_waitingGump);
    gump = new CGump_0043de30(-1, -1, "Entering Britannia... ", reinterpret_cast<int>(g_charListParent), 1, 1, 0);
    g_waitingGump = gump;
    gump->lastChild->visible = 0;
    g_waitingGump->LinkInto(nullptr, 1);
    PORT_THISCALL4(&Build_Packet_0x5D, void, g_packetBuilder, uint8_t *, int, int, int)(pkt, a, b, c);   // Build_Packet_0x5D_LoginCharacter, RET 0x10
    reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket(pkt);
}
HOOK_FUNC(0x0044e360, ShowEnteringBritannia);

// 0x0044e430  size 0x13e
// Show the "Creating character... " dialog and send the create-character packet 0x00 built from the
// character creation globals around 0x0069e2b0 and the slot at 0x0069f9e8.
void __cdecl SendCreateCharacter()
{
    uint8_t pkt[0x68];
    void *mem;
    CGump *gump;

    if (g_waitingGump != nullptr)
        decomp::vdelete(g_waitingGump);
    gump = new CGump_0043de30(-1, -1, "Creating character... ", reinterpret_cast<int>(g_createParent), 1, 1, 0);
    g_waitingGump = gump;
    gump->lastChild->visible = 0;
    g_waitingGump->LinkInto(nullptr, 1);
    // Build_Packet_0x00_CreateCharacter: 21 stack args, RET 0x54
    PORT_THISCALL21(&Build_Packet_0x00, void, g_packetBuilder, uint8_t *, const char *, const char *, uint8_t, uint8_t, uint8_t, uint8_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint32_t)(pkt, g_playerName, g_password, g_0069e2b1,
        g_0069e2b4, g_0069e2b5, g_0069e2b6, g_0069e2bc,
        g_0069e2b8, g_0069e2ba, g_0069e2c0, g_0069e2be,
        g_0069e2c2, g_0069e2c3, g_0069e2c4, g_0069e2c5,
        g_0069e2c6, g_0069e2c7, g_selectedServer, g_0069e2c9,
        GLOBAL_AS(uint32_t, g_selectedCharSlot));
    reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket(pkt);
}
HOOK_FUNC(0x0044e430, SendCreateCharacter);

// 0x0044e570  size 0xc7
// Show the "Verifying Account... " dialog and send the account login packet 0x80 (ECX, EDX, stack arg
// forwarded to the builder 0x00465490). RET 4.
void __fastcall SendAccountLogin(int a, int b, int c)
{
    uint8_t pkt[0x40];
    void *mem;
    CGump *gump;

    if (g_waitingGump != nullptr)
        decomp::vdelete(g_waitingGump);
    gump = new CGump_0043de30(-1, -1, "Verifying Account... ", reinterpret_cast<int>(g_loginParent), 1, 1, 0);
    g_waitingGump = gump;
    gump->lastChild->visible = 0;
    g_waitingGump->LinkInto(nullptr, 1);
    PORT_THISCALL4(&Pkt_Build_0x80_AccountLogin, void, g_packetBuilder, uint8_t *, int, int, int)(pkt, a, b, c);   // Pkt_Build_0x80_AccountLogin, RET 0x10
    reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket(pkt);
}
HOOK_FUNC(0x0044e570, SendAccountLogin);

// 0x0044e640  size 0x108
// Send a single-click packet (0x09, builder 0x00463260) for every mobile (not the player, not flagged
// 0x80 at +0x98, with a graphic) and every item whose slot 0x54 answers yes, that lies in the 640x480
// view at the scroll origin.
void __cdecl SingleClickVisible()
{
    uint8_t pkt[8];
    CObj *o;

    for (o = GLOBAL_AS(CObj *, g_mobileListHead); o != nullptr; o = RAWF(CObj *, o, 0xbc)) {
        if ((RAWF(uint8_t, o, 0x98) & 0x80) != 0)
            continue;
        if (o->graphic == 0)
            continue;
        if (o == g_player)
            continue;
        if (o->rectX1 < g_scrollX)
            continue;
        if (o->rectX0 > g_scrollX + 0x280)
            continue;
        if (o->rectY1 < g_scrollY)
            continue;
        if (o->rectY0 > g_scrollY + 0x1e0)
            continue;
        PORT_THISCALL2(&Build_Packet_0x09, void, g_packetBuilder, uint8_t *, int)(pkt, RAWF(int32_t, o, 0x78));   // Build_Packet_0x09_SingleClick, RET 8
        reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket(pkt);
    }
    for (o = g_dynamicListHead; o != nullptr; o = RAWF(CObj *, o, 0x80)) {
        if (decomp::vcall(PORT_RET(int), o, 0x54) == 0)
            continue;
        if (o->rectX1 < g_scrollX)
            continue;
        if (o->rectX0 > g_scrollX + 0x280)
            continue;
        if (o->rectY1 < g_scrollY)
            continue;
        if (o->rectY0 > g_scrollY + 0x1e0)
            continue;
        PORT_THISCALL2(&Build_Packet_0x09, void, g_packetBuilder, uint8_t *, int)(pkt, RAWF(int32_t, o, 0x78));
        reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket(pkt);
    }
}
HOOK_FUNC(0x0044e640, SingleClickVisible);

// 0x0044e750  size 0x1be
// Connect to the login server: seed rand with the tick count, start at a random login server and try each
// in turn until the connection object (0x0069e0ec, handle at +4, -1 = none) has a socket; then the same
// over the backup servers. Returns 1 when connected, 0 (and clears 0x006f493c) when every try failed.
int __cdecl ConnectLoginServer()
{
    void *conn;
    int remaining;
    int count;
    int r;
    int idx;

    g_flag6f493c = 1;
    conn = g_pktSender;
    if (RAWF(int32_t, conn, 4) != -1)
        return 1;
    srand(GetTickCount());
    count = g_loginServerCount;
    remaining = count;
    if (count != 0)
        g_loginServerIndex = rand() % count;
    else
        g_loginServerIndex = 0;
    SelectLoginAddress(0);
    r = reinterpret_cast<CClientSocket *>(g_pktSender)->Connect(g_hostIp, g_hostPort);   // CClientSocket::Connect
    while (r == 0) {
        idx = g_loginServerIndex;
        if (idx >= 0) {
            idx = idx + 1;
            g_loginServerIndex = idx;
            if (idx == g_loginServerCount)
                g_loginServerIndex = 0;
        }
        conn = g_pktSender;
        remaining = remaining - 1;
        if (RAWF(int32_t, conn, 4) != -1)
            break;
        if (remaining == 0)
            break;
        SelectLoginAddress(0);
        r = reinterpret_cast<CClientSocket *>(g_pktSender)->Connect(g_hostIp, g_hostPort);
    }
    count = g_loginServerCount;
    if (count != 0)
        g_loginServerIndex = rand() % count;
    else
        g_loginServerIndex = 0;
    conn = g_pktSender;
    if (RAWF(int32_t, conn, 4) == -1) {
        count = g_backupServerCount;
        remaining = count;
        if (count != 0) {
            g_backupServerIndex = rand() % count;
            SelectLoginAddress(1);
            r = reinterpret_cast<CClientSocket *>(g_pktSender)->Connect(g_hostIp, g_hostPort);
            while (r == 0) {
                idx = g_backupServerIndex;
                if (idx >= 0) {
                    idx = idx + 1;
                    g_backupServerIndex = idx;
                    if (idx == g_backupServerCount)
                        g_backupServerIndex = 0;
                }
                conn = g_pktSender;
                remaining = remaining - 1;
                if (RAWF(int32_t, conn, 4) != -1)
                    break;
                if (remaining == 0)
                    break;
                SelectLoginAddress(1);
                r = reinterpret_cast<CClientSocket *>(g_pktSender)->Connect(g_hostIp, g_hostPort);
            }
        }
    }
    conn = g_pktSender;
    if (RAWF(int32_t, conn, 4) == -1) {
        g_flag6f493c = 0;
        return 0;
    }
    return 1;
}
HOOK_FUNC(0x0044e750, ConnectLoginServer);
