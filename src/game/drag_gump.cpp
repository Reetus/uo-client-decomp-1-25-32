// CDragGump mouse-up. The SEH frame of the original (try level 0 / 2) is omitted.
#include "drag_gump.h"

#include <cstdio>

#include "../runtime/ported.h"
#include "pos3.h"
#include "batch02_helpers.h"
#include "globals.h"
#include "gump.h"
#include "gump_manager.h"
#include "client_misc_ops_ports.h"
#include "cell_grid.h"
#include "object_manager.h"
#include "login_gumps.h"
#include "obj.h"
#include "packet_builders_batch24_ports.h"
#include "client_socket.h"
#include "gump_base_ops_ports.h"
#include "bark_gump_ui_ports.h"
#include "object_manager_ops_ports.h"
#include "map_world_ops_ports.h"
#include "map_world.h"
#include "paperdoll_gump.h"
#include "container_gump.h"
#include "sound_manager.h"

#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)
#undef g_worldPicker   // the class header's typed form; this file addresses the object by offset
#define g_worldPicker GLOBAL_PTR(void, g_worldPicker_storage)
#define g_objectMgr2 GLOBAL_PTR(void, g_objMgr_storage)             // object table (lookup by position / reach test)
#define g_soundObject GLOBAL_PTR(void, g_soundManager_storage)
#define g_dropPending GLOBAL_AS(int, g_dragGump)

// Calls into code that is not ported yet.
#define Gump_ToScreen(self, a1, a2) (reinterpret_cast<CGump *>(self)->LocalToScreen(a1, a2))           // gump -> screen coordinates (ECX = gump)
#define Gump_ToLocal(self, a1, a2) (reinterpret_cast<CGump *>(self)->ScreenToLocal(a1, a2))            // screen -> gump coordinates (ECX = gump)
#define GumpMgr_HitTest(self, a1, a2, a3, a4) (reinterpret_cast<CGumpManager *>(self)->GumpAtPoint(a1, a2, a3, a4))
#define IsOptionSet() (Misc_Return0())                       // ? flag test (called twice per check)
#define World_Pick(self, a1, a2, a3) ((char *)reinterpret_cast<CWorldPicker *>(self)->GetObjectAt(a1, a2, a3))
#define Map_GetZ(self, a1, a2) (reinterpret_cast<CObjectManager *>(self)->GroundZ(a1, a2))
#define Map_Highlight PORT_FUNC3(&Stub_00421390, void, __fastcall, int, int, int)  // ECX = x, EDX = y, stack z
#define Objects_PickAt(o, a1, a2) PORT_THISCALL2(&ObjectManager_GetObjectAtScreen, char *, (o), int, int)(a1, a2)
#define Obj_IsContainerLike(self) (reinterpret_cast<CDynamicObj *>(self)->IsReachable())
#define Packet_Drop(o, a1, a2, a3, a4, a5, a6) PORT_THISCALL6(&Build_Packet_0x08, void, (o), char *, int, int, int, int, int)(a1, a2, a3, a4, a5, a6)   // pkt, held, x, y, z, dest
#define Packet_Equip(o, a1, a2, a3, a4) PORT_THISCALL4(&Build_Packet_0x13, void, (o), char *, int, unsigned, int)(a1, a2, a3, a4)
#define Net_Send(self, a1) (reinterpret_cast<CClientSocket *>(self)->SendPacket((const uint8_t *)(a1)))
#define GumpMgr_GetArtRect(o, a1, a2) PORT_THISCALL2(&ContainerGumpIdForItem, int, (o), int, int *)(a1, a2)
#define GumpMgr_Fix(o, a1) PORT_THISCALL1(&GetContainerGumpRect, void, (o), int)(a1)
#define Map_Visible(o, a1, a2) PORT_THISCALL2(&ObjectManager_Always1, int, (o), int, int)(a1, a2)
#define Map_Reach(o, a1, a2, a3) PORT_THISCALL3(&Map_LineOfSightClear, int, (o), Pos3, Pos3, int)(a1, a2, a3)
#define Map_CellHeights(self, a1, a2) ((signed char *)reinterpret_cast<CMap *>(self)->GetNeighborZ(a1, a2))
#define Gump_SlotAt(self, a1, a2, a3) (reinterpret_cast<CPaperdollGump *>(self)->HitTestLayer(a1, a2, a3))
#define Gump_Release(self) (reinterpret_cast<CPaperdollGump *>(self)->RedrawFrame())
#define Gump_TryDrop(self, a1, a2) (reinterpret_cast<CContainerGump *>(self)->DropHeldItemAt(a1, a2))
#define Item_CanWear(a1) ((int)Graphic_IsInRanges_0x00456840((uint32_t)(a1)))              // ECX = graphic (word)
#define Music_Play(self, a1, a2, a3, a4, a5, a6) (reinterpret_cast<CSoundManager *>(self)->PlaySound(a1, (long)(a2), (long)(a3), a4, a5, a6))

static inline void delete_gump(void *g)
{
    if (g != nullptr)
        PORT_VSLOT1(g, 0, void, int)(1);      // deleting dtor
}

// 0x00429b70  size 0x7d2
void __thiscall CDragGump::MouseUp(int sx, int sy)
{
    char *self = reinterpret_cast<char *>(this);
    char pkt[0x20];
    char pkt2[0x10];
    char dbg[0xc0];
    int rect[4];                  // {left, top, right, bottom}
    Pos3 targetPt;
    Pos3 playerPt;
    int tileY;
    int tileX;
    unsigned char z;
    CGump *g;                     // gump under the cursor
    char *target;                 // object under the cursor
    int slot;
    char *obj;
    int r;
    char *cell;

    Gump_ToScreen(this, &sx, &sy);
    g = GumpMgr_HitTest(g_gumpManager, sx, sy, this, 1);
    if (g == nullptr)
        goto done;
    if (PORT_VSLOT0(g, 0x68, int)() == 0)
        goto done;

    if (g_heldSerial == -1) {                                   // nothing held
        if (g != g_screenGump)
            goto done;
        if (IsOptionSet() == 0 || IsOptionSet() == 0) {
            sx = sx + width / 2 - 0x16;
            sy = sy + height / 2 - 0x16;
        }
        obj = World_Pick(g_worldPicker, sx, sy, 0);
        if (obj == nullptr)
            goto done;
        tileY = RAW_AT(int16_t, obj, 0x22);
        tileX = RAW_AT(int16_t, obj, 0x20);
        z = static_cast<unsigned char>(Map_GetZ(g_objectMgr2, tileX, tileY));
        if (IsOptionSet() != 0)
            Map_Highlight(tileX, tileY, static_cast<int>(z));
        delete_gump(this);
        goto done;
    }

    if (g == g_screenGump) {                                 // dropped on the world view
        target = Objects_PickAt(g_objectMgr2, sx, sy);
        if (target == nullptr)
            goto drop_on_ground;
        if (Obj_IsContainerLike(target) == 0)
            goto fail;
        if (PORT_VSLOT0(target, 0x24, int)() != 0) {
            // drop into the container-like object
            Packet_Drop(g_packetBuilder, pkt, g_heldSerial, 0xffff, 0xffff, 0, RAW_AT(int32_t, target, 0x78));
            Net_Send(g_pktSender, pkt);
            delete_gump(this);
            g_dropPending = 0;
            return;
        }
        if (PORT_VSLOT0(target, 0x30, int)() != 0) {
            // target is a gump-backed object: must be large enough to hold the dragged item
            r = GumpMgr_GetArtRect(g_gumpManager, RAW_AT(uint16_t, target, 0x38), rect);
            GumpMgr_Fix(g_gumpManager, r);
            if (rect[2] - rect[0] < width)
                goto fail;
            if (rect[3] - rect[1] < height)
                goto fail;
            Packet_Drop(g_packetBuilder, pkt, g_heldSerial, 0xffff, 0xffff, 0, RAW_AT(int32_t, target, 0x78));
            Net_Send(g_pktSender, pkt);
            delete_gump(this);
            g_dropPending = 0;
            return;
        }
        // stackable merge test
        if (RAW_AT(uint16_t, target, 0x3c) != 0) {
            int g2 = static_cast<int>(RAW_AT(int8_t, target, 0x3a)) + static_cast<int>(RAW_AT(uint16_t, target, 0x38));
            if ((g_tileData[g2 * 0x28 + 1] & 8) != 0) {
                if (g_heldCount != 0) {
                    if ((g_heldFlags & 0x800) != 0) {                       // TEST AH, 8
                        if (RAW_AT(uint16_t, target, 0x38) == g_heldGraphic) {
                            if (RAW_AT(uint16_t, target, 0x3e) == g_heldHue) {
                                Packet_Drop(g_packetBuilder, pkt, g_heldSerial,
                                            RAW_AT(uint16_t, target, 0x20),
                                            RAW_AT(uint16_t, target, 0x22),
                                            RAW_AT(uint8_t, target, 0x24),
                                            RAW_AT(int32_t, target, 0x78));
                                Net_Send(g_pktSender, pkt);
                                delete_gump(this);
                                g_dropPending = 0;
                                return;
                            }
                        }
                    }
                }
            }
        }
        if (PORT_VSLOT0(target, 0x1c, int)() != 0)
            goto drop_on_ground;
        if ((*reinterpret_cast<uint32_t *>(g_tileData + RAW_AT(uint16_t, target, 0x38) * 0x28) & 0x200) == 0)   // TEST AH, 2
            goto fail;

drop_on_ground:
        if (IsOptionSet() == 0 || IsOptionSet() == 0) {
            sx = sx + width / 2 - 0x16;
            sy = sy + height / 2 - 0x16;
        }
        target = World_Pick(g_worldPicker, sx, sy, 0);
        if (target == nullptr)
            goto done;
        sprintf(dbg, "%d,%d", g_scrollY - RAW_AT(int32_t, target, 8) - sx, g_scrollX - RAW_AT(int32_t, target, 4) - sy);   // unused debug string
        tileY = RAW_AT(int16_t, target, 0x22);
        tileX = RAW_AT(int16_t, target, 0x20);
        z = 0;
        {
            Pos3 pp = *reinterpret_cast<Pos3 *>(&g_player->x);
            playerPt.x = pp.x;
            playerPt.y = pp.y;
            playerPt.z = static_cast<int16_t>(pp.z + 0xc);
            targetPt.x = static_cast<int16_t>(tileX);
            targetPt.y = static_cast<int16_t>(tileY);
            targetPt.z = 0;
        }
        if (Map_Visible(g_objectMgr2, tileX, tileY) != 0
            && (tileX - g_viewCenterX < 0 ? g_viewCenterX - tileX : tileX - g_viewCenterX) <= 3
            && (tileY - g_viewCenterY < 0 ? g_viewCenterY - tileY : tileY - g_viewCenterY) <= 3
            && Map_Reach(g_mapObject, playerPt, targetPt, 0x40) != 0) {
            // reachable
        } else {
            if (IsOptionSet() == 0) {
                Music_Play(g_soundObject, 0x51, 0, 0, 0, 0, 0);
                goto done;
            }
        }

        if (PORT_VSLOT0(target, 0x1c, int)() != 0)
            goto z_from_map;
        for (;;) {
            if (PORT_VSLOT0(target, 0x1c, int)() != 0)
                break;
            {
                uint32_t fl = *reinterpret_cast<uint32_t *>(g_tileData + RAW_AT(uint16_t, target, 0x38) * 0x28);
                if ((fl & 0x200) != 0)                           // TEST AH, 2
                    break;
                if ((fl & 0x40) != 0) {                          // TEST AL, 0x40: blocked
                    Music_Play(g_soundObject, 0x51, 0, 0, 0, 0, 0);
                    goto done;
                }
            }
            target = RAW_AT(char *, target, 0x18);
            if (target == nullptr)
                break;
        }
        if (target != nullptr) {
            if (PORT_VSLOT0(target, 0x1c, int)() != 0) {
z_from_map:
                cell = reinterpret_cast<char *>(Map_CellHeights(g_mapObject, tileX, tileY));
                {
                    int sum = static_cast<int>(cell[3]) + static_cast<int>(cell[1]);       // MOVSX bytes
                    z = static_cast<unsigned char>((sum - (sum >> 31)) >> 1);
                }
            } else {
                z = static_cast<unsigned char>(g_tileData[RAW_AT(uint16_t, target, 0x38) * 0x28 + 0x12]
                                               + RAW_AT(uint8_t, target, 0x24));
            }
        }
        Packet_Drop(g_packetBuilder, pkt, g_heldSerial, tileX, tileY, static_cast<int>(z), -1);
        Net_Send(g_pktSender, pkt);
        delete_gump(this);
        goto done;
    }

    // ---- dropped on some other gump ----
    if (PORT_VSLOT0(g, 0x78, int)() != 0) {
        Gump_ToLocal(g, &sx, &sy);
        slot = Gump_SlotAt(g, sx, sy, 1);
        if (slot != -1) {
            char *slotGump = RAW_AT(char *, g->owner, static_cast<size_t>(slot) * 4 + 0xc4);
            if (PORT_VSLOT0(slotGump, 0x30, int)() != 0) {
                char *s2 = RAW_AT(char *, g->owner, static_cast<size_t>(slot) * 4 + 0xc4);
                Packet_Drop(g_packetBuilder, pkt, g_heldSerial, 0xffff, 0xffff, 0, RAW_AT(int32_t, s2, 0x78));
                Net_Send(g_pktSender, pkt);
                if (sourceGump != nullptr) {
                    RAW_AT(int32_t, sourceGump, 0x9c) = -1;
                    Gump_Release(sourceGump);
                    sourceGump = nullptr;
                }
                delete_gump(this);
            }
        }
    }

    // 0x0042a1ad: reached even after "delete this" above and reads this->sourceGump again,
    // as the binary does (use after free in the original; kept literally).
    if (g == *reinterpret_cast<CGump **>(self + 0x94)) {
        RAW_AT(int32_t, *reinterpret_cast<CGump **>(self + 0x94), 0x9c) = -1;
        Gump_Release(*reinterpret_cast<CGump **>(self + 0x94));
        {
            unsigned int b5 = g_tileData[g_heldGraphic * 0x28 + 5];
            Packet_Equip(g_packetBuilder, pkt2, g_heldSerial, b5, RAW_AT(int32_t, g->owner, 0x78));
        }
        Net_Send(g_pktSender, pkt2);
        delete_gump(this);
        goto done;
    }

    if (PORT_VSLOT0(g, 0x90, int)() != 0) {
        if (g->owner == nullptr)
            goto fail;
        if (Obj_IsContainerLike(g->owner) == 0)
            goto fail;
        Gump_ToLocal(g, &sx, &sy);
        if (Gump_TryDrop(g, sx, sy) == 0)
            goto fail;
        delete_gump(this);
        goto done;
    }

    if (PORT_VSLOT0(g, 0xd0, int)() == 0)
        goto fail;
    if (Item_CanWear(g_heldGraphic) == 0)
        goto fail;
    Packet_Drop(g_packetBuilder, pkt, g_heldSerial, 0xffff, 0xffff, 0, RAW_AT(int32_t, g->owner, 0x78));
    Net_Send(g_pktSender, pkt);
    delete_gump(this);
    g_dropPending = 0;
    return;

fail:
    Music_Play(g_soundObject, 0x51, 0, 0, 0, 0, 0);
done:
    return;
}
HOOK_VMETHOD(0x00429b70, CDragGump, MouseUp);
