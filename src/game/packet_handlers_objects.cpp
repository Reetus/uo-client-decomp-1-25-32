// Inbound packet handlers for speech, objects and mobiles (second pass, batch 21):
//   0x1C ascii speech 0x00457850        0x4F global light 0x004577f0       0x76 new subserver 0x00458080
//   0x1D delete object 0x00458220       0x20 draw player 0x004582b0        0x22 move accept 0x00458580
//   0x77 mobile moving 0x004585f0       0x89 corpse equip 0x00458850       0x1A world item 0x00458cc0
//   0x39 group 0x00458f50               0x21 move reject 0x00459400        0x25 add to container 0x00459820
//   0x3C container contents 0x004599a0  0x28 / 0x29 drop clear 0x00459f00 / 0x00459f70
//   0x2F fight occurring 0x00459f90     0x53 client code 0x00459ff0        0x2D mobile attributes 0x0045a0f0
//   0xA1 / 0xA2 / 0xA3 hits / mana / stamina bar 0x0045a3b0 / 0x0045a500 / 0x0045a650
//   0x2E equip item 0x0045a7a0
// plus two stdcall stubs at 0x004577d0 (RET 4, argument unused) and the constructor / scalar deleting
// destructor of the class with vtable 0x0049fc10 (0x00457780 / 0x00457790).
//
// All handlers are __stdcall (RET 4) with the packet pointer. The first field is at "base" (3 for a
// variable-length opcode, 1 otherwise), see packet_cursor.h. The SEH frame of 0x0045a7a0 around its
// operator new + constructor calls is omitted. Several handlers push bytes read from the packet as
// dwords whose upper three bytes are stack leftovers in the original; they are passed zero-extended.
#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "container_gump.h"
#include "globals.h"
#include "gump.h"
#include "gump_lookup.h"
#include "gump_manager.h"
#include "map_world.h"
#include "music_manager.h"
#include "object_manager.h"
#include "crt.h"
#include "obj_00457780.h"
#include "packet_cursor.h"
#include "radar_gump.h"
#include "raw_field.h"
#include "status_gump.h"
#include "vcall.h"
#include "walk.h"
#include "obj.h"
#include "obj_pool_alloc_ports.h"
#include "dirty_rects_ports.h"
#include "app_support_ports.h"
#include "text_gump_ctor_ports.h"
#include "window_helpers_31_ports.h"
#include "weather.h"
#include "app_session_ops_ports.h"
#include "client_helpers_31.h"
#include "vtables.h"

// ---------------------------------------------------------------------------
// Globals (single-file use).
// ---------------------------------------------------------------------------
#define g_msgTarget GLOBAL_PTR(void, g_msgTarget_storage)               // on-screen message / journal object
#define g_lightLevel GLOBAL_AS(uint32_t, g_lightAmbient)
#define g_backpackGump GLOBAL_AS(void *, g_openContainerGump)
#define g_containerLoading GLOBAL_AS(uint32_t, g_redrawBlock)
#define g_barkGumpList GLOBAL_AS(void *, g_barkListHead)

// ---------------------------------------------------------------------------
// Original code not ported yet.
// ---------------------------------------------------------------------------
static inline uint32_t Hue_Fix(uint32_t hue) { return CObj_SanitizeHue(hue); }
static inline void *Item_PoolNew(uint32_t serial, uint32_t graphic, uint32_t param)
{
    return DynamicObj_PoolNew(serial, graphic, param);
}
static inline void Obj_ToMobile(void *item, void *mobile, uint32_t layer)    // CObj::ToEquipped
{
    reinterpret_cast<CObj *>(item)->ToEquipped((CMobile *)(mobile), (int)(layer));
}
static void Mgr_AddOrUpdateObject(uint32_t serial, uint32_t graphic, uint32_t param, uint32_t qty, uint32_t x,
                                  uint32_t y, uint32_t z, uint32_t dir, uint32_t hue, uint32_t flags)
{
    (&g_objMgr)->CObjectManager::AddOrUpdateObject(serial, graphic, (int8_t)(param), (uint16_t)(qty), (int)(x), (int)(y), (int)(z), (uint8_t)(dir), hue, flags);
}
static int Mob_PendingUpdate(uint32_t ecx, uint32_t edx)               // 0x004522f0
{
    return PendingUpdate_Has(ecx, edx);
}
void __fastcall Profanity_Apply(char *text);                           // 0x00457460, object_manager_ops.cpp

// Raw big-endian field reads at a byte offset of the packet (the original reads them inline).
#define rd32(p, off) (*reinterpret_cast<const uint32_t *>((p) + (off)))
#define rd16(p, off) (*reinterpret_cast<const uint16_t *>((p) + (off)))

// ---------------------------------------------------------------------------
// 0x00457780  size 0x9   / 0x00457790  size 0x23
// Constructor and scalar deleting destructor of a class that is only a vtable pointer
// (vtable 0x0049fc10). Placeholder name by address.
// ---------------------------------------------------------------------------
CObj_00457780::CObj_00457780()
{
}
HOOK_CTOR(0x00457780, CObj_00457780, ());

inline CObj_00457780::~CObj_00457780()
{
}
HOOK_VTABLE(0x0049fc10, CObj_00457780);
HOOK_SDD(0x00457790, CObj_00457780);   // slot 0

// 0x004577c0  size 0x7
// The destructor as the atexit thunk 0x0044aa90 jumps to it (ECX = object): only the vptr store.
void __fastcall Obj457780_Dtor(void *self)
{
    reinterpret_cast<CObj_00457780 *>(self)->CObj_00457780::~CObj_00457780();
}
HOOK_FUNC(0x004577c0, Obj457780_Dtor);

// ---------------------------------------------------------------------------
// 0x004577d0  size 0x11   (RET 4, the argument is not read)
// Calls 0x0047d580 on the object at 0x00528698.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle_004577d0(uint8_t *packet)
{
    (void)packet;
    reinterpret_cast<CObj0047d490 *>(GLOBAL_PTR(void, g_regionMusic_storage))->NopIfVersioned();
}
HOOK_FUNC(0x004577d0, Packet_Handle_004577d0);

// ---------------------------------------------------------------------------
// 0x004577f0  size 0x57
// Inbound 0x4F: global light level (one byte). When it changes, redraw the radar and invalidate.
// ---------------------------------------------------------------------------
void __stdcall Packet_GlobalLight(uint8_t *packet)
{
    uint32_t level = packet[pkt::Base(packet)];
    if (g_lightLevel != level) {
        g_lightLevel = level;
        if (g_radarGump != nullptr)
            g_radarGump->Redraw();                       // 0x0046a4f0
        InvalidateAll();           // InvalidateAll
    }
}
HOOK_FUNC(0x004577f0, Packet_GlobalLight);

// ---------------------------------------------------------------------------
// 0x00457850  size 0x30c
// Inbound 0x1C: ascii speech: serial, body, type, hue, font, name[30], text.
// ---------------------------------------------------------------------------
void __stdcall Packet_AsciiSpeech(uint8_t *packet)
{
    char buf[0x600];

    int base = pkt::Base(packet);
    uint32_t serial = ntohl(rd32(packet, base));
    ntohs(rd16(packet, base + 4));                   // body, discarded
    uint8_t type = packet[base + 6];
    uint32_t hue = ntohs(rd16(packet, base + 7));
    uint32_t font = ntohs(rd16(packet, base + 9));
    char *name = reinterpret_cast<char *>(packet) + base + 0xb;
    char *text = reinterpret_cast<char *>(packet) + base + 0x29;

    if (name == nullptr || text == nullptr || g_suppressSpeech != 0)
        return;
    if (g_profanityOn != 0)
        Profanity_Apply(text);                            // 0x00457460

    const char *msg = text;
    switch (type) {
    case 0:
    case 2:
    case 3:
    case 4:
    case 8:
    case 9: {
        CDynamicObj *obj = g_objMgr.FindBySerial(serial);
        if (obj != nullptr) {
            if (App_IsInList_0069e8b0(serial) != 0)
                return;
            strcpy(g_barkPrefix, name);
            g_barkState = 2;
            obj->CreateBarkKeepTwo(text, static_cast<uint16_t>(hue), static_cast<uint16_t>(font), nullptr);
            return;
        }
        break;
    }
    case 1:
        if (serial == 0xffffffffu)
            sprintf(buf, "[System]: %s", text);
        else
            sprintf(buf, "%s: %s", name, text);
        msg = buf;
        break;
    case 6: {
        CDynamicObj *obj = nullptr;
        if (serial != 0xffffffffu)
            obj = g_objMgr.FindBySerial(serial);
        if (obj != nullptr) {
            if (App_IsInList_0069e8b0(serial) != 0)
                return;
            strcpy(g_barkPrefix, name);
            g_barkState = 3;
            // The "this" of 0x00436f70 is an item here (it reads this+0x7c, the container).
            CGump *owner = reinterpret_cast<CGump_00436f70 *>(obj)->FindOpenGumpForTarget();
            obj->CreateBarkKeepTwo(text, static_cast<uint16_t>(hue), static_cast<uint16_t>(font), owner);
            return;
        }
        break;
    }
    case 7: {
        CDynamicObj *obj = g_objMgr.FindBySerial(serial);
        if (obj != nullptr) {
            if (App_IsInList_0069e8b0(serial) != 0)
                return;
            strcpy(g_barkPrefix, name);
            g_barkState = 2;
            obj->CreateBarkReplace(text, static_cast<uint16_t>(hue), static_cast<uint16_t>(font), nullptr);
            return;
        }
        break;
    }
    default:
        return;
    }
    PORT_THISCALL3(&CreateBarkGump, void, g_msgTarget, uint32_t, uint32_t, const char *)(hue & 0xffff, font & 0xffff, msg);   // journal / bark on the object at 0x006486b0
}
HOOK_FUNC(0x00457850, Packet_AsciiSpeech);

// ---------------------------------------------------------------------------
// 0x00458080  size 0x199
// Inbound 0x76: new subserver: x, y, z (unused), map id byte, then four scene words.
// ---------------------------------------------------------------------------
void __stdcall Packet_NewSubserver(uint8_t *packet)
{
    int base = pkt::Base(packet);
    ntohs(rd16(packet, base));                       // discarded
    ntohs(rd16(packet, base + 2));                   // discarded
    ntohs(rd16(packet, base + 4));                   // discarded
    uint8_t mapId = packet[base + 6];
    uint32_t w7 = ntohs(rd16(packet, base + 7));
    uint32_t w9 = ntohs(rd16(packet, base + 9));
    uint32_t wb = ntohs(rd16(packet, base + 0xb));
    uint32_t wd = ntohs(rd16(packet, base + 0xd));
    g_sceneW3 = wd & 0xffff;
    g_sceneW2 = wb & 0xffff;
    g_sceneW0 = w7 & 0xffff;
    g_sceneW1 = w9 & 0xffff;
    g_map.SetPos(-1, -1, -1, mapId);                      // 0x00445f00
    GLOBAL_AS(uint32_t, g_flag6f479c) = 1;
}
HOOK_FUNC(0x00458080, Packet_NewSubserver);

// ---------------------------------------------------------------------------
// 0x00458220  size 0x8c
// Inbound 0x1D: delete object by serial (not the player; a mobile with a pending update is kept).
// ---------------------------------------------------------------------------
void __stdcall Packet_DeleteObject(uint8_t *packet)
{
    uint32_t serial = ntohl(rd32(packet, pkt::Base(packet)));
    CDynamicObj *obj = g_objMgr.FindBySerial(serial);
    if (obj == nullptr || obj == reinterpret_cast<CDynamicObj *>(g_player))
        return;
    if (decomp::vcall(PORT_RET(int), obj, 0x24) != 0) {             // is mobile
        if (Mob_PendingUpdate(0, static_cast<uint32_t>(obj->serial)) != 0)
            return;
    }
    if (static_cast<CMobile *>(g_player)->equip[0] == obj)
        g_objMgr.heldState = 0;                           // 0x0068cec8
    g_objMgr.QueueLabels(reinterpret_cast<CGump *>(obj)); // 0x00457080 (its argument is the object)
    decomp::vcall_void(obj, 0x04);
}
HOOK_FUNC(0x00458220, Packet_DeleteObject);

// ---------------------------------------------------------------------------
// 0x004582b0  size 0x2ce
// Inbound 0x20: draw player: serial, body, hue, flags, x, y, (word), direction, z.
// ---------------------------------------------------------------------------
void __stdcall Packet_DrawPlayer(uint8_t *packet)
{
    int base = pkt::Base(packet);
    uint32_t serial = ntohl(rd32(packet, base));
    uint32_t body = ntohs(rd16(packet, base + 4));
    uint32_t hue = ntohs(rd16(packet, base + 7));
    uint8_t flags = packet[base + 9];
    uint32_t x = ntohs(rd16(packet, base + 0xa));
    uint32_t y = ntohs(rd16(packet, base + 0xc));
    uint32_t unkWord = ntohs(rd16(packet, base + 0xe));
    uint8_t facing = packet[base + 0x10];
    int8_t z = static_cast<int8_t>(packet[base + 0x11]);

    if (serial != g_playerSerial) {
        // Error box: text 0x004ad4f4, caption "Error" (0x004a81bc).
        ShowMessageBoxAndExit(
            "Z change received for non-player object", "Error", 0);
        return;
    }

    if (g_backpackGump != nullptr) {
        decomp::vdelete(g_backpackGump);
        g_backpackGump = nullptr;
    }
    g_walkFlag648860 = 0;
    g_map.SetPos(static_cast<int>(x & 0xffff), static_cast<int>(y & 0xffff), z, static_cast<int>(unkWord & 0xffff));

    CMobile *player = static_cast<CMobile *>(g_player);
    decomp::vcall_void(player, 0x14);
    g_player->x = static_cast<int16_t>(x);
    g_player->y = static_cast<int16_t>(y);
    g_player->z = z;
    g_player->graphic = static_cast<uint16_t>(body & 0x3fff);
    g_player->facing = facing;
    decomp::vcall_void(g_player, 0x88);
    decomp::vcall_void(g_player, 0x10);
    g_player->hue = static_cast<uint16_t>(Hue_Fix(hue & 0xffff));
    uint8_t oldFlags = static_cast<CDynamicObj *>(g_player)->flags98;
    static_cast<CDynamicObj *>(g_player)->flags98 = flags;
    g_walk.Reset(-1, -1, -1, -1);                         // 0x00449a20
    PORT_THISCALL0(&CWeather::Stop, void, GLOBAL_PTR(void, g_weatherObj_storage))();   // weather stop

    if (oldFlags != static_cast<CDynamicObj *>(g_player)->flags98) {
        CGump *gump = g_gumpListTail;
        while (gump != nullptr) {
            if (decomp::vcall(PORT_RET(int), gump, 0xac) != 0 && gump->owner == g_player)
                static_cast<CStatusGump *>(gump)->RedrawFrame(1, 1, 1, 1);   // 0x00468e00
            gump = gump->prev;
        }
    }
}
HOOK_FUNC(0x004582b0, Packet_DrawPlayer);

// ---------------------------------------------------------------------------
// 0x00458580  size 0x6e
// Inbound 0x22: move accepted: sequence byte, notoriety byte.
// ---------------------------------------------------------------------------
void __stdcall Packet_MoveAccept(uint8_t *packet)
{
    int base = pkt::Base(packet);
    uint8_t seq = packet[base];
    int8_t notoriety = static_cast<int8_t>(packet[base + 1]);
    if (g_player != nullptr)
        decomp::vcall_void(g_player, 0x94, static_cast<int>(notoriety));
    g_walk.HandleMoveAck(seq);                            // 0x004498e0
}
HOOK_FUNC(0x00458580, Packet_MoveAccept);

// ---------------------------------------------------------------------------
// 0x004585f0  size 0x251
// Inbound 0x77: mobile moving: serial, body, x, y, z, direction, hue, flags, notoriety.
// ---------------------------------------------------------------------------
void __stdcall Packet_MobileMoving(uint8_t *packet)
{
    int base = pkt::Base(packet);
    uint32_t serial = ntohl(rd32(packet, base));
    uint32_t body = ntohs(rd16(packet, base + 4));
    uint32_t x = ntohs(rd16(packet, base + 6));
    uint32_t y = ntohs(rd16(packet, base + 8));
    uint8_t z = packet[base + 0xa];
    uint8_t dir = packet[base + 0xb];
    uint32_t hue = ntohs(rd16(packet, base + 0xc));
    uint8_t flags = packet[base + 0xe];
    int8_t notoriety = static_cast<int8_t>(packet[base + 0xf]);

    CDynamicObj *obj = g_objMgr.FindBySerial(serial);
    if (obj == nullptr)
        return;
    if (decomp::vcall(PORT_RET(int), obj, 0x24) != 0)               // is mobile
        decomp::vcall_void(obj, 0x94, static_cast<int>(notoriety));

    if (serial != g_playerSerial) {
        Mgr_AddOrUpdateObject(serial, body, 0, 0, x & 0xffff, y & 0xffff, z, dir, hue, flags);   // 0x00455760
        return;
    }

    CMobile *player = static_cast<CMobile *>(g_player);
    if (player == nullptr)
        return;
    uint8_t oldFlags = player->flags98;
    if (player->inWorld != 0) {                           // +0x1c
        decomp::vcall_void(player, 0x14);
        static_cast<CDynamicObj *>(g_player)->flags98 = flags;
        decomp::vcall_void(g_player, 0x10);
    } else {
        player->flags98 = flags;
    }
    if (oldFlags != static_cast<CDynamicObj *>(g_player)->flags98) {
        CGump *gump = g_gumpListTail;
        while (gump != nullptr) {
            if (decomp::vcall(PORT_RET(int), gump, 0xac) != 0 && gump->owner == g_player)
                static_cast<CStatusGump *>(gump)->RedrawFrame(1, 1, 1, 1);   // 0x00468e00
            gump = gump->prev;
        }
    }
}
HOOK_FUNC(0x004585f0, Packet_MobileMoving);

// ---------------------------------------------------------------------------
// 0x00458850  size 0xd2
// Inbound 0x89: corpse equipment: corpse serial, then (layer byte, item serial) pairs ended by layer 0.
// The corpse (slot 0x54 true) keeps the item serials in a table at +0xb8 indexed by layer.
// ---------------------------------------------------------------------------
void __stdcall Packet_CorpseEquip(uint8_t *packet)
{
    uint32_t serial = ntohl(rd32(packet, pkt::Base(packet)));
    int pos = 4;
    CDynamicObj *obj = g_objMgr.FindBySerial(serial);
    CDynamicObj *corpse = nullptr;
    if (obj != nullptr && decomp::vcall(PORT_RET(int), obj, 0x54) != 0)
        corpse = obj;
    int i = 0;
    do {
        uint8_t layer = packet[pkt::Base(packet) + pos];
        pos++;
        if (layer == 0)
            break;
        uint32_t item = ntohl(rd32(packet, pkt::Base(packet) + pos));
        pos += 4;
        if (corpse != nullptr)
            RAWF(uint32_t, corpse, 0xb8 + static_cast<unsigned>(layer) * 4) = item;
        i++;
    } while (i < 0x28);
}
HOOK_FUNC(0x00458850, Packet_CorpseEquip);

// ---------------------------------------------------------------------------
// 0x00458cc0  size 0x289
// Inbound 0x1A: world item. Serial bit 31 = has count; graphic bit 15 = has stack byte;
// x bit 15 = has direction; y bit 15 = has hue, y bit 14 = has flags.
// ---------------------------------------------------------------------------
void __stdcall Packet_WorldItem(uint8_t *packet)
{
    int base = pkt::Base(packet);
    uint32_t hasCount = 0;
    uint8_t hasDir = 0;
    uint32_t hue = 0;
    uint8_t hasFlags = 0;
    uint8_t hasStack = 0;
    uint8_t stackByte = 0;

    uint32_t serial = ntohl(rd32(packet, base));
    if ((serial & 0x80000000u) != 0) {
        serial &= 0x7fffffffu;
        hasCount = 1;
    }
    uint32_t graphic = ntohs(rd16(packet, base + 4));
    int off = 6;
    if ((graphic & 0x8000u) != 0) {
        hasStack = 1;
        graphic &= 0x7fffu;
        stackByte = hasStack;
    }
    if (hasStack != 0) {
        off = 7;
        stackByte = packet[base + 6];
    }

    uint32_t count;
    if (static_cast<uint16_t>(hasCount) != 0) {
        count = ntohs(rd16(packet, base + off));
        off += 2;
    } else {
        count = 1;
    }

    uint32_t x = ntohs(rd16(packet, base + off));
    off += 2;
    if ((x & 0x8000u) != 0) {
        hasDir = 1;
        x &= 0x7fffu;
    }

    uint32_t y = ntohs(rd16(packet, base + off));
    off += 2;
    uint32_t hueOut;
    if ((y & 0x8000u) != 0) {
        hueOut = 1;
        y &= 0x7fffu;
    } else {
        hueOut = hue;
    }
    if ((y & 0x4000u) != 0) {
        hasFlags = 1;
        y &= 0x3fffu;
    }

    uint8_t dir = 0;
    if (hasDir != 0) {
        dir = packet[base + off];
        off++;
    }
    uint8_t z = packet[base + off];
    off++;
    if (static_cast<uint16_t>(hueOut) != 0) {
        hue = ntohs(rd16(packet, base + off));
        off += 2;
        hueOut = hue;
    }
    uint8_t flags = 0;
    if (hasFlags != 0)
        flags = packet[base + off];

    if (serial != g_playerSerial)
        Mgr_AddOrUpdateObject(serial, graphic, stackByte, count, x & 0xffff, y & 0xffff, z, dir, hueOut, flags);   // 0x00455760
}
HOOK_FUNC(0x00458cc0, Packet_WorldItem);

// ---------------------------------------------------------------------------
// 0x00458f50  size 0xc5
// Inbound 0x39: group change (leader serial, member serial). The barks go over the player or the member.
// ---------------------------------------------------------------------------
void __stdcall Packet_Group(uint8_t *packet)
{
    int base = pkt::Base(packet);
    uint32_t leader = ntohl(rd32(packet, base));
    uint32_t member = ntohl(rd32(packet, base + 4));
    if (leader == 0) {
        if (member == 0) {
            g_player->CreateBarkKeepTwo("Illegal group.", 0, 3, nullptr);
            return;
        }
        if (member == 1) {
            g_player->CreateBarkKeepTwo("Removed from group.", 0, 3, nullptr);
            return;
        }
    }
    if (leader == g_playerSerial) {
        CDynamicObj *obj = g_objMgr.FindBySerial(member);
        obj->CreateBarkKeepTwo("Now grouped.", 0, 3, nullptr);   // ? the original does not test obj for null
    }
}
HOOK_FUNC(0x00458f50, Packet_Group);

// ---------------------------------------------------------------------------
// 0x00459400  size 0xdf
// Inbound 0x21: move rejected: sequence, x, y, direction, z. Resets the walk state to that spot.
// ---------------------------------------------------------------------------
void __stdcall Packet_MoveReject(uint8_t *packet)
{
    int base = pkt::Base(packet);
    uint8_t seq = packet[base];
    uint32_t x = ntohs(rd16(packet, base + 1));
    uint32_t y = ntohs(rd16(packet, base + 3));
    uint8_t dir = packet[base + 5];
    int8_t z = static_cast<int8_t>(packet[base + 6]);
    g_walk.Reset(seq, static_cast<int>(x & 0xffff), static_cast<int>(y & 0xffff), z);   // 0x00449a20
    g_player->facing = dir;
}
HOOK_FUNC(0x00459400, Packet_MoveReject);

// ---------------------------------------------------------------------------
// 0x00459820  size 0x179
// Inbound 0x25: add one item to a container: serial, graphic, stack byte, count, x, y, container, hue.
// ---------------------------------------------------------------------------
void __stdcall Packet_AddToContainer(uint8_t *packet)
{
    int base = pkt::Base(packet);
    uint32_t serial = ntohl(rd32(packet, base));
    uint32_t graphic = ntohs(rd16(packet, base + 4));
    uint8_t stackByte = packet[base + 6];
    uint32_t count = ntohs(rd16(packet, base + 7));
    uint32_t x = ntohs(rd16(packet, base + 9));
    uint32_t y = ntohs(rd16(packet, base + 0xb));
    uint32_t container = ntohl(rd32(packet, base + 0xd));
    uint32_t hue = ntohs(rd16(packet, base + 0x11));
    g_containerLoading = 0;
    g_objMgr.CreateObject(serial, graphic, stackByte, static_cast<uint16_t>(count), static_cast<uint16_t>(x),
                          static_cast<uint16_t>(y), static_cast<int>(container), static_cast<uint16_t>(hue));   // 0x00456da0
}
HOOK_FUNC(0x00459820, Packet_AddToContainer);

// ---------------------------------------------------------------------------
// 0x004599a0  size 0x2fa
// Inbound 0x3C: container contents: count word, then count items of 19 bytes (serial, graphic, stack
// byte, count, x, y, container, hue). Before the first item the container's open gump is closed
// (when the container sits in a mobile and answers slot 0xa0), then the gump is redrawn.
// ---------------------------------------------------------------------------
void __stdcall Packet_ContainerContents(uint8_t *packet)
{
    int base = pkt::Base(packet);
    uint32_t total = ntohs(rd16(packet, base)) & 0xffffu;
    uint32_t pos = 2;
    int i = 0;
    uint32_t container = 0;              // ? uninitialised in the original when the count is 0
    g_containerLoading = 1;
    if (static_cast<int32_t>(total) > 0) {
        do {
            uint32_t serial = ntohl(rd32(packet, base + pos));
            pos += 4;
            uint32_t graphic = ntohs(rd16(packet, base + pos));
            pos += 2;
            uint8_t stackByte = packet[base + pos];
            pos += 1;
            uint32_t count = ntohs(rd16(packet, base + pos));
            pos += 2;
            uint32_t x = ntohs(rd16(packet, base + pos));
            pos += 2;
            uint32_t y = ntohs(rd16(packet, base + pos));
            pos += 2;
            container = ntohl(rd32(packet, base + pos));
            pos += 4;
            uint32_t hue = ntohs(rd16(packet, base + pos));
            pos += 2;

            if (i == 0) {
                CDynamicObj *cobj = g_objMgr.FindBySerial(container);
                if (cobj != nullptr && decomp::vcall(PORT_RET(int), cobj, 0x30) != 0) {
                    CObj *outer = cobj->container;
                    if (outer != nullptr && decomp::vcall(PORT_RET(int), outer, 0x24) != 0) {
                        CGump *gump = RAWF(CGump *, cobj->container, 0x88);
                        if (gump != nullptr && decomp::vcall(PORT_RET(int), gump, 0xa0) != 0) {
                            gump = RAWF(CGump *, cobj->container, 0x88);
                            if (gump != nullptr)
                                decomp::vdelete(gump);
                        }
                    }
                    reinterpret_cast<CContainerObj *>(cobj)->DestroyChildren();
                }
            }
            g_objMgr.CreateObject(serial, graphic, stackByte, static_cast<uint16_t>(count), static_cast<uint16_t>(x),
                                  static_cast<uint16_t>(y), static_cast<int>(container), static_cast<uint16_t>(hue));   // 0x00456da0
            i++;
        } while (i < static_cast<int32_t>(total));
    }

    g_containerLoading = 0;
    CDynamicObj *cobj = g_objMgr.FindBySerial(container);
    if (cobj == nullptr)
        return;
    if (decomp::vcall(PORT_RET(int), cobj, 0x30) == 0)
        return;
    CGump *gump = RAWF(CGump *, cobj, 0xac);
    if (gump == nullptr)
        return;
    int redraw = decomp::vcall(PORT_RET(int), gump, 0xd0);
    gump = RAWF(CGump *, cobj, 0xac);
    if (redraw != 0) {
        int rect[4];
        rect[0] = 0;
        rect[1] = 0;
        rect[2] = gump->width - 1;
        rect[3] = gump->height - 1;
        (void)rect;                                       // the original builds this rect but slot 0x114 takes no argument
        decomp::vcall_void(gump, 0x114);
        return;
    }
    if (decomp::vcall(PORT_RET(int), gump, 0x90) != 0) {
        gump = RAWF(CGump *, cobj, 0xac);
        int rect[4];
        rect[0] = 0;
        rect[1] = 0;
        rect[2] = gump->width - 1;
        rect[3] = gump->height - 1;
        static_cast<CContainerGump *>(gump)->RedrawFrame(rect);   // 0x00426b70
    }
}
HOOK_FUNC(0x004599a0, Packet_ContainerContents);

// ---------------------------------------------------------------------------
// 0x00459f00  size 0x63
// Inbound 0x28: drop clear (two words, ignored): the item in hand is released.
// ---------------------------------------------------------------------------
void __stdcall Packet_DropClear_0x28(uint8_t *packet)
{
    int base = pkt::Base(packet);
    ntohs(rd16(packet, base));
    ntohs(rd16(packet, base + 2));
    g_objMgr.heldState = 0;                               // 0x0068cec8
}
HOOK_FUNC(0x00459f00, Packet_DropClear_0x28);

// ---------------------------------------------------------------------------
// 0x00459f70  size 0x11
// Inbound 0x29: drop clear: the item in hand is released.
// ---------------------------------------------------------------------------
void __stdcall Packet_DropClear_0x29(uint8_t *packet)
{
    (void)packet;
    g_objMgr.heldState = 0;                               // 0x0068cec8
}
HOOK_FUNC(0x00459f70, Packet_DropClear_0x29);

// ---------------------------------------------------------------------------
// 0x00459f90  size 0x5d
// Inbound 0x2F: fight occurring (attacker serial at +1 discarded, defender serial at +5 becomes the target).
// ---------------------------------------------------------------------------
void __stdcall Packet_FightOccurring(uint8_t *packet)
{
    int base = pkt::Base(packet);
    ntohl(rd32(packet, base + 1));                   // discarded
    GLOBAL_AS(uint32_t, g_combatTargetSerial) = ntohl(rd32(packet, base + 5));
}
HOOK_FUNC(0x00459f90, Packet_FightOccurring);

// ---------------------------------------------------------------------------
// 0x00459ff0  size 0xf3
// Inbound 0x53: client code byte. 1 closes the scroll choice gump. Any other value tears the world
// down: held item, scroll choice gump, every object's bark gumps, all bark gumps, plays track 0x2a and
// leaves war mode.
// ---------------------------------------------------------------------------
void __stdcall Packet_ClientCode(uint8_t *packet)
{
    uint8_t code = packet[pkt::Base(packet)];
    g_gumpMgr.DeleteGumpsSlot80();                        // 0x00439ff0
    if (code == 1) {
        if (GLOBAL_AS(void *, g_scrollChoiceGump) != nullptr) {
            decomp::vdelete(GLOBAL_AS(void *, g_scrollChoiceGump));
            GLOBAL_AS(void *, g_scrollChoiceGump) = nullptr;
        }
        return;
    }

    if (g_objMgr.dragGump != nullptr)                     // 0x0068ce8c
        decomp::vdelete(g_objMgr.dragGump);
    if (GLOBAL_AS(void *, g_scrollChoiceGump) != nullptr)
        decomp::vdelete(GLOBAL_AS(void *, g_scrollChoiceGump));
    CObj *o = g_objMgr.listTail;                          // 0x0068ce58
    g_suppressSpeech = 1;
    for (; o != nullptr; o = o->link40) {
        while (o->barkList != nullptr)
            decomp::vdelete(o->barkList);
    }
    while (g_barkGumpList != nullptr)
        decomp::vdelete(g_barkGumpList);
    g_musicManager.PlayTrack(0x2a, 1, 1);                 // 0x0047ef60
    CMobile *player = static_cast<CMobile *>(g_player);
    if (player != nullptr && player->unk_150 != 0)
        SetWarMode(0);  // SetWarMode
}
HOOK_FUNC(0x00459ff0, Packet_ClientCode);

// ---------------------------------------------------------------------------
// 0x0045a0f0  size 0x2b1
// Inbound 0x2D: mobile attributes: serial, hits max, hits, mana max, mana, stamina max, stamina (words).
// Stores them in the mobile (+0x1dc..+0x1f0) and refreshes its status gump.
// ---------------------------------------------------------------------------
void __stdcall Packet_MobileAttributes(uint8_t *packet)
{
    uint32_t hitsDiff = 1;               // [ebp-0x14]
    uint32_t manaDiff = 1;               // [ebp-0x18]
    uint32_t stamDiff = 1;               // [ebp-0x20]
    uint32_t flag = 0;                   // [ebp-4]

    int base = pkt::Base(packet);
    uint32_t serial = ntohl(rd32(packet, base));
    uint32_t hitsMax = ntohs(rd16(packet, base + 4));
    uint32_t hits = ntohs(rd16(packet, base + 6));
    uint32_t manaMax = ntohs(rd16(packet, base + 8));
    uint32_t mana = ntohs(rd16(packet, base + 10));
    uint32_t stamMax = ntohs(rd16(packet, base + 12));
    uint32_t stam = ntohs(rd16(packet, base + 14));

    CDynamicObj *obj = g_objMgr.FindBySerial(serial);
    if (obj == nullptr)
        return;
    if (decomp::vcall(PORT_RET(int), obj, 0x24) == 0)               // is mobile
        return;
    if (decomp::vcall(PORT_RET(int), obj, 0x28) != 0) {
        RAWF(uint32_t, obj, 0x1dc) = hits & 0xffff;
        RAWF(uint32_t, obj, 0x1e0) = hitsMax & 0xffff;
        RAWF(uint32_t, obj, 0x1e8) = manaMax & 0xffff;
        RAWF(uint32_t, obj, 0x1e4) = mana & 0xffff;
        RAWF(uint32_t, obj, 0x1f0) = stamMax & 0xffff;
        RAWF(uint32_t, obj, 0x1ec) = stam & 0xffff;
    }

    CGump *gump = g_gumpListTail;
    while (gump != nullptr) {
        if (decomp::vcall(PORT_RET(int), gump, 0xac) != 0 && gump->owner == obj)
            break;
        gump = gump->prev;
    }
    if (gump == nullptr)
        return;

    uint32_t stamDiffOut = 0;            // EAX at 0x0045a2dd
    if (RAWF(uint16_t, gump, 0xc2) == static_cast<uint16_t>(hits))
        hitsDiff = 0;
    uint32_t manaDiffOut = (RAWF(uint16_t, gump, 0xca) == static_cast<uint16_t>(mana)) ? 0 : manaDiff;
    if (RAWF(uint16_t, gump, 0xc6) != static_cast<uint16_t>(stam))
        stamDiffOut = stamDiff;
    if (RAWF(uint16_t, gump, 0xc4) != static_cast<uint16_t>(hitsMax))
        flag = 1;
    if (RAWF(uint16_t, gump, 0xcc) != static_cast<uint16_t>(manaMax))
        flag = 1;
    if (RAWF(uint16_t, gump, 0xc8) == static_cast<uint16_t>(stamMax))   // ? equal sets the flag (jne skips it)
        flag = 1;
    RAWF(uint16_t, gump, 0xc4) = static_cast<uint16_t>(hitsMax);
    RAWF(uint16_t, gump, 0xc2) = static_cast<uint16_t>(hits);
    RAWF(uint16_t, gump, 0xc6) = static_cast<uint16_t>(stam);
    RAWF(uint16_t, gump, 0xcc) = static_cast<uint16_t>(manaMax);
    RAWF(uint16_t, gump, 0xca) = static_cast<uint16_t>(mana);
    RAWF(uint16_t, gump, 0xc8) = static_cast<uint16_t>(stamMax);
    static_cast<CStatusGump *>(gump)->RedrawFrame(static_cast<int>(flag), static_cast<int>(hitsDiff),
                                                  static_cast<int>(manaDiffOut), static_cast<int>(stamDiffOut));   // 0x00468e00
}
HOOK_FUNC(0x0045a0f0, Packet_MobileAttributes);

// ---------------------------------------------------------------------------
// 0x0045a3b0  size 0x14e
// Inbound 0xA1: hits update: serial, max, current (words).
// ---------------------------------------------------------------------------
void __stdcall Packet_UpdateHealthBar(uint8_t *packet)
{
    int base = pkt::Base(packet);
    uint32_t serial = ntohl(rd32(packet, base));
    uint32_t maxValue = ntohs(rd16(packet, base + 4));
    uint32_t cur = ntohs(rd16(packet, base + 6));

    CDynamicObj *obj = g_objMgr.FindBySerial(serial);
    if (obj == nullptr)
        return;
    if (decomp::vcall(PORT_RET(int), obj, 0x24) == 0)               // is mobile
        return;
    if (decomp::vcall(PORT_RET(int), obj, 0x28) != 0) {
        RAWF(uint32_t, obj, 0x1e0) = maxValue & 0xffff;
        RAWF(uint32_t, obj, 0x1dc) = cur & 0xffff;
    }
    CGump *gump = g_gumpListTail;
    while (gump != nullptr) {
        if (decomp::vcall(PORT_RET(int), gump, 0xac) != 0 && gump->owner == obj)
            break;
        gump = gump->prev;
    }
    if (gump == nullptr)
        return;
    uint32_t curDiff = (RAWF(uint16_t, gump, 0xc2) == static_cast<uint16_t>(cur)) ? 0 : 1;
    uint32_t maxDiff = (RAWF(uint16_t, gump, 0xc4) != static_cast<uint16_t>(maxValue)) ? 1 : 0;
    RAWF(uint16_t, gump, 0xc4) = static_cast<uint16_t>(maxValue);
    RAWF(uint16_t, gump, 0xc2) = static_cast<uint16_t>(cur);
    static_cast<CStatusGump *>(gump)->RedrawFrame(static_cast<int>(maxDiff), static_cast<int>(curDiff), 0, 0);   // 0x00468e00
}
HOOK_FUNC(0x0045a3b0, Packet_UpdateHealthBar);

// ---------------------------------------------------------------------------
// 0x0045a500  size 0x14e
// Inbound 0xA2: mana update: serial, max, current (words); mobile +0x1e8 / +0x1e4, status gump +0xcc / +0xca.
// ---------------------------------------------------------------------------
void __stdcall Packet_UpdateManaBar(uint8_t *packet)
{
    int base = pkt::Base(packet);
    uint32_t serial = ntohl(rd32(packet, base));
    uint32_t maxValue = ntohs(rd16(packet, base + 4));
    uint32_t cur = ntohs(rd16(packet, base + 6));

    CDynamicObj *obj = g_objMgr.FindBySerial(serial);
    if (obj == nullptr)
        return;
    if (decomp::vcall(PORT_RET(int), obj, 0x24) == 0)               // is mobile
        return;
    if (decomp::vcall(PORT_RET(int), obj, 0x28) != 0) {
        RAWF(uint32_t, obj, 0x1e8) = maxValue & 0xffff;
        RAWF(uint32_t, obj, 0x1e4) = cur & 0xffff;
    }
    CGump *gump = g_gumpListTail;
    while (gump != nullptr) {
        if (decomp::vcall(PORT_RET(int), gump, 0xac) != 0 && gump->owner == obj)
            break;
        gump = gump->prev;
    }
    if (gump == nullptr)
        return;
    uint32_t curDiff = (RAWF(uint16_t, gump, 0xca) == static_cast<uint16_t>(cur)) ? 0 : 1;
    uint32_t maxDiff = (RAWF(uint16_t, gump, 0xcc) != static_cast<uint16_t>(maxValue)) ? 1 : 0;
    RAWF(uint16_t, gump, 0xcc) = static_cast<uint16_t>(maxValue);
    RAWF(uint16_t, gump, 0xca) = static_cast<uint16_t>(cur);
    static_cast<CStatusGump *>(gump)->RedrawFrame(static_cast<int>(maxDiff), static_cast<int>(curDiff), 0, 0);   // 0x00468e00
}
HOOK_FUNC(0x0045a500, Packet_UpdateManaBar);

// ---------------------------------------------------------------------------
// 0x0045a650  size 0x14e
// Inbound 0xA3: stamina update: serial, max, current (words); mobile +0x1f0 / +0x1ec, status gump +0xc8 / +0xc6.
// ---------------------------------------------------------------------------
void __stdcall Packet_UpdateStaminaBar(uint8_t *packet)
{
    int base = pkt::Base(packet);
    uint32_t serial = ntohl(rd32(packet, base));
    uint32_t maxValue = ntohs(rd16(packet, base + 4));
    uint32_t cur = ntohs(rd16(packet, base + 6));

    CDynamicObj *obj = g_objMgr.FindBySerial(serial);
    if (obj == nullptr)
        return;
    if (decomp::vcall(PORT_RET(int), obj, 0x24) == 0)               // is mobile
        return;
    if (decomp::vcall(PORT_RET(int), obj, 0x28) != 0) {
        RAWF(uint32_t, obj, 0x1f0) = maxValue & 0xffff;
        RAWF(uint32_t, obj, 0x1ec) = cur & 0xffff;
    }
    CGump *gump = g_gumpListTail;
    while (gump != nullptr) {
        if (decomp::vcall(PORT_RET(int), gump, 0xac) != 0 && gump->owner == obj)
            break;
        gump = gump->prev;
    }
    if (gump == nullptr)
        return;
    uint32_t curDiff = (RAWF(uint16_t, gump, 0xc6) == static_cast<uint16_t>(cur)) ? 0 : 1;
    uint32_t maxDiff = (RAWF(uint16_t, gump, 0xc8) != static_cast<uint16_t>(maxValue)) ? 1 : 0;
    RAWF(uint16_t, gump, 0xc8) = static_cast<uint16_t>(maxValue);
    RAWF(uint16_t, gump, 0xc6) = static_cast<uint16_t>(cur);
    static_cast<CStatusGump *>(gump)->RedrawFrame(static_cast<int>(maxDiff), static_cast<int>(curDiff), 0, 0);   // 0x00468e00
}
HOOK_FUNC(0x0045a650, Packet_UpdateStaminaBar);

// ---------------------------------------------------------------------------
// 0x0045a7a0  size 0x2ca
// Inbound 0x2E: equip item: item serial, graphic, stack byte, layer, wearer serial, hue.
// The item is found or created (a class with the tiledata flag 0x200000 for flagged graphics, otherwise
// the pooled item) and put on the wearer; the wearer's own update hooks (slots 0x14 / 0x10) wrap it.
// ---------------------------------------------------------------------------
void __stdcall Packet_EquipItem(uint8_t *packet)
{
    int base = pkt::Base(packet);
    uint32_t serial = ntohl(rd32(packet, base));
    uint32_t graphic = ntohs(rd16(packet, base + 4));
    uint8_t stackByte = packet[base + 6];
    uint8_t layer = packet[base + 7];
    uint32_t wearer = ntohl(rd32(packet, base + 8));
    uint32_t hue = ntohs(rd16(packet, base + 0xc));

    if (wearer == g_playerSerial) {
        CDynamicObj *item = g_objMgr.FindBySerial(serial);
        if (item == nullptr) {
            if (ObjectManager_TileFlagTest_0x200000(graphic) != 0) {     // 0x00457010
                item = new CContainerObj(serial, graphic, stackByte, 1);   // 0x00453930
            } else {
                item = static_cast<CDynamicObj *>(Item_PoolNew(serial, graphic & 0xffff, stackByte));    // 0x004516a0
            }
        } else {
            decomp::vcall_void(item, 0x50);
            item->graphic = static_cast<uint16_t>(graphic & 0x3fff);
            item->unk_3a = stackByte;
        }
        if (item != nullptr) {
            item->hue = static_cast<uint16_t>(Hue_Fix(hue & 0xffff));
            decomp::vcall_void(g_player, 0x14);
            Obj_ToMobile(item, g_player, layer);                        // 0x00452d40
            decomp::vcall_void(g_player, 0x10);
        }
        return;
    }

    CDynamicObj *wearerObj = g_objMgr.FindBySerial(wearer);
    if (wearerObj == nullptr)
        return;
    CDynamicObj *item = g_objMgr.FindBySerial(serial);
    if (item == nullptr) {
        if (ObjectManager_TileFlagTest_0x200000(graphic) != 0) {         // 0x00457010
            item = new CContainerObj(serial, graphic, stackByte, 1);   // 0x00453930
        } else {
            item = static_cast<CDynamicObj *>(Item_PoolNew(serial, graphic & 0xffff, stackByte));        // 0x004516a0
        }
    } else {
        decomp::vcall_void(item, 0x50);
    }
    if (item != nullptr) {
        item->hue = static_cast<uint16_t>(Hue_Fix(hue & 0xffff));
        decomp::vcall_void(wearerObj, 0x14);
        Obj_ToMobile(item, wearerObj, layer);                           // 0x00452d40
        decomp::vcall_void(wearerObj, 0x10);
    }
}
HOOK_FUNC(0x0045a7a0, Packet_EquipItem);
