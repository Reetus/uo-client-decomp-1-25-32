#include "obj.h"
#include "pos3.h"
#include <new>
#include "obj_inline.h"

#include <cstdlib>
#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "globals.h"
#include "map_world.h"
#include "crt.h"
#include "part_list.h"
#include "raw_field.h"
#include "vcall.h"
#include "light_tables_ports.h"
#include "radar_gump.h"
#include "dirty_rects_ports.h"
#include "shop_gump.h"
#include "map_world_ops_ports.h"
#include "world_obj_ops.h"
#include "mobile_anim_queue.h"
#include "bark_gump.h"
#include "container_gump.h"
#include "packet_builders_b25_ports.h"
#include "client_socket.h"
#include "small_methods_u1_ports.h"
#include "spell_icon_gump.h"
#include "circle_trans.h"
#include "sound_manager.h"
#include "sound_ops_ports.h"
#include "vtables.h"

// Pool and list globals (Ghidra addresses).
#define g_lightFlagList GLOBAL_AS(CObj *, g_objectManager_storage) // objects with tiledata flag 0x1000000
#define g_lightPanel GLOBAL_AS(void *, g_radarGump)
#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)
#undef g_soundManager   // the class header's typed form; this file addresses the object by offset
#define g_soundManager GLOBAL_PTR(void, g_soundManager_storage)
#define g_tickIndexHead GLOBAL_PTR(void, g_tickIndexHead_storage)
#define g_mobileSeed GLOBAL_AS(int, g_gameTick)

// ---------------------------------------------------------------------------
// Pool allocation
// ---------------------------------------------------------------------------

// 0x004514e0  size 0x155
// Pool "new" + init of a 0x78-byte object; ECX = graphic id. When the free list is empty a slab of
// 0x200 objects (operator new 0xf004: count cookie + 0x200 * 0x78) is vector-constructed and threaded
// onto the free list (slots 0x1ff down to 1; slot 0 is returned). The SEH frame is omitted.
CObj *__fastcall CObj_PoolNew(int graphic)
{
    char *obj;
    char *prevFree;
    char *slot;
    int n;

    if (g_poolFree == nullptr) {
        g_poolBuilding = 1;
        obj = reinterpret_cast<char *>(new CObj[0x200]);   // operator new 0xf004: count cookie + 0x200 * 0x78
        prevFree = g_poolFree;   // reloaded after the call, still 0 here
        g_poolBuilding = 0;
        g_poolTotal = g_poolTotal + 0x200;
        slot = obj + 0xef88;     // slot 0x1ff
        n = 0x1ff;
        do {
            *reinterpret_cast<char **>(slot + 0x40) = prevFree;
            prevFree = slot;
            slot = slot - 0x78;
            n--;
        } while (n != 0);
        g_poolFree = prevFree;
    } else {
        obj = g_poolFree;
        g_poolFree = *reinterpret_cast<char **>(obj + 0x40);
    }
    g_poolLive = g_poolLive + 1;

    CObj *o = reinterpret_cast<CObj *>(obj);
    o->link44 = nullptr;
    o->link40 = nullptr;
    o->link4c = nullptr;
    o->link48 = nullptr;
    o->unk_64 = 0;
    o->unk_60 = 0;
    o->onLightList = 0;
    o->link54 = nullptr;
    o->link50 = nullptr;
    o->link5c = nullptr;
    o->link58 = nullptr;
    o->graphic = 0;
    o->hue = 0;
    o->facing = 0;
    o->unlinked = 1;
    o->x = static_cast<int16_t>(0xffff);
    o->y = static_cast<int16_t>(0xffff);
    o->z = 0;
    o->barkList = nullptr;
    o->flags74 = 0;
    o->unk_3a = 0;
    o->unk_3c = 0;
    o->unk_70 = 0;
    o->graphic = static_cast<uint16_t>(graphic & 0x3fff);
    o->unk_3c = *reinterpret_cast<uint16_t *>(g_tileData + graphic * 0x28 + 8);
    o->hue = *reinterpret_cast<uint16_t *>(g_tileData + graphic * 0x28 + 0xe);
    return o;
}
HOOK_FUNC(0x004514e0, CObj_PoolNew);

// ---------------------------------------------------------------------------
// CObj
// ---------------------------------------------------------------------------

// 0x00451be0 / 0x00451c70: the constructor and destructor of CObj are inline (obj_inline.h).
HOOK_CTOR(0x00451be0, CObj, ());
HOOK_DTOR(0x00451c70, CObj);
HOOK_VTABLE(0x0049f7e0, CObj);
HOOK_VDD(0x00451640, CObj);

// 0x00452580  size 0x160
// Probably CObj::ToWorld(Coords): link the object into the world lists and set x/y/z.
// The SEH frame is omitted. Stack args: xy dword (x | y << 16) and z word, RET 8.
void __thiscall CObj::ToWorld(uint32_t xy, uint16_t zz)
{
    link44 = g_worldListHead;               // next = old head of the world list
    if (g_worldListHead != nullptr)
        g_worldListHead->link40 = this;     // old head->prev = this
    if (g_worldListTail == nullptr)
        g_worldListTail = this;
    g_worldListHead = this;
    link40 = nullptr;
    x = static_cast<int16_t>(xy);
    y = static_cast<int16_t>(xy >> 16);
    z = static_cast<int16_t>(zz);
    unlinked = 0;
    decomp::vcall_void(this, 0x10);

    if (*reinterpret_cast<uint32_t *>(g_tileData + graphic * 0x28) & 0x1000000) {
        CObj *old = g_lightFlagList;
        link50 = old;
        if (old != nullptr)
            old->link54 = this;
        link54 = nullptr;
        g_lightFlagList = this;
    }

    uint32_t flags = decomp::vcall(PORT_RET(uint32_t), this, 0x40);   // tiledata flags
    if (flags & 0x20000) {
        CObj *old = g_list20000Head;
        link58 = old;
        if (old != nullptr)
            old->link5c = this;
        link5c = nullptr;
        if (g_list20000Tail == nullptr)
            g_list20000Tail = this;
        g_list20000Head = this;
    }

    flags = decomp::vcall(PORT_RET(uint32_t), this, 0x40);
    if (flags & 0x800000) {
        if (onLightList == 0) {
            onLightList = 1;
            CObj *old = g_lightListHead;
            link48 = old;
            if (old != nullptr)
                old->link4c = this;
            link4c = nullptr;
            if (g_lightListTail == nullptr)
                g_lightListTail = this;
            g_lightListHead = this;
            (this->*PORT_METHOD0(CObj, void, &InvalidateLightRect))();
        }
        // the player-light block runs whenever the 0x800000 flag is set, independent of onLightList
        if (this == g_player) {
            int lightLevel = static_cast<CDynamicObj *>(this)->light;
            if (lightLevel != 0) {
                g_playerLight = g_playerLight + lightLevel;
                if (g_lightPanel != nullptr)
                    PORT_FUNC1(&CRadarGump::Redraw, void, __fastcall, void *)(g_lightPanel);
                PORT_FUNC0(&InvalidateAll, void, __cdecl)();
            }
        }
    }
}
HOOK_METHOD(0x00452580, CObj, ToWorld);

// 0x00453320  size 0xae
// Create a bark gump for this object, keeping at most two existing ones. The list at +0x68 is walked via
// next = +0xac to its tail, then the tail is deleted going backwards via prev = +0xb0 for (count - 2)
// entries (the prev link is read before the delete). The SEH frame is omitted. RET 0x10.
void *__thiscall CObj::CreateBarkKeepTwo(const char *text, uint16_t barkHue, uint16_t barkFont, CGump *param5)
{
    void *last = nullptr;
    int count = 0;
    void *g = barkList;
    while (g != nullptr) {
        last = g;
        g = RAWF(void *, g, 0xac);
        count++;
    }
    if (count >= 3) {
        int n = count - 2;
        do {
            void *older = RAWF(void *, last, 0xb0);
            if (last != nullptr)
                decomp::vdelete(last);
            n--;
            last = older;
        } while (n != 0);
    }
    return new CBarkGump(this, const_cast<char *>(text), barkHue, barkFont, param5);   // 0x004012a0
}
HOOK_METHOD(0x00453320, CObj, CreateBarkKeepTwo);

// 0x004533d0  size 0x87
// Create a bark gump for this object after deleting all existing ones. The SEH frame is omitted. RET 0x10.
void *__thiscall CObj::CreateBarkReplace(const char *text, uint16_t barkHue, uint16_t barkFont, CGump *param5)
{
    void *g = barkList;
    while (g != nullptr) {
        decomp::vdelete(g);
        g = barkList;
    }
    return new CBarkGump(this, const_cast<char *>(text), barkHue, barkFont, param5);   // 0x004012a0
}
HOOK_METHOD(0x004533d0, CObj, CreateBarkReplace);

// ---------------------------------------------------------------------------
// CDynamicObj
// ---------------------------------------------------------------------------

// 0x00451f70  size 0x1f7
// Destructor of the 0xa8-byte pooled object; ~CObj is inlined after it, then ~CWorldObj is called.
CDynamicObj::~CDynamicObj()
{
    if (g_suppressFlag == 0) {
        if (buf_a0 != nullptr)
            operator delete(buf_a0);
        PartList *p = partList;
        if (p != nullptr) {
            PORT_FUNC1(&PartList::Clear, void, __fastcall, PartList *)(p);   // destroy all entries
            operator delete(p);
        }
    }
    if (gump88 != nullptr) {
        if (gump88->owner == this)
            gump88->owner = nullptr;
    }
    if (gump8c != nullptr) {
        if (gump8c->owner == this)
            gump8c->owner = nullptr;
    }
    // A top-level gump whose virtual +0x90 is true and whose +0x9c == this is cleared and the scan STOPS;
    // otherwise, if virtual +0x110 is true, its children (head +0x44, next +0x3c) are scanned, a matching
    // child is cleared and the scan CONTINUES with the next top-level gump.
    CGump *gump = g_gumpListTail;
    while (gump != nullptr) {
        if (decomp::vcall(PORT_RET(int), gump, 0x90) != 0) {
            if (RAWF(void *, gump, 0x9c) == this) {
                RAWF(void *, gump, 0x9c) = nullptr;
                break;
            }
        }
        if (decomp::vcall(PORT_RET(int), gump, 0x110) != 0) {
            CGump *child = gump->lastChild;
            while (child != nullptr) {
                if (decomp::vcall(PORT_RET(int), child, 0x90) != 0) {
                    if (RAWF(void *, child, 0x9c) == this) {
                        RAWF(void *, child, 0x9c) = nullptr;
                        break;
                    }
                }
                child = child->prev;
            }
        }
        gump = gump->prev;
    }
    if (this == g_selectedObj)
        g_selectedObj = nullptr;
    if (unlinked == 0)
        this->CDynamicObj::ToVoid();
    // unlink from the list g_dynListHead (next +0x80) / g_dynListTail (prev +0x84)
    if (listNext != nullptr)
        listNext->listPrev = listPrev;
    else
        g_dynListTail = listPrev;
    if (listPrev != nullptr)
        listPrev->listNext = listNext;
    else
        g_dynListHead = listNext;
    g_childDeleteFlag = 1;
    while (barkList != nullptr)
        decomp::vdelete(barkList);   // virtual slot 0: deleting dtor
    g_childDeleteFlag = 0;
    // ~CObj (0x00451c70, inline) and ~CWorldObj (0x0042ad70) follow.
}
HOOK_DTOR(0x00451f70, CDynamicObj);
HOOK_VTABLE(0x0049f860, CDynamicObj);
HOOK_VDD(0x00451800, CDynamicObj);

// 0x004526e0  size 0x1b0
// Probably CObj::ToContainer(CContainerObj *, Coords): put this item into a container. The graphic of the
// CONTAINER selects the sorted variants. The SEH frame is omitted; 0x1904-byte frame.
void __thiscall CDynamicObj::ToContainer(CContainerObj *cont, uint32_t xy, uint16_t zz)
{
    char buf[0x1900];
    uint16_t cg = cont->graphic;

    if (cg == 0x2af8) {   // 11000
        ToContainerSortX(cont, xy, zz);
        return;
    }
    if (cg == 0x0fa6) {
        this->CDynamicObj::ToContainerSortY(cont, xy, zz);   // sort by y
        return;
    }
    if (cg == 0x0efa) {
        this->CDynamicObj::ToContainerSortQty(cont, xy, zz);   // sort by quantity
        return;
    }

    link40 = cont->firstChild;   // push-front into the container's child list
    if (cont->firstChild != nullptr)
        cont->firstChild->link44 = this;
    x = static_cast<int16_t>(xy);
    y = static_cast<int16_t>(xy >> 16);
    z = static_cast<int16_t>(zz);
    cont->firstChild = this;
    container = cont;
    unlinked = 0;

    CGump *gump = cont->contGump;
    if (gump != nullptr) {
        if (decomp::vcall(PORT_RET(int), gump, 0xd0) != 0) {
            decomp::vcall_void(cont->contGump, 0x114);
        } else if (decomp::vcall(PORT_RET(int), cont->contGump, 0x90) != 0) {
            decomp::thiscall_at_void(PORT_ADDR(&CContainerGump::AddToGump), cont->contGump, this);
        } else if (decomp::vcall(PORT_RET(int), cont->contGump, 0xdc) != 0) {
            decomp::thiscall_at_void(PORT_ADDR(&Pkt_Build_0x71_BBoardSimple), g_packetBuilder, buf, 4, cont->serial, serial);
            decomp::thiscall_at_void(PORT_ADDR(&CClientSocket::SendPacket), g_pktSender, buf);
        }
    }
    if (decomp::vcall(PORT_RET(int), cont, 0x54) != 0) {
        if (cont->container == nullptr) {
            decomp::vcall_void(cont, 0x14);
            decomp::vcall_void(cont, 0x10);
        }
    }
}
HOOK_METHOD(0x004526e0, CDynamicObj, ToContainer);

// 0x00452890  size 0xea
// CObj::ToContainerSortX(CContainerObj *, Coords): sorted insert by x into a container. The compare is a
// signed 16-bit x > node->x. The gump-list scan runs whether or not container+0xac was set. The SEH frame
// is omitted.
void __thiscall CDynamicObj::ToContainerSortX(CContainerObj *cont, uint32_t xy, uint16_t zz)
{
    CObj *prevNode = nullptr;
    CObj *cur = cont->firstChild;

    while (cur != nullptr && static_cast<int16_t>(xy) > cur->x) {
        prevNode = cur;
        cur = cur->link40;
    }
    link40 = cur;
    if (cur != nullptr)
        cur->link44 = this;
    link44 = prevNode;
    if (prevNode != nullptr)
        prevNode->link40 = this;
    if (cont->firstChild == cur)
        cont->firstChild = this;
    container = cont;
    x = static_cast<int16_t>(xy);
    y = static_cast<int16_t>(xy >> 16);
    z = static_cast<int16_t>(zz);
    unlinked = 0;
    if (cont->contGump != nullptr)
        decomp::thiscall_at_void(PORT_ADDR(&CContainerGump::AddToGump), cont->contGump, this);
    CGump *g = g_gumpListTail;
    while (g != nullptr) {
        if (decomp::vcall(PORT_RET(int), g, 0xa0) != 0 && g->owner == cont->container && RAWF(int32_t, g, 0xbc) == 0) {
            decomp::thiscall_at_void(PORT_ADDR(&CShopGump::CheckCartItem), g, serial);
            PORT_FUNC1(&CShopGump::RedrawFrame, void, __fastcall, CGump *)(g);
        }
        g = g->prev;
    }
}
HOOK_METHOD(0x00452890, CDynamicObj, ToContainerSortX);

HOOK_DTOR(0x0040caf0, Pos3);

// 0x00452f20  size 0x12c
// Is this object within `range` tiles (Chebyshev) of the view origin (g_map.viewX / viewY)? The player
// always passes. Mobiles use the step table when stepCount > 0; carried items widen the range by the part
// list extent ([this+0x94]->+0x10) and use the outermost owner's position. Compares are signed. RET 4.
int __thiscall CDynamicObj::IsInRange(int range)
{
    Pos3 pos;

    if (this == static_cast<CDynamicObj *>(g_player))
        return 1;
    if (decomp::vcall(PORT_RET(int), this, 0x24) != 0) {   // is a mobile
        CMobile *m = static_cast<CMobile *>(this);
        int n = m->stepCount;
        if (n > 0) {
            pos = *reinterpret_cast<Pos3 *>(&m->steps[n - 1]);
        } else {
            pos = *reinterpret_cast<Pos3 *>(&x);
        }
    } else if (decomp::vcall(PORT_RET(int), this, 0x2c) != 0) {   // carried by someone/something
        CObj *p = this;
        if (partList != nullptr)
            range = range + partList->maxExtent;
        while (static_cast<CDynamicObj *>(p)->container != nullptr)
            p = static_cast<CDynamicObj *>(p)->container;
        pos = *reinterpret_cast<Pos3 *>(&p->x);
    } else {
        pos = *reinterpret_cast<Pos3 *>(&x);
    }
    if (abs(pos.x - g_map.viewX) <= range && abs(pos.y - g_map.viewY) <= range)
        return 1;
    return 0;
}
HOOK_METHOD(0x00452f20, CDynamicObj, IsInRange);


// 0x00452e10  size 0x10e
// Is this object reachable: within 3 tiles and the line-of-sight test (0x00446190, thiscall on the map
// with an object position, the player position (z + 0xc) and 0x40) passes. The SEH frame is omitted.
int __thiscall CDynamicObj::IsReachable()
{
    if (IsInRange(3) == 0)
        return 0;
    Pos3 objPos = *reinterpret_cast<Pos3 *>(&x);
    if (decomp::vcall(PORT_RET(int), this, 0x2c) != 0) {   // is carried / has an owner
        CObj *p = this;
        while (static_cast<CDynamicObj *>(p)->container != nullptr)
            p = static_cast<CDynamicObj *>(p)->container;   // outermost owner
        objPos = *reinterpret_cast<Pos3 *>(&p->x);
    }
    Pos3 playerPos = *reinterpret_cast<Pos3 *>(&g_player->x);
    playerPos.z = playerPos.z + 0xc;
    return PORT_THISCALL3(&Map_LineOfSightClear, int, GLOBAL_PTR(void, g_map_storage), Pos3, Pos3, int)(objPos, playerPos, 0x40) != 0 ? 1 : 0;
}
HOOK_METHOD(0x00452e10, CDynamicObj, IsReachable);

// ---------------------------------------------------------------------------
// Item names
// ---------------------------------------------------------------------------

// 0x00453050  size 0x131
// Builds an item display name into the static buffer at 0x00721088 and returns it. ECX = tile graphic id,
// EDX = flag. flag == 0: optional article (tiledata flags & 0xc000: 0x4000 "a ", 0x8000 "an ",
// 0xc000 "the ") and, inside %...%, SKIP the text before '/' and copy the text after it. flag != 0: no
// article and, inside %...%, copy the text before '/' (or '%') and skip the rest up to the closing '%'.
char *__fastcall BuildItemName(int graphic, int flag)
{
    const char *src;
    char *dst;
    uint32_t bits;

    g_itemNameBuf[0] = 0;
    if (flag == 0) {
        bits = *reinterpret_cast<uint32_t *>(g_tileData + graphic * 0x28) & 0xc000;
        switch (bits) {
        case 0x4000:
            strcat(g_itemNameBuf, reinterpret_cast<const char *>(GADDR(str_004ad174)));
            break;
        case 0x8000:
            strcat(g_itemNameBuf, reinterpret_cast<const char *>(GADDR(str_004ad178)));
            break;
        case 0xc000:
            strcat(g_itemNameBuf, reinterpret_cast<const char *>(GADDR(str_004ad17c)));
            break;
        }
    }
    src = reinterpret_cast<const char *>(g_tileData + graphic * 0x28 + 0x13);   // tiledata name
    dst = g_itemNameBuf + strlen(g_itemNameBuf);
    while (*src != 0) {
        if (*src == '%') {
            if (flag != 0) {
                src++;
                while (*src != 0 && *src != '/' && *src != '%') {
                    *dst = *src;
                    dst++;
                    src++;
                }
                while (*src != 0 && *src != '%')
                    src++;
                if (*src == '%')
                    src++;
            } else {
                src++;
                while (*src != 0 && *src != '/' && *src != '%')
                    src++;
                if (*src == '/')
                    src++;
                while (*src != 0 && *src != '%') {
                    *dst = *src;
                    dst++;
                    src++;
                }
                if (*src == '%')
                    src++;
            }
        } else {
            *dst = *src;
            dst++;
            src++;
        }
    }
    *dst = 0;
    return g_itemNameBuf;
}
HOOK_FUNC(0x00453050, BuildItemName);

// ---------------------------------------------------------------------------
// Other CObj-derived destructors
// ---------------------------------------------------------------------------

// 0x004539e0  size 0xe3
// Destructor of the container-like class (vtable 0x0049f8e0). The children lists at +0xb0 and +0xa8 are
// drained with virtual slot +4 (not the deleting dtor); the gump at +0xac is detached from its owner's
// slots +0xb8/+0xbc only when virtual +0x90 is true and the owner (gump+0xa0) is non-null, then deleted
// via virtual slot 0 with arg 1. The SEH frame is omitted.
CContainerObj::~CContainerObj()
{
    if (unlinked == 0)
        this->CDynamicObj::ToVoid();
    CObj *p = list_b0;
    while (p != nullptr) {
        decomp::vcall_void(p, 4);
        p = list_b0;
    }
    CGump *g = contGump;
    if (g != nullptr) {
        if (decomp::vcall(PORT_RET(int), g, 0x90) != 0) {
            g = contGump;
            void *owner = RAWF(void *, g, 0xa0);
            if (owner != nullptr) {
                if (RAWF(CGump *, owner, 0xb8) == g)
                    RAWF(CGump *, owner, 0xb8) = nullptr;
                else if (RAWF(CGump *, owner, 0xbc) == g)
                    RAWF(CGump *, owner, 0xbc) = nullptr;
            }
        }
        g = contGump;
        if (g != nullptr)
            decomp::vdelete(g);
    }
    p = firstChild;
    while (p != nullptr) {
        decomp::vcall_void(p, 4);
        p = firstChild;
    }
}
HOOK_DTOR(0x004539e0, CContainerObj);
HOOK_VTABLE(0x0049f8e0, CContainerObj);
HOOK_SDD(0x00453900, CContainerObj);

// 0x00453bc0  size 0x9b
// Destructor of the class with vtable 0x0049f960. The object at +0xb0 is deleted with virtual slot 0 and
// arg 1; the singly linked list at +0xa8 is freed (next at node+8) and +0xa8 / +0xac are cleared before the
// base destructor. The SEH frame is omitted.
CObjVt49f960::~CObjVt49f960()
{
    if (unlinked == 0)
        this->CDynamicObj::ToVoid();
    if (obj_b0 != nullptr)
        decomp::vdelete(obj_b0);
    void *node = list_a8;
    while (node != nullptr) {
        void *following = RAWF(void *, node, 8);
        if (node != nullptr)
            operator delete(node);
        node = following;
    }
    list_a8 = nullptr;
    unk_ac = 0;
}
HOOK_DTOR(0x00453bc0, CObjVt49f960);
HOOK_VTABLE(0x0049f960, CObjVt49f960);
HOOK_SDD(0x00453af0, CObjVt49f960);

// 0x00453e90  size 0x55
// Destructor of the class with vtable 0x0049f9e0 (derives from the 0x0049f8e0 class). The SEH frame is omitted.
CObjVt49f9e0::~CObjVt49f9e0()
{
    if (unlinked == 0)
        this->CDynamicObj::ToVoid();
}
HOOK_DTOR(0x00453e90, CObjVt49f9e0);
HOOK_VTABLE(0x0049f9e0, CObjVt49f9e0);
HOOK_SDD(0x00453e00, CObjVt49f9e0);


// 0x00454a10  size 0x1f0
// Footstep sound for a mobile. Calls 0x004548e0 (ECX = graphic, EDX = facing byte, 10 stack args, RET 0x28)
// for the animation id and frame, then plays sound 0x12b/0x12c (0x129/0x12a for animation 0x18) on the
// step frames. Player: volume -1000 through 0x004800f0; others: position this+0x20 and 1.0f through 0x00480010.
void __thiscall CMobile::FootstepSound()
{
    int anim;
    int frame;

    if (graphic >= 0x3e8)
        return;
    if (decomp::vcall(PORT_RET(int), this, 0x8c) != 0)
        return;
    if (flag148 != 0)
        return;
    if (flags98 & 0x80)
        return;
    int mounted;
    if (graphic >= 0x190 && equip[25] != nullptr)   // +0x128: the mount slot
        mounted = 1;
    else
        mounted = 0;
    int fixedFrame = decomp::vcall(PORT_RET(int), this, 0x84);
    if (decomp::vcall(PORT_RET(int), this, 0x28) != 0) {   // is the player
        CObj *item = equip[1];   // +0xc8
        int war;
        if (item != nullptr && (*reinterpret_cast<uint32_t *>(g_tileData + item->graphic * 0x28) & 0x800000) != 0)
            war = 1;
        else
            war = 0;
        GetMobileAnimation(
            graphic, facing, g_walkTick, fixedFrame, animFrame, unk_1b8 + unk_1ac, g_walkDirState, mounted, flag148, war, &anim, &frame);
    } else {
        GetMobileAnimation(
            graphic, facing, moveTick, fixedFrame, animFrame, unk_1b8 + unk_1ac, moving, mounted, 0, 0, &anim, &frame);
    }
    int sound = 0xffff;
    if (graphic >= 0x190) {
        if (anim >= 0 && anim <= 2 && g_ccVal2cc != 0) {
            if (frame == 1)
                sound = 0x12b;
            else if (frame == 6)
                sound = 0x12c;
        } else if (anim == 3 && g_ccVal2cc != 0) {
            if (frame == 1)
                sound = 0x12b;
            else if (frame == 6)
                sound = 0x12c;
        } else if (anim <= 7 && g_ccVal2cc != 0) {
            if (frame == 1)
                sound = 0x12b;
            else if (frame == 6)
                sound = 0x12c;
        } else if (anim == 0x17 && g_ccVal2cc != 0) {
            if (frame == 1)
                sound = 0x12b;
            else if (frame == 6)
                sound = 0x12c;
        } else if (anim == 0x18 && g_ccVal2cc != 0) {
            if (frame == 1)
                sound = 0x129;
            else if (frame == 3)
                sound = 0x12a;
        }
    }
    if (sound != 0xffff) {
        if (decomp::vcall(PORT_RET(int), this, 0x28) != 0)
            decomp::thiscall_at_void(PORT_ADDR(&CSoundManager::PlaySound), g_soundManager, sound, 0, -1000, 0, 0xe, 1);
        else
            decomp::thiscall_at_void(PORT_ADDR(&PlaySound3D), g_soundManager, sound, static_cast<void *>(&x), 0x3f800000u, 0, 0xe, 1);
    }
}
HOOK_VMETHOD(0x00454a10, CMobile, FootstepSound);
