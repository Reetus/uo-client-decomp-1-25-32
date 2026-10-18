#include "gump_callbacks.h"

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "crt.h"
#include "packet_builders_batch24_ports.h"
#include "client_socket.h"
#include "resize_gump.h"
#include "globals.h"

// The SEH frame of the original (try level around operator new) is omitted.


// 0x00412210  size 0x65
void __thiscall CGumpCallbacks::SendIdRequest()
{
    char text[128];
    char packet[0x178 - 0x80];   // ? only the base (ebp-0x178) is known from the LEA, the size is approximate
    int v = unk_08;
    sprintf(text, "%d %d", v, 0);
    // 0x00463170: thiscall on the object at 0x0069e0e0, (packet, 0x24, text, strlen(text)), RET 0x10
    PORT_THISCALL4(&Build_Packet_0x12, void, GLOBAL_PTR(void, g_packetBuilder_storage), char *, int, char *, size_t)(packet, 0x24, text, strlen(text));
    // 0x00478610: thiscall on the object pointed to by 0x0069e0ec, (packet), RET 4
    reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(packet));
    g_macroLastItem = v;
}
HOOK_METHOD(0x00412210, CGumpCallbacks, SendIdRequest);

// 0x00412330  size 0xa3
void __thiscall CGumpCallbacks::AddBackdropChild()
{
    // RET 0x24, (x, y, w, h, w, 0xdc, w, 0x1cc - y, parent); the original dereferences g below even when new failed
    CGump *g = new CResizeGump(x, y, width, height, width, 0xdc, width, 0x1cc - y, this);
    g->unk_58 = static_cast<int32_t>(reinterpret_cast<uintptr_t>(gfx::AllocTagged(g->height * g->width * 2, "gump background")));
    gfx::AddChild(g, nullptr);
}
HOOK_METHOD(0x00412330, CGumpCallbacks, AddBackdropChild);
