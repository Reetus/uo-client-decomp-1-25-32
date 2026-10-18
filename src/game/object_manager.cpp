// CObjectManager (0x0068ce58) and the class with vtable 0x0049fb90.
// 0x00454da0 .. 0x00457230. See object_manager.h for the layout.
//
// The SEH frames the original wraps around operator new + constructor calls
// are omitted. Objects whose classes are not complete (mobiles, gumps) are
// reached with RAWF at their original byte offsets and virtual calls go
// through decomp::vcall at the original vtable byte offsets.
#include "object_manager.h"

#include <cstdlib>
#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "obj.h"
#include "world_obj_ops.h"
#include "mobile_anim_queue.h"
#include "object_manager_ops_ports.h"
#include "bark_gump.h"
#include "anim_data_ports.h"
#include "window_helpers_31_ports.h"
#include "cstr.h"
#include "client_misc_ops_ports.h"
#include "gump_manager.h"
#include "packet_builders_batch24_ports.h"
#include "client_socket.h"
#include "drag_gump.h"
#include "obj_pool_alloc_ports.h"
#include "mobile_idle.h"
#include "vtables.h"

// ---------------------------------------------------------------------------
// Globals used by this file (the ones shared with other files are in the headers).
// ---------------------------------------------------------------------------
#define g_playerObj GLOBAL_AS(void *, g_player)            // g_player as a raw pointer
#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)
#define g_tileTable (g_objMgr.tileItems)                    // DAT_0068ceb8

// ---------------------------------------------------------------------------
// Original code not ported yet.
// ---------------------------------------------------------------------------
// CObj family (ECX = object)
static void Obj_BaseCtor(void *o) { reinterpret_cast<CObj *>(o)->Ctor(); }                     // 0x78-byte base ctor
static void Obj_BaseDtor(void *o) { reinterpret_cast<CObj *>(o)->Dtor(); }
static void Obj_ToWorld(void *o, uint32_t xy, uint32_t z) { reinterpret_cast<CObj *>(o)->ToWorld(xy, (uint16_t)(z)); }
static void Obj_ToVoid(void *o) { reinterpret_cast<CObj *>(o)->CObj::ToVoid(); }
static bool Obj_InRange(void *o, int range) { return reinterpret_cast<CDynamicObj *>(o)->IsInRange(range); }
static bool Obj_IsReachable(void *o) { return reinterpret_cast<CDynamicObj *>(o)->IsReachable(); }
static void Obj_SetNotifyDirty(void *o) { reinterpret_cast<CWorldObj *>(o)->InvalidateEquipment(); }
static int Obj_TryWalkStep(void *o) { return reinterpret_cast<CMobileAnimQueue *>(o)->FinishAnim(); }
static void Mobile_IdleA(void *o) { reinterpret_cast<CMobileIdle *>(o)->RandomIdleAnim(); }
static void Mobile_IdleB(void *o) { PORT_THISCALL0(&Combat_FaceTarget, void, o)(); }
static void Obj_UpdateFacing(void *o, uint32_t v) { CDynamicObj_SetStatusFlags((CDynamicObj *)(o), v); }
static void Obj_ToContainer(void *o, void *container, ObjCoords pos) { PORT_THISCALL2(&CDynamicObj::ToContainer, void, o, void *, ObjCoords)(container, pos); }
static void *Obj_CreateBark(void *o, const char *text, uint32_t hue, uint32_t font, int param)
{
    return reinterpret_cast<CObj *>(o)->CreateBarkKeepTwo(text, (uint16_t)(hue), (uint16_t)(font), (CGump *)(param));
}
static void Light_Update(void *o) { reinterpret_cast<CBarkGump *>(o)->Reposition(); }                      // per bark gump of a mobile
static void Light_Finish(void *o) { reinterpret_cast<CBarkGump *>(o)->StackAndClamp(); }
static void Misc_Refresh() { PORT_THISCALL0(&ObjectManager_UpdateAnimations, void, GLOBAL_PTR(void, g_animData_storage))(); }
static void OutOfMemory(const void *ecx, const void *edx) { ShowMessageBoxAndExit((const char *)(ecx), (const char *)(edx), 0); }
// CObjectManager (ECX = 0x0068ce58)
static uint16_t Mgr_GroundZ(CObjectManager *m, uint16_t x, uint16_t y) { return (uint16_t)m->CObjectManager::GroundZ((int)(x), (int)(y)); }
// Label records
static void LabelNode_Dtor(void *n) { reinterpret_cast<LabelNode *>(n)->FreeRecord(); }
static void LabelRec_Dtor(void *rec) { reinterpret_cast<LabelRecord *>(rec)->DtorText(); }   // = string dtor on rec+8
static void Str_Ctor(void *s) { reinterpret_cast<CStr *>(s)->Ctor(); }
static void Str_Assign(void *s, const char *v) { reinterpret_cast<CStr *>(s)->Assign(v); }
static void Str_Dtor(void *s) { reinterpret_cast<CStr *>(s)->Dtor(); }
static char *Str_Get(void *s) { return reinterpret_cast<CStr *>(s)->GetBuffer(); }                // cdecl-style: pushes left by the caller
// Misc
static int IsGameMaster() { return Misc_Return0(); }
static void *Gump_Create(void *obj, int type, int parent, int flag)
{
    return (void *)reinterpret_cast<CGumpManager *>(g_gumpManager)->CreateGump((CObj *)(obj), type, (CGump *)(parent), flag);
}
static void Pkt_BuildPickup(uint8_t *pkt, uint32_t serial, uint32_t qty)
{
    PORT_THISCALL3(&Build_Packet_0x07, void, g_packetBuilder, uint8_t *, uint32_t, uint32_t)(pkt, serial, qty);
}
static void Net_Send(const uint8_t *pkt) { reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket(pkt); }
static void *Item_PoolNew(uint32_t serial, uint32_t graphic, uint32_t param)
{
    return DynamicObj_PoolNew(serial, graphic, param);
}

// Signed byte of the step-frame table for `graphic` (index 0: walk, 1: run).
static int step_frames(void *o, int which)
{
    return static_cast<int>(g_stepFrameTable[RAWF(uint16_t, o, 0x38) * 2 + which]);
}

// Step length in ticks of the step in progress.
static int step_ticks(void *o)
{
    if (RAWF(int32_t, o, 0x198) != 0)
        return step_frames(o, 0) + (RAWF(int32_t, o, 0x19c) / 11) * 2;
    if (RAWF(int32_t, o, 0x144) != 0)
        return step_frames(o, 1);
    return step_frames(o, 0);
}

// Is `o` one of the graphics (or mounts) that count as a walking step in place?
static bool is_walk_graphic(void *o)
{
    uint32_t g = RAWF(uint16_t, o, 0x38) & 0x3fff;
    if (g == 0xd || g == 0xf || g == 0x10 || g == 0x23e)
        return true;
    if (static_cast<int>(g) >= 0x190) {
        void *mount = RAWF(void *, o, 0xcc);
        if (mount != nullptr) {
            uint32_t f = decomp::vcall(PORT_RET(uint32_t), mount, 0x40);
            if ((f & 0x800000) != 0)
                return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// 0x00454da0  size 0xa7
// Constructor of the class with vtable 0x0049fb90.
// ---------------------------------------------------------------------------
CObjVt49fb90::CObjVt49fb90(uint32_t graphicId, uint16_t tileX, uint16_t tileY)
{
    CObjVt49fb90 *oldHead = static_cast<CObjVt49fb90 *>(g_objMgr.firstFb90);
    next = oldHead;
    if (oldHead != nullptr)
        oldHead->prev = this;
    prev = nullptr;
    g_objMgr.firstFb90 = this;
    unk_3a = 0;
    graphic = static_cast<uint16_t>(graphicId & 0x3fff);
    uint16_t groundZ = Mgr_GroundZ(&g_objMgr, tileX, tileY);
    Obj_ToWorld(this, static_cast<uint32_t>(tileX) | (static_cast<uint32_t>(tileY) << 16), groundZ);
}
HOOK_CTOR(0x00454da0, CObjVt49fb90, (uint32_t, uint16_t, uint16_t));

// ---------------------------------------------------------------------------
// 0x00454e90  size 0x7f
// Destructor of the class with vtable 0x0049fb90.
// ---------------------------------------------------------------------------
CObjVt49fb90::~CObjVt49fb90()
{
    if (unlinked == 0)
        Obj_ToVoid(this);
    if (next != nullptr)
        next->prev = prev;
    if (prev != nullptr)
        prev->next = next;
    if (g_objMgr.firstFb90 == this)
        g_objMgr.firstFb90 = next;
}
HOOK_DTOR(0x00454e90, CObjVt49fb90);
HOOK_VTABLE(0x0049fb90, CObjVt49fb90);
HOOK_SDD(0x00454e60, CObjVt49fb90);

// ---------------------------------------------------------------------------
// 0x00454f10  size 0xa4
// Object manager constructor: clears the lists, allocates the two tiledata buffers.
// ---------------------------------------------------------------------------
CObjectManager::CObjectManager()
{
    listTail = nullptr;
    listHead = nullptr;
    unk_2c = 0;
    unk_08 = 0;
    unk_0c = 0;
    unk_10 = 0;
    unk_14 = 0;
    list18 = nullptr;
    unk_1c = 0;
    unk_20 = 0;
    firstFb90 = nullptr;
    dragGump = nullptr;
    tileItems = static_cast<uint8_t *>(::operator new(0xa0000));      // item tiledata, 0x4000 * 0x28
    tileTerrain = static_cast<uint8_t *>(::operator new(0x70000));    // terrain tiledata
    LoadTileData();                                                 // loads tiledata into the buffers
    heldState = 0;
    unk_68 = 0x4000;
    unk_6c = 0x4000;
    MobileAnim_ClearActive();
    unk_5c = 0;
}
HOOK_CTOR(0x00454f10, CObjectManager, ());

// ---------------------------------------------------------------------------
// 0x00454fc0  size 0xc9
// Object manager destructor: frees the tiledata buffers; the label list member then frees its nodes.
// ---------------------------------------------------------------------------
CObjectManager::~CObjectManager()
{
    if (tileItems != nullptr)
        operator delete(tileItems);
    if (tileTerrain != nullptr)
        operator delete(tileTerrain);
}
HOOK_DTOR(0x00454fc0, CObjectManager);

// ---------------------------------------------------------------------------
// 0x00456140  size 0x646
// Per-tick object update. One pass over the list at +0x18 (refresh by range),
// one over the list at +0x24 (mobile walk steps, pixel offsets).
// ---------------------------------------------------------------------------

// Pass 2 body for one object; returns the object to continue with.
static void *update_one(void *o)
{
    void *nextObj = RAWF(void *, o, 0x80);
    void *ed = nextObj;
    int hadStep = 1;
    int redraw;

    if (RAWF(int32_t, o, 0x7c) == 0 && !Obj_InRange(o, 0x12)) {
        if (ed != nullptr) {
            for (;;) {
                if (RAWF(int32_t, ed, 0x7c) == 0)
                    break;
                ed = RAWF(void *, ed, 0x80);
                if (ed == nullptr)
                    break;
            }
            nextObj = ed;
        }
        decomp::vcall_void(o, 0x04);
        return nextObj;
    }

    if (decomp::vcall(PORT_RET(int), o, 0x28) != 0) {                        // slot 0x28: player mobile
        if (RAWF(uint8_t, o, 0x6c) != 0)
            return nextObj;
        void *p = g_playerObj;
        if (RAWF(int32_t, p, 0x148) == 0)
            return nextObj;
        bool hit = is_walk_graphic(p);
        p = g_playerObj;
        if (hit) {
            if (RAWF(int32_t, p, 0x1c) != 0) {
                decomp::vcall_void(p, 0x14);
                g_walkTick = g_walkTick + 1;
                p = g_playerObj;
                decomp::vcall_void(p, 0x10);
                p = g_playerObj;
            } else {
                g_walkTick = g_walkTick + 1;
            }
        }
        if (RAWF(int32_t, p, 0x150) != 0)
            Mobile_IdleB(p);
        else if (RAWF(int32_t, p, 0x1a4) == 0)
            Mobile_IdleA(p);
        return nextObj;
    }
    if (decomp::vcall(PORT_RET(int), o, 0x24) == 0)                          // slot 0x24: is mobile
        return nextObj;

    // ---- mobile movement step ----
    int edv = 0;
    redraw = 0;
    if (RAWF(int32_t, o, 0x13c) != 0 || RAWF(int32_t, o, 0x184) > 0) {
        // a step is in progress or queued
        RAWF(uint32_t, o, 0x14c) = g_gameTick;
        if (RAWF(int32_t, o, 0x1c) != 0) {
            Obj_SetNotifyDirty(o);
            decomp::vcall_void(o, 0x14);
            redraw = 1;
        }
        if (RAWF(int32_t, o, 0x1a4) != 0) {
            RAWF(int32_t, o, 0x1a4) = 0;
            edv = Obj_TryWalkStep(o);
        } else {
            edv = 0;
        }
        RAWF(int32_t, o, 0x140) = RAWF(int32_t, o, 0x140) + 1;
    } else if (RAWF(int32_t, o, 0x148) == 0) {
        // idle, not yet "settled"
        if (static_cast<uint32_t>(g_gameTick - RAWF(uint32_t, o, 0x14c)) > 0x7d0) {
            if (RAWF(int32_t, o, 0x1c) != 0) {
                decomp::vcall_void(o, 0x14);
                redraw = 1;
            }
            RAWF(int32_t, o, 0x148) = 1;
            RAWF(int32_t, o, 0x140) = 3;
        }
    } else {
        // settled
        if (RAWF(int32_t, o, 0x150) == 0 && RAWF(int32_t, o, 0x1a4) == 0)
            Mobile_IdleA(o);
        if (is_walk_graphic(o)) {
            if (RAWF(int32_t, o, 0x1c) != 0) {
                decomp::vcall_void(o, 0x14);
                redraw = 1;
            }
            RAWF(int32_t, o, 0x140) = RAWF(int32_t, o, 0x140) + 1;
        }
    }
    if (edv != 0)
        return nextObj;

    hadStep = 0;
    if (RAWF(int32_t, o, 0x13c) != 0) {
        RAWF(int32_t, o, 0x13c) = RAWF(int32_t, o, 0x13c) - 1;
        hadStep = 1;
    }
    if (RAWF(int32_t, o, 0x13c) == 0 && RAWF(int32_t, o, 0x184) > 0) {
        // pop the next queued step (10-byte entries at o+0x154)
        int dx = static_cast<int>(RAWF(int16_t, o, 0x20)) - static_cast<int>(RAWF(int16_t, o, 0x154));
        int adx = dx < 0 ? -dx : dx;
        int dy = static_cast<int>(RAWF(int16_t, o, 0x22)) - static_cast<int>(RAWF(int16_t, o, 0x156));
        int ady = dy < 0 ? -dy : dy;
        uint8_t fl = RAWF(uint8_t, o, 0x15c);
        int bz = static_cast<int>(RAWF(int16_t, o, 0x158));
        RAWF(uint8_t, o, 0x6e) = fl & 0x7f;
        int dz = static_cast<int>(RAWF(int16_t, o, 0x24)) - bz;
        int climb = (dz >= 0x16);
        RAWF(int32_t, o, 0x198) = climb;
        if (climb)
            RAWF(int32_t, o, 0x19c) = dz;
        RAWF(int32_t, o, 0x144) = static_cast<uint32_t>(fl) >> 7;
        RAWF(uint16_t, o, 0x38) = RAWF(uint16_t, o, 0x15a) & 0x3fff;
        RAWF(uint32_t, o, 0x17c) = RAWF(uint32_t, o, 0x20);
        RAWF(uint16_t, o, 0x180) = RAWF(uint16_t, o, 0x24);
        RAWF(uint32_t, o, 0x20) = RAWF(uint32_t, o, 0x154);
        RAWF(uint16_t, o, 0x24) = RAWF(uint16_t, o, 0x158);
        Obj_UpdateFacing(o, RAWF(uint8_t, o, 0x15d));
        if (adx < 2 && ady < 2 && ady + adx > 0)
            RAWF(int32_t, o, 0x13c) = step_ticks(o) - 1;
        int ecx = 1;
        uint8_t *q = static_cast<uint8_t *>(o) + 0x154;
        if (RAWF(int32_t, o, 0x184) > ecx) {
            do {
                uint8_t *src = q + 0xa;
                uint8_t *dst = q;
                ecx++;
                q += 0xa;
                RAWF(uint32_t, dst, 0) = RAWF(uint32_t, src, 0);
                RAWF(uint32_t, dst, 4) = RAWF(uint32_t, src, 4);
                RAWF(uint16_t, dst, 8) = RAWF(uint16_t, src, 8);
            } while (ecx < RAWF(int32_t, o, 0x184));
        }
        RAWF(int32_t, o, 0x184) = RAWF(int32_t, o, 0x184) - 1;
    }

    // per-tick pixel offsets for the step in progress
    int n = RAWF(int32_t, o, 0x13c);
    RAWF(int32_t, o, 0x188) = 0;
    RAWF(int32_t, o, 0x18c) = 0;
    if (n != 0) {
        int t = step_ticks(o);
        if (t == 0) {
            if (g_zeroStepWarned == 0)
                OutOfMemory(GLOBAL_PTR(void, g_004ad1b0_storage), GLOBAL_PTR(void, g_004ad1c4_storage));
            t = 1;
            g_zeroStepWarned = g_zeroStepWarned + 1;
        }
        n = RAWF(int32_t, o, 0x13c);
        switch (RAWF(uint8_t, o, 0x6e)) {
        case 0:
            RAWF(int32_t, o, 0x188) = (n * 22) / t;
            RAWF(int32_t, o, 0x18c) = (-(n * 11) * 2) / t;
            break;
        case 1:
            RAWF(int32_t, o, 0x18c) = 0;
            RAWF(int32_t, o, 0x188) = ((n * 11) * 4) / t;
            break;
        case 2:
            RAWF(int32_t, o, 0x188) = (n * 22) / t;
            RAWF(int32_t, o, 0x18c) = RAWF(int32_t, o, 0x188);
            break;
        case 3:
            RAWF(int32_t, o, 0x188) = 0;
            RAWF(int32_t, o, 0x18c) = ((n * 11) * 4) / t;
            break;
        case 4:
            RAWF(int32_t, o, 0x188) = (-(n * 11) * 2) / t;
            RAWF(int32_t, o, 0x18c) = (n * 22) / t;
            break;
        case 5:
            RAWF(int32_t, o, 0x188) = (-(n * 11) * 4) / t;
            RAWF(int32_t, o, 0x18c) = 0;
            break;
        case 6:
            RAWF(int32_t, o, 0x188) = (-(n * 11) * 2) / t;
            RAWF(int32_t, o, 0x18c) = RAWF(int32_t, o, 0x188);
            break;
        case 7:
            RAWF(int32_t, o, 0x188) = 0;
            RAWF(int32_t, o, 0x18c) = (-(n * 11) * 4) / t;
            break;
        default:
            break;
        }
    }
    if (redraw != 0) {
        decomp::vcall_void(o, 0x88);
        decomp::vcall_void(o, 0x10);
        if (hadStep != 0)
            Obj_SetNotifyDirty(o);
    }
    void *bark = RAWF(void *, o, 0x68);
    if (bark != nullptr && hadStep != 0) {
        do {
            Light_Update(bark);
            bark = RAWF(void *, bark, 0xac);
        } while (bark != nullptr);
        void *c = RAWF(void *, o, 0x68);
        if (c != nullptr)
            Light_Finish(c);
    }
    return nextObj;
}

void __thiscall CObjectManager::UpdateMovement()
{
    // pass 1: list at this+0x18 (next at +0x60)
    void *o = list18;
    if (o != nullptr) {
        do {
            bool r = Obj_InRange(o, 0x12);
            uint8_t b = RAWF(uint8_t, o, 0x6c);
            if (r) {
                if (b != 0)
                    Obj_ToWorld(o, RAWF(uint32_t, o, 0x20), RAWF(uint16_t, o, 0x24));
            } else {
                if (b == 0)
                    decomp::vcall_void(o, 0x50);
            }
            o = RAWF(void *, o, 0x60);
        } while (o != nullptr);
    }

    // pass 2: list at this+0x24 (next at +0x80)
    o = list24;
    while (o != nullptr)
        o = update_one(o);
    Misc_Refresh();
}
HOOK_METHOD(0x00456140, CObjectManager, UpdateMovement);

// ---------------------------------------------------------------------------
// 0x00456b80  size 0x1ef
// Begin picking up an object: fill the held-item state, send the pickup
// packet, create the drag gump.
// ---------------------------------------------------------------------------
void __thiscall CObjectManager::BeginPickUp(CDynamicObj *obj, uint16_t qty)
{
    uint8_t pkt[12];

    if (!Obj_IsReachable(obj)) {
        if (IsGameMaster() == 0)
            return;
    }
    if (qty == 0 && obj->unk_3c > 1 &&
        (tileItems[(obj->graphic + static_cast<int>(static_cast<signed char>(obj->unk_3a))) * 0x28 + 1] & 8) != 0) {
        void *g = Gump_Create(obj, 0x31, 0, 0);
        RAWF(int32_t, g, 0x94) = 1;
        RAWF(int32_t, g, 0x6c) = RAWF(int32_t, g, 0x1c) / 2;
        RAWF(int32_t, g, 0x70) = RAWF(int32_t, g, 0x20) / 2;
        g_dragGump = g;
        return;
    }
    memcpy(&heldPos, &RAWF(uint32_t, obj, 0x20), sizeof(uint32_t));
    heldPos.z = RAWF(uint16_t, obj, 0x24);
    heldGraphic = obj->graphic;
    heldOffset = obj->unk_3a;
    heldSerial = static_cast<uint32_t>(obj->serial);
    uint32_t hue = obj->hue;
    uint32_t hi = hue & 0x8000;
    uint32_t lo = hue & 0x7fff;
    if (lo != 0) {
        if (lo < 3000)
            hi = lo | hi;
        else
            hi = hi | 1;
    }
    heldHue = static_cast<uint16_t>(hi);
    if (qty == 0)
        qty = obj->unk_3c;
    heldQty = qty;
    heldFlag40 = obj->flags98;
    heldTileFlags = *reinterpret_cast<uint32_t *>(tileItems + (obj->graphic + static_cast<int>(static_cast<signed char>(obj->unk_3a))) * 0x28);
    CObj *parent = obj->container;
    heldContainer = nullptr;
    heldMobile = nullptr;
    if (parent != nullptr) {
        if (decomp::vcall(PORT_RET(int), parent, 0x24) != 0) {                // slot 0x24: is mobile
            heldMobile = parent;
            int layer = 1;
            CDynamicObj **slot = reinterpret_cast<CDynamicObj **>(reinterpret_cast<uint8_t *>(parent) + 0xc8);
            do {
                if (*slot == obj)
                    heldLayer = static_cast<uint16_t>(layer);
                layer++;
                slot++;
            } while (layer < 0x1e);
        } else {
            if (decomp::vcall(PORT_RET(int), parent, 0x30) != 0)              // slot 0x30: is container
                heldContainer = parent;
        }
    }
    decomp::vcall_void(obj, 0x04);
    heldState = 1;
    Pkt_BuildPickup(pkt, heldSerial, heldQty);
    Net_Send(pkt);
    dragGump = new CDragGump();
}
HOOK_METHOD(0x00456b80, CObjectManager, BeginPickUp);

// ---------------------------------------------------------------------------
// 0x00456da0  size 0x259
// Create an object (mobile / item / corpse) and place it in a container or mobile.
// RET 0x20: serial, graphic, param, qty, x, y, parentSerial, hue.
// ---------------------------------------------------------------------------
void __thiscall CObjectManager::CreateObject(uint32_t serial, uint32_t graphic, uint32_t param, uint16_t qty,
                                             uint16_t x, uint16_t y, int parentSerial, uint16_t hue)
{
    void *parent;
    if (static_cast<uint32_t>(parentSerial) == g_playerSerial) {
        parent = g_playerObj;
    } else {
        void *p = list24;
        for (;;) {
            if (p == nullptr) {
                parent = nullptr;
                break;
            }
            if (RAWF(int32_t, p, 0x78) == parentSerial) {
                parent = p;
                break;
            }
            p = RAWF(void *, p, 0x80);
        }
    }
    if (parent == nullptr)
        return;
    if (decomp::vcall(PORT_RET(int), parent, 0x30) == 0)                      // slot 0x30: is container
        return;

    void *existing = list24;
    for (;;) {
        if (existing == nullptr)
            break;
        if (RAWF(uint32_t, existing, 0x78) == serial)
            break;
        existing = RAWF(void *, existing, 0x80);
    }
    if (existing != nullptr)
        decomp::vcall_void(existing, 0x04);

    void *obj;
    if ((serial & 0x40000000) != 0) {
        if (static_cast<uint16_t>(graphic) == 0x2006) {
            obj = new CObjVt49f9e0(static_cast<int>(serial), 0x2006);   // corpse
        } else {
            uint32_t flags = *reinterpret_cast<uint32_t *>(tileItems + (graphic & 0xffff) * 0x28);
            if ((flags & 0x100000) != 0) {
                obj = new CObjVt49f960(static_cast<int>(serial), static_cast<int>(graphic), static_cast<uint8_t>(param), 1);
            } else if ((flags & 0x200000) != 0) {
                obj = new CContainerObj(static_cast<int>(serial), static_cast<int>(graphic), static_cast<uint8_t>(param), 1);
            } else {
                obj = Item_PoolNew(serial, graphic & 0xffff, param & 0xff);
            }
        }
    } else {
        obj = new CMobile(static_cast<int>(serial), static_cast<int>(graphic), static_cast<uint8_t>(param));   // mobile
    }

    uint32_t h = hue & 0xffff;
    uint32_t hi = h & 0x8000;
    uint32_t lo = h & 0x7fff;
    if (lo != 0) {
        if (lo < 0xbb8)
            hi = lo | hi;
        else
            hi = hi | 1;
    }
    // The created object is not null-checked before use (as in the original).
    RAWF(uint16_t, obj, 0x3c) = qty;
    RAWF(uint16_t, obj, 0x3e) = static_cast<uint16_t>(hi);
    ObjCoords pos = { x, y, 0, 0 };
    Obj_ToContainer(obj, parent, pos);
}
HOOK_METHOD(0x00456da0, CObjectManager, CreateObject);

// ---------------------------------------------------------------------------
// 0x00457030  size 0x20
// Tiledata flag 0x100000 of `graphic`. __stdcall (RET 4).
// ---------------------------------------------------------------------------
uint32_t __stdcall ObjectManager_TileFlagTest_0x100000(uint32_t graphic)
{
    return *reinterpret_cast<uint32_t *>(g_tileTable + (graphic & 0xffff) * 0x28) & 0x100000;
}
HOOK_FUNC(0x00457030, ObjectManager_TileFlagTest_0x100000);

// ---------------------------------------------------------------------------
// 0x00457080  size 0x1ae
// Queue a label record for each child of `gump` (pending overhead-name list).
// ---------------------------------------------------------------------------
void __thiscall CObjectManager::QueueLabels(CGump *gump)
{
    void *child = RAWF(void *, gump, 0x68);
    if (child == nullptr)
        return;
    do {
        LabelRecord *rec = new LabelRecord(RAWF(uint32_t, gump, 0x78), RAWF(const char *, child, 0xd4),
                                           RAWF(uint16_t, child, 0xd8), RAWF(uint16_t, child, 0xda),
                                           RAWF(uint32_t, child, 0x94), RAWF(uint32_t, child, 0x98));
        LabelRecord *recOut = rec;

        LabelNode *tail = labels.tail;
        LabelNode *node;
        if (tail != nullptr) {
            LabelNode *tailNext = tail->next;
            node = static_cast<LabelNode *>(::operator new(0xc));
            if (node != nullptr) {
                node->next = nullptr;
                node->prev = nullptr;
                node->record = nullptr;
            } else {
                node = nullptr;
            }
            // (not null-checked in the original)
            node->next = tailNext;
            node->prev = tail;
            tail->next = node;
            if (tailNext != nullptr) {
                tailNext->prev = node;
                labels.count++;
            } else {
                LabelNode *head = labels.head;
                labels.tail = node;
                if (head == nullptr)
                    labels.head = node;
                labels.count = labels.count + 1;
            }
        } else {
            node = static_cast<LabelNode *>(::operator new(0xc));
            if (node != nullptr) {
                node->next = nullptr;
                node->prev = nullptr;
                node->record = nullptr;
            } else {
                node = nullptr;
            }
            labels.tail = node;
            labels.head = node;
            labels.count = labels.count + 1;
        }

        if (node != nullptr) {
            LabelRecord *old = node->record;
            if (old != nullptr) {
                old->text.~CStr();
                operator delete(old);
                node->record = nullptr;
            }
            node->record = recOut;
        }
        child = RAWF(void *, child, 0xac);
    } while (child != nullptr);
}
HOOK_METHOD(0x00457080, CObjectManager, QueueLabels);

// ---------------------------------------------------------------------------
// 0x00457230  size 0xe2
// Flush the pending label records that belong to `obj`.
// ---------------------------------------------------------------------------
void __thiscall CObjectManager::FlushLabels(CDynamicObj *obj)
{
    LabelNode *node = labels.tail;
    if (node == nullptr)
        return;
    do {
        LabelRecord *rec = node->record;
        if (rec->serial == static_cast<uint32_t>(obj->serial)) {
            uint32_t savedFlag = g_labelFlag;
            g_labelFlag = 1;
            uint16_t a = rec->hue;
            uint16_t b = rec->font;
            char *s = Str_Get(&rec->text);
            // 0x00453320 pops 0x10 bytes (text, hue, font, param). The original pushes 0, EDX, EAX before the
            // string getter (which does not pop them) and then the text, so the extra arguments are
            // EAX = (rec->serial & 0xffff0000) | hue, EDX = (obj->serial & 0xffff0000) | font, and 0.
            uint32_t hueArg = (rec->serial & 0xffff0000u) | a;
            uint32_t fontArg = (static_cast<uint32_t>(obj->serial) & 0xffff0000u) | b;
            void *gump = Obj_CreateBark(obj, s, hueArg, fontArg, 0);
            g_labelFlag = savedFlag;
            RAWF(uint32_t, gump, 0x94) = rec->unk_1c;
            RAWF(uint32_t, gump, 0x98) = rec->unk_20;
            LabelRecord *dead = nullptr;
            LabelNode *prevNode = node->prev;
            LabelNode *nextNode = node->next;
            if (prevNode != nullptr)
                prevNode->next = nextNode;
            else
                labels.head = nextNode;
            if (nextNode != nullptr)
                nextNode->prev = prevNode;
            else
                labels.tail = prevNode;
            dead = node->record;
            node->record = nullptr;
            operator delete(node);
            labels.count = labels.count - 1;
            if (dead != nullptr) {
                LabelRec_Dtor(dead);
                operator delete(dead);
            }
            node = prevNode;
        } else {
            node = node->prev;
        }
    } while (node != nullptr);
}
HOOK_METHOD(0x00457230, CObjectManager, FlushLabels);
