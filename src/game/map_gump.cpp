#include "map_gump.h"

#include "../runtime/ported.h"
#include "globals.h"
#include "obj.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "yes_no_gump.h"
#include "object_manager.h"
#include "gump_manager.h"
#include "status_gump.h"
#include "map_world_ops_ports.h"
#include "packet_builders_batch24_ports.h"
#include "client_socket.h"
#include "client_misc_ops_ports.h"
#include "gump.h"
#include "small_methods_u1_ports.h"
#include "vtables.h"
#include "class_0049c690.h"

#undef g_objMgr   // the class header's typed form; this file addresses the object by offset
#define g_objMgr GLOBAL_PTR(void, g_objMgr_storage)        // object manager: world list tail/head, serial list at +0x24
#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)

// 0x00444180  size 0x108
// CMapGump constructor.
CMapGump::CMapGump()
{
    name = const_cast<char *>("map gump");
    dirty = 0;
    unk_a4 = 0;
    unk_a0 = 0;
    unk_9c = 0;
    unk_98 = 0;
    unk_ac = 0;
    unk_a8 = 0;
    unk_94 = 0;
    unk_70 = 0;
    unk_6c = 0;
    unk_b4 = 0;
    unk_b0 = 0;
    unk_bc = 0;
    unk_b8 = 0;
    unk_cc = 0;
    unk_c8 = 0;
    unk_c4 = 0;
    unk_c0 = 0;
    y = 0;
    x = 0;
    owner = nullptr;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    frameIndex = 0;
    frameCount = 1;
    int32_t *fd = FrameData_Alloc(g_frameDataAllocator, 8, "framedata for CMapGump");
    frameData = fd;
    fd[0] = 0;
    fd[1] = 0;
    height = 0;
    width = 0;
    rowStride = 0;
    unk_d4 = 0;
    unk_d8 = 0;
    CGump_AddChild(this, nullptr, 1);
}
HOOK_CTOR(0x00444180, CMapGump, ());

// 0x004449f0  size 0x76
// Confirmation dialog "This will lower notoriety !". The SEH frame is omitted.
void __fastcall ConfirmLowerNotorietyA(void *self)
{
    g_dlgOwner = self;
    CGump *dlg = new CYesNoGump("This will lower notoriety !", reinterpret_cast<void *>(PORT_ADDR(&U1_LowerNotorietyYes)), reinterpret_cast<void *>(PORT_ADDR(&DeleteGumpCallback)), 0);
    CGump_AddChild(dlg, nullptr, 1);
}
HOOK_FUNC(0x004449f0, ConfirmLowerNotorietyA);

// 0x00444ee0  size 0x19a
// Left button down at (x, y): pick up / start dragging the object under the cursor.
void __thiscall CMapGump::OnLButtonDown(int px, int py)
{
    unk_a4 = 1;
    if (decomp::vcall(PORT_RET(int), g_player, 0x8c) == 0) {
        CObj *obj = PORT_THISCALL2(&ObjectManager_GetObjectAtScreen, CObj *, g_objMgr, int, int)(px, py);   // object under the cursor
        if (obj == nullptr)
            return;
        if (decomp::vcall(PORT_RET(int), obj, 0x24) != 0) {   // obj is a mobile
            CGump *g = reinterpret_cast<CGumpManager *>(g_gumpManager)->FindGumpForObject(obj, 0, 0, g_gumpListTail);
            if (g == nullptr) {
                g = new CStatusGump(obj, 1);
                // height * width * 2
                g->unk_58 = PORT_THISCALL2(&Map_Alloc, int, g_frameDataAllocator, int, const char *)(g->height * g->width * 2, "gump background");
                CGump_AddChild(g, nullptr, 1);
            }
            RAWF(int32_t, g, 0xe0) = 1;
            g->unk_6c = g->unk_24 / 2;
            g->unk_70 = g->unk_28 / 2;
            GLOBAL_AS(CGump *, g_dragGump) = g;
            g_dragGump2 = g;
            g_statusBlock = obj;
            char pkt[0x20];   // packet buffer (the builder writes a 10-byte command)
            PORT_THISCALL3(&Build_Packet_0x34, void, g_packetBuilder, char *, int, int)(pkt, 4, RAWF(int32_t, obj, 0x78));
            reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(pkt));
            return;
        }
        // not a mobile
        if (g_holdingFlag != 0)
            return;
        if (decomp::vcall(PORT_RET(int), obj, 0x5c) == 0) {
            if (PORT_FUNC0(&Misc_Return0, int, __cdecl)() == 0)
                return;
            if (PORT_FUNC0(&Misc_Return0, int, __cdecl)() == 0)
                return;
        }
        reinterpret_cast<CObjectManager *>(g_objMgr)->BeginPickUp((CDynamicObj *)(obj), (uint16_t)(0));
    }
    unk_98 = 0;
    unk_ac = 0;
}
HOOK_METHOD(0x00444ee0, CMapGump, OnLButtonDown);
