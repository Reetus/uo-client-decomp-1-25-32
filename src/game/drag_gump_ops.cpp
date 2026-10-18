// CDragGump: constructor, destructor, mouse forwarding (0x00429430 .. 0x00429ae0).
// The SEH frames of the original (try level around the constructor / destructor bodies) are omitted.
#include "drag_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch02_helpers.h"
#include "globals.h"
#include "crt.h"
#include "paperdoll_gump.h"
#include "object_manager_ops_ports.h"
#include "art_record_ops_ports.h"
#include "art_row_blits_ports.h"
#include "mouse_manager.h"
#include "window_helpers_31_ports.h"
#include "gump.h"
#include "gump_manager.h"
#include "art_row_blits_hue_ports.h"
#include "vtables.h"

#define g_pDragGump GLOBAL_AS(CDragGump *, g_dragGump)
#define g_heldHueDword GLOBAL_AS(uint32_t, g_heldHue)        // low word = hue + 1 (0 = none)
#define g_heldItemPtr GLOBAL_AS(int, g_heldGump)
#define g_dragWord648920 GLOBAL_AS(int, g_paperdollRedrawFlag)
#define g_mouseManager GLOBAL_PTR(void, g_mouse_storage)
#define g_artManager GLOBAL_PTR(void, g_artCache_storage)
#define g_itemManager GLOBAL_PTR(void, g_objMgr_storage)
#define g_appObject GLOBAL_PTR(void, g_app_storage)

// 0x00457050: (graphic) RET 4, ECX not read; true for graphics 0x28e and 0xeed.
#define Item_IsCoinGraphic(o, a1) PORT_THISCALL1(&Graphic_Is_0x28e_Or_0xeed, int, (o), unsigned)(a1)
// 0x00476130: art record for an id (ECX = art manager), RET 4. Width at +4, height at +6.
#define Art_Get(o, a1) PORT_THISCALL1(&ArtRecord_Get, uint16_t *, (o), unsigned)(a1)
// 0x004767c0: ECX = destination pixels, EDX = row width (stored in globals).
#define Blit_SetTarget(a1, a2) (SetBlitSurface((uint16_t *)(a1), a2))
// 0x00476990: ECX = row blitter, EDX = art record, stack (x, y), RET 8.
#define Blit_DrawArt PORT_FUNC4(&RleArtWalkUnclipped, void, __fastcall, uintptr_t, uint16_t *, int, int)
#define MouseMgr_GetPos(self, a1, a2) (reinterpret_cast<CMouseManager *>(self)->GetMousePos(a1, a2))
#define Cursor_Set(o, a1) PORT_THISCALL1(&SetCursorForRegion, void, (o), int)(a1)
#define Gump_ScreenToLocal(self, a1, a2) (reinterpret_cast<CGump *>(self)->ScreenToLocal(a1, a2))
#define Gump_LocalToScreen(self, a1, a2) (reinterpret_cast<CGump *>(self)->LocalToScreen(a1, a2))
#define GumpMgr_HitTest(self, a1, a2, a3, a4) (reinterpret_cast<CGumpManager *>(self)->GumpAtPoint(a1, a2, a3, a4))

// 0x00429430  size 0x3aa
CDragGump::CDragGump()
{
    g_dragWord648920 = 0;
    name = const_cast<char *>("drag gump");
    dirty = 0;
    mouseDown = 0;
    sourceGump = nullptr;
    g_pDragGump = this;
    uint16_t *art = nullptr;            // ebp-0x10
    owner = nullptr;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    frameIndex = 0;
    frameCount = 1;
    int32_t *fd = FrameData_Alloc(g_frameDataAllocator, 8, "framedata for CDragGump");
    frameData = fd;
    fd[0] = 0;
    fd[1] = 0;
    int offset5 = 0;                    // ebp-0x18: second copy at (5, 5)
    int isGumpArt = 0;                  // ebp-0x14: graphics 0x3584..0x35a1 are drawn from gump art
    uint16_t graphic = g_heldGraphic;
    if (graphic >= 0x3584 && graphic <= 0x35a1)
        isGumpArt = 1;
    if (isGumpArt == 0) {
        if (Item_IsCoinGraphic(g_itemManager, graphic) != 0) {
            if (g_heldCount > 5)
                art = Art_Get(g_artManager, static_cast<unsigned>(g_heldGraphic) + 0x4002);
            else if (g_heldCount > 1)
                art = Art_Get(g_artManager, static_cast<unsigned>(g_heldGraphic) + 0x4001);
            else
                art = Art_Get(g_artManager, static_cast<unsigned>(g_heldGraphic) + 0x4000);
        } else {
            art = Art_Get(g_artManager, static_cast<unsigned>(g_heldGraphic) + 0x4000);
            if (g_heldCount > 1) {
                int idx = static_cast<int>(g_heldGraphic) + static_cast<int>(g_heldByteAt99);
                uint32_t fl = *reinterpret_cast<uint32_t *>(g_tileData + idx * 0x28);
                if ((fl & 0x800) != 0)          // TEST CH, 8
                    offset5 = 1;
            }
        }
        if (art == nullptr)
            art = Art_Get(g_artManager, 0x4000);
        int w = art[2];
        width = w;
        int h = art[3];
        height = h;
        if (offset5 != 0) {
            width = w + 5;
            height = h + 5;
        }
    } else {
        unk_50 = static_cast<int>(graphic) - 0x2c69;
        GumpArtSize(unk_50, &width, &height);
    }
    rowStride = width;
    frameData[1] = ptr_i32(FrameData_Alloc(g_frameDataAllocator, rowStride * height * 2 + 4, "drag gump"));
    uint16_t *bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
    bmp[0] = static_cast<uint16_t>(width);
    bmp[1] = static_cast<uint16_t>(height);
    memset(bmp + 2, 0, static_cast<size_t>(height * width * 2));
    Blit_SetTarget(bmp + 2, width);

    uintptr_t blitter;
    if (static_cast<uint16_t>(g_heldHueDword) != 0) {
        g_hueOverride = static_cast<int>(g_heldHueDword & 0xffff) - 1;
        if ((*reinterpret_cast<uint32_t *>(g_tileData + g_heldGraphic * 0x28) & 0x40000) != 0)
            blitter = PORT_ADDR(&RowBlitKeyedLookupAll);
        else
            blitter = PORT_ADDR(&RowBlitLookupAll);
    } else {
        blitter = PORT_ADDR(&RowBlitCopyAll);
    }

    if (isGumpArt == 0) {
        Blit_DrawArt(blitter, art, 0, 0);
        if (offset5 != 0)
            Blit_DrawArt(blitter, art, 5, 5);
    } else {
        uint16_t *fb = static_cast<uint16_t *>(i32_ptr(frameData[1]));
        DrawGumpArt(ROWBLIT_COPY, unk_50, 0, 0, fb + 2, fb[0], fb[1]);
    }

    if (g_is565 != 0) {
        uint16_t *px = static_cast<uint16_t *>(i32_ptr(frameData[1])) + 2;
        int n = height * width;
        if (n > 0) {
            int i = 0;
            do {
                unsigned v = *px++;
                px[-1] = static_cast<uint16_t>(((v & 0xffe0) << 1) | (v & 0x1f));
                i++;
            } while (i < height * width);
        }
    }

    int mx;
    int my;
    MouseMgr_GetPos(g_mouseManager, &mx, &my);
    x = mx - width / 2;
    y = my - height / 2;
    CGump_AddChild(this, nullptr, 1);
    Cursor_Set(g_appObject, 0);
    int lx = mx;
    int ly = my;
    Gump_ScreenToLocal(this, &lx, &ly);
    MouseMove(lx, ly);
}
HOOK_CTOR(0x00429430, CDragGump, ());

CDragGump *__thiscall CDragGump::Ctor()
{
    return new (this) CDragGump();
}

HOOK_VTABLE(0x0049e7f0, CDragGump);
HOOK_SDD(0x004297e0, CDragGump);

// 0x00429810  size 0xc1
CDragGump::~CDragGump()
{
    g_holdingFlag = 0;
    g_heldItemPtr = 0;
    g_pDragGump = nullptr;
    if (sourceGump != nullptr && RAW_AT(int32_t, sourceGump, 0x9c) != -1) {
        RAW_AT(int32_t, sourceGump, 0x9c) = -1;
        static_cast<CPaperdollGump *>(sourceGump)->RedrawFrame();
    }
    if (frameData != nullptr && frameData[1] != 0) {
        FrameData_Free(g_frameDataAllocator, i32_ptr(frameData[1]), "draggump frame0");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        FrameData_Free(g_frameDataAllocator, frameData, "draggump framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00429810, CDragGump);

// 0x004298e0  size 0x1a
void __thiscall CDragGump::MouseOver(int mx, int my)
{
    g_pDragGump = this;
    PORT_VSLOT2(this, 0x28, void, int, int)(mx, my);
}
HOOK_VMETHOD(0x004298e0, CDragGump, MouseOver);

// 0x00429900  size 0x15f
void __thiscall CDragGump::MouseMove(int mx, int my)
{
    unsigned slotLayer = g_tileData[g_heldGraphic * 0x28 + 5];
    CGump *oldTarget = sourceGump;
    Move(mx - width / 2, my - height / 2, 1, nullptr, 1);
    Gump_LocalToScreen(this, &mx, &my);
    CGump *under = GumpMgr_HitTest(g_gumpManager, mx, my, this, 0);
    if (g_heldSerial != -1) {
        if (under != nullptr) {
            int accepts = PORT_VSLOT0(under, 0x78, int)();
            if (accepts != 0) {
                if (under != sourceGump
                    && (*reinterpret_cast<uint32_t *>(g_tileData + g_heldGraphic * 0x28) & 0x400000) != 0
                    && RAW_AT(int32_t, under->owner, slotLayer * 4 + 0xc4) == 0) {
                    sourceGump = under;
                    RAW_AT(int32_t, under, 0x9c) = static_cast<int32_t>(slotLayer);
                }
            } else {
                if (sourceGump != nullptr) {
                    RAW_AT(int32_t, sourceGump, 0x9c) = -1;
                    sourceGump = nullptr;
                }
            }
        } else {
            if (sourceGump != nullptr) {
                RAW_AT(int32_t, sourceGump, 0x9c) = -1;
                sourceGump = nullptr;
            }
        }
        if (oldTarget != sourceGump) {
            if (oldTarget != nullptr)
                static_cast<CPaperdollGump *>(oldTarget)->RedrawFrame();
            if (sourceGump != nullptr)
                static_cast<CPaperdollGump *>(sourceGump)->RedrawFrame();
        }
    }
    if (mouseDown != 0 && under == g_screenGump) {
        Gump_ScreenToLocal(under, &mx, &my);
        PORT_VSLOT2(under, 0x28, void, int, int)(mx, my);
    }
}
HOOK_VMETHOD(0x00429900, CDragGump, MouseMove);

// 0x00429a60  size 0x71
void __thiscall CDragGump::RButtonDown(int mx, int my)
{
    mouseDown = 1;
    Gump_LocalToScreen(this, &mx, &my);
    CGump *g = GumpMgr_HitTest(g_gumpManager, mx, my, this, 0);
    if (g == g_screenGump) {
        Gump_ScreenToLocal(g, &mx, &my);
        PORT_VSLOT2(g, 0x20, void, int, int)(mx, my);
        PORT_VSLOT2(g, 0x18, void, int, int)(mx, my);
    }
}
HOOK_VMETHOD(0x00429a60, CDragGump, RButtonDown);

// 0x00429ae0  size 0x71
void __thiscall CDragGump::RButtonUp(int mx, int my)
{
    mouseDown = 0;
    Gump_LocalToScreen(this, &mx, &my);
    CGump *g = GumpMgr_HitTest(g_gumpManager, mx, my, this, 0);
    if (g == g_screenGump) {
        Gump_ScreenToLocal(g, &mx, &my);
        PORT_VSLOT2(g, 0x20, void, int, int)(mx, my);
        PORT_VSLOT2(g, 0x1c, void, int, int)(mx, my);
    }
}
HOOK_VMETHOD(0x00429ae0, CDragGump, RButtonUp);

// 0x00429b60  size 0xb  slot 0x2c: runs slot 0x2c of the screen gump (was the free ScreenGumpSlot2c_00429b60).
void __thiscall CDragGump::Tick()
{
    void *g = GLOBAL_AS(void *, g_screenGump);
    PORT_VSLOT0(g, 0x2c, void)();
}
HOOK_VMETHOD(0x00429b60, CDragGump, Tick);
