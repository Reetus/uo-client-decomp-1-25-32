// CObj / CDynamicObj / CContainerObj operations: hue and status helpers, the pending-update table,
// moving objects between the world, containers and equipment, the container and list classes.
// 0x00452170 .. 0x00453e30 (porting pass 2, batch 20). The SEH frames of the original are omitted.
#include <new>

#include "obj.h"
#include "obj_inline.h"

#include "../runtime/ported.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "light_tables_ports.h"
#include "world_obj_ops.h"
#include "paperdoll_gump.h"
#include "shop_gump.h"
#include "container_gump.h"
#include "bb_gump.h"
#include "radar_gump.h"
#include "dirty_rects_ports.h"
#include "client_misc_ops_ports.h"
#include "art_record_ops_ports.h"
#include "gump_art_ops_ports.h"
#include "vtables.h"

// Lists and globals (Ghidra addresses).
#define g_worldListTail GLOBAL_AS(CObj *, g_objMgr_storage)    // oldest object of the world list (next at +0x40)
#define g_animObjList GLOBAL_AS(CObj *, g_objectManager_storage)      // objects with tiledata flag 0x1000000 (next at +0x50)

// One entry of the pending object update table (16 bytes, array pointer at 0x00721194, count at 0x00721198).
struct PendingUpdate {
    uint32_t f0;
    uint32_t f1;    // serial the update is for
    uint32_t f2;    // facing byte
    uint32_t f3;
};

// Original code not ported here.
#define Item_Invalidate(o) PORT_THISCALL0(&InvalidateLightRect, void, (o))()                     // ECX = item
#define World_Invalidate(self) (reinterpret_cast<CWorldObj *>(self)->Invalidate())                    // ECX = object (CWorldObj::Invalidate)
#define Paperdoll_Redraw(self) (reinterpret_cast<CPaperdollGump *>(self)->RedrawFrame())                    // ECX = paperdoll gump
#define Shop_CheckCartItem(self, a1) (reinterpret_cast<CShopGump *>(self)->CheckCartItem(a1))             // ECX = shop gump, serial
#define Shop_RedrawFrame(self) (reinterpret_cast<CShopGump *>(self)->RedrawFrame())                    // ECX = shop gump
#define ContGump_Redraw(self, a1) (reinterpret_cast<CContainerGump *>(self)->RedrawFrame(a1))        // ECX = container gump, clip rect
#define ContGump_AddToGump(self, a1) (reinterpret_cast<CContainerGump *>(self)->AddToGump(a1))          // ECX = container gump, item
#define BBGump_RemoveLine(self, a1) (reinterpret_cast<CBBGump *>(self)->RemoveLine((CGump *)(a1)))           // ECX = bulletin board, line gump
#define Radar_Redraw(self) (reinterpret_cast<CRadarGump *>(self)->Redraw())                        // ECX = radar gump
#define Screen_InvalidateAll() (InvalidateAll())
#define IsGameMaster() (Misc_Return0())                    // ? calling convention not annotated
#define ArtTable_Get(o, a1) PORT_THISCALL1(&ArtRecord_Get, void *, (o), int)(a1)                 // ECX = 0x0068d2d8, art id
#define GumpArtSize(a1, a2, a3) (GumpArtSizeImpl(a1, a2, a3))

static inline CGump *GumpPrev(CGump *g)
{
    return RAWF(CGump *, g, 0x3c);
}

// ---------------------------------------------------------------------------
// Hue and status helpers
// ---------------------------------------------------------------------------

// 0x00452170  size 0x28
// Keeps bit 15 of the hue; the low 15 bits only when they are below 0xbb8, otherwise the low bits become 1.
uint32_t __fastcall CObj_SanitizeHue(uint32_t hue)
{
    uint32_t flag = hue & 0x8000;
    int32_t low = static_cast<int32_t>(hue & 0x7fff);
    if (low == 0)
        return flag;
    if (low < 0 || low >= 0xbb8)
        return flag | 1;
    return static_cast<uint32_t>(low) | flag;
}
HOOK_FUNC(0x00452170, CObj_SanitizeHue);

// 0x004521a0  size 0x74
// Set the status byte (+0x98). For a mobile (virtual slot 0x24): when another mobile becomes hidden (bit 7) all
// gumps owned by it are deleted first; +0x150 gets bit 6 of the flags, cleared again when slot 0x8c says so.
void __fastcall CDynamicObj_SetStatusFlags(CDynamicObj *obj, uint32_t flags)
{
    if (decomp::vcall(PORT_RET(int), obj, 0x24) != 0) {
        if (obj != g_player && (flags & 0x80) != 0 && (obj->flags98 & 0x80) == 0) {
            CGump *g = g_gumpListTail;
            while (g != nullptr) {
                if (g->owner == obj) {
                    decomp::vdelete(g);
                    g = g_gumpListTail;
                } else {
                    g = GumpPrev(g);
                }
            }
        }
        RAWF(int32_t, obj, 0x150) = static_cast<int32_t>((flags >> 6) & 1);
        if (decomp::vcall(PORT_RET(int), obj, 0x8c) != 0)
            RAWF(int32_t, obj, 0x150) = 0;
    }
    obj->flags98 = static_cast<uint8_t>(flags);
}
HOOK_FUNC(0x004521a0, CDynamicObj_SetStatusFlags);

// ---------------------------------------------------------------------------
// The +0x60/+0x64 list (DAT_0068ce70 / DAT_0068ce74)
// ---------------------------------------------------------------------------

// 0x00452220  size 0x2c
void __thiscall CObj::LinkList70()
{
    CObj *oldHead = g_list70Head;
    RAWF(CObj *, this, 0x60) = oldHead;
    if (oldHead != nullptr)
        RAWF(CObj *, oldHead, 0x64) = this;
    RAWF(CObj *, this, 0x64) = nullptr;
    CObj *oldTail = g_list70Tail;
    g_list70Head = this;
    if (oldTail == nullptr)
        g_list70Tail = this;
}
HOOK_METHOD(0x00452220, CObj, LinkList70);

// 0x00452250  size 0x3c
void __thiscall CObj::UnlinkList70()
{
    if (g_list70Head == this)
        g_list70Head = RAWF(CObj *, this, 0x60);
    if (g_list70Tail == this)
        g_list70Tail = RAWF(CObj *, this, 0x64);
    CObj *n = RAWF(CObj *, this, 0x60);
    if (n != nullptr)
        RAWF(CObj *, n, 0x64) = RAWF(CObj *, this, 0x64);
    CObj *p = RAWF(CObj *, this, 0x64);
    if (p != nullptr)
        RAWF(CObj *, p, 0x60) = RAWF(CObj *, this, 0x60);
}
HOOK_METHOD(0x00452250, CObj, UnlinkList70);

// ---------------------------------------------------------------------------
// Pending object updates
// ---------------------------------------------------------------------------

// 0x00452290  size 0x56
// ECX = key. The first pending update whose field 1 equals the key names (field 0) an item of the global
// list; returns that item, or null when there is no such update or item.
CDynamicObj *__fastcall PendingUpdate_FindObj(uint32_t key)
{
    int n = g_pendingCount;
    int i = 0;
    if (n <= 0)
        return nullptr;
    PendingUpdate *arr = g_pendingUpdates;
    while (arr[i].f1 != key) {
        i++;
        if (i >= n)
            return nullptr;
    }
    CDynamicObj *o = g_dynListHead;
    if (o == nullptr)
        return nullptr;
    uint32_t serial = arr[i].f0;
    while (static_cast<uint32_t>(o->serial) != serial) {
        o = o->listNext;
        if (o == nullptr)
            return nullptr;
    }
    return o;
}
HOOK_FUNC(0x00452290, PendingUpdate_FindObj);

// 0x004522f0  size 0x36
// ECX = a, EDX = b: 1 when some pending update has field 0 == a or field 1 == b.
int __fastcall PendingUpdate_Has(uint32_t a, uint32_t b)
{
    int n = g_pendingCount;
    for (int i = 0; i < n; i++) {
        if (g_pendingUpdates[i].f0 == a || g_pendingUpdates[i].f1 == b)
            return 1;
    }
    return 0;
}
HOOK_FUNC(0x004522f0, PendingUpdate_Has);

// 0x00452330  size 0x155
// ECX = a, EDX = b, one stack argument c (RET 4). Walks the table: an entry whose field 0 equals the current a
// or whose field 1 equals the current b is taken (a and b become that entry's fields), the later entries are
// shifted down, the count drops by one and the item with serial a gets facing (field 2 & 7) with bit 7 set
// when field 3 is zero, then is moved out of and back into the world when it is in it. The index still
// advances after a removal, so the entry that slid into its place is skipped. Afterwards, when c is zero,
// the item with serial b (the last b) is destroyed through virtual slot 4.
void __fastcall PendingUpdate_Apply(uint32_t a, uint32_t b, int c)
{
    uint32_t curA = a;
    uint32_t curB = b;
    int n = g_pendingCount;
    int i = 0;
    if (n > 0) {
        do {
            PendingUpdate *arr = g_pendingUpdates;
            uint32_t f0 = arr[i].f0;
            if (f0 == curA || arr[i].f1 == curB) {
                curA = f0;
                uint32_t f1 = arr[i].f1;
                uint32_t f2 = arr[i].f2;
                curB = f1;
                uint32_t f3 = arr[i].f3;
                if (i < n - 1 && i + 1 < n) {
                    int j = i + 1;
                    do {
                        arr[j - 1].f0 = arr[j].f0;
                        arr[j - 1].f1 = arr[j].f1;
                        arr[j - 1].f2 = arr[j].f2;
                        arr[j - 1].f3 = arr[j].f3;
                        j++;
                        n = g_pendingCount;
                        arr = g_pendingUpdates;
                    } while (j < n);
                }
                n = g_pendingCount - 1;
                g_pendingCount = n;
                CDynamicObj *o = g_dynListHead;
                while (o != nullptr && static_cast<uint32_t>(o->serial) != curA)
                    o = o->listNext;
                if (o != nullptr) {
                    uint8_t f = static_cast<uint8_t>(f2) & 7;
                    uint8_t extra = (f3 == 0) ? 0x80 : 0;
                    o->facing = static_cast<uint8_t>(f | extra);
                    if (o->inWorld != 0) {
                        decomp::vcall_void(o, 0x14);
                        decomp::vcall_void(o, 0x10);
                        n = g_pendingCount;
                    }
                }
            }
            i++;
        } while (i < n);
    }
    if (c == 0) {
        CDynamicObj *o = g_dynListHead;
        while (o != nullptr && static_cast<uint32_t>(o->serial) != curB)
            o = o->listNext;
        if (o != nullptr)
            decomp::vcall_void(o, 4);
    }
}
HOOK_FUNC(0x00452330, PendingUpdate_Apply);

// 0x00452490  size 0xed
// ECX = a, EDX = b, stack c and d (RET 8). Unless an entry with field 0 == a exists, grow the table by one
// entry (new array, copy, free the old one) and append {a, b, c, d}.
void __fastcall PendingUpdate_Add(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    int n = g_pendingCount;
    for (int i = 0; i < n; i++) {
        if (g_pendingUpdates[i].f0 == a)
            return;
    }
    PendingUpdate *fresh = static_cast<PendingUpdate *>(::operator new(static_cast<size_t>(n + 1) << 4));
    n = g_pendingCount;
    if (n != 0) {
        for (int i = 0; i < n; i++) {
            fresh[i].f0 = g_pendingUpdates[i].f0;
            fresh[i].f1 = g_pendingUpdates[i].f1;
            fresh[i].f2 = g_pendingUpdates[i].f2;
            fresh[i].f3 = g_pendingUpdates[i].f3;
            n = g_pendingCount;
        }
    }
    if (g_pendingUpdates != nullptr) {
        operator delete(g_pendingUpdates);
        n = g_pendingCount;
    }
    fresh[n].f0 = a;
    fresh[g_pendingCount].f1 = b;
    fresh[g_pendingCount].f2 = c;
    fresh[g_pendingCount].f3 = d;
    int newCount = g_pendingCount + 1;
    g_pendingUpdates = fresh;
    g_pendingCount = newCount;
}
HOOK_FUNC(0x00452490, PendingUpdate_Add);

// ---------------------------------------------------------------------------
// Moving objects between the world, containers and equipment
// ---------------------------------------------------------------------------

// 0x00452980  size 0xa3
// Insert into the container's child list (next at +0x40, prev at +0x44) before the first child whose y is not
// above this one's, set x / y / z and tell the container gump. RET 0xc.
void __thiscall CDynamicObj::ToContainerSortY(CContainerObj *cont, uint32_t xy, uint16_t zz)
{
    int16_t newY = static_cast<int16_t>(xy >> 16);
    CObj *prevNode = nullptr;
    CObj *node = cont->firstChild;
    while (node != nullptr && newY < node->y) {
        prevNode = node;
        node = node->link40;
    }
    link40 = node;
    if (node != nullptr)
        node->link44 = this;
    link44 = prevNode;
    if (prevNode != nullptr)
        prevNode->link40 = this;
    if (cont->firstChild == node)
        cont->firstChild = this;
    container = cont;
    x = static_cast<int16_t>(xy);
    y = newY;
    z = static_cast<int16_t>(zz);
    unlinked = 0;
    if (cont->contGump != nullptr)
        ContGump_AddToGump(cont->contGump, this);   // 0x004274f0
}
HOOK_METHOD(0x00452980, CDynamicObj, ToContainerSortY);

// 0x00452a30  size 0xba
// Same, ordered by quantity (+0x3c, unsigned word): before the first child whose quantity is not below this one's.
void __thiscall CDynamicObj::ToContainerSortQty(CContainerObj *cont, uint32_t xy, uint16_t zz)
{
    CObj *prevNode = nullptr;
    CObj *node = cont->firstChild;
    if (node != nullptr) {
        uint16_t qty = unk_3c;
        while (qty > node->unk_3c) {
            prevNode = node;
            node = node->link40;
            if (node == nullptr)
                break;
        }
    }
    link40 = node;
    if (node != nullptr)
        node->link44 = this;
    link44 = prevNode;
    if (prevNode != nullptr)
        prevNode->link40 = this;
    if (cont->firstChild == node)
        cont->firstChild = this;
    container = cont;
    x = static_cast<int16_t>(xy);
    y = static_cast<int16_t>(xy >> 16);
    z = static_cast<int16_t>(zz);
    unlinked = 0;
    if (cont->contGump != nullptr) {
        if (decomp::vcall(PORT_RET(int), cont->contGump, 0xd0) != 0)
            decomp::vcall_void(cont->contGump, 0x114);
    }
}
HOOK_METHOD(0x00452a30, CDynamicObj, ToContainerSortQty);

// 0x00452af0  size 0x246
// Take the object out of the world and every list it is in (animated, light, flag 0x20000 and the world
// list), leaving a mobile's equipment slot when its container is a mobile (redrawing the paperdolls whose
// owner is this object, as the original compares), or the child list of a container; the shop gump that
// showed it (container graphic 0x2af8) re-checks its cart afterwards.
void __thiscall CObj::ToVoid()
{
    CGump *shopGump = nullptr;   // [ebp-4]

    if (inWorld != 0)
        decomp::vcall_void(this, 0x14);

    if (*reinterpret_cast<uint32_t *>(g_tileData + graphic * 0x28) & 0x1000000) {
        if (link50 != nullptr)
            link50->link54 = link54;
        if (link54 != nullptr)
            link54->link50 = link50;
        if (g_animObjList == this)
            g_animObjList = link50;
        link54 = nullptr;
        link50 = nullptr;
    }

    if (onLightList != 0) {
        if (g_lightListHead == this)
            g_lightListHead = link48;
        if (g_lightListTail == this)
            g_lightListTail = link4c;
        if (link48 != nullptr)
            link48->link4c = link4c;
        if (link4c != nullptr)
            link4c->link48 = link48;
        onLightList = 0;
        Item_Invalidate(this);   // 0x0044f140
    }

    if (decomp::vcall(PORT_RET(uint32_t), this, 0x40) & 0x20000) {
        if (g_list20000Head == this)
            g_list20000Head = link58;
        if (g_list20000Tail == this)
            g_list20000Tail = link5c;
        if (link58 != nullptr)
            link58->link5c = link5c;
        if (link5c != nullptr)
            link5c->link58 = link58;
    }

    CObj *&owner = RAWF(CObj *, this, 0x7c);
    if (decomp::vcall(PORT_RET(int), this, 0x2c) != 0 && owner != nullptr) {
        if (decomp::vcall(PORT_RET(int), owner, 0x24) != 0) {
            CObj *cont = owner;
            uint32_t layer = RAWF(uint8_t, this, 0x99);
            if (layer == 0x19)
                RAWF(int32_t, cont, 0x194) = 0;
            if (layer == 1 || layer == 2)
                RAWF(uint8_t, cont, 0x1a0) = 0;
            if (RAWF(CObj *, cont, 0xc4 + layer * 4) == this) {
                RAWF(CObj *, cont, 0xc4 + layer * 4) = nullptr;
                for (CGump *g = g_gumpListTail; g != nullptr; g = GumpPrev(g)) {
                    if (decomp::vcall(PORT_RET(int), g, 0x78) != 0 && g->owner == this)
                        Paperdoll_Redraw(g);   // 0x00467680
                }
                owner = nullptr;
            }
        } else {
            CObj *cont = owner;
            if (RAWF(CObj *, cont, 0xa8) == this)
                RAWF(CObj *, cont, 0xa8) = link40;
            owner = nullptr;
            if (cont->graphic == 0x2af8) {
                for (CGump *g = g_gumpListTail; g != nullptr; g = GumpPrev(g)) {
                    if (decomp::vcall(PORT_RET(int), g, 0xa0) != 0 && g->owner == RAWF(CObj *, cont, 0x7c) &&
                        RAWF(int32_t, g, 0xbc) == 0)
                        shopGump = g;
                }
            }
        }
    }

    if (g_worldListHead == this)
        g_worldListHead = link44;
    if (g_worldListTail == this)
        g_worldListTail = link40;
    if (link44 != nullptr)
        link44->link40 = link40;
    if (link40 != nullptr)
        link40->link44 = link44;
    link44 = nullptr;
    link40 = nullptr;
    unlinked = 1;
    if (shopGump != nullptr) {
        Shop_CheckCartItem(shopGump, RAWF(int32_t, this, 0x78));   // 0x0046d140
        Shop_RedrawFrame(shopGump);                                // 0x0046d280
    }
}
HOOK_VMETHOD(0x00452af0, CObj, ToVoid);

// 0x00452d40  size 0xc8
// Equip this object on `mobile` at `layer` (stored at +0x99): fills the mobile's equipment slot, flags the
// two-hand / mount state, links the object into the light list when it emits light and redraws the paperdolls
// owned by the mobile. RET 8.
void __thiscall CObj::ToEquipped(CMobile *mobile, int layer)
{
    RAWF(uint8_t, this, 0x99) = static_cast<uint8_t>(layer);
    RAWF(CObj *, mobile, 0xc4 + layer * 4) = this;
    RAWF(CObj *, this, 0x7c) = mobile;
    link44 = nullptr;
    link40 = nullptr;
    unlinked = 0;
    if (layer == 2 || layer == 1)
        RAWF(uint8_t, mobile, 0x1a0) = 1;
    if (layer == 0x19)
        RAWF(int32_t, mobile, 0x194) = 1;
    if (layer == 0)
        return;
    if (decomp::vcall(PORT_RET(uint32_t), this, 0x40) & 0x800000) {
        if (onLightList == 0) {
            onLightList = 1;
            link48 = g_lightListHead;
            if (link48 != nullptr)
                link48->link4c = this;
            link4c = nullptr;
            CObj *oldTail = g_lightListTail;
            g_lightListHead = this;
            if (oldTail == nullptr)
                g_lightListTail = this;
            Item_Invalidate(this);   // 0x0044f140
        }
    }
    for (CGump *g = g_gumpListTail; g != nullptr; g = GumpPrev(g)) {
        if (decomp::vcall(PORT_RET(int), g, 0x78) != 0 && g->owner == mobile)
            Paperdoll_Redraw(g);   // 0x00467680
    }
}
HOOK_METHOD(0x00452d40, CObj, ToEquipped);

// ---------------------------------------------------------------------------
// Name, art and tiledata queries
// ---------------------------------------------------------------------------

// 0x00453190  size 0x2d
// BuildItemName(graphic + signed offset, flag) where flag is 1 when tiledata flag 0x800 (virtual slot 0x40)
// is set and the quantity is above 1.
char *__thiscall CObj::ItemName()
{
    uint32_t tileFlags = decomp::vcall(PORT_RET(uint32_t), this, 0x40);
    int flag = (tileFlags & 0x800) != 0 && unk_3c > 1;
    return BuildItemName(static_cast<int8_t>(unk_3a) + graphic, flag);   // 0x00453050
}
HOOK_METHOD(0x00453190, CObj, ItemName);

// 0x004531c0  size 0x92
// Art record (table at 0x0068d2d8) of the object: graphics above 0x4000 have none. When virtual slot 0x7c is
// set the art id is graphic + 0x4000 / 0x4001 / 0x4002 for quantity <= 1 / <= 5 / more, otherwise
// graphic + signed offset (+0x3a) + 0x4000.
void *__thiscall CObj::ArtRecord()
{
    if (graphic > 0x4000)
        return nullptr;
    if (decomp::vcall(PORT_RET(int), this, 0x7c) != 0) {
        if (unk_3c <= 1)
            return ArtTable_Get(GLOBAL_PTR(void, g_artCache_storage), graphic + 0x4000);   // 0x00476130
        if (unk_3c <= 5)
            return ArtTable_Get(GLOBAL_PTR(void, g_artCache_storage), graphic + 0x4001);   // 0x00476130
        return ArtTable_Get(GLOBAL_PTR(void, g_artCache_storage), graphic + 0x4002);       // 0x00476130
    }
    return ArtTable_Get(GLOBAL_PTR(void, g_artCache_storage), static_cast<int8_t>(unk_3a) + graphic + 0x4000);   // 0x00476130
}
HOOK_VMETHOD(0x004531c0, CObj, ArtRecord);

// 0x004532c0  size 0x39
int __thiscall CObj::IsStackedDisplay()
{
    int tile = static_cast<int8_t>(unk_3a) + graphic;
    if (*reinterpret_cast<uint32_t *>(g_tileData + tile * 0x28) & 0x800) {
        if (decomp::vcall(PORT_RET(int), this, 0x7c) == 0) {
            if (unk_3c > 1)
                return 1;
        }
    }
    return 0;
}
HOOK_VMETHOD(0x004532c0, CObj, IsStackedDisplay);

// 0x00453460  size 0x39
int __thiscall CDynamicObj::IsEquipped()
{
    CObj *cont = container;
    if (cont != nullptr && decomp::vcall(PORT_RET(int), cont, 0x24) != 0) {
        for (int i = 0; i < 0x1e; i++) {
            if (RAWF(CObj *, cont, 0xc4 + i * 4) == this)
                return 1;
        }
        return 0;
    }
    return 0;
}
HOOK_VMETHOD(0x00453460, CDynamicObj, IsEquipped);

// 0x004534a0  size 0x3b
int __thiscall CDynamicObj::IsMovable()
{
    if (IsGameMaster() != 0)   // 0x00478cd0
        return 1;
    if ((flags98 & 0x20) != 0)
        return 1;
    int tile = graphic + static_cast<int8_t>(unk_3a);
    if (g_tileData[tile * 0x28 + 4] <= 0x5a)
        return 1;
    return 0;
}
HOOK_VMETHOD(0x004534a0, CDynamicObj, IsMovable);

// 0x004534e0  size 0x3c0
// Dynamic-object version of ToVoid. A container-less item that leaves the world with a light level takes it
// off the player's light sum (and refreshes the radar). Everything the item sat in is redrawn: the owner mobile's
// paperdolls, or the container gumps and their children showing the owner (clip rect from the art size);
// the bulletin board line gump (+0x88) is removed, the gumps at +0x8c and +0x90 deleted.
void __thiscall CDynamicObj::ToVoid()
{
    CObj *cont = nullptr;        // [ebp-4]
    CGump *shopGump = nullptr;   // [ebp-8]
    int rect[4];                 // [ebp-0x20]: left, top, right, bottom

    if (container == nullptr) {
        if (inWorld != 0)
            decomp::vcall_void(this, 0x14);
    } else if (decomp::vcall(PORT_RET(int), container, 0x24) != 0) {
        World_Invalidate(container);   // 0x0042bcf0
    }

    if (*reinterpret_cast<uint32_t *>(g_tileData + graphic * 0x28) & 0x1000000) {
        if (link50 != nullptr)
            link50->link54 = link54;
        if (link54 != nullptr)
            link54->link50 = link50;
        if (g_animObjList == this)
            g_animObjList = link50;
        link54 = nullptr;
        link50 = nullptr;
    }

    if (onLightList != 0) {
        if (g_lightListHead == this)
            g_lightListHead = link48;
        if (g_lightListTail == this)
            g_lightListTail = link4c;
        if (link48 != nullptr)
            link48->link4c = link4c;
        if (link4c != nullptr)
            link4c->link48 = link48;
        onLightList = 0;
        Item_Invalidate(this);   // 0x0044f140
    }

    if (decomp::vcall(PORT_RET(uint32_t), this, 0x40) & 0x20000) {
        if (g_list20000Head == this)
            g_list20000Head = link58;
        if (g_list20000Tail == this)
            g_list20000Tail = link5c;
        if (link58 != nullptr)
            link58->link5c = link5c;
        if (link5c != nullptr)
            link5c->link58 = link58;
    }

    if (container != nullptr) {
        if (decomp::vcall(PORT_RET(int), container, 0x54) != 0)
            World_Invalidate(container);   // 0x0042bcf0
        cont = container;
        if (decomp::vcall(PORT_RET(int), this, 0x58) != 0) {
            RAWF(CObj *, container, 0xc4 + RAWF(uint8_t, this, 0x99) * 4) = nullptr;
            for (CGump *g = g_gumpListTail; g != nullptr; g = GumpPrev(g)) {
                if (decomp::vcall(PORT_RET(int), g, 0x78) != 0 && g->owner == cont)
                    Paperdoll_Redraw(g);   // 0x00467680
            }
        } else if (RAWF(CObj *, container, 0xa8) == this) {
            RAWF(CObj *, container, 0xa8) = link40;
        }
        container = nullptr;
        if (cont->graphic == 0x2af8) {
            for (CGump *g = g_gumpListTail; g != nullptr; g = GumpPrev(g)) {
                if (decomp::vcall(PORT_RET(int), g, 0xa0) != 0 && g->owner == RAWF(CObj *, cont, 0x7c) &&
                    RAWF(int32_t, g, 0xbc) == 0)
                    shopGump = g;
            }
        }
    } else if (light != 0) {
        g_playerLight = g_playerLight - light;
        if (GLOBAL_AS(void *, g_radarGump) != nullptr)
            Radar_Redraw(GLOBAL_AS(void *, g_radarGump));   // 0x0046a4f0
        Screen_InvalidateAll();          // 0x0044fdf0
    }

    if (g_worldListHead == this)
        g_worldListHead = link44;
    if (g_worldListTail == this)
        g_worldListTail = link40;
    if (link44 != nullptr)
        link44->link40 = link40;
    if (link40 != nullptr)
        link40->link44 = link44;
    link44 = nullptr;
    link40 = nullptr;
    unlinked = 1;

    if (cont != nullptr) {
        bool haveRect = true;
        uint16_t gfx = graphic;
        if (gfx >= 0x3584 && gfx <= 0x35a1) {
            int w;
            int h;
            GumpArtSize(gfx - 0x2c69, &w, &h);   // 0x00433e70
            int left = x;
            rect[0] = left;
            rect[2] = w + left - 1;
            int top = y;
            rect[1] = top;
            rect[3] = h + top - 1;
        } else {
            uint16_t *art = decomp::vcall(PORT_RET(uint16_t *), this, 0x74);
            if (art == nullptr) {
                haveRect = false;
            } else {
                int left = x;
                rect[0] = left;
                rect[2] = art[2] + left - 1;
                int top = y;
                rect[1] = top;
                rect[3] = art[3] + top - 1;
            }
        }
        if (haveRect) {
            for (CGump *g = g_gumpListTail; g != nullptr; g = GumpPrev(g)) {
                if (decomp::vcall(PORT_RET(int), g, 0x90) != 0 && g->owner == cont) {
                    ContGump_Redraw(g, rect);   // 0x00426b70
                } else {
                    for (CGump *c = g->lastChild; c != nullptr; c = GumpPrev(c)) {
                        if (decomp::vcall(PORT_RET(int), c, 0x90) != 0 && c->owner == cont)
                            ContGump_Redraw(c, rect);   // 0x00426b70
                    }
                }
            }
        }
    }

    if (shopGump != nullptr) {
        Shop_CheckCartItem(shopGump, serial);   // 0x0046d140
        Shop_RedrawFrame(shopGump);             // 0x0046d280
    }

    if (gump88 != nullptr) {
        if (decomp::vcall(PORT_RET(int), gump88, 0xe0) != 0) {
            CGump *line = gump88;
            BBGump_RemoveLine(line->parent, line);   // 0x00403a10
        } else if (gump88 != nullptr) {
            decomp::vdelete(gump88);
        }
    }
    if (gump8c != nullptr)
        decomp::vdelete(gump8c);
    CGump *g90 = RAWF(CGump *, this, 0x90);
    if (g90 != nullptr)
        decomp::vdelete(g90);
}
HOOK_VMETHOD(0x004534e0, CDynamicObj, ToVoid);

// 0x004538a0  size 0x56
// For the player only: while linked, the light level difference goes into DAT_00721030 and the radar and
// screen are refreshed. The new level is stored either way. RET 4.
void __thiscall CDynamicObj::SetLight(int newLight)
{
    if (this == g_player && light != newLight) {
        if (unlinked == 0) {
            g_playerLight = g_playerLight + (newLight - light);
            if (GLOBAL_AS(void *, g_radarGump) != nullptr)
                Radar_Redraw(GLOBAL_AS(void *, g_radarGump));   // 0x0046a4f0
            Screen_InvalidateAll();          // 0x0044fdf0
        }
        light = newLight;
    }
}
HOOK_METHOD(0x004538a0, CDynamicObj, SetLight);

// ---------------------------------------------------------------------------
// Container, list and multi classes
// ---------------------------------------------------------------------------

// 0x00453930  size 0xa5
// Constructor: the dynamic object constructor (0x00451d80), the serial, the byte at +0x3a, the graphic and
// vtable 0x0049f8e0. A zero flag takes the object off the global list again (the list that 0x00451d80 put
// it on), and +0xb8 is cleared.
CContainerObj::CContainerObj(int ser, int gfx, uint8_t b3a, int flag)
{
    serial = ser;
    unk_3a = b3a;
    firstChild = nullptr;
    contGump = nullptr;
    list_b0 = nullptr;
    graphic = static_cast<uint16_t>(gfx & 0x3fff);
    if (flag == 0) {
        if (listNext != nullptr)
            listNext->listPrev = listPrev;
        if (listPrev != nullptr)
            listPrev->listNext = listNext;
        if (this == g_dynListHead)
            g_dynListHead = listNext;
        if (this == g_dynListTail)
            g_dynListTail = listPrev;
    }
    unk_b8 = 0;
}
HOOK_CTOR(0x00453930, CContainerObj, (int, int, uint8_t, int));

// The old name, for the callers that still construct in place (packet_handlers_*.cpp).
CContainerObj *__thiscall CContainerObj::Ctor(int ser, int gfx, uint8_t b3a, int flag)
{
    return new (this) CContainerObj(ser, gfx, b3a, flag);
}

// 0x00453ad0  size 0x1e
void __thiscall CContainerObj::DestroyChildren()
{
    while (firstChild != nullptr)
        decomp::vcall_void(firstChild, 4);
}
HOOK_METHOD(0x00453ad0, CContainerObj, DestroyChildren);

// 0x00453b20  size 0x9f
// Same as CContainerObj::Ctor with vtable 0x0049f960 and without the +0xb8 clear. RET 0x10.
CObjVt49f960::CObjVt49f960(int ser, int gfx, uint8_t b3a, int flag)
{
    serial = ser;
    unk_3a = b3a;
    list_a8 = nullptr;
    unk_ac = 0;
    obj_b0 = nullptr;
    graphic = static_cast<uint16_t>(gfx & 0x3fff);
    if (flag == 0) {
        if (listNext != nullptr)
            listNext->listPrev = listPrev;
        if (listPrev != nullptr)
            listPrev->listNext = listNext;
        if (this == g_dynListHead)
            g_dynListHead = listNext;
        if (this == g_dynListTail)
            g_dynListTail = listPrev;
    }
}
HOOK_CTOR(0x00453b20, CObjVt49f960, (int, int, uint8_t, int));

// The old name, for the callers that still construct in place (packet_handlers_*.cpp).
CObjVt49f960 *__thiscall CObjVt49f960::Ctor(int ser, int gfx, uint8_t b3a, int flag)
{
    return new (this) CObjVt49f960(ser, gfx, b3a, flag);
}

// A point of the +0xa8 list of CObjVt49f960 (operator new 0xc).
struct PointNode {
    uint16_t x;     // +0
    uint16_t y;     // +2
    uint32_t pad_4;
    PointNode *next;    // +8
};

// 0x00453c60  size 0x175
// Edit the singly linked point list at +0xa8 (count at +0xac). Command (low byte of the first argument), jump
// table at 0x00453dd8: 1 append a new last point, 2 insert a new point at index `idx` (0 = front), 3 set the
// coordinates of point `idx`, 4 remove point `idx`, 5 clear. Any other command does nothing. RET 0x10.
void __thiscall CObjVt49f960::ListOp(int cmd, int idx, int px, int py)
{
    PointNode *&head = reinterpret_cast<PointNode *&>(list_a8);
    PointNode *prevNode;    // esi
    PointNode *nextNode;    // edi

    switch (static_cast<uint32_t>((cmd & 0xff) - 1)) {
    case 0: {
        prevNode = head;
        if (prevNode != nullptr) {
            while (prevNode->next != nullptr)
                prevNode = prevNode->next;
            nextNode = prevNode->next;
        } else {
            nextNode = nullptr;
        }
        break;
    }
    case 1: {
        uint8_t at = static_cast<uint8_t>(idx);
        if (at == 0) {
            nextNode = head;
            prevNode = nullptr;
        } else {
            prevNode = head;
            int steps = at - 1;
            if (steps > 0) {
                do {
                    prevNode = prevNode->next;
                    steps--;
                } while (steps != 0);
            }
            nextNode = prevNode->next;
        }
        break;
    }
    case 3: {
        PointNode *first = head;
        int steps = idx & 0xff;
        PointNode *target = first;
        if (steps > 0) {
            do {
                target = target->next;
                steps--;
            } while (steps != 0);
        }
        if (target != first) {
            PointNode *before = first;
            PointNode *n = before->next;
            if (n != target) {
                do {
                    before = n;
                    n = before->next;
                } while (n != target);
            }
            before->next = target->next;
        } else {
            head = target->next;
        }
        if (target != nullptr)
            operator delete(target);
        unk_ac = unk_ac - 1;
        return;
    }
    case 2: {
        PointNode *p = head;
        int steps = idx & 0xff;
        if (steps > 0) {
            do {
                p = p->next;
                steps--;
            } while (steps != 0);
        }
        p->x = static_cast<uint16_t>(px);
        p->y = static_cast<uint16_t>(py);
        return;
    }
    case 4: {
        PointNode *n = head;
        if (n != nullptr) {
            do {
                PointNode *following = n->next;
                if (n != nullptr)
                    operator delete(n);
                n = following;
            } while (n != nullptr);
        }
        head = nullptr;
        unk_ac = 0;
        return;
    }
    default:
        return;
    }

    // cases 1 and 2 (append / insert): a new point between prevNode and nextNode
    PointNode *node = static_cast<PointNode *>(::operator new(0xc));
    node->x = static_cast<uint16_t>(px);
    node->y = static_cast<uint16_t>(py);
    node->next = nextNode;
    if (prevNode != nullptr) {
        prevNode->next = node;
        unk_ac = unk_ac + 1;
    } else {
        head = node;
        unk_ac = unk_ac + 1;
    }
}
HOOK_METHOD(0x00453c60, CObjVt49f960, ListOp);

// 0x00453e30  size 0x59
// Constructor (serial, graphic): object size 0x124; +0xa8..+0xb8 cleared, vtable 0x0049f9e0, +0xbc..+0x123 zeroed
// (0x1a dwords). RET 8.
CObjVt49f9e0::CObjVt49f9e0(int ser, int gfx)
{
    for (int i = 0; i < 0x1a; i++)
        tail_bc[i] = 0;
    serial = ser;
    facing = 0;
    graphic = static_cast<uint16_t>(gfx & 0x3fff);
}
HOOK_CTOR(0x00453e30, CObjVt49f9e0, (int, int));

// The old name, for the callers that still construct in place (packet_handlers_*.cpp).
CObjVt49f9e0 *__thiscall CObjVt49f9e0::Ctor(int ser, int gfx)
{
    return new (this) CObjVt49f9e0(ser, gfx);
}
