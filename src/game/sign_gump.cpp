// CSignGump (vtable 0x0049ef70, "sign gump"): constructor, destructors, draw and drag handlers.
// gumps.cxx, 0x0043d3f0..0x0043d710. The SEH frames of the original are omitted.
#include "sign_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "batch02_helpers.h"
#include "globals.h"
#include "crt.h"
#include "gump_base_ops_ports.h"
#include "gump_art_ops_ports.h"
#include "art_row_blits_ports.h"
#include "vtables.h"

#define kSignGumpVtable reinterpret_cast<uintptr_t>(vtbl_CSignGump)
#define Sg_GetContainerRect(a1, a2) (GetContainerGumpRect(a1, a2))

// 0x0043d420  size 0x198
// Constructor, RET 0x18: (x, y, artId, a4 (not read), text, owner). The frame0 bitmap is the size
// of gump art `artId`; the art is drawn into it, then `text` wrapped into the art's container
// rectangle (0x0043adf0), font 1, hue 0x386.
CSignGump::CSignGump(int px, int py, int artId, int, char *text, CObj *own)
{
    x = px;
    y = py;
    owner = own;
    name = const_cast<char *>("sign gump");   // 0x004a844c
    dirty = 0;
    unk_50 = artId;
    unk_60 = 0;
    int w, h;
    GumpArtSizeImpl(artId, &w, &h);   // 0x00433e70
    width = w;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    height = h;
    frameIndex = 0;
    frameCount = 1;

    int32_t *fd = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CDumbGump"));
    memset(fd, 0, static_cast<size_t>(frameCount) << 3);
    frameData = fd;

    uint8_t *bmp = static_cast<uint8_t *>(gfx::AllocTagged(height * width * 2 + 4, "dumb frame0"));
    frameData[1] = ptr_i32(bmp);
    memset(bmp + 4, 0, static_cast<size_t>(height * width) * 2);
    *reinterpret_cast<uint16_t *>(bmp) = static_cast<uint16_t>(width);
    *reinterpret_cast<uint16_t *>(bmp + 2) = static_cast<uint16_t>(height);
    DrawGumpArt(reinterpret_cast<RowBlitFn>(static_cast<uintptr_t>(PORT_ADDR(&RowBlitCopyNonZero))), unk_50, 0, 0,
                reinterpret_cast<uint16_t *>(bmp + 4), *reinterpret_cast<uint16_t *>(bmp), *reinterpret_cast<uint16_t *>(bmp + 2));   // 0x00433ed0

    int rc[4];
    Sg_GetContainerRect(unk_50, rc);   // 0x0043adf0
    gfx::TextDrawWrapped(rc[0], rc[1], 1, 0x386, text, rc[2] - rc[0] + 1,
                         static_cast<uint8_t *>(i32_ptr(frameData[1])) + 4, width, rc, 0, 0);   // 0x00432260
}
HOOK_CTOR(0x0043d420, CSignGump, (int, int, int, int, char *, CObj *));

HOOK_VTABLE(0x0049ef70, CSignGump);
HOOK_SDD(0x0043d3f0, CSignGump);

// 0x0043d5c0  size 0x8b
// Destructor body: free the frame bitmap and frame table, then the CGump base. SEH frame omitted.
CSignGump::~CSignGump()
{
    int32_t bmp = frameData[1];
    if (bmp != 0) {
        gfx::FreeTagged(i32_ptr(bmp), "sign frame0 pBMP");   // 0x00446310
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "sign framedata");   // 0x00446310
        frameData = nullptr;
    }
}
HOOK_DTOR(0x0043d5c0, CSignGump);

// 0x0043d650  size 0x88
// Draw handler (vtable slot +0x08), RET 4: blit frame0 at the gump's screen origin when it is
// visible and overlaps `clip`.
void __thiscall CSignGump::Draw(int *clip)
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
    GFX_BlitFrame(PTR_AT(void, PORT_ADDR(&RowBlitCopyNonZero)), bmp + 4, clip, w, h, sx, sy, g_backBuffer, 0x280, 0x1e0);   // 0x004342e0
}
HOOK_VMETHOD(0x0043d650, CSignGump, Draw);

// 0x0043d6e0  size 0x28
// Mouse down (vtable slot 0x10), RET 8: bring to front and start dragging from (mx, my).
void __thiscall CSignGump::MouseDown(int mx, int my)
{
    BringToFront(0);   // 0x0043ba70
    dragging = 1;
    unk_6c = mx;
    unk_70 = my;
}
HOOK_VMETHOD(0x0043d6e0, CSignGump, MouseDown);

// 0x0043d710  size 0x2c
// Mouse move (vtable slot 0x28), RET 8: while dragging, move by the distance from the drag anchor.
void __thiscall CSignGump::MouseMove(int mx, int my)
{
    if (dragging != 0)
        Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);   // 0x0043b140
}
HOOK_VMETHOD(0x0043d710, CSignGump, MouseMove);
