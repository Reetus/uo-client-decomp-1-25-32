// Inbound packet handlers for the world / object stream:
//   0x1B login confirm (player mobile init)         0x00457b90
//   0x78 mobile incoming (with equipment list)       0x00458930
//   0x24 open container / gump by serial + gump id   0x00459020
//   0x90 map display (open a map item's gump)        0x00459200
//   0x23 drag / throw animation                      0x004594e0
//   0x27 pick-up rejected                            0x00459ca0
//
// All are __stdcall (RET 4) with the packet pointer. The packet's first field
// is at "base" (3 for a variable-length opcode, 1 otherwise), see
// packet_cursor.h. The SEH frames the original wraps around operator new +
// constructor calls are omitted.
#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"
#include "gump.h"
#include "object_manager.h"
#include "crt.h"
#include "packet_cursor.h"
#include "raw_field.h"
#include "vcall.h"
#include "obj.h"
#include "obj_pool_alloc_ports.h"
#include "gump_manager.h"
#include "spell_book_gump.h"
#include "course_gump.h"
#include "text_gump_ctor_ports.h"
#include "map_world.h"
#include "app_session_ops_ports.h"
#include "world_renderer.h"
#include "missile_gump_new_ports.h"
#include "main_window_ports.h"
#include "pos3.h"

// ---------------------------------------------------------------------------
// Globals (single-file use).
// ---------------------------------------------------------------------------
#define g_playerObj GLOBAL_AS(void *, g_player)
#define g_app69e0e8 GLOBAL_PTR(void, g_app_storage)
#define g_msgTarget GLOBAL_PTR(void, g_msgTarget_storage)               // on-screen message / journal object
#define g_helperObj69e0c0 GLOBAL_PTR(void, g_worldPicker_storage)
#define g_mapView GLOBAL_PTR(void, g_map_storage)                  // g_mapObject
#define g_dlgA GLOBAL_AS(void *, g_waitingGump)
#define g_dlgB GLOBAL_AS(void *, g_prevGump)
#define g_dlgC GLOBAL_AS(void *, g_charCreationGump)
#define g_dlgD GLOBAL_AS(void *, g_cityChoiceGump)
#define g_dlgE GLOBAL_AS(void *, g_radarGump)
#define g_welcomeText "Welcome to Ultima Online!"                // string at 0x004ad4d0

// ---------------------------------------------------------------------------
// Original code not ported yet.
// ---------------------------------------------------------------------------
static inline void SetTitleSuffix(const char *suffix) { PORT_THISCALL0(&SetMainWindowTitle, void, const_cast<char *>(suffix))(); }   // ECX = suffix
static inline void *FindObject(uint32_t serial) { return (&g_objMgr)->CObjectManager::FindBySerial(serial); }
static inline void Obj_ToWorld(void *o, uint32_t xy, uint32_t z) { reinterpret_cast<CObj *>(o)->ToWorld(xy, (uint16_t)(z)); }
static inline void Obj_ToMobile(void *o, void *mobile, uint32_t layer) { reinterpret_cast<CObj *>(o)->ToEquipped((CMobile *)(mobile), (int)(layer)); }
static inline uint32_t Hue_Fix(uint32_t hue) { return CObj_SanitizeHue(hue); }
static inline void *Item_PoolNew(uint32_t serial, uint32_t graphic, uint32_t offset)
{
    return DynamicObj_PoolNew(serial, graphic, offset);
}
#define Mgr_TileFlag100000(g) (PORT_THISCALL1(&ObjectManager_TileFlagTest_0x100000, uint32_t, &g_objMgr, uint16_t)(g) != 0)
#define Mgr_TileFlag200000(g) (PORT_THISCALL1(&ObjectManager_TileFlagTest_0x200000, uint32_t, &g_objMgr, uint16_t)(g) != 0)
static inline void Mgr_CreateMobile(uint32_t serial, uint32_t body, uint32_t x, uint32_t y, uint32_t z, uint32_t dir,
                             uint32_t hue, uint32_t flags)
{
    (&g_objMgr)->CObjectManager::AddOrUpdateObject(serial, body, (int8_t)(0), (uint16_t)(0), (int)(x), (int)(y), (int)(z), (uint8_t)(dir), hue, flags);
}
static inline void *Gump_Create(void *obj, uint32_t gumpId, int parent, int flag)
{
    return (void *)reinterpret_cast<CGumpManager *>(g_gumpManager)->CreateGump((CObj *)(obj), (int)(gumpId), (CGump *)(parent), flag);
}
static inline void Gump_Show(void *gump) { CGump_AddChild(static_cast<CGump *>(gump), nullptr, 1); }
static inline void Journal_Say(const char *text) { PORT_THISCALL3(&CreateBarkGump, void, g_msgTarget, int, int, const char *)(0, 3, text); }
static inline void Map_SetView(uint32_t x, uint32_t y, uint32_t z, uint32_t extra)
{
    reinterpret_cast<CMap *>(g_mapView)->SetPos(x, y, z, extra);
}
static inline void App_Refresh() { PORT_THISCALL0(&ResetWorldState, void, g_app69e0e8)(); }
static inline void Helper_Refresh() { reinterpret_cast<CWorldRenderer *>(g_helperObj69e0c0)->UpdateRoofHideLimit(); }
static inline void Effect_CoordsCoords(uint32_t g, ObjCoords s, ObjCoords d)
{
    PORT_THISCALL6(&NewMissileGump_PosToPos, void, g_gumpManager, uint32_t, ObjCoords, ObjCoords, int, int, int)(g, s, d, 5, 1, 0);
}
static inline void Effect_CoordsObj(uint32_t g, ObjCoords s, void *d)
{
    PORT_THISCALL6(&NewMissileGump_PosToObj, void, g_gumpManager, uint32_t, ObjCoords, void *, int, int, int)(g, s, d, 5, 1, 0);
}
static inline void Effect_ObjCoords(uint32_t g, void *s, ObjCoords d)
{
    PORT_THISCALL6(&NewMissileGump_ObjToPos, void, g_gumpManager, uint32_t, void *, ObjCoords, int, int, int)(g, s, d, 5, 1, 0);
}
static inline void Effect_ObjObj(uint32_t g, void *s, void *d)
{
    PORT_THISCALL6(&NewMissileGump_ObjToObj, void, g_gumpManager, uint32_t, void *, void *, int, int, int)(g, s, d, 5, 1, 0);
}

// Raw big-endian field reads at a byte offset of the packet (the original reads them inline; a function call here
// would also stay out of line in the handlers that have an exception frame).
#define rd32(p, off) (*reinterpret_cast<const uint32_t *>((p) + (off)))
#define rd16(p, off) (*reinterpret_cast<const uint16_t *>((p) + (off)))

// ---------------------------------------------------------------------------
// 0x00457b90  size 0x4c6
// Inbound packet 0x1B: login confirm / player mobile init ("Welcome to Ultima Online!").
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x1B(uint8_t *packet)
{
    char buf[500];
    uint8_t out[4];

    if (g_oldConn != nullptr) {
        decomp::vdelete(g_oldConn);
        g_oldConn = nullptr;
    }
    strcpy(buf, g_playerName);
    strcat(buf, g_titlePartA);
    strcat(buf, g_titlePartB);
    strcat(buf, g_titlePartC);
    SetTitleSuffix(buf);

    int base = pkt::Base(packet);
    uint32_t serial = ntohl(rd32(packet, base));
    ntohl(rd32(packet, base + 4));                      // discarded
    uint32_t body = ntohs(rd16(packet, base + 8));
    uint32_t x = ntohs(rd16(packet, base + 0xa));
    uint32_t y = ntohs(rd16(packet, base + 0xc));
    uint32_t z = ntohs(rd16(packet, base + 0xe));
    uint8_t facing = packet[base + 0x10];
    uint8_t extra = packet[base + 0x11];
    ntohl(rd32(packet, base + 0x12));                   // discarded
    uint32_t w0 = ntohs(rd16(packet, base + 0x16));
    uint32_t w1 = ntohs(rd16(packet, base + 0x18));
    uint32_t w2 = ntohs(rd16(packet, base + 0x1a));
    uint32_t w3 = ntohs(rd16(packet, base + 0x1c));
    g_sceneW3 = w3 & 0xffff;
    g_sceneW2 = w2 & 0xffff;
    g_sceneW0 = w0 & 0xffff;
    g_sceneW1 = w1 & 0xffff;

    if (serial != g_playerSerial)
        g_playerSerial = serial;
    void *player = g_playerObj;
    RAWF(uint32_t, player, 0x78) = serial;
    RAWF(uint16_t, g_playerObj, 0x38) = static_cast<uint16_t>(body & 0x3fff);
    App_Refresh();
    RAWF(uint8_t, g_playerObj, 0x6e) = facing;
    RAWF(uint32_t, g_playerObj, 0x190) = z & 0xffff;
    Obj_ToWorld(g_playerObj, (x & 0xffff) | ((y & 0xffff) << 16), z & 0xffff);
    Map_SetView(x & 0xffff, y & 0xffff, z & 0xffff, extra);
    g_loginFlag648928 = 0;
    if (g_dlgA != nullptr) { decomp::vdelete(g_dlgA); g_dlgA = nullptr; }
    if (g_dlgB != nullptr) { decomp::vdelete(g_dlgB); g_dlgB = nullptr; }
    if (g_dlgC != nullptr) { decomp::vdelete(g_dlgC); g_dlgC = nullptr; }
    if (g_dlgD != nullptr) { decomp::vdelete(g_dlgD); g_dlgD = nullptr; }
    Journal_Say(g_welcomeText);
    memset(g_backBuffer, 0, 0x25800 * 4);
    GLOBAL_AS(uint32_t, g_flag6f479c) = 1;
    Helper_Refresh();
    if (g_tipEnabled != 0) {
        uint32_t v = g_tipNumber;
        out[0] = 0xa7;
        if (pkt::IsVariable(out))
            pkt::OrigSetCursor(out, 3);
        else
            g_pktFixedCursor = 1;
        pkt::OrigAppendU16(out, v);
        pkt::OrigAppendU8(out, 1);
        pkt::Send(out);
    }
}
HOOK_FUNC(0x00457b90, Packet_Handle0x1B);

// ---------------------------------------------------------------------------
// 0x00458930  size 0x364
// Inbound packet 0x78: mobile incoming (header like 0x77, then the equipment
// list terminated by serial 0).
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x78(uint8_t *packet)
{
    uint32_t serial = ntohl(rd32(packet, pkt::Base(packet)));
    uint32_t body = ntohs(rd16(packet, pkt::Base(packet) + 4));
    uint32_t x = ntohs(rd16(packet, pkt::Base(packet) + 6));
    uint32_t y = ntohs(rd16(packet, pkt::Base(packet) + 8));
    int b = pkt::Base(packet);
    uint8_t z = packet[b + 0xa];
    uint8_t dir = packet[b + 0xb];
    uint32_t hue0 = ntohs(rd16(packet, b + 0xc));
    uint8_t flags = packet[b + 0xe];
    uint8_t noto = packet[b + 0xf];

    if (serial != g_playerSerial)
        Mgr_CreateMobile(serial, body, x & 0xffff, y & 0xffff, z, dir, hue0, flags);
    void *mob = FindObject(serial);
    if (mob != nullptr) {
        if (decomp::vcall(PORT_RET(int), mob, 0x24) != 0) {                   // slot 0x24: is mobile
            decomp::vcall_void(mob, 0x14);
            decomp::vcall_void(mob, 0x94, static_cast<int>(static_cast<signed char>(noto)));   // set notoriety
        } else {
            mob = nullptr;
        }
    }

    uint32_t itemSerial = ntohl(rd32(packet, (pkt::OrigBase(packet) & 0xffff) + 0x10));
    uint32_t off = 0x14;
    if (itemSerial != 0) {
        do {
            uint32_t graphic = ntohs(rd16(packet, (pkt::OrigBase(packet) & 0xffff) + off));
            off += 2;
            uint8_t layer = packet[(pkt::OrigBase(packet) & 0xffff) + off];
            off++;
            uint32_t hue;
            if ((graphic & 0x8000) != 0) {
                graphic &= 0x7fff;
                hue = ntohs(rd16(packet, (pkt::OrigBase(packet) & 0xffff) + off));
                off += 2;
            } else {
                hue = 0;
            }
            if (mob != nullptr) {
                void *item;
                void *it = FindObject(itemSerial);
                if (it == nullptr) {
                    if (Mgr_TileFlag200000(graphic)) {
                        item = new CContainerObj(itemSerial, graphic, 0, 1);
                    } else {
                        it = Item_PoolNew(itemSerial, graphic & 0xffff, 0);
                        item = it;
                    }
                } else {
                    decomp::vcall_void(it, 0x50);
                    item = it;
                }
                RAWF(uint16_t, item, 0x3e) = static_cast<uint16_t>(Hue_Fix(hue & 0xffff));
                Obj_ToMobile(item, mob, layer & 0xffu);
            }
            itemSerial = ntohl(rd32(packet, (pkt::OrigBase(packet) & 0xffff) + off));
            off += 4;
        } while (itemSerial != 0);
    }
    if (mob != nullptr)
        decomp::vcall_void(mob, 0x10);
}
HOOK_FUNC(0x00458930, Packet_Handle0x78);

// ---------------------------------------------------------------------------
// 0x00459020  size 0x1d5
// Inbound packet 0x24: open a container / gump by object serial + gump id.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x24(uint8_t *packet)
{
    uint32_t serial = ntohl(rd32(packet, pkt::Base(packet)));
    uint32_t gumpId = ntohs(rd16(packet, pkt::Base(packet) + 4));
    void *obj;

    if (serial == 0xffffffffu) {
        Gump_Create(nullptr, gumpId & 0xffff, 0, 0);
        return;
    }
    if (serial == g_playerSerial)
        obj = g_playerObj;
    else
        obj = FindObject(serial);

    if (static_cast<uint16_t>(gumpId) == 0xffff) {
        void *book = nullptr;
        if (decomp::vcall(PORT_RET(int), obj, 0x30) != 0) {                   // slot 0x30: is container
            void *b = RAWF(void *, obj, 0xac);
            if (b != nullptr) {
                book = b;
                decomp::vcall_void(book, 0x114);
            }
        }
        if (book != nullptr)
            return;
        Gump_Show(new CSpellBookGump(-1, -1, (CObj *)(obj)));
        return;
    }
    if (static_cast<uint16_t>(gumpId) == 0xa || static_cast<uint16_t>(gumpId) == 0x30)
        Gump_Create(obj, gumpId & 0xffff, 0, 0);
    if (static_cast<uint16_t>(gumpId) == 0x1392 || static_cast<uint16_t>(gumpId) == 0x1393) {
        if (serial == 1) {
            Gump_Create(obj, gumpId & 0xffff, 0, 0);
        } else if (serial == 0) {
            if (g_dlgE != nullptr) {
                decomp::vdelete(g_dlgE);
                g_dlgE = nullptr;
            }
        }
    }
    int flag;
    if (static_cast<uint16_t>(gumpId) == 0x1f4 || static_cast<uint16_t>(gumpId) == 0x1f5) {
        flag = 0;
    } else {
        if (obj == nullptr)
            return;
        if (decomp::vcall(PORT_RET(int), obj, 0x30) == 0)
            return;
        flag = 1;
    }
    Gump_Create(obj, gumpId & 0xffff, 0, flag);
}
HOOK_FUNC(0x00459020, Packet_Handle0x24);

// ---------------------------------------------------------------------------
// 0x00459200  size 0x1fb
// Inbound packet 0x90: map display (open a map item's gump).
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x90(uint8_t *packet)
{
    uint32_t serial = ntohl(rd32(packet, pkt::Base(packet)));
    uint32_t graphic = ntohs(rd16(packet, pkt::Base(packet) + 4));
    uint32_t w0 = ntohs(rd16(packet, pkt::Base(packet) + 6));
    uint32_t w1 = ntohs(rd16(packet, pkt::Base(packet) + 8));
    uint32_t w2 = ntohs(rd16(packet, pkt::Base(packet) + 0xa));
    uint32_t w3 = ntohs(rd16(packet, pkt::Base(packet) + 0xc));
    uint32_t w4 = ntohs(rd16(packet, pkt::Base(packet) + 0xe));
    uint32_t w5 = ntohs(rd16(packet, pkt::Base(packet) + 0x10));

    // The object is not null-checked before the virtual call (as in the original).
    void *obj = FindObject(serial);
    if (decomp::vcall(PORT_RET(int), obj, 0x60) != 0) {
        if (static_cast<uint16_t>(graphic) >= 0x139d) {
            Gump_Show(new CCourseGump(0, 0, graphic & 0xffff, obj, w0, w1, w2, w3, w4, w5));
        }
    }
}
HOOK_FUNC(0x00459200, Packet_Handle0x90);

// ---------------------------------------------------------------------------
// 0x004594e0  size 0x336
// Inbound packet 0x23: drag / throw animation (graphic, offset, two discarded
// words, source serial/x/y/z, destination serial/x/y/z).
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x23(uint8_t *packet)
{
    Pos3 srcPos;
    Pos3 destPos;
    void *srcObj = nullptr;
    void *destObj = nullptr;

    uint32_t graphic = ntohs(rd16(packet, pkt::Base(packet)));
    uint8_t gOffset = packet[(pkt::IsVariableW(packet) ? 3 : 1) + 2];
    ntohs(rd16(packet, (pkt::IsVariableW(packet) ? 3 : 1) + 3));       // result discarded
    ntohs(rd16(packet, pkt::Base(packet) + 5));                         // result discarded
    uint32_t srcSerial = ntohl(rd32(packet, pkt::Base(packet) + 7));
    uint32_t srcX = ntohs(rd16(packet, pkt::Base(packet) + 0xb));
    uint32_t srcY = ntohs(rd16(packet, pkt::Base(packet) + 0xd));
    uint8_t srcZ = packet[pkt::OrigBase(packet) + 0xf];
    uint32_t destSerial = ntohl(rd32(packet, (pkt::OrigBase(packet) & 0xffff) + 0x10));
    uint32_t destX = ntohs(rd16(packet, (pkt::OrigBase(packet) & 0xffff) + 0x14));
    uint32_t destY = ntohs(rd16(packet, (pkt::OrigBase(packet) & 0xffff) + 0x16));
    uint8_t destZ = packet[(pkt::OrigBase(packet) & 0xffff) + 0x18];

    if (srcSerial != 0)
        srcObj = (&g_objMgr)->CObjectManager::FindBySerial(srcSerial);
    if (srcObj != nullptr) {
        if (decomp::vcall(PORT_RET(int), srcObj, 0x24) == 0)           // slot 0x24: is mobile
            srcObj = nullptr;
    }
    void *dest;
    if (destSerial != 0)
        dest = (&g_objMgr)->CObjectManager::FindBySerial(destSerial);
    else
        dest = destObj;
    if (dest != nullptr) {
        if (decomp::vcall(PORT_RET(int), dest, 0x24) == 0) {
            destObj = nullptr;
            dest = nullptr;
        }
    }

    srcPos.x = static_cast<int16_t>(srcX);
    srcPos.y = static_cast<int16_t>(srcY);
    srcPos.z = srcZ;
    destPos.x = static_cast<int16_t>(destX);
    destPos.y = static_cast<int16_t>(destY);
    destPos.z = destZ;
    uint32_t g = static_cast<uint32_t>(gOffset) + graphic;

    if (srcObj == nullptr) {
        if (dest == nullptr) {
            PORT_THISCALL6(&NewMissileGump_PosToPos, void, g_gumpManager, uint32_t, Pos3, Pos3, int, int, int)(g, srcPos, destPos, 5, 1, 0);
            return;
        }
        PORT_THISCALL6(&NewMissileGump_PosToObj, void, g_gumpManager, uint32_t, Pos3, void *, int, int, int)(g, srcPos, dest, 5, 1, 0);
        return;
    }
    if (dest != nullptr) {
        Effect_ObjObj(g, srcObj, dest);
        return;
    }
    PORT_THISCALL6(&NewMissileGump_ObjToPos, void, g_gumpManager, uint32_t, void *, Pos3, int, int, int)(g, srcObj, destPos, 5, 1, 0);
}
HOOK_FUNC(0x004594e0, Packet_Handle0x23);

// ---------------------------------------------------------------------------
// 0x00459ca0  size 0x25d
// Inbound packet 0x27: pick-up rejected (reason byte). Rebuild the held item
// from the object manager's held-item state and put it back where it came from.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x27(uint8_t *packet)
{
    CObjectManager &m = g_objMgr;
    Pos3 pos;
    uint8_t reason = packet[pkt::Base(packet)];
    void *obj;

    if (m.heldGraphic == 0x2006) {
        obj = new CObjVt49f9e0(m.heldSerial, m.heldGraphic);
    } else if (Mgr_TileFlag100000(m.heldGraphic)) {
        obj = new CObjVt49f960(m.heldSerial, m.heldGraphic, m.heldOffset, 1);
    } else if (Mgr_TileFlag200000(m.heldGraphic)) {
        obj = new CContainerObj(m.heldSerial, m.heldGraphic, m.heldOffset, 1);
    } else {
        obj = Item_PoolNew(m.heldSerial, m.heldGraphic, static_cast<uint32_t>(static_cast<int>(static_cast<signed char>(m.heldOffset))));
    }

    pos.x = m.heldPos.x;
    pos.y = m.heldPos.y;
    pos.z = m.heldPos.z;
    RAWF(uint16_t, obj, 0x3e) = static_cast<uint16_t>(Hue_Fix(m.heldHue));
    RAWF(uint16_t, obj, 0x3c) = m.heldQty;
    RAWF(uint8_t, obj, 0x98) = m.heldFlag40;
    if (m.heldMobile != nullptr) {
        void *mob = m.heldMobile;
        decomp::vcall_void(mob, 0x14);
        Obj_ToMobile(obj, m.heldMobile, m.heldLayer & 0xffffu);
        mob = m.heldMobile;
        decomp::vcall_void(mob, 0x10);
    } else if (m.heldContainer != nullptr) {
        PORT_THISCALL2(&CDynamicObj::ToContainer, void, obj, void *, Pos3)(m.heldContainer, pos);
    } else {
        PORT_THISCALL1(&CObj::ToWorld, void, obj, Pos3)(pos);
    }
    if (m.dragGump != nullptr) {
        decomp::vdelete(m.dragGump);
        m.dragGump = nullptr;
    }
    m.heldState = 0;
    if (reason < 5)
        Journal_Say(g_pickupMessages[reason]);
}
HOOK_FUNC(0x00459ca0, Packet_Handle0x27);
