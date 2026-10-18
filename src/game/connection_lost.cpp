// Connection-lost handler (0x00476ef0).
#include <windows.h>

#include <cstdint>

#include "../runtime/ported.h"
#include "batch06_helpers.h"
#include "globals.h"
#include "gump_manager.h"
#include "client_socket_ports.h"
#include "app_session_ops_ports.h"
#include "client_socket.h"
#include "screen_flow_ports.h"
#include <winsock.h>
#include "vtables.h"
#include "crt.h"

#define g_alertGump GLOBAL_AS(void *, g_waitingGump)
#define g_socketObject GLOBAL_AS(uint32_t *, g_pktSender)   // socket object (0x20040 bytes, vtable 0x004a0830)

// 0x00439f60 on the gump manager: create an alert gump (text, 0, 0, 1)
#define GumpManager_Alert(o, a1, a2, a3, a4) PORT_THISCALL4(&CreateGump2ac, void *, (o), const char *, int, int, int)(a1, a2, a3, a4)
// 0x00477310: lazy global table init, no arguments
#define Socket_InitTables() (BuildHuffmanTree())
// 0x0044e250 with ECX = 0
#define Net_Reset(a1) (SelectLoginAddress(a1))
// 0x00476d30 on the socket: connect (ip, port)
#define Socket_Connect(self, a1, a2) (reinterpret_cast<CClientSocket *>(self)->Connect(a1, (uint32_t)(uint16_t)(a2)))
// 0x0044e360: ECX = player name, EDX = account, stack = key
#define Net_Login(a1, a2, a3) (ShowEnteringBritannia((int)(a1), (int)(a2), a3))
// 0x00436d20 on the pixel-format / display object 0x0068dd98
#define Display_Pump(o) PORT_THISCALL0(&DeathFadeStep, void, (o))()
// Winsock thunks in the original image (same Winsock instance as the rest of the client).

// 0x00476ef0  size 0x1a6  connection-lost handler: shows "Connection lost", counts down, then recreates the socket and reconnects
// __stdcall with one argument that is never read (RET 4). The SEH frame (try level around the socket construction) is omitted.
void __stdcall ConnectionLostTick(int unusedArg)
{
    (void)unusedArg;
    CClientSocket *s;

    if (g_flag6f493c == 0)
        return;

    if (g_alertGump == nullptr) {
        g_alertGump = GumpManager_Alert(g_gumpManager, "Connection lost", 0, 0, 1);
    }

    g_reconnectCountdown = g_reconnectCountdown - 1;
    if (g_reconnectCountdown <= 0) {
        g_reconnectCountdown = 0xd;
        if (g_inputActive == 0) {
            if (g_socketObject != nullptr) {
                b6_delete_gump(g_socketObject);   // deleting destructor, vtable slot 0, arg 1
            }
            s = new CClientSocket();
            g_socketObject = reinterpret_cast<uint32_t *>(s);
            Net_Reset(0);
            Socket_Connect(g_socketObject, g_hostIp, g_hostPort);
            if (g_socketObject[1] != 0xffffffff) {
                Net_Login(g_playerName, g_password, g_selectedCharSlot);
            }
        }
    }
    Display_Pump(b6_pixel_format);
    Sleep(0x4c);
}
HOOK_FUNC(0x00476ef0, ConnectionLostTick);
