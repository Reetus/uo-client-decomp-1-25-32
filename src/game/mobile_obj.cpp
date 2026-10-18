// The constructors and the destructor of the mobile class (CMobile, vtable 0x0049fa60). They live in a file of their
// own because the empty constructor and destructor of the 10-byte step element (MobileStep: 0x0040cf20 and 0x0040caf0)
// are paired here, and one translation unit can pair an original address only once.
#include "obj.h"
#include "obj_inline.h"

#include "../runtime/ported.h"
#include "app.h"
#include "globals.h"
#include "map_world.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "world_obj_ops.h"
#include "mobile_anim_queue.h"
#include "container_gump.h"
#include "packet_builders_b25_ports.h"
#include "client_socket.h"
#include "small_methods_u1_ports.h"
#include "circle_trans.h"
#include "vtables.h"

#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)
#define g_tickIndexHead GLOBAL_PTR(void, g_tickIndexHead_storage)
#define g_mobileSeed GLOBAL_AS(int, g_gameTick)

HOOK_DTOR(0x0040caf0, MobileStep);
HOOK_CTOR(0x0040cf20, MobileStep, ());

// 0x00453ef0  size 0x15f
// Default constructor of the mobile class, final vtable 0x0049fa60. The dynamic object constructor runs first, then
// the container constructor (inline), then the members; the 4 steps are vector-constructed (0x0040cf20).
CMobile::CMobile()
{
    listPrev = nullptr;
    unk_13c = 0;
    moveTick = 0;
    moving = 0;
    flag148 = 0;
    unk_150 = 0;
    stepCount = 0;
    unk_194 = 0;
    zMoving = 0;
    unk_1a4 = 0;
    unk_1a8 = 0;
    animFrame = 0;
    unk_1ac = 0;
    unk_1b8 = 0;
    unk_1bc = 0;
    gumpList1c0 = nullptr;
    unk_1c4 = 0;
    unk_1c8 = 0;
    unk_1cc = 0;
    unk_1d0 = -1;
    gump1d4 = nullptr;
    unk_1d8 = 0;
    CMobile *oldHead = g_mobileListHead;
    listNext = oldHead;   // link into the mobile list: next = old head
    if (oldHead != nullptr)
        oldHead->listPrev = this;
    g_mobileListHead = this;
    for (int i = 0; i < 0x1e; i++)   // rep stosd, ecx = 0x1e, edi = this + 0xc4
        equip[i] = nullptr;
    facing = 0;
    for (int i = 0; i < 10; i++)   // rep stosd, ecx = 0xa, edi = this + 0x154
        reinterpret_cast<int32_t *>(steps)[i] = 0;
    partList = nullptr;
    unk_14c = g_mobileSeed;
    g_mobileCount = g_mobileCount + 1;
}
HOOK_CTOR(0x00453ef0, CMobile, ());

// 0x00454090  size 0x17a
// Constructor taking (serial, graphic, byte); identical to the default one except that the three arguments are
// stored after the two clears. RET 0xc.
CMobile::CMobile(int ser, int gfx, uint8_t b3a)
{
    listPrev = nullptr;
    unk_13c = 0;
    moveTick = 0;
    moving = 0;
    flag148 = 0;
    unk_150 = 0;
    stepCount = 0;
    unk_194 = 0;
    zMoving = 0;
    unk_1a4 = 0;
    unk_1a8 = 0;
    animFrame = 0;
    unk_1ac = 0;
    unk_1b8 = 0;
    unk_1bc = 0;
    gumpList1c0 = nullptr;
    unk_1c4 = 0;
    unk_1c8 = 0;
    unk_1cc = 0;
    unk_1d0 = -1;
    gump1d4 = nullptr;
    unk_1d8 = 0;
    CMobile *oldHead = g_mobileListHead;
    listNext = oldHead;
    if (oldHead != nullptr)
        oldHead->listPrev = this;
    g_mobileListHead = this;
    for (int i = 0; i < 0x1e; i++)
        equip[i] = nullptr;
    facing = 0;
    for (int i = 0; i < 10; i++)
        reinterpret_cast<int32_t *>(steps)[i] = 0;
    serial = ser;
    graphic = static_cast<uint16_t>(gfx & 0x3fff);
    unk_3a = b3a;
    partList = nullptr;
    unk_14c = g_mobileSeed;
    g_mobileCount = g_mobileCount + 1;
}
HOOK_CTOR(0x00454090, CMobile, (int, int, uint8_t));

// 0x00454210  size 0x1f1
// Destructor of the mobile class (vtable 0x0049fa60). The SEH frame is omitted. The +0x1c0 list and the
// gump scan read next BEFORE the delete.
CMobile::~CMobile()
{
    char pkt[0x34];

    PendingUpdate_Apply(0, serial, 1);   // ECX = 0, EDX = serial, push 1
    if (inWorld != 0)
        this->CMobile::RemoveFromWorld();
    if (unk_1d0 != -1) {
        if (g_suppressFlag == 0)
            PORT_FUNC1(&CMobileAnimQueue::FinishAnim, void, __fastcall, CMobile *)(this);
    }
    CGump *n = gumpList1c0;
    while (n != nullptr) {
        CGump *nx = RAWF(CGump *, n, 0x100);
        if (n != nullptr)
            decomp::vdelete(n);
        n = nx;
    }
    CGump *g = g_gumpListTail;
    while (g != nullptr) {
        CGump *gnext = g->prev;
        if (decomp::vcall(PORT_RET(int), g, 0xf0) != 0) {
            if (RAWF(void *, g, 0x9c) == this) {
                if (g != nullptr)
                    decomp::vdelete(g);
            }
        }
        g = gnext;
    }
    if (gump1d4 != nullptr) {
        void *o = RAWF(void *, gump1d4, 0xb8);
        decomp::thiscall_at_void(PORT_ADDR(&Pkt_Build_0x6F_SecureTrade), g_packetBuilder, pkt, 1, RAWF(int32_t, RAWF(void *, o, 0x34), 0x78), 0, 0);
        decomp::thiscall_at_void(PORT_ADDR(&CClientSocket::SendPacket), g_pktSender, pkt);
        gump1d4->owner = nullptr;
        if (gump1d4 != nullptr)
            decomp::vdelete(gump1d4);
    }
    void *r = decomp::thiscall_at(PORT_RET(void *), PORT_ADDR(&CObjList::Find), g_tickIndexHead, this);
    if (r != nullptr)
        decomp::thiscall_at_void(PORT_ADDR(&CObjList::Remove), g_tickIndexHead, r);
    CPlayer *pl = static_cast<CPlayer *>(g_player);
    if (pl != nullptr) {
        if (pl->unk_1f8 != 0) {
            if (serial == pl->unk_1fc)
                pl->unk_1f8 = 0;
        }
    }
    if (unlinked == 0)
        this->CDynamicObj::ToVoid();
    for (int i = 0; i < 0x1e; i++) {   // 30 equipment slots at +0xc4
        CObj *p = equip[i];
        if (p != nullptr)
            decomp::vcall_void(p, 4);
    }
    if (listNext != nullptr)   // unlink from the mobile list (next +0xbc, prev +0xc0)
        listNext->listPrev = listPrev;
    if (listPrev != nullptr)
        listPrev->listNext = listNext;
    else
        g_mobileListHead = listNext;
    g_mobileCount = g_mobileCount - 1;
}
HOOK_DTOR(0x00454210, CMobile);
HOOK_VTABLE(0x0049fa60, CMobile);
HOOK_SDD(0x00454060, CMobile);
