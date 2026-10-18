#include <cstdlib>
#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "batch01_helpers.h"
#include "button_gump.h"
#include "font_manager.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "scroll_thumb_gump.h"
#include "gump_art_ops_ports.h"
#include "window_helpers_31_ports.h"
#include "gump_base_ops_ports.h"
#include "vtables.h"

// The SEH frames of the originals (try level around the base constructor) are omitted.


// 0x00434410: ECX = art id, EDX = x, stack = y, RET 4; result in AX: non-zero when the art pixel is opaque.
#define Art_PixelHit(a1, a2, a3) (GumpArtPixel(a1, a2, a3))

#define BG_DrawArtClipped PORT_FUNC8(&DrawGumpArtClipped, void, __fastcall, void *, int, const int *, int, int, void *, int, int)

static void buttonCallback(CButtonGump *b)
{
    // The click handler is a __thiscall code address called with the parent gump in ECX.
    PORT_THISCALL0((uint32_t)(b->callback), void, b->parent)();
}

static inline uint16_t *frameBitmap(const CGump *g, int slot)
{
    return reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(g->frameData[slot * 2 + 1])));
}

// 0x00413110  size 0x132
// Rectangle button constructor: no art, the rectangle (x1, y1)-(x2, y2) inclusive gives the size.
CButtonGump::CButtonGump(int x1, int y1, int x2, int y2, CGump *p5, int type, int frame, int callbackAddr, int p9)
{
    (void)p5;   // not read by the original
    param_94 = type;
    callback = callbackAddr;
    param_b4 = p9;
    int dx = x2 - x1;
    unk_c8 = 0;
    int w = dx + 1;
    flag_b0 = 0;
    unk_b8 = 0;
    unk_ac = 0;
    unk_bc = 0;
    unk_50 = 0;
    artDown = 0;
    unk_a8 = 0;
    artHover = 0;
    owner = nullptr;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    x = x1;
    name = const_cast<char *>("Button gump");
    unk_c0 = 1;
    y = y1;
    if (w > 0 && (w & 1) == 1) {   // an odd width is rounded up
        rowStride = dx + 2;
        width = dx + 2;
    } else {
        rowStride = w;
        width = w;
    }
    height = (y2 - y1) + 1;
    frameIndex = frame;
    frameCount = 2;
    int32_t *fd = static_cast<int32_t *>(gfx::AllocTagged(0x10, "framedata for CButtonGump"));
    frameData = fd;
    memset(fd, 0, static_cast<size_t>(frameCount) * 8);
}
HOOK_CTOR(0x00413110, CButtonGump, (int, int, int, int, CGump *, int, int, int, int));

HOOK_VTABLE(0x0049ca90, CButtonGump);
HOOK_SDD(0x004130e0, CButtonGump);

// 0x00413250  size 0xa3
CButtonGump::~CButtonGump()
{
    if (frameData[1] != 0) {
        gfx::FreeTagged(reinterpret_cast<void *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[1]))), "ButtonGump frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData[3] != 0) {
        gfx::FreeTagged(reinterpret_cast<void *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[3]))), "ButtonGump frame1 pBMP");
        frameData[3] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "ButtonGump framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00413250, CButtonGump);

// 0x00413300  size 0x17
void __thiscall CButtonGump::RButtonDown(int a, int b)
{
    PORT_VSLOT2(parent, 0x18, void, int, int)(a, b);
}
HOOK_VMETHOD(0x00413300, CButtonGump, RButtonDown);

// 0x00413320  size 0x17
void __thiscall CButtonGump::RButtonUp(int a, int b)
{
    PORT_VSLOT2(parent, 0x1c, void, int, int)(a, b);
}
HOOK_VMETHOD(0x00413320, CButtonGump, RButtonUp);

// 0x00413340  size 0x82
// Press: bring the parent to the front. Type 0 / 2 buttons go down; type 2 also fires at once.
void __thiscall CButtonGump::MouseDown(int mx, int my)
{
    (void)mx;
    (void)my;
    if (parent != nullptr) {
        CGump_BringToFront(parent, 0);   // 0x0043ba70
        parent->unk_0c = 0;
    }
    unk_c8 = 1;
    if (flag_b0 == 0) {
        switch (param_94) {
        case 0:
            frameIndex = 1;
            break;
        case 2:
            frameIndex = 1;
            if (unk_ac != 0)
                parent->unk_08 = unk_9c;
            if (callback != 0)
                PORT_THISCALL0((uint32_t)callback, void, parent)();
            unk_98 = 5;
            break;
        }
    }
}
HOOK_VMETHOD(0x00413340, CButtonGump, MouseDown);

// 0x004133d0  size 0xaf
// Release: type 0 fires, type 1 toggles and fires then notifies the parent (slot 0x44), type 2 pops back up.
void __thiscall CButtonGump::MouseUp(int mx, int my)
{
    (void)mx;
    (void)my;
    unk_c8 = 0;
    g_dragGump = nullptr;
    if (parent != nullptr)
        parent->unk_0c = 0;
    if (flag_b0 == 0) {
        switch (param_94) {
        case 2:
            frameIndex = 0;
            break;
        case 1:
            frameIndex = (frameIndex == 0) ? 1 : 0;
            if (unk_ac != 0)
                parent->unk_08 = unk_9c;
            if (callback != 0)
                PORT_THISCALL0((uint32_t)(callback), void, parent)();
            PORT_VSLOT1(parent, 0x44, void, CGump *)(this);
            break;
        case 0:
            frameIndex = 0;
            if (unk_ac != 0)
                parent->unk_08 = unk_9c;
            if (callback != 0)
                PORT_THISCALL0((uint32_t)(callback), void, parent)();
            break;
        }
    }
}
HOOK_VMETHOD(0x004133d0, CButtonGump, MouseUp);

// 0x00413480  size 0x1d
void __thiscall CButtonGump::MouseOver(int mx, int my)
{
    (void)mx;
    (void)my;
    unk_bc = 1;
    (g_app.*PORT_METHOD1(CApp, void, &SetCursorForRegion, int))(0);   // set the default mouse cursor
}
HOOK_VMETHOD(0x00413480, CButtonGump, MouseOver);

// 0x00413500  size 0x1d5
void __thiscall CButtonGump::Draw(int *clip)
{
    if (visible == 0)
        return;
    int ox = 0;
    int oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);
    int rect[4];
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = width + ox;
    rect[3] = height + oy;
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, void *, int *)(clip, rect) == 0)
        return;

    if (unk_b8 != 0) {
        // frame bitmap button: blit the bitmap of the current frame
        uint16_t *bmp = frameBitmap(this, frameIndex);
        GFX_BlitFrame(gfx::artPlain(), bmp + 2, clip, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
        return;
    }
    int art;
    if (frameIndex == 1) {
        if (artDown == 0)
            return;
        if (flag_b0 != 0) {
            g_hueOverride = 0x3c0;
            gfx::DrawArtClipped(gfx::artHued(), artDown, static_cast<const int *>(clip), ox, oy, g_backBuffer, 0x280, 0x1e0);
            return;
        }
        art = artDown;
    } else if (unk_bc != 0 && artHover != 0 && flag_b0 == 0) {
        gfx::DrawArtClipped(gfx::artPlain(), artHover, static_cast<const int *>(clip), ox, oy, g_backBuffer, 0x280, 0x1e0);
        return;
    } else {
        if (unk_50 == 0)
            return;
        if (flag_b0 != 0) {
            g_hueOverride = 0x3c0;
            gfx::DrawArtClipped(gfx::artHued(), unk_50, static_cast<const int *>(clip), ox, oy, g_backBuffer, 0x280, 0x1e0);
            return;
        }
        art = unk_50;
    }
    gfx::DrawArtClipped(gfx::artPlain(), art, static_cast<const int *>(clip), ox, oy, g_backBuffer, 0x280, 0x1e0);
}
HOOK_VMETHOD(0x00413500, CButtonGump, Draw);

// 0x004136e0  size 0x60
int __thiscall CButtonGump::HitTest(int mx, int my)
{
    if (visible != 0 && mx >= 0 && my >= 0 && mx < width && my < height) {
        if (unk_c0 == 0)
            return 1;
        if (unk_b8 != 0 || Art_PixelHit(unk_50, mx, my) != 0)   // 0x00434410
            return 1;
    }
    return 0;
}
HOOK_VMETHOD(0x004136e0, CButtonGump, HitTest);

// ---------------------------------------------------------------------------------------------------------
// CScrollThumbGump ("Thumb gump")

// 0x00413740  size 0x113
CScrollThumbGump::CScrollThumbGump(int x_, int y_, int vertical_, int limit, int parent_, int artId)
{
    (void)parent_;   // not read by the original
    name = const_cast<char *>("Thumb gump");
    unk_b0 = 0;
    hot = 0;
    unk_50 = artId;
    artAlt = 0;
    owner = nullptr;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    gfx::ArtSize(artId, &width, &height);
    rowStride = width;
    frameIndex = 0;
    frameCount = 1;
    x = x_;
    y = y_;
    vertical = vertical_;
    minX = x_;
    if (vertical_ != 0) {
        maxX = x_;
        minY = y_;
        maxY = limit - height;
    } else {
        minY = y_;
        maxY = y_;
        maxX = limit - width;
    }
    int32_t *fd = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CThumbGump"));
    frameData = fd;
    memset(fd, 0, static_cast<size_t>(frameCount) * 8);
}
HOOK_CTOR(0x00413740, CScrollThumbGump, (int, int, int, int, int, int));

HOOK_VTABLE(0x0049cba8, CScrollThumbGump);
HOOK_SDD(0x00413860, CScrollThumbGump);

// 0x00413890  size 0xb
CScrollThumbGump::~CScrollThumbGump()
{
}
HOOK_DTOR(0x00413890, CScrollThumbGump);

// 0x004138a0  size 0x1d
void __thiscall CScrollThumbGump::MouseDown(int mx, int my)
{
    unk_b0 = 1;
    unk_6c = mx;
    unk_70 = my;
}
HOOK_VMETHOD(0x004138a0, CScrollThumbGump, MouseDown);

// 0x004138c0  size 0x64
// Release: tell the parent (slot 0x34) the position in 0..range.
void __thiscall CScrollThumbGump::MouseUp(int mx, int my)
{
    (void)mx;
    (void)my;
    int v = vertical;
    unk_b0 = 0;
    g_dragGump = nullptr;
    CGump *p = parent;
    if (v != 0) {
        PORT_VSLOT3(p, 0x34, void, int, int, CGump *)(y - minY, maxY - minY, this);
    } else {
        PORT_VSLOT3(p, 0x34, void, int, int, CGump *)(x - minX, maxX - minX, this);
    }
}
HOOK_VMETHOD(0x004138c0, CScrollThumbGump, MouseUp);

// 0x00413930  size 0x21
void __thiscall CScrollThumbGump::MouseOver(int mx, int my)
{
    (void)mx;
    (void)my;
    (g_app.*PORT_METHOD1(CApp, void, &SetCursorForRegion, int))(0);   // set the default mouse cursor
    hot = 1;
}
HOOK_VMETHOD(0x00413930, CScrollThumbGump, MouseOver);

// 0x00413970  size 0xc4
// Drag: move within the limits, then tell the parent (slot 0x30) the position in 0..range.
void __thiscall CScrollThumbGump::MouseMove(int mx, int my)
{
    if (unk_b0 == 0)
        return;
    int nx = (mx - unk_6c) + x;
    int ny = (my - unk_70) + y;
    if (ny < minY)
        ny = minY;
    if (ny > maxY)
        ny = maxY;
    if (nx < minX)
        nx = minX;
    if (nx > maxX)
        nx = maxX;
    Move(nx - x, ny - y, 1, nullptr, 1);   // 0x0043b140
    CGump *p = parent;
    if (vertical != 0) {
        PORT_VSLOT3(p, 0x30, void, int, int, CGump *)(y - minY, maxY - minY, this);
    } else {
        PORT_VSLOT3(p, 0x30, void, int, int, CGump *)(x - minX, maxX - minX, this);
    }
}
HOOK_VMETHOD(0x00413970, CScrollThumbGump, MouseMove);

// 0x00413a40  size 0xda
void __thiscall CScrollThumbGump::Draw(int *clip)
{
    if (visible == 0)
        return;
    int ox = 0;
    int oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);
    int rect[4];
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = width + ox;
    rect[3] = height + oy;
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, void *, int *)(clip, rect) == 0)
        return;
    if (hot != 0 && artAlt != 0) {
        BG_DrawArtClipped(gfx::artPlain(), artAlt, static_cast<const int *>(clip), ox, oy, g_backBuffer, 0x280, 0x1e0);
        return;
    }
    BG_DrawArtClipped(gfx::artPlain(), unk_50, static_cast<const int *>(clip), ox, oy, g_backBuffer, 0x280, 0x1e0);
}
HOOK_VMETHOD(0x00413a40, CScrollThumbGump, Draw);

// 0x00413b20  size 0xd4
// Place the thumb along its track by the share of the parent's wrapped text that lies above the cursor line.
void __thiscall CScrollThumbGump::PlaceFromTextList()
{
    CGump *p = parent;
    uint8_t *node = GLOBAL_AS(uint8_t *, g_textListHead);
    int wrapWidth = RAWF(int32_t, p, 0xb8) - RAWF(int32_t, p, 0xb0) + 1;
    int before = 0;
    int outW;
    int outH;
    int outBase;
    if (node != RAWF(uint8_t *, p, 0xc8)) {
        do {
            g_fontMgr.MeasureTextWrapped(0, RAWF(char *, node, 0), wrapWidth, &outW, &outH, &outBase, 2);
            before += outH;
            node = RAWF(uint8_t *, node, 0x18);
        } while (node != RAWF(uint8_t *, p, 0xc8));
    }
    int total = before;
    if (node != nullptr) {
        do {
            g_fontMgr.MeasureTextWrapped(0, RAWF(char *, node, 0), wrapWidth, &outW, &outH, &outBase, 2);
            total += outH;
            node = RAWF(uint8_t *, node, 0x18);
        } while (node != nullptr);
    }
    y = minY;
    y = (maxY - minY + 1) * (RAWF(int32_t, p, 0xcc) + before) / total + minY;
}
HOOK_METHOD(0x00413b20, CScrollThumbGump, PlaceFromTextList);

// 0x00413c00  size 0x6b
void __thiscall CScrollThumbGump::SetPosition(int value, int range)
{
    if (vertical != 0) {
        y = minY;
        y += (maxY - minY + 1) * value / range;
        if (y > maxY)
            y = maxY;
    } else {
        x = minX;
        x += (maxX - minX + 1) * value / range;
        if (x > maxX)
            x = maxX;
    }
}
HOOK_METHOD(0x00413c00, CScrollThumbGump, SetPosition);
