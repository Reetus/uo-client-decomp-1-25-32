// CMapGump, second pass: the event slots of the world view gump (vtable 0x0049f5e0), the click handlers, the
// auto-walk (path) driver, and the cleanup of the entry list at 0x00444090. 0x00444090 .. 0x00445280.
// The SEH frames of the original are omitted.
#include "map_gump.h"

#include <windows.h>

#include <cstdio>

#include "../runtime/ported.h"
#include "cell_grid.h"
#include "entry_list.h"
#include "globals.h"
#include "multi_placer.h"
#include "obj.h"
#include "crt.h"
#include "path_search.h"
#include "raw_field.h"
#include "vcall.h"
#include "walk.h"
#include "client_misc_ops_ports.h"
#include "object_manager.h"
#include "object_manager_ops_ports.h"
#include "world_obj_ops.h"
#include "mobile_draw_ops_ports.h"
#include "mouse_manager.h"
#include "window_helpers_31_ports.h"
#include "client_socket.h"
#include "packet_builders_batch24_ports.h"
#include "walk_queue_ports.h"
#include "class_0049c690.h"
#include "yes_no_gump.h"
#include "gump.h"
#include "client_helpers_31.h"
#include "char_creation_pages.h"
#include "small_methods_u1_ports.h"
#include "map_gump_ops_ports.h"
#include "vtables.h"

#define MG_ObjMgr GLOBAL_PTR(void, g_objMgr_storage)               // object manager
#define MG_MouseMgr GLOBAL_PTR(void, g_mouse_storage)
#define MG_DragGump GLOBAL_AS(CGump *, g_dragGump)
#define MG_TargetCallback GLOBAL_AS(void *, g_targetCallback)     // called with ECX = the clicked object
#define MG_TargetGump GLOBAL_AS(void *, g_screenGump)
#define MG_App GLOBAL_PTR(void, g_app_storage)
#define MG_NetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)
#define MG_PathSearch GLOBAL_AS(CPathSearch, g_pathSearch_storage)

// 0x00478cd0: returns 0 (stub); the original passes (x, y) in ECX / EDX.
#define MG_Stub PORT_FUNC2(&Misc_Return0, int, __fastcall, int, int)
// 0x00455120: the object under the screen point (x, y), on the object manager, RET 8.
#define MG_PickAt(o, a1, a2) PORT_THISCALL2(&ObjectManager_GetObjectAtScreen, CObj *, (o), int, int)(a1, a2)
// 0x00456d70: object with the given serial, RET 4.
#define MG_FindBySerial(self, a1) ((CObj *)reinterpret_cast<CObjectManager *>(self)->FindBySerial((uint32_t)(a1)))
// 0x00456890 (use / open an item) and 0x00456a10 (begin a use), on the object manager, RET 4.
#define MG_UseObject(o, a1) PORT_THISCALL1(&Object_Use, void, (o), CObj *)(a1)
#define MG_BeginUse(o, a1) PORT_THISCALL1(&Object_ShowName, void, (o), CObj *)(a1)
// 0x0042bcf0: invalidate the screen rect of an object.
#define MG_Invalidate(self) (reinterpret_cast<CWorldObj *>(self)->Invalidate())
// 0x0042dbc0: name hue (notoriety colour) of a mobile; ECX = mobile.
#define MG_NameHue(a1) (Mobile_NameHue((CMobile *)(a1)))
// 0x00446840: set the mouse cursor, RET 4. 0x00446d20: direction the mouse points to (no arguments).
#define MG_SetCursor(self, a1) (reinterpret_cast<CMouseManager *>(self)->SetMouseCursor(a1))
#define MG_MouseDir(self) (reinterpret_cast<CMouseManager *>(self)->GetWalkDirection())
// 0x00482be0: set the cursor on the application object, RET 4.
#define MG_AppSetCursor(o, a1) PORT_THISCALL1(&SetCursorForRegion, void, (o), int)(a1)
// 0x00478610: send a packet on the connection object, RET 4.
#define MG_SendPacket(self, a1) (reinterpret_cast<CClientSocket *>(self)->SendPacket((const uint8_t *)(a1)))
// Packet builders (on the object at 0x0069e0e0): 0x09 single click, 0x05 attack request, 0x06 double click.
#define MG_BuildSingleClick(o, a1, a2) PORT_THISCALL2(&Build_Packet_0x09, void, (o), uint8_t *, int)(a1, a2)
#define MG_BuildAttack(o, a1, a2) PORT_THISCALL2(&Build_Packet_0x05, void, (o), uint8_t *, int)(a1, a2)
#define MG_BuildDoubleClick(o, a1, a2) PORT_THISCALL2(&Build_Packet_0x06, void, (o), uint8_t *, int)(a1, a2)
// 0x00448c90: (x, y, z, dir, &out) in ECX / EDX / stack, RET 0xc.
#define MG_WalkProbe(a1, a2, a3, a4, a5) (Walk_FindStandZ(a1, a2, a3, a4, a5))

static inline PathNode *ChildOf(PathNode *node)
{
    return reinterpret_cast<PathNode *>(static_cast<uintptr_t>(static_cast<uint32_t>(node->unk_28)));
}
static inline void SetChild(PathNode *node, PathNode *child)
{
    node->unk_28 = static_cast<int32_t>(reinterpret_cast<uintptr_t>(child));
}

// ---------------------------------------------------------------------------
// 0x00444090, 0x004440e0: cleanup of the entry list.
// ---------------------------------------------------------------------------

// 0x004440e0  size 0x9c
void __thiscall CEntry_004440e0::Release()
{
    int i = 0;
    if (items != nullptr && itemCount > 0) {
        do {
            void *item = items[i];
            if (item != nullptr) {
                reinterpret_cast<CObj0047d430 *>(item)->FreeBuffer();   // destroy the item
                operator delete(item);
            }
            i++;
        } while (i < itemCount);
        i = 0;
    }
    operator delete(block4);
    operator delete(block8);
    operator delete(items);
    operator delete(block28);
    operator delete(block2c);
    inUse = 0;
    block4 = nullptr;
    block8 = nullptr;
    unk_0c = i;
    unk_10 = i;
    unk_14 = i;
    unk_18 = i;
    unk_1c = i;
    itemCount = i;
    items = nullptr;
    block28 = nullptr;
    block2c = nullptr;
    next = nullptr;
}
HOOK_METHOD(0x004440e0, CEntry_004440e0, Release);

// 0x00444090  size 0x4e
void __thiscall CEntryList_00444090::Release()
{
    if (inited == 0)
        return;
    if (first != nullptr) {
        CEntry_004440e0 *entry;
        CEntry_004440e0 *following;
        do {
            entry = first;
            following = entry->next;
            if (entry != nullptr) {
                entry->Release();   // 0x004440e0
                operator delete(entry);
            }
            first = following;
        } while (following != nullptr);
    }
    operator delete(block);
    block = nullptr;
    inited = 0;
}
HOOK_METHOD(0x00444090, CEntryList_00444090, Release);

// ---------------------------------------------------------------------------
// CMapGump
// ---------------------------------------------------------------------------

HOOK_VTABLE(0x0049f5e0, CMapGump);
HOOK_SDD(0x00444290, CMapGump);

// 0x004442c0  size 0xb
// The original ends with a jump to the CGump destructor (0x0043c470), which the compiler now emits.
CMapGump::~CMapGump()
{
}
HOOK_DTOR(0x004442c0, CMapGump);

// 0x004442d0  size 0x2f
// (x, y) lies on the 640 x 480 screen.
int __thiscall CMapGump::HitTest(int sx, int sy)
{
    if (sx < 0 || sy < 0 || sx >= 0x280 || sy >= 0x1e0)
        return 0;
    return 1;
}
HOOK_VMETHOD(0x004442d0, CMapGump, HitTest);

// 0x00444300  size 0xf0
// Vtable slot 0x10, left button down. A loaded multi that is being placed is dropped first (or the click is
// swallowed when it is not in the placing state). A second press within the double-click time sets +0xac to 2;
// otherwise the object under the cursor is remembered by serial (+0x94) when it answers slot 0x2c.
void __thiscall CMapGump::MouseDown(int mx, int my)
{
    if (g_multiPlacer != nullptr && g_loadedMulti != nullptr) {
        if (g_multiPlacer->loaded == 1) {
            g_multiPlacer->FreeTable();   // 0x0040cc70
            g_multiPlacer->loaded = 0;
        } else {
            reinterpret_cast<CWorldObj *>(g_multiPlacer)->CWorldObj::ToVoid();   // empty procedure
            return;
        }
    }
    if (MG_Stub(mx, my) != 0)
        return;
    int state = unk_ac;
    unk_a0 = 1;
    if (state == 1) {
        unk_ac = 2;
        unk_6c = mx;
        unk_70 = my;
        return;
    }
    unk_ac = 1;
    unk_a8 = static_cast<int32_t>(GetTickCount());
    CObj *obj = MG_PickAt(MG_ObjMgr, mx, my);   // 0x00455120
    if (obj != nullptr && decomp::vcall(PORT_RET(int), obj, 0x2c) != 0) {
        unk_94 = RAWF(int32_t, obj, 0x78);
        unk_6c = mx;
        unk_70 = my;
        return;
    }
    unk_94 = 0;
    unk_6c = mx;
    unk_70 = my;
}
HOOK_VMETHOD(0x00444300, CMapGump, MouseDown);

// 0x004443f0  size 0xda
// Vtable slot 0x14, left button up: end a drag, or turn the press into a double click (+0xac == 2) or a single
// click (nothing dragged, +0xac == 0, +0xa4 clear).
void __thiscall CMapGump::MouseUp(int mx, int my)
{
    if (MG_Stub(mx, my) != 0)
        return;
    int wasDragging = unk_98;
    unk_a0 = 0;
    if (wasDragging != 0) {
        unk_98 = 0;
        unk_ac = 0;
        unk_a4 = 0;
        MG_DragGump = nullptr;
        return;
    }
    if (unk_ac == 2) {
        int serial = unk_94;
        unk_ac = 0;
        if (serial != 0) {
            CObj *obj = MG_FindBySerial(MG_ObjMgr, serial);   // 0x00456d70
            if (obj == nullptr)
                obj = MG_PickAt(MG_ObjMgr, mx, my);   // 0x00455120
            OnDoubleClick(obj);   // 0x00444d50
            unk_94 = 0;
        } else {
            CObj *obj = MG_PickAt(MG_ObjMgr, mx, my);
            OnDoubleClick(obj);
        }
    } else if (unk_a4 == 0 && unk_ac == 0) {
        OnSingleClick(mx, my);   // 0x00444a70
    }
    unk_a4 = 0;
    MG_DragGump = nullptr;
}
HOOK_VMETHOD(0x004443f0, CMapGump, MouseUp);

// 0x004444d0  size 0x6c
// Vtable slot 0x18, right button down: +0xbc counts the presses (1, then 2 within the double-click time).
void __thiscall CMapGump::RButtonDown(int mx, int my)
{
    if (MG_Stub(mx, my) != 0)
        return;
    unk_b0 = 1;
    if (static_cast<uint32_t>(unk_bc) > 0) {
        unk_bc = 2;
    } else {
        unk_bc = 1;
        unk_b8 = static_cast<int32_t>(GetTickCount());
    }
    unk_c0 = mx;
    unk_c4 = my;
    unk_b4 = 1;
}
HOOK_VMETHOD(0x004444d0, CMapGump, RButtonDown);

// 0x00444540  size 0xad
// Vtable slot 0x1c, right button up: a double press starts the auto-walk to the object under the cursor; a single
// one stops the running walk.
void __thiscall CMapGump::RButtonUp(int mx, int my)
{
    if (MG_Stub(mx, my) != 0)
        return;
    int presses = unk_bc;
    unk_b0 = 0;
    if (presses == 2) {
        unk_bc = 0;
        StartPathfind(mx, my);   // 0x004450a0
        return;
    }
    if (unk_b4 != 0 && presses == 0) {
        if (g_walk.blocked == 1)
            return;
        if (g_walk.cur < g_walk.count)
            return;
        unk_b4 = 0;
        g_walk.RequestWalkIdle(MG_MouseDir(MG_MouseMgr), -1);   // 0x00446d20, 0x00449100
    }
    if (MG_DragGump != nullptr) {
        if (decomp::vcall(PORT_RET(int), MG_DragGump, 0x8c) == 0)
            MG_DragGump = nullptr;
    }
}
HOOK_VMETHOD(0x00444540, CMapGump, RButtonUp);

// 0x004445f0  size 0x102
// Vtable slot 0x20, mouse move: set the cursor, remember the position, and keep the two hover highlights
// (+0xd4: a mobile, shown while the player is in war mode or a target cursor is up; +0xd8: an object that answers
// slots 0x2c and 0x5c) in step with the object under the mouse.
void __thiscall CMapGump::MouseOver(int mx, int my)
{
    if (MG_Stub(mx, my) != 0)
        return;
    if (g_targetCallbackActive != 0)
        MG_SetCursor(MG_MouseMgr, 0x13);   // 0x00446840
    else
        MG_SetCursor(MG_MouseMgr, 0);
    unk_c8 = mx;
    unk_cc = my;
    CObj *obj = MG_PickAt(MG_ObjMgr, mx, my);   // 0x00455120
    CObj *item = obj;
    if (RAWF(int32_t, g_player, 0x150) != 0 || g_targetCallbackActive != 0) {
        if (obj != nullptr) {
            if (decomp::vcall(PORT_RET(int), obj, 0x24) == 0)
                obj = nullptr;
        }
        if (obj == g_player)
            obj = nullptr;
        CObj *old = reinterpret_cast<CObj *>(static_cast<uintptr_t>(unk_d4));
        if (obj != old) {
            unk_d4 = static_cast<int32_t>(reinterpret_cast<uintptr_t>(obj));
            if (old != nullptr)
                MG_Invalidate(old);   // 0x0042bcf0
            CObj *cur = reinterpret_cast<CObj *>(static_cast<uintptr_t>(unk_d4));
            if (cur != nullptr)
                MG_Invalidate(cur);
        }
    }
    if (item != nullptr) {
        if (decomp::vcall(PORT_RET(int), item, 0x2c) == 0)
            item = nullptr;
    }
    if (item != nullptr) {
        if (decomp::vcall(PORT_RET(int), item, 0x5c) == 0)
            item = nullptr;
    }
    CObj *oldItem = reinterpret_cast<CObj *>(static_cast<uintptr_t>(unk_d8));
    if (item != oldItem) {
        unk_d8 = static_cast<int32_t>(reinterpret_cast<uintptr_t>(item));
        if (oldItem != nullptr)
            MG_Invalidate(oldItem);
        CObj *cur = reinterpret_cast<CObj *>(static_cast<uintptr_t>(unk_d8));
        if (cur != nullptr)
            MG_Invalidate(cur);
    }
}
HOOK_VMETHOD(0x004445f0, CMapGump, MouseOver);

// 0x00444700  size 0x37
// Vtable slot 0x24: drop the two hover highlights.
void __thiscall CMapGump::MouseLeave()
{
    CObj *a = reinterpret_cast<CObj *>(static_cast<uintptr_t>(unk_d4));
    if (a != nullptr) {
        unk_d4 = 0;
        MG_Invalidate(a);   // 0x0042bcf0
    }
    CObj *b = reinterpret_cast<CObj *>(static_cast<uintptr_t>(unk_d8));
    if (b != nullptr) {
        unk_d8 = 0;
        MG_Invalidate(b);
    }
}
HOOK_VMETHOD(0x00444700, CMapGump, MouseLeave);

// 0x00444740  size 0xce
// Vtable slot 0x28, mouse move with a button held: moving more than 4 pixels cancels a pending right double
// press, and turns a pending left press into a drag of the object under the press position.
void __thiscall CMapGump::MouseMove(int mx, int my)
{
    if (MG_Stub(mx, my) != 0)
        return;
    if (unk_b0 != 0 && static_cast<uint32_t>(unk_bc) > 0) {
        int dx = mx - unk_c0;
        if (dx < 0)
            dx = -dx;
        int dy = my - unk_c4;
        if (dy < 0)
            dy = -dy;
        if (dx > 4 || dy > 4)
            unk_bc = 0;
    }
    if (unk_a0 != 0 && static_cast<uint32_t>(unk_ac) > 0) {
        int dx = mx - unk_6c;
        if (dx < 0)
            dx = -dx;
        int dy = my - unk_70;
        if (dy < 0)
            dy = -dy;
        if (dx > 4 || dy > 4) {
            unk_98 = 1;
            unk_ac = 0;
            OnLButtonDown(unk_6c, unk_70);   // 0x00444ee0
        }
    }
    unk_c8 = mx;
    unk_cc = my;
}
HOOK_VMETHOD(0x00444740, CMapGump, MouseMove);

// 0x00444810  size 0xc6
// Vtable slot 0x2c, idle: keep walking while the right button is held, expire the double-click windows, and turn a
// left press that waited out the double-click time into a single click or a drag.
void __thiscall CMapGump::Tick()
{
    if (Misc_Return0() != 0)
        return;
    if (unk_b0 != 0 || unk_b4 != 0) {
        unk_b4 = 0;
        g_walk.RequestWalkIdle(MG_MouseDir(MG_MouseMgr), -1);   // 0x00446d20, 0x00449100
    }
    uint32_t now = GetTickCount();
    uint32_t dbl = GetDoubleClickTime();
    if (unk_bc == 1 && now - static_cast<uint32_t>(unk_b8) > dbl)
        unk_bc = 0;
    if (unk_ac == 1 && now - static_cast<uint32_t>(unk_a8) > dbl) {
        int pressed = unk_a0;
        unk_ac = 0;
        unk_94 = 0;
        if (pressed == 0) {
            OnSingleClick(unk_6c, unk_70);   // 0x00444a70
            return;
        }
        unk_98 = 1;
        OnLButtonDown(unk_6c, unk_70);   // 0x00444ee0
    }
}
HOOK_VMETHOD(0x00444810, CMapGump, Tick);

// 0x004448e0  size 0x10e
// Advance the auto-walk by one step: find which of the 8 directions leads from the current path node to the next
// one, request that step (the direction is passed through the probe at 0x00448c90 first), and drop the path at the end.
void __cdecl Pathfind_NextStep()
{
    char text[0x200];
    if (g_pathActive == 0)
        return;
    if (g_walk.count >= 3)
        return;
    PathNode *node = g_pathCurrentNode;
    int curX = node->x;
    g_pathX = curX;
    int curY = node->y;
    g_pathY = curY;
    g_pathFacing = RAWF(int32_t, node, 0x1c);
    PathNode *nextNode = ChildOf(node);
    if (nextNode == nullptr) {
        g_pathActive = 0;
        MG_PathSearch.FreeLists();   // 0x00466120
        return;
    }
    int dy = nextNode->y - curY;
    int dx = nextNode->x - curX;
    int dir = -1;
    for (int d = 0; d < 8; d++) {
        if (g_dirDx[d] == dx && g_dirDy[d] == dy) {
            dir = d;
            break;
        }
    }
    if (dir == g_pathFacing) {
        node = nextNode;
        g_pathCurrentNode = node;
    }
    RAWF(int32_t, node, 0x1c) = dir;
    PathNode *cur = g_pathCurrentNode;
    sprintf(text, "At: %d, %d.  Going to: %d, %d.  dx,dy: %d, %d.  Dir: %d",   // 0x0048b1d0
            static_cast<int>(g_player->x), static_cast<int>(g_player->y), cur->x, cur->y, dx, dy, dir);
    if (dir == -1)
        return;
    int out;
    MG_WalkProbe(g_player->x, g_player->y, g_player->z, dir, &out);   // 0x00448c90
    g_walk.RequestWalk((dir + 1) & 7, -1);   // 0x00449180
}
HOOK_FUNC(0x004448e0, Pathfind_NextStep);

// 0x00444a70  size 0x253
// Single click at (x, y). With a target cursor up it answers the cursor (after the "lower notoriety" warning for an
// attack on an innocent); otherwise Alt without Ctrl on a mobile toggles following it, and a plain click looks at
// the object (use / begin-use) and sends the single click packet.
void __thiscall CMapGump::OnSingleClick(int mx, int my)
{
    CObj *obj;
    if (g_targetCallbackActive != 0 && MG_TargetCallback != nullptr && g_modalActive == 0) {
        obj = static_cast<CObj *>(g_worldPicker.GetObjectAt(mx, my, g_targetCursorType != 1));   // 0x0042fd50
        if (g_warnOnAttack != 0 && obj != nullptr && decomp::vcall(PORT_RET(int), obj, 0x24) != 0 && g_targetFlags != 0 &&
            ((g_targetFlags == 1 && MG_NameHue(obj) == 0x59) ||   // 0x0042dbc0
             (g_targetFlags == 2 && MG_NameHue(obj) == 0x22))) {
            g_targetCallbackActive = 0;
            g_targetFlags = 0;
            MG_AppSetCursor(MG_App, 0);   // 0x00482be0
            RAWF(int32_t, MG_TargetGump, 0xd4) = 0;
            MG_Invalidate(obj);   // 0x0042bcf0
            ConfirmLowerNotorietyA(reinterpret_cast<void *>(static_cast<uintptr_t>(RAWF(uint32_t, obj, 0x78))));   // 0x004449f0
            return;
        }
        reinterpret_cast<void(__fastcall *)(CObj *)>(MG_TargetCallback)(obj);
        if (obj != nullptr)
            MG_Invalidate(obj);
        return;
    }

    obj = MG_PickAt(MG_ObjMgr, mx, my);   // 0x00455120
    if (obj == nullptr)
        return;
    SHORT alt = GetAsyncKeyState(0x12);
    SHORT ctrl = GetAsyncKeyState(0x11);
    if ((alt & 0x8000) != 0 && (ctrl & 0x8000) == 0 && decomp::vcall(PORT_RET(int), obj, 0x24) != 0) {
        if (obj == g_player) {
            int following = RAWF(int32_t, g_player, 0x1f8);
            if (following != 0)
                g_player->CreateBarkKeepTwo("Stopped following.", 0, 3, nullptr);   // 0x00453320
            else
                g_player->CreateBarkKeepTwo("Not following.", 0, 3, nullptr);
            RAWF(int32_t, g_player, 0x1f8) = 0;
            if (g_pathActive != 0) {
                MG_PathSearch.FreeLists();   // 0x00466120
                g_pathActive = 0;
            }
            return;
        }
        g_player->CreateBarkKeepTwo("Now following.", 0, 3, nullptr);
        RAWF(int32_t, g_player, 0x1f8) = 1;
        RAWF(int32_t, g_player, 0x1fc) = RAWF(int32_t, obj, 0x78);
        RAWF(int32_t, g_player, 0x200) = static_cast<int>(g_player->x) - static_cast<int>(obj->x);
        RAWF(int32_t, g_player, 0x204) = static_cast<int>(g_player->y) - static_cast<int>(obj->y);
        return;
    }
    if (decomp::vcall(PORT_RET(int), obj, 0x2c) == 0 || Misc_Return0() != 0)
        MG_BeginUse(MG_ObjMgr, obj);   // 0x00456a10
    if (decomp::vcall(PORT_RET(int), obj, 0x2c) != 0) {
        uint8_t pkt[8];
        MG_BuildSingleClick(MG_NetBuilder, pkt, RAWF(int32_t, obj, 0x78));   // 0x00463260
        MG_SendPacket(g_pktSender, pkt);   // 0x00478610
    }
}
HOOK_METHOD(0x00444a70, CMapGump, OnSingleClick);

// 0x00444cd0  size 0x76
// Confirmation dialog "This will lower notoriety !" (variant B). ECX = the owner value, kept at 0x005285fc.
static void __fastcall ShowNotorietyWarning_B(void *self)
{
    GLOBAL_AS(void *, MG_NotorietyOwner) = self;
    CGump *dlg = new CYesNoGump("This will lower notoriety !", reinterpret_cast<void *>(PORT_ADDR(&CGumpHandlers04::SendPacket05AndDelete)), reinterpret_cast<void *>(PORT_ADDR(&DeleteGumpCallback)), 0);
    dlg->LinkInto(nullptr, 1);   // CGump::LinkInto
}
HOOK_FUNC(0x00444cd0, ShowNotorietyWarning_B);

// 0x00444d50  size 0x190
// Double click on `obj`: a plain item is used (the whole stack of identical items at the same tile when the
// serial has bit 0x40000000 and slot 0x54 allows it); a mobile is attacked (after the warning for an innocent)
// or double-clicked when the player is not in war mode.
void __thiscall CMapGump::OnDoubleClick(CObj *obj)
{
    if (obj == nullptr)
        return;
    if (decomp::vcall(PORT_RET(int), obj, 0x2c) == 0)
        return;
    if (g_targetCallbackActive != 0)
        return;
    uint8_t pkt[8];
    if (decomp::vcall(PORT_RET(int), obj, 0x28) == 0 &&
        (decomp::vcall(PORT_RET(int), obj, 0x24) == 0 || RAWF(int32_t, g_player, 0x150) != 0)) {
        if ((RAWF(uint32_t, obj, 0x78) & 0x40000000) != 0) {
            if (decomp::vcall(PORT_RET(int), obj, 0x54) != 0) {
                CObj *top = obj;
                CObj *up = RAWF(CObj *, obj, 0x18);
                if (up != nullptr) {
                    do {
                        top = up;
                        up = RAWF(CObj *, top, 0x18);
                    } while (up != nullptr);
                }
                if (top == nullptr)
                    return;
                do {
                    if (decomp::vcall(PORT_RET(int), top, 0x18) != 0 && decomp::vcall(PORT_RET(int), top, 0x54) != 0 && top->x == obj->x &&
                        top->y == obj->y && top->z == obj->z)
                        MG_UseObject(MG_ObjMgr, top);   // 0x00456890
                    top = RAWF(CObj *, top, 0x14);
                } while (top != nullptr);
                return;
            }
            MG_UseObject(MG_ObjMgr, obj);   // 0x00456890
            return;
        }
        if (decomp::vcall(PORT_RET(int), obj, 0x28) != 0)
            return;
        if (decomp::vcall(PORT_RET(int), obj, 0x24) == 0)
            return;
        if (RAWF(int32_t, g_player, 0x150) == 0)
            return;
        if (g_warnOnAttack != 0 && MG_NameHue(obj) == 0x59) {   // 0x0042dbc0
            ShowNotorietyWarning_B(reinterpret_cast<void *>(static_cast<uintptr_t>(RAWF(uint32_t, obj, 0x78))));   // 0x00444cd0
            return;
        }
        MG_BuildAttack(MG_NetBuilder, pkt, RAWF(int32_t, obj, 0x78));   // 0x00463010
        MG_SendPacket(g_pktSender, pkt);   // 0x00478610
        return;
    }
    MG_BuildDoubleClick(MG_NetBuilder, pkt, RAWF(int32_t, obj, 0x78));   // 0x00462d30
    MG_SendPacket(g_pktSender, pkt);
}
HOOK_METHOD(0x00444d50, CMapGump, OnDoubleClick);

// 0x00445080  size 0x1b
// Link the nodes of the current path back to front: every ancestor of the current node gets the node below it
// as its +0x28 child.
void __cdecl PathReverseLinks()
{
    PathNode *below = g_pathCurrentNode;
    PathNode *up = below->parent;
    while (up != nullptr) {
        PathNode *node = up;
        SetChild(node, below);
        below = node;
        up = node->parent;
    }
}
HOOK_FUNC(0x00445080, PathReverseLinks);

// 0x004450a0  size 0x1d2
// Start auto-walking to the object under (x, y): search a path from where the queued walk steps will leave the
// player (view position advanced by every unacknowledged step that keeps its direction), first exactly then with a
// tolerance of 1; on success link the nodes front to back and make the start node current.
void __thiscall CMapGump::StartPathfind(int mx, int my)
{
    if (MG_PathEnabled == 0)
        return;
    if (g_pathActive != 0)
        return;
    if (g_walk.blocked == 1)
        return;
    if (g_walk.cur < g_walk.count)
        return;
    CWorldObj *target = g_worldPicker.GetObjectAt(mx, my, 0);   // 0x0042fd50
    if (target == nullptr)
        return;
    int goalX = target->x;
    int goalY = target->y;
    int goalZ = target->z;
    int startX = g_viewCenterX;
    int startY = g_viewCenterY;
    int startZ = MG_ViewZ;
    if (g_walk.count > 0) {
        WalkEntry *entry = g_walk.Entry(0);
        int left = g_walk.count;
        do {
            if (entry->acked == 0 && entry->dir == entry->nextDir) {
                startX += g_dirDx[entry->dir];
                startY += g_dirDy[entry->dir];
                startZ = entry->z;
            }
            entry++;
            left--;
        } while (left != 0);
    }
    PathNode *goal = MG_PathSearch.Run(startX, startY, startZ, goalX, goalY, goalZ, 0x1f4, 0);   // 0x00466160
    g_pathCurrentNode = goal;
    if (goal == nullptr) {
        goal = MG_PathSearch.Run(startX, startY, startZ, goalX, goalY, goalZ, 0x1f4, 1);
        g_pathCurrentNode = goal;
        if (goal == nullptr) {
            g_player->CreateBarkKeepTwo("Can't get there", 0, 3, nullptr);   // 0x00453320
            g_pathActive = 0;
            return;
        }
    }
    g_pathActive = 1;
    PathNode *start = goal;
    PathNode *up = goal->parent;
    PathNode *below = goal;
    while (up != nullptr) {
        PathNode *node = up;
        SetChild(node, below);
        below = node;
        up = node->parent;
    }
    PathNode *root = start->parent;
    if (root != nullptr) {
        PathNode *step;
        do {
            start = root;
            step = start->parent;
            root = step;
        } while (step != nullptr);
    }
    g_pathCurrentNode = start;
    g_pathX = start->x;
    g_pathY = start->y;
    g_pathZ = start->z;
    g_pathFacing = g_player->facing;
    RAWF(int32_t, start, 0x1c) = g_pathFacing;
    g_pathStartNode = g_pathCurrentNode;
    g_player->CreateBarkKeepTwo("Pathfinding!", 0, 3, nullptr);   // 0x00453320
}
HOOK_METHOD(0x004450a0, CMapGump, StartPathfind);

// 0x00445280  size 0x7d
// Chebyshev-style distance between two (x, y) int16 pairs; 9999 when exactly one x is on the x >= 0x1400 half.
// (0x00445300 is the sibling that takes the second point as scalars.)
int __stdcall MapDistancePoints(const int16_t *a, const int16_t *b)
{
    int16_t x2 = b[0];
    int16_t x1 = a[0];
    if ((b[0] >= 0x1400) ^ (a[0] >= 0x1400))
        return 0x270f;
    int dx = static_cast<int>(x1) - static_cast<int>(x2);
    if (dx < 0)
        dx = -dx;
    int dy = static_cast<int>(a[1]) - static_cast<int>(b[1]);
    if (dy < 0)
        dy = -dy;
    if (x1 < 0x1400) {
        int wx = 0x1400 - dx;
        if (dx > wx)
            dx = wx;
        int wy = 0x1000 - dy;
        if (dy > wy)
            dy = wy;
    }
    if (dx <= dy)
        dx = dy;
    return dx;
}
HOOK_FUNC(0x00445280, MapDistancePoints);
