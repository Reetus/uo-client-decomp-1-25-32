// CContainerGump: constructor, mouse handlers, frame redraw, hit tests and drops (vtable 0x0049e5c0).
// The SEH frame of the constructor (try level 1 around the button `new`) is omitted.
// The destructor body 0x004261d0 is in container_gump.cpp.
#include <windows.h>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "batch02_helpers.h"
#include "button_gump.h"
#include "container_gump.h"
#include "globals.h"
#include "gump_manager.h"
#include "crt.h"
#include "vcall.h"
#include "sound_manager.h"
#include "window_helpers_31_ports.h"
#include "mouse_manager.h"
#include "gump.h"
#include "book_gump.h"
#include "object_manager_ops_ports.h"
#include "object_manager.h"
#include "packet_builders_batch24_ports.h"
#include "client_socket.h"
#include "client_misc_ops_ports.h"
#include "app_support_ports.h"
#include "gump_base_ops_ports.h"
#include "art_row_blits_ports.h"
#include "gump_art_ops_ports.h"
#include "art_record_ops_ports.h"
#include "art_blit_ports.h"
#include "art_row_blits_hue_ports.h"
#include "vtables.h"

#define g_hueRegister GLOBAL_AS(uint32_t, g_hueOverride)             // hue index read by the hued row blitters
#undef g_objMgr   // the class header's typed form; this file addresses the object by offset
#define g_objMgr GLOBAL_PTR(void, g_objMgr_storage)
#define g_mouseManager GLOBAL_PTR(void, g_mouse_storage)
#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)
#define g_appObject GLOBAL_PTR(void, g_app_storage)
#define g_soundObject GLOBAL_PTR(void, g_soundManager_storage)
#define g_spriteObject GLOBAL_PTR(void, g_artCache_storage)

#define Music_Play(self, a1, a2, a3, a4, a5, a6) (reinterpret_cast<CSoundManager *>(self)->PlaySound(a1, (long)(a2), (long)(a3), a4, a5, a6))
#define Cursor_Set(o, a1) PORT_THISCALL1(&SetCursorForRegion, void, (o), int)(a1)            // on the object at 0x0069e0e8
#define Mouse_SetCursor(self, a1) (reinterpret_cast<CMouseManager *>(self)->SetMouseCursor(a1))
#define Gump_LocalToScreen(self, a1, a2) (reinterpret_cast<CGump *>(self)->LocalToScreen(a1, a2))
#define Gump_ScreenToLocal(self, a1, a2) (reinterpret_cast<CGump *>(self)->ScreenToLocal(a1, a2))
#define Gump_Uniconify(self) (reinterpret_cast<CBookGump *>(self)->Uniconify())
#define Obj_Use(o, a1) PORT_THISCALL1(&Object_Use, void, (o), void *)(a1)             // on the object manager (0x0068ce58)
#define Obj_BeginUse(o, a1) PORT_THISCALL1(&Object_ShowName, void, (o), void *)(a1)
#define Obj_PickUp(self, a1, a2) (reinterpret_cast<CObjectManager *>(self)->BeginPickUp((CDynamicObj *)(a1), (uint16_t)(a2)))
#define Pkt_SingleClick(o, a1, a2) PORT_THISCALL2(&Build_Packet_0x09, void, (o), uint8_t *, uint32_t)(a1, a2)
#define Pkt_DropItem(o, a1, a2, a3, a4, a5, a6) PORT_THISCALL6(&Build_Packet_0x08, void, (o), uint8_t *, int, int, int, int, int)(a1, a2, a3, a4, a5, a6)   // pkt, held serial, x, y, z, dest
#define Pkt_DropItemNarrow(o, a1, a2, a3, a4, a5, a6) PORT_THISCALL6(&Build_Packet_0x08, void, (o), uint8_t *, int, uint16_t, uint16_t, uint8_t, int)(a1, a2, a3, a4, a5, a6)
#define Net_Send(self, a1) (reinterpret_cast<CClientSocket *>(self)->SendPacket((const uint8_t *)(a1)))
#define Opt_IsSet() (Misc_Return0())               // ? flag test; calling convention not annotated
#define Rand_Mod(a1) (App_RandomMod(a1))
#define Rect_Clip(a1, a2, a3) (GumpRectClip(a1, (const int *)(a2), a3))   // ECX = clip, EDX = &rect, stack = &out
#define Mgr_Intersects(o, a1, a2) PORT_THISCALL2(&GumpRectIntersects, int, (o), const int *, int *)(a1, a2)
#define Mgr_GetRect(o, a1, a2) PORT_THISCALL2(&GetContainerGumpRect, void, (o), int, int *)(a1, a2)    // (container gump id, &rect)
#define Mgr_IdForItem(o, a1) PORT_THISCALL1(&ContainerGumpIdForItem, int, (o), uint16_t)(a1)   // ECX = gump manager
#define Tile_SetTarget(a1, a2) (SetBlitSurface((uint16_t *)(a1), a2))         // ECX = buffer, EDX = width
#define Art_PixelHit(a1, a2, a3) (GumpArtPixel(a1, a2, a3))     // ECX = art id, EDX = x, stack = y
#define Sprite_PixelHit(o, a1, a2, a3) PORT_THISCALL3(&ArtRecord_PixelAt, uint16_t, (o), void *, int, int)(a1, a2, a3)       // (sprite, x, y)
#define Art_BlitConv PORT_FUNC9(&GumpBlitBitmap, void, __fastcall, void *, void *, int, int, int, int, void *, int, int)
#define Rle_Walk PORT_FUNC5(&RleArtWalk, void, __fastcall, void *, void *, const int *, int, int)

static inline bool IsCorpseArt(int artId)
{
    return artId == 0x91a || artId == 0x92e;
}


// 0x00425e30  size 0x31d
// Builds the frame bitmap (second frame and a rectangle button for the backpack art 0x3c), draws it,
// remembers the player's backpack gump and plays the opening sound.
CContainerGump::CContainerGump(int px, int py, CGump *unusedParent, CObj *container, int nFrames, int w, int h, int gumpId)
{
    (void)unusedParent;
    int rect[4];
    name = g_004a7b64;                // "container gump"
    dirty = 0;
    unk_b0 = 0;
    mouseDown = 0;
    unk_a8 = 0;
    dragStarted = 0;
    clickState = 0;
    clickTick = 0;
    closing = 0;
    unk_a0 = nullptr;
    animFrame = 0;
    animCounter = 0;
    hoverObj = nullptr;
    x = px;
    y = py;
    if (gumpId == 0x91a)
        titleHeight = 0x14;
    else
        titleHeight = 0;
    owner = container;
    if (container != nullptr) {
        if (PORT_VSLOT0(container, 0x30, int)() != 0)
            RAW_AT(CContainerGump *, owner, 0xac) = this;
    }
    rowStride = w;
    width = w;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    height = h + titleHeight;
    frameIndex = 0;
    int count = 2;
    if (gumpId != 0x3c)
        count = nFrames;
    frameCount = count;
    frameData = FrameData_Alloc(g_frameDataAllocator, count * 8, "framedata for CContainerGump");
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    frameData[1] = ptr_i32(gfx::AllocTagged(height * width * 2 + 4, "container frame"));
    memset(static_cast<char *>(i32_ptr(frameData[1])) + 4, 0, static_cast<size_t>(height * width) * 2);
    *static_cast<uint16_t *>(i32_ptr(frameData[1])) = static_cast<uint16_t>(width);
    static_cast<uint16_t *>(i32_ptr(frameData[1]))[1] = static_cast<uint16_t>(height);
    unk_50 = gumpId;
    if (gumpId == 0x3c) {
        frameCount = 2;
        GumpArtSize(0x50, &unk_24, &unk_28);
        frameData[3] = ptr_i32(gfx::AllocTagged(unk_24 * unk_28 * 2 + 4, "container frame1"));
        memset(static_cast<char *>(i32_ptr(frameData[3])) + 4, 0, static_cast<size_t>(unk_24 * unk_28) * 2);
        *static_cast<uint16_t *>(i32_ptr(frameData[3])) = static_cast<uint16_t>(unk_24);
        static_cast<uint16_t *>(i32_ptr(frameData[3]))[1] = static_cast<uint16_t>(unk_28);
        DrawGumpArt(ROWBLIT_COPY, 0x50, 0, 0, static_cast<uint16_t *>(i32_ptr(frameData[3])) + 2, unk_24, unk_28);
        CButtonGump *button = new CButtonGump(0x67, 0xa1, 0x7b, 0xb4, this, 0, 0, PORT_ADDR(&CBookGump::Iconify), 0);
        CGump_AddChild(button, this, 1);              // called with a null control when the allocation failed, as in the original
    }
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = width - 1;
    rect[3] = height - 1;
    RedrawFrame(rect);
    if (owner != nullptr && RAW_AT(uint8_t, owner, 0x99) == 0x1d && RAW_AT(CObj *, owner, 0x7c) == g_player)
        g_openContainerGump = this;

    int snd;
    switch (static_cast<uint16_t>(unk_50)) {          // opening sound by container art id
    case 0x3c: case 0x3d:
        snd = 0x48;
        break;
    case 0x3f: case 0x41:
        snd = 0x4f;
        break;
    case 0x42: case 0x43: case 0x44: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4e: case 0x4f:
        snd = 0x2d;
        break;
    case 0x3e: case 0x48: case 0x4d: case 0x51:
        snd = 0x2f;
        break;
    default:
        return;
    }
    Music_Play(g_soundObject, snd, 0, 0, 0, 0, 0);
}
HOOK_CTOR(0x00425e30, CContainerGump, (int, int, CGump *, CObj *, int, int, int, int));

HOOK_VTABLE(0x0049e5c0, CContainerGump);
HOOK_SDD(0x004261a0, CContainerGump);

// 0x004263c0  size 0x8b  close request: with a parent, forward a screen-to-local converted point to the
// parent's slot 0x1c; without one, delete the child gumps and this gump.
void __thiscall CContainerGump::RButtonUp(int px, int py)
{
    if (parent != nullptr) {
        int x = px;
        px = py;
        Gump_LocalToScreen(this, &x, &px);
        Gump_ScreenToLocal(parent, &x, &px);
        PORT_VSLOT2(parent, 0x1c, void, int, int)(x, px);
        return;
    }
    closing = 1;
    CGump *c = lastChild;
    while (c != nullptr) {
        PORT_VSLOT1(c, 0x00, void, int)(1);         // deleting dtor
        c = lastChild;
    }
    g_dragGump = nullptr;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}
HOOK_VMETHOD(0x004263c0, CContainerGump, RButtonUp);

// 0x00426450  size 0x7a  left button pressed: bring to front, count the click, remember the point and the object under it.
void __thiscall CContainerGump::MouseDown(int px, int py)
{
    if (parent != nullptr)
        CGump_BringToFront(parent, 0);
    else
        CGump_BringToFront(this, 0);
    mouseDown = 1;
    if (clickState == 1) {
        clickState = 2;
    } else {
        clickState = 1;
        clickTick = GetTickCount();
    }
    unk_6c = px;
    unk_70 = py;
    hoverObj = nullptr;
    if (unk_60 == 0)
        hoverObj = GetChildObjectAt(px, py);
}
HOOK_VMETHOD(0x00426450, CContainerGump, MouseDown);

// 0x004264d0  size 0x157  left button released: finish a drag, or act on a double click (use) / single click.
void __thiscall CContainerGump::MouseUp(int px, int py)
{
    (void)px;
    (void)py;
    uint8_t pkt[8];
    mouseDown = 0;
    if (dragStarted != 0) {
        dragStarted = 0;
        clickState = 0;
        unk_b0 = 0;
        g_dragGump = nullptr;
        return;
    }
    if (clickState == 2) {
        clickState = 0;
        if (g_targetCallbackActive == 0) {
            if (unk_60 != 0) {
                Gump_Uniconify(this);
                unk_b0 = 0;
                g_dragGump = nullptr;
                return;
            }
            if (PORT_VSLOT0(g_player, 0x8c, int)() == 0) {
                if (hoverObj != nullptr) {
                    Obj_Use(g_objMgr, hoverObj);
                    unk_b0 = 0;
                    g_dragGump = nullptr;
                    return;
                }
            }
        }
    } else if (unk_b0 == 0 && clickState == 0 && hoverObj != nullptr) {
        if (g_targetCallbackActive != 0 && g_targetCallback != 0 && g_modalActive == 0) {
            decomp::thiscall_at_void(g_targetCallback, hoverObj);
            unk_b0 = 0;
            g_dragGump = nullptr;
            return;
        }
        if (Opt_IsSet() != 0)
            Obj_BeginUse(g_objMgr, hoverObj);
        Pkt_SingleClick(g_packetBuilder, pkt, RAW_AT(uint32_t, hoverObj, 0x78));
        Net_Send(g_pktSender, pkt);
    }
    unk_b0 = 0;
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x004264d0, CContainerGump, MouseUp);

// 0x00426630  size 0x19f  mouse move: set the cursor, track the object under the mouse and redraw the old and new one.
void __thiscall CContainerGump::MouseOver(int px, int py)
{
    int rect[4];
    int aw, ah;
    if (unk_60 == 0 && g_targetCallbackActive != 0)
        Mouse_SetCursor(g_mouseManager, 0x13);
    else
        Cursor_Set(g_appObject, 0);

    if (unk_60 != 0)
        return;
    void *old = hoverObj;
    int corpse = 0;
    if (unk_50 == 0x91a || unk_50 == 0x92e)
        corpse = 1;
    void *cur = GetChildObjectAt(px, py);
    hoverObj = cur;
    if (old == cur)
        return;

    if (old != nullptr) {
        if (corpse != 0) {
            GumpArtSize(RAW_AT(uint16_t, old, 0x38) - 0x2c69, &aw, &ah);
            rect[0] = RAW_AT(int16_t, old, 0x20);
            rect[1] = RAW_AT(int16_t, old, 0x22);
            rect[2] = aw + rect[0];
            rect[3] = ah + rect[1];
        } else {
            void *spr = PORT_VSLOT0(old, 0x74, void *)();
            PORT_VSLOT0(old, 0x78, void *)();
            rect[0] = RAW_AT(int16_t, old, 0x20);
            rect[1] = RAW_AT(int16_t, old, 0x22);
            rect[2] = RAW_AT(uint16_t, spr, 4) + rect[0];
            rect[3] = RAW_AT(uint16_t, spr, 6) + rect[1];
        }
        RedrawFrame(rect);
    }
    if (hoverObj != nullptr) {
        void *o = hoverObj;
        if (corpse != 0) {
            GumpArtSize(RAW_AT(uint16_t, o, 0x38) - 0x2c69, &aw, &ah);
            o = hoverObj;
            rect[0] = RAW_AT(int16_t, o, 0x20);
            rect[1] = RAW_AT(int16_t, o, 0x22);
            rect[2] = aw + rect[0];
            rect[3] = ah + rect[1];
        } else {
            void *spr = PORT_VSLOT0(o, 0x74, void *)();
            o = hoverObj;
            PORT_VSLOT0(o, 0x78, void *)();
            o = hoverObj;
            rect[0] = RAW_AT(int16_t, o, 0x20);
            rect[1] = RAW_AT(int16_t, o, 0x22);
            rect[2] = RAW_AT(uint16_t, spr, 4) + rect[0];
            rect[3] = RAW_AT(uint16_t, spr, 6) + rect[1];
        }
        RedrawFrame(rect);
    }
}
HOOK_VMETHOD(0x00426630, CContainerGump, MouseOver);

// 0x004267d0  size 0xbb  mouse left the gump: forget the hovered object and redraw it.
void __thiscall CContainerGump::MouseLeave()
{
    int rect[4];
    int aw, ah;
    void *old = hoverObj;
    int corpse;
    hoverObj = nullptr;
    if (unk_50 == 0x91a || unk_50 == 0x92e)
        corpse = 1;
    else
        corpse = 0;
    if (old == nullptr)
        return;
    if (corpse != 0) {
        GumpArtSize(RAW_AT(uint16_t, old, 0x38) - 0x2c69, &aw, &ah);
        rect[0] = RAW_AT(int16_t, old, 0x20);
        rect[2] = aw + rect[0];
        rect[1] = RAW_AT(int16_t, old, 0x22);
        rect[3] = ah + rect[1];
    } else {
        void *spr = PORT_VSLOT0(old, 0x74, void *)();
        PORT_VSLOT0(old, 0x78, void *)();
        rect[0] = RAW_AT(int16_t, old, 0x20);
        rect[1] = RAW_AT(int16_t, old, 0x22);
        rect[2] = RAW_AT(uint16_t, spr, 4) + rect[0];
        rect[3] = RAW_AT(uint16_t, spr, 6) + rect[1];
    }
    RedrawFrame(rect);
}
HOOK_VMETHOD(0x004267d0, CContainerGump, MouseLeave);

// 0x00426890  size 0x110  mouse drag: move the gump when no object is hovered, else start picking the object up
// once the pointer moved more than 4 pixels.
void __thiscall CContainerGump::MouseMove(int px, int py)
{
    if (hoverObj == nullptr) {
        if (mouseDown == 0)
            return;
        if (parent != nullptr)
            parent->Move(px - unk_6c, py - unk_70, 1, nullptr, 1);
        else
            Move(px - unk_6c, py - unk_70, 1, nullptr, 1);
        return;
    }
    if (mouseDown == 0)
        return;
    if (clickState <= 0)
        return;
    if (abs(px - unk_6c) <= 4) {
        if (abs(py - unk_70) <= 4)
            return;
    }
    dragStarted = 1;
    clickState = 0;
    if (unk_a0 != nullptr) {
        if (RAW_AT(void *, unk_a0, 0xb8) != this) {
            hoverObj = nullptr;
            return;
        }
    }
    if (g_holdingFlag != 0)
        return;
    if (PORT_VSLOT0(g_player, 0x8c, int)() != 0)
        return;
    Obj_PickUp(g_objMgr, hoverObj, 0);
    mouseDown = 0;
    dragStarted = 0;
    clickState = 0;
}
HOOK_VMETHOD(0x00426890, CContainerGump, MouseMove);

// 0x004269a0  size 0x86  idle animation of the container with art id 9: every random 5..24 ticks switch the
// frame (art 0x45 or 0x46) drawn at (0x2d, 0x1e).
void __thiscall CContainerGump::Update()
{
    if (unk_50 != 9)
        return;
    animCounter = animCounter + 1;
    if (animCounter != g_animThreshold)
        return;
    g_animThreshold = Rand_Mod(0x14) + 5;
    animFrame = animFrame + 1;
    animCounter = 0;
    if (static_cast<uint32_t>(animFrame) > 1)
        animFrame = 0;
    uint16_t *bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
    DrawGumpArt(ROWBLIT_COPY, animFrame + 0x45, 0x2d, 0x1e, bmp + 2, bmp[0], bmp[1]);
}
HOOK_VMETHOD(0x004269a0, CContainerGump, Update);

// 0x00426a30  size 0x131  timer: a single press older than the double-click time is resolved as a single click
// (or the start of a pick-up when the button is still down).
void __thiscall CContainerGump::Tick()
{
    uint8_t pkt[8];
    DWORD now = GetTickCount();
    DWORD dbl = GetDoubleClickTime();
    if (clickState != 1)
        return;
    if (now - clickTick <= dbl)
        return;
    clickState = 0;
    if (mouseDown == 0) {
        if (hoverObj == nullptr)
            return;
        if (g_targetCallbackActive != 0 && g_targetCallback != 0 && g_modalActive == 0) {
            decomp::thiscall_at_void(g_targetCallback, hoverObj);
            return;
        }
        if (Opt_IsSet() != 0)
            Obj_BeginUse(g_objMgr, hoverObj);
        Pkt_SingleClick(g_packetBuilder, pkt, RAW_AT(uint32_t, hoverObj, 0x78));
        Net_Send(g_pktSender, pkt);
        return;
    }
    dragStarted = 1;
    if (unk_a0 != nullptr) {
        if (RAW_AT(void *, unk_a0, 0xb8) != this) {
            hoverObj = nullptr;
            return;
        }
    }
    if (hoverObj == nullptr)
        return;
    if (g_holdingFlag != 0)
        return;
    if (PORT_VSLOT0(g_player, 0x8c, int)() != 0)
        return;
    Obj_PickUp(g_objMgr, hoverObj, 0);
    mouseDown = 0;
    dragStarted = 0;
    clickState = 0;
}
HOOK_VMETHOD(0x00426a30, CContainerGump, Tick);

// 0x00426b70  size 0x3f1  redraw the part of the frame bitmap inside `clip`: background art, then every contained
// object's sprite (hued, highlighted when hovered) and its +5,+5 outline; RGB565 displays go through a 555 buffer.
void __thiscall CContainerGump::RedrawFrame(const int *clip)
{
    int full[4];
    int r[4];
    void *sprite = nullptr;
    void *outline;
    uint16_t *tmp = nullptr;
    void *obj;
    int corpse;

    if (g_suppressFlag != 0)
        return;
    {
        uint16_t *bmp0 = static_cast<uint16_t *>(i32_ptr(frameData[1]));
        full[0] = 0;
        full[1] = 0;
        full[2] = bmp0[0];
        full[3] = bmp0[1];
    }
    Rect_Clip(clip, full, r);
    if (titleHeight != 0) {
        r[0] = 0;
        r[1] = 0;
        r[2] = width - 1;
        r[3] = height - 1;
        memset(static_cast<char *>(i32_ptr(frameData[1])) + 4, 0, static_cast<size_t>(height * width) * 2);
    }
    corpse = IsCorpseArt(unk_50) ? 1 : 0;
    {
        uint16_t *bmp0 = static_cast<uint16_t *>(i32_ptr(frameData[1]));
        gfx::DrawArtClipped(ROWBLIT_COPY, unk_50, r, 0, titleHeight, bmp0 + 2, bmp0[0], bmp0[1]);
    }
    obj = RAW_AT(void *, owner, 0xa8);
    if (obj == nullptr)
        return;
    while (RAW_AT(void *, obj, 0x40) != nullptr)
        obj = RAW_AT(void *, obj, 0x40);

    if (g_is565 != 0) {
        uint16_t *bmp0 = static_cast<uint16_t *>(i32_ptr(frameData[1]));
        tmp = static_cast<uint16_t *>(::operator new(static_cast<size_t>(bmp0[1] * bmp0[0]) * 2 + 4));
        bmp0 = static_cast<uint16_t *>(i32_ptr(frameData[1]));
        tmp[0] = bmp0[0];
        tmp[1] = bmp0[1];
        memset(tmp + 2, 0, static_cast<size_t>(tmp[0] * tmp[1]) * 2);
        Tile_SetTarget(tmp + 2, tmp[0]);
    } else {
        uint16_t *bmp0 = static_cast<uint16_t *>(i32_ptr(frameData[1]));
        Tile_SetTarget(bmp0 + 2, bmp0[0]);
    }

    for (; obj != nullptr; obj = RAW_AT(void *, obj, 0x44)) {
        uint16_t gfxId = RAW_AT(uint16_t, obj, 0x38);
        if (gfxId < 0x4000) {
            uint8_t *rec = g_tileData + gfxId * 0x28;
            if ((*reinterpret_cast<uint32_t *>(rec) & 0x400000) != 0) {
                uint8_t t = rec[5];
                if (t == 0xb || t == 0x10)
                    continue;
            }
        }
        if (corpse == 0) {
            sprite = PORT_VSLOT0(obj, 0x74, void *)();
            outline = PORT_VSLOT0(obj, 0x78, void *)();
        } else {
            outline = nullptr;
        }
        uint16_t hue = RAW_AT(uint16_t, obj, 0x3e);
        int ox = RAW_AT(int16_t, obj, 0x20);
        int oy = RAW_AT(int16_t, obj, 0x22);
        int hovered = obj == hoverObj;
        if (hue == 0 && !hovered) {
            uint16_t *bmp0 = static_cast<uint16_t *>(i32_ptr(frameData[1]));
            if (corpse != 0)
                DrawGumpArt(ROWBLIT_COPY, RAW_AT(uint16_t, obj, 0x38) - 0x2c69, ox, oy, bmp0 + 2, bmp0[0], bmp0[1]);
            else
                Rle_Walk(PTR_AT(void, PORT_ADDR(&RowBlitCopyAll)), sprite, r, ox, oy);
            if (outline != nullptr)
                Rle_Walk(PTR_AT(void, PORT_ADDR(&RowBlitCopyAll)), sprite, r, ox + 5, oy + 5);
            continue;
        }
        if (hovered)
            g_hueRegister = 0x35;
        else if (hue > 0xea60)
            g_hueRegister = 1;
        else
            g_hueRegister = hue - 1;
        if (corpse != 0) {
            uint16_t *bmp0 = static_cast<uint16_t *>(i32_ptr(frameData[1]));
            DrawGumpArt(reinterpret_cast<RowBlitFn>(PORT_ADDR(&RowBlitLookupNonZero)), RAW_AT(uint16_t, obj, 0x38) - 0x2c69, ox, oy, bmp0 + 2,
                        bmp0[0], bmp0[1]);
        } else {
            if (!hovered && (PORT_VSLOT0(obj, 0x40, uint32_t)() & 0x40000) != 0)
                Rle_Walk(PTR_AT(void, PORT_ADDR(&RowBlitKeyedLookupAll)), sprite, r, ox, oy);
            else
                Rle_Walk(PTR_AT(void, PORT_ADDR(&RowBlitLookupAll)), sprite, r, ox, oy);
        }
        if (outline == nullptr)
            continue;
        if (!hovered && (PORT_VSLOT0(obj, 0x40, uint32_t)() & 0x40000) != 0)
            Rle_Walk(PTR_AT(void, PORT_ADDR(&RowBlitKeyedLookupAll)), sprite, r, ox + 5, oy + 5);
        else
            Rle_Walk(PTR_AT(void, PORT_ADDR(&RowBlitLookupAll)), sprite, r, ox + 5, oy + 5);
    }

    if (g_is565 != 0) {
        int n = tmp[0] * tmp[1];
        uint16_t *p = tmp + 2;
        while (n > 0) {
            uint32_t v = *p;
            *p = static_cast<uint16_t>(((v & 0xffe0) << 1) | (v & 0x1f));
            p++;
            n--;
        }
        uint16_t *dst = static_cast<uint16_t *>(i32_ptr(frameData[1])) + 2;
        Art_BlitConv(ROWBLIT_COPY, tmp + 2, tmp[0], tmp[1], 0, 0, dst, tmp[0], tmp[1]);
        operator delete(tmp);
    }
}
HOOK_METHOD(0x00426b70, CContainerGump, RedrawFrame);

// 0x00426f70  size 0xa2  draw the current frame bitmap (second frame when +0x60 is set) at the gump's screen origin.
void __thiscall CContainerGump::Draw(int *clip)
{
    int rect[4];
    int sx = 0;
    int sy = 0;
    uint16_t *bmp;
    if (unk_60 != 0)
        bmp = static_cast<uint16_t *>(i32_ptr(frameData[3]));
    else
        bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
    Gump_LocalToScreen(this, &sx, &sy);
    rect[0] = sx;
    rect[1] = sy;
    rect[2] = sx + bmp[0];
    rect[3] = sy + bmp[1];
    if (Mgr_Intersects(g_gumpManager, clip, rect) != 0)
        GFX_BlitFrame(ROWBLIT_COPY, bmp + 2, clip, bmp[0], bmp[1], sx, sy, g_backBuffer, 0x280, 0x1e0);
}
HOOK_VMETHOD(0x00426f70, CContainerGump, Draw);

// 0x00427020  size 0x5c  non-zero when the frame bitmap pixel at (x, y) is not 0.
int __thiscall CContainerGump::HitTest(int px, int py)
{
    uint16_t *bmp;
    if (unk_60 != 0)
        bmp = static_cast<uint16_t *>(i32_ptr(frameData[3]));
    else
        bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
    int w = bmp[0];
    int h = bmp[1];
    bmp += 2;
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return bmp[w * py + px] != 0;
}
HOOK_VMETHOD(0x00427020, CContainerGump, HitTest);

// 0x00427080  size 0x210  the contained object (searched from the +0xa8 end through +0x40) whose sprite, or
// its +5,+5 outline, is opaque at (x, y).
void *__thiscall CContainerGump::GetChildObjectAt(int px, int py)
{
    int aw, ah;
    if (px < 0 || py < 0 || px >= width || py >= height)
        return nullptr;
    int corpse = IsCorpseArt(unk_50) ? 1 : 0;
    void *obj = RAW_AT(void *, owner, 0xa8);
    if (obj == nullptr)
        return nullptr;
    do {
        uint16_t gfxId = RAW_AT(uint16_t, obj, 0x38);
        if (gfxId < 0x4000) {
            uint8_t *rec = g_tileData + gfxId * 0x28;
            if ((*reinterpret_cast<uint32_t *>(rec) & 0x400000) != 0) {
                uint8_t t = rec[5];
                if (t == 0xb || t == 0x10) {
                    obj = RAW_AT(void *, obj, 0x40);
                    continue;
                }
            }
        }
        if (corpse == 0) {
            void *spr = PORT_VSLOT0(obj, 0x74, void *)();
            void *outline = PORT_VSLOT0(obj, 0x78, void *)();
            if (spr != nullptr) {
                int ox = RAW_AT(int16_t, obj, 0x20);
                int oy = RAW_AT(int16_t, obj, 0x22);
                if (px >= ox && py >= oy && px <= RAW_AT(uint16_t, spr, 4) + ox - 1 &&
                    py <= RAW_AT(uint16_t, spr, 6) + oy - 1) {
                    if (Sprite_PixelHit(GLOBAL_PTR(void, g_artCache_storage), spr, px - RAW_AT(int16_t, obj, 0x20), py - oy) != 0)
                        return obj;
                }
                if (outline != nullptr) {
                    ox = RAW_AT(int16_t, obj, 0x20);
                    oy = RAW_AT(int16_t, obj, 0x22);
                    if (px >= ox + 5 && py >= oy + 5 && px <= RAW_AT(uint16_t, spr, 4) + ox + 4 &&
                        py <= RAW_AT(uint16_t, spr, 6) + oy + 4) {
                        if (Sprite_PixelHit(GLOBAL_PTR(void, g_artCache_storage), spr, px - ox - 5, py - oy - 5) != 0)
                            return obj;
                    }
                }
            }
        } else {
            int art = RAW_AT(uint16_t, obj, 0x38) - 0x2c69;
            GumpArtSize(art, &aw, &ah);
            int ox = RAW_AT(int16_t, obj, 0x20);
            int oy = RAW_AT(int16_t, obj, 0x22);
            if (px >= ox && py >= oy && px <= aw + ox - 1 && py <= ah + oy - 1) {
                if (Art_PixelHit(art, px - ox, py - RAW_AT(int16_t, obj, 0x22)) != 0)
                    return obj;
            }
        }
        obj = RAW_AT(void *, obj, 0x40);
    } while (obj != nullptr);
    return nullptr;
}
HOOK_METHOD(0x00427080, CContainerGump, GetChildObjectAt);

// 0x00427290  size 0x258  drop the held item at (x, y): onto a stackable item or a container item under the point,
// else into this container at the point (clamped to the container's gump rectangle). Sends the drop packet.
int __thiscall CContainerGump::DropHeldItemAt(int px, int py)
{
    int rect[4];
    uint8_t pkt[0x14];
    int flag = 0;
    uint16_t heldW;
    uint16_t heldH;
    void *target;

    if (parent != nullptr)
        CGump_BringToFront(parent, 0);
    else
        CGump_BringToFront(this, 0);
    if (g_heldGump != nullptr)
        CGump_BringToFront(static_cast<CGump *>(g_heldGump), 0);   // ? ECX is the held gump here, not this
    if (unk_a0 != nullptr && RAW_AT(void *, unk_a0, 0xb8) != this)
        return 0;
    heldW = RAW_AT(uint16_t, g_heldGump, 0x1c);
    heldH = RAW_AT(uint16_t, g_heldGump, 0x20);
    target = GetChildObjectAt(px, py);
    if (target != nullptr) {
        if (PORT_VSLOT0(target, 0x30, int)() == 0 &&
            (RAW_AT(uint16_t, target, 0x3c) == 0 || (g_tileData[(RAW_AT(int8_t, target, 0x3a) + RAW_AT(uint16_t, target, 0x38)) * 0x28 + 1] & 8) == 0 ||
             g_heldCount == 0 || (g_heldFlags & 0x800) == 0 || RAW_AT(uint16_t, target, 0x38) != g_heldGraphic || RAW_AT(uint16_t, target, 0x3e) != g_heldHue)) {
            flag = 1;
        } else {
            if (PORT_VSLOT0(target, 0x30, int)() != 0) {
                Mgr_GetRect(g_gumpManager, Mgr_IdForItem(g_gumpManager, RAW_AT(uint16_t, target, 0x38)), rect);
                if (rect[2] - rect[0] < heldW)
                    return 0;
                if (rect[3] - rect[1] < heldH)
                    return 0;
                Pkt_DropItem(g_packetBuilder, pkt, g_heldSerial, 0xffff, 0xffff, 0, RAW_AT(int32_t, target, 0x78));
            } else {
                // The original passes y and z with stale upper register bits; the builder only uses the low word / byte.
                Pkt_DropItemNarrow(g_packetBuilder, pkt, g_heldSerial, RAW_AT(uint16_t, target, 0x20), RAW_AT(uint16_t, target, 0x22),
                             RAW_AT(uint8_t, target, 0x24), RAW_AT(int32_t, target, 0x78));
            }
            Net_Send(g_pktSender, pkt);
        }
    }
    if (target != nullptr && flag == 0)
        return 1;

    Mgr_GetRect(g_gumpManager, Mgr_IdForItem(g_gumpManager, RAW_AT(uint16_t, owner, 0x38)), rect);
    px -= heldW >> 1;
    py -= heldH >> 1;
    if (px + heldW > rect[2])
        px = rect[2] - heldW;
    if (py + heldH > rect[3])
        py = rect[3] - heldH;
    if (px < rect[0])
        px = rect[0];
    if (py < rect[1])
        py = rect[1];
    Pkt_DropItem(g_packetBuilder, pkt, g_heldSerial, px, py, 0, RAW_AT(int32_t, owner, 0x78));
    Net_Send(g_pktSender, pkt);
    return 1;
}
HOOK_METHOD(0x00427290, CContainerGump, DropHeldItemAt);

// 0x004274f0  size 0x121  redraw the area of `obj` (clamped to the frame) unless redraws are blocked by 0x007211a8.
void __thiscall CContainerGump::AddToGump(void *obj)
{
    int rect[4];
    int aw, ah;
    void *spr = nullptr;
    int corpse = 0;
    if (unk_50 == 0x91a || unk_50 == 0x92e)
        corpse = 1;

    if (corpse != 0) {
        GumpArtSize(RAW_AT(uint16_t, obj, 0x38) - 0x2c69, &aw, &ah);
        rect[0] = RAW_AT(int16_t, obj, 0x20);
        rect[1] = RAW_AT(int16_t, obj, 0x22);
        rect[2] = aw + rect[0];
        rect[3] = ah + rect[1];
    } else {
        spr = PORT_VSLOT0(obj, 0x74, void *)();
        if (spr != nullptr) {
            rect[0] = RAW_AT(int16_t, obj, 0x20);
            rect[1] = RAW_AT(int16_t, obj, 0x22);
            rect[2] = RAW_AT(uint16_t, spr, 4) + rect[0];
            rect[3] = RAW_AT(uint16_t, spr, 6) + rect[1];
        }
    }
    if (corpse == 0 && spr == nullptr)
        return;
    if (rect[0] < 0)
        rect[0] = 0;
    if (rect[0] >= width)
        rect[0] = width - 1;
    if (rect[2] < 0)
        rect[2] = 0;
    if (rect[2] >= width)
        rect[2] = width - 1;
    if (rect[1] < 0)
        rect[1] = 0;
    if (rect[1] >= height)
        rect[1] = height - 1;
    if (rect[3] < 0)
        rect[3] = 0;
    if (rect[3] >= height)
        rect[3] = height - 1;
    if (g_redrawBlock == 0)
        RedrawFrame(rect);
}
HOOK_METHOD(0x004274f0, CContainerGump, AddToGump);
