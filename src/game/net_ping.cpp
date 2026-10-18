// Server-list UDP ping: 0x004667b0.
//
// Sends 20 probe datagrams to every server of the list, waits (up to 8 seconds
// after the last send) for the echoes and stores per server the sum of the 20
// round trip times (200 ms substituted for a lost probe). All socket calls go
// through the Winsock thunks of the original image (jmp through the import
// table): socket 0x00498652, bind 0x00498670, ioctlsocket 0x00498640,
// sendto 0x0049866a, recvfrom 0x00498664, closesocket 0x00498634.
#include <winsock.h>
#include <windows.h>

#include <cstdint>
#include <cstring>

#include "../runtime/ported.h"
#include "gump_manager.h"
#include "mouse_manager.h"
#include "packet_cursor.h"
#include "globals.h"

#define g_pingRtt(i) (g_pingRtts[(i)])        // [server * 20 + round]
#define g_pingSendTick(i) (g_pingSendTicks[(i)])

namespace {
struct PingAddr {                 // sockaddr_in
    uint16_t family;
    uint16_t port;
    uint32_t addr;
    uint8_t zero[8];
};
static_assert(sizeof(PingAddr) == 0x10, "PingAddr");


void ping_message(uint32_t text) { CreateGump2ac(static_cast<int>(text), 0, 0, 0); }   // 0x00439f60

// Mouse busy cursor bracket: IncBusy (0x004468b0) / DecBusy (0x004468c0), then the cursor 0x11
// (0x00446840) set with the gate at 0x005285c8 forced to 1.
void ping_cursor()
{
    int32_t saved = g_inputActive;
    g_inputActive = 1;
    g_mouse.SetMouseCursor(0x11);   // 0x00446840
    g_inputActive = saved;
}
} // namespace

// ---------------------------------------------------------------------------
// 0x004667b0  size 0x3aa
// Ping every server of the list. No arguments.
// Call sites folded into the wrappers above: 0x00439f60 (CreateGump2ac, 4x), 0x00446840
// (SetMouseCursor, 2x), 0x00498652 socket, 0x00498640 ioctlsocket, 0x0049866a sendto,
// 0x00498664 recvfrom, 0x00498670 bind, 0x00498634 closesocket.
// Deviation: the 0x28-byte probe buffer is zeroed, the original leaves it
// uninitialised after the port, server and round bytes.
// ---------------------------------------------------------------------------
void __cdecl Net_PingServerList()
{
    PingAddr dest[32];
    uint8_t sendPkt[0x28];
    uint8_t recvBuf[0x28];
    PingAddr local;
    int failed;
    int round;
    uint32_t tick;
    int sock;
    int i;

    g_mouse.IncBusy();   // 0x004468b0
    ping_cursor();

    sock = socket(2, 2, 0);
    if (sock == -1) {
        ping_message(GADDR(str_004add64));   // 0x00439f60 "Socket Creation Failed"
        goto done;
    }

    std::memset(&local, 0, sizeof(local));
    local.family = 2;
    local.port = htons(g_pingPort);
    if (bind(sock, reinterpret_cast<const sockaddr *>(&local), 0x10) == -1) {
        ping_message(GADDR(str_004add58));   // 0x00439f60 "Bind Failed"
    } else {
        uint32_t nonblock = 1;
        ioctlsocket(sock, 0x8004667eu, reinterpret_cast<u_long *>(&nonblock));

        std::memset(dest, 0, sizeof(dest));
        for (i = 0; i < g_serverCount; i++) {
            dest[i].family = 2;
            dest[i].addr = htonl(g_pingServerIps[i]);
            dest[i].port = htons(g_pingPort);
        }

        std::memset(sendPkt, 0, sizeof(sendPkt));
        for (round = 0; round < 0x14; round++) {
            if (g_serverCount > 0) {
                for (i = 0; i < g_serverCount; i++) {
                    g_pingRtt(i * 0x14 + round) = 0;
                    uint16_t portNet = htons(g_pingPort);
                    std::memcpy(sendPkt, &portNet, 2);
                    sendPkt[2] = static_cast<uint8_t>(i);
                    sendPkt[3] = static_cast<uint8_t>(round);
                    tick = GetTickCount();
                    failed = 0;
                    if (sendto(sock, reinterpret_cast<const char *>(sendPkt), 0x28, 0, reinterpret_cast<const sockaddr *>(&dest[i]), 0x10) < 0) {   // 0x0049866a
                        tick += 4000;
                        for (;;) {
                            if (static_cast<int32_t>(tick) < static_cast<int32_t>(GetTickCount())) {
                                failed = 1;
                                break;
                            }
                            if (sendto(sock, reinterpret_cast<const char *>(sendPkt), 0x28, 0, reinterpret_cast<const sockaddr *>(&dest[i]), 0x10) >= 0)
                                break;
                        }
                    }
                    if (failed != 0) {
                        ping_message(GADDR(str_004add38));   // 0x00439f60 "Couldn't send for 4 seconds."
                        break;
                    }
                    g_pingSendTick(i * 0x14 + round) = GetTickCount();
                }
            }
        }

        tick = GetTickCount();
        failed = 0;
        int got = 0;
        do {
            recvBuf[2] = 0xff;
            recvBuf[3] = 0xff;
            if (recvfrom(sock, reinterpret_cast<char *>(recvBuf), 0x28, 0, nullptr, nullptr) < 0) {   // 0x00498664
                int32_t deadline = static_cast<int32_t>(tick) + 8000;
                for (;;) {
                    if (deadline < static_cast<int32_t>(GetTickCount())) {
                        failed = 1;
                        break;
                    }
                    if (recvfrom(sock, reinterpret_cast<char *>(recvBuf), 0x28, 0, nullptr, nullptr) >= 0)
                        break;
                }
            }
            if (failed == 0) {
                uint32_t now = GetTickCount();
                int srv = static_cast<int8_t>(recvBuf[2]);
                int rnd = static_cast<int8_t>(recvBuf[3]);
                if (srv < 0 || srv >= g_serverCount || rnd >= 0x14) {
                    ping_message(GADDR(str_004add24));   // 0x00439f60 "Bogus return ping"
                } else {
                    int idx = rnd + srv * 0x14;
                    int32_t rtt = static_cast<int32_t>(now - g_pingSendTick(idx));
                    g_pingRtt(idx) = rtt;
                    if (!(rtt > 0))
                        g_pingRtt(idx) = 1;
                }
                got++;
                if (got == g_serverCount * 0x14)
                    failed = 1;
            }
        } while (failed == 0);
    }

    // Per-server sum of the 20 round trips; 200 for a probe that never came back.
    {
        int rttIndex = 0;
        for (i = 0; i < g_serverCount; i++) {
            g_serverPing[i] = 0;
            for (int k = 0x14; k != 0; k--) {
                int32_t v = g_pingRtt(rttIndex);
                rttIndex++;
                if (v == 0)
                    v = 200;
                g_serverPing[i] = static_cast<uint16_t>(g_serverPing[i] + static_cast<uint16_t>(v));
            }
        }
    }
    closesocket(sock);

done:
    g_mouse.DecBusy();   // 0x004468c0
    ping_cursor();
}
HOOK_FUNC(0x004667b0, Net_PingServerList);
