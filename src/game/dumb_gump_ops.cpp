// CDumbGump (vtable 0x0049ee58, "dumb gump"): constructor, destructors and the draw handler.
// gumps.cxx, 0x0043d0a0..0x0043d2e0. The SEH frames of the original are omitted.
#include "dumb_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "batch02_helpers.h"
#include "globals.h"
#include "crt.h"
#include "art_record_ops_ports.h"
#include "art_row_blits_ports.h"
#include "art_blit_ports.h"
#include "art_row_blits_hue_ports.h"
#include "vtables.h"

#define Dg_TileArt(o, a1) PORT_THISCALL1(&ArtRecord_Get, uint8_t *, (o), uint32_t)(a1)
#define Dg_ArtCache GLOBAL_PTR(void, g_artCache_storage)
#define Dg_SetTarget(a1, a2) (SetBlitSurface((uint16_t *)(a1), a2))
#define Dg_RleWalk PORT_FUNC5(&RleArtWalk, void, __fastcall, void *, uint8_t *, int *, int, int)
#define kDumbGumpVtable reinterpret_cast<uintptr_t>(vtbl_CDumbGump)

// 0x0043d0a0  size 0x17d
// Constructor, RET 0x20: (x, y, a3 (not read), width, height, tileArt, pixels or 0, gumpArt or -1).
// One frame whose 16-bit bitmap (width, height, then the pixels) starts cleared, is filled from
// `pixels` when given, and gets gump art `gumpArt` drawn into it when it is not -1.
CDumbGump::CDumbGump(int px, int py, int, int w, int h, int tileArtId, const void *pixels, int gumpArt)
{
    x = px;
    tileArt = tileArtId;
    height = h;
    useTileArt = 0;
    altBlit = 0;
    y = py;
    name = const_cast<char *>("dumb gump");   // 0x004a841c
    dirty = 0;
    unk_50 = 0;
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    width = w;
    frameIndex = 0;
    frameCount = 1;

    int32_t *fd = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CDumbGump"));
    memset(fd, 0, static_cast<size_t>(frameCount) << 3);
    frameData = fd;

    uint8_t *bmp = static_cast<uint8_t *>(gfx::AllocTagged(width * height * 2 + 4, "dumb frame0"));
    frameData[1] = ptr_i32(bmp);
    *reinterpret_cast<uint16_t *>(bmp) = static_cast<uint16_t>(width);
    *reinterpret_cast<uint16_t *>(bmp + 2) = static_cast<uint16_t>(height);
    memset(bmp + 4, 0, static_cast<size_t>(width * height) * 2);
    if (pixels != nullptr)
        memcpy(bmp + 4, pixels, static_cast<size_t>(h * w) * 2);
    if (gumpArt != -1) {
        uint8_t *b = static_cast<uint8_t *>(i32_ptr(frameData[1]));
        DrawGumpArt(reinterpret_cast<RowBlitFn>(static_cast<uintptr_t>(PORT_ADDR(&RowBlitCopyAll))), gumpArt, 0, 0,
                    reinterpret_cast<uint16_t *>(b + 4), *reinterpret_cast<uint16_t *>(b), *reinterpret_cast<uint16_t *>(b + 2));   // 0x00433ed0
    }
}
HOOK_CTOR(0x0043d0a0, CDumbGump, (int, int, int, int, int, int, const void *, int));

HOOK_VTABLE(0x0049ee58, CDumbGump);
HOOK_SDD(0x0043d220, CDumbGump);

// 0x0043d250  size 0x8b
// Destructor body: free the frame bitmap and the frame table, then the CGump base. SEH frame omitted.
CDumbGump::~CDumbGump()
{
    int32_t bmp = frameData[1];
    if (bmp != 0) {
        gfx::FreeTagged(i32_ptr(bmp), "dumb frame0 pBMP");   // 0x00446310
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "dumb framedata");   // 0x00446310
        frameData = nullptr;
    }
}
HOOK_DTOR(0x0043d250, CDumbGump);

// 0x0043d2e0  size 0x10d
// Draw handler (vtable slot +0x08), RET 4. When the gump is visible and its frame rect overlaps
// `clip`: tile art `tileArt` is drawn at the gump's screen origin when +0x94 is set, otherwise frame0
// is blitted (row blitter 0x004764c0, or 0x004764f0 when +0x98 is set).
void __thiscall CDumbGump::Draw(int *clip)
{
    if (visible == 0)
        return;
    int sx = 0;
    int sy = 0;
    for (CGump *g = this; g != nullptr; g = g->parent) {
        sx += g->x;
        sy += g->y;
    }
    uint8_t *bmp = static_cast<uint8_t *>(i32_ptr(frameData[1]));
    int w = *reinterpret_cast<uint16_t *>(bmp);
    int h = *reinterpret_cast<uint16_t *>(bmp + 2);
    int right = sx + w;
    int bottom = sy + h;
    if (!(clip[2] > sx && clip[0] < right && clip[3] > sy && clip[1] < bottom))
        return;

    if (useTileArt != 0) {
        uint8_t *art = Dg_TileArt(Dg_ArtCache, static_cast<uint32_t>(tileArt));   // 0x00476130
        if (art == nullptr)
            return;
        Dg_SetTarget(g_backBuffer, 0x280);
        Dg_RleWalk(PTR_AT(void, PORT_ADDR(&RowBlitCopyAll)), art, clip, sx, sy);
        return;
    }
    if (altBlit == 0)
        GFX_BlitFrame(PTR_AT(void, PORT_ADDR(&RowBlitCopyNonZero)), bmp + 4, clip, w, h, sx, sy, g_backBuffer, 0x280, 0x1e0);   // 0x004342e0
    else
        GFX_BlitFrame(PTR_AT(void, PORT_ADDR(&RowBlitBlendNonZero)), bmp + 4, clip, w, h, sx, sy, g_backBuffer, 0x280, 0x1e0);   // 0x004342e0
}
HOOK_VMETHOD(0x0043d2e0, CDumbGump, Draw);
