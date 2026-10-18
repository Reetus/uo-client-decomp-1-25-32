// CResizeGump second pass (batch 30).
#include "resize_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "batch06_helpers.h"
#include "dumb_gump.h"
#include "crt.h"
#include "bitmap16_draw_ports.h"
#include "vtables.h"
#include "globals.h"

// 0x00432c70: fill (x0, y0, x1, y1, color, bitmap) inclusive, RET 0x18; ECX is the text object (not read).
#define RS_FillRect(a1, a2, a3, a4, a5, a6) (PORT_THISCALL6(&Bitmap16_FillRect, void, gfx::textObj(), int, int, int, int, int, void *)(a1, a2, a3, a4, a5, (a6)))
// 0x00436870 on the pixel-format object: (r, g, b) -> 16-bit pixel.
#define RS_White b6_make_color(b6_pixel_format, 0xff, 0xff, 0xff)

// 0x0047b230  size 0x3f4
CResizeGump::CResizeGump(int x_, int y_, int curW_, int curH_, int minW_, int minH_, int maxW_, int maxH_, CGump *target_)
{
    CDumbGump *edge;
    int oddMaxW;

    x = x_;
    y = y_;
    height = maxH_;
    oddMaxW = maxW_ % 2;
    name = const_cast<char *>("resize gump");
    dirty = 0;
    unk_58 = 0;
    unk_60 = 0;
    unk_50 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    width = maxW_;
    if (oddMaxW == 1)
        width = maxW_ + 1;
    prevW = 0;
    minW = minW_;
    prevH = 0;
    if (minW_ % 2 == 1)
        minW = minW_ + 1;
    minH = minH_;
    maxW = maxW_;
    if (oddMaxW == 1)
        maxW = maxW_ + 1;
    maxH = maxH_;
    curW = curW_;
    curH = curH_;
    if (curW_ < minW)
        curW = minW;
    if (curH_ < minH_)
        curH = minH_;
    if (curW > maxW)
        curW = maxW;
    if (curH > maxH_)
        curH = maxH_;
    if (curW % 2 == 1)
        curW = curW + 1;
    target = target_;
    frameIndex = 0;
    frameCount = 0;

    // top edge
    edge = new CDumbGump(0, 0, reinterpret_cast<int>(this), maxW, 2, 1, nullptr, -1);   // 0x0043d0a0
    edge->unk_58 = static_cast<int32_t>(reinterpret_cast<uintptr_t>(gfx::AllocTagged(edge->width * edge->height * 2, "gump background")));
    edge->LinkInto(this, 0);   // 0x0043bcc0
    RS_FillRect(0, 0, curW - 1, 1, RS_White, b6_frame_bitmap(edge, 0));   // 0x00432c70, color 0x00436870

    // right edge
    edge = new CDumbGump(curW - 2, 0, reinterpret_cast<int>(this), 2, maxH, 2, nullptr, -1);   // 0x0043d0a0
    edge->unk_58 = static_cast<int32_t>(reinterpret_cast<uintptr_t>(gfx::AllocTagged(edge->width * edge->height * 2, "gump background")));
    edge->LinkInto(this, 0);   // 0x0043bcc0
    RS_FillRect(0, 0, 1, curH - 1, RS_White, b6_frame_bitmap(edge, 0));   // 0x00432c70, color 0x00436870

    // bottom edge
    edge = new CDumbGump(0, curH - 2, reinterpret_cast<int>(this), maxW, 2, 3, nullptr, -1);   // 0x0043d0a0
    edge->unk_58 = static_cast<int32_t>(reinterpret_cast<uintptr_t>(gfx::AllocTagged(edge->width * edge->height * 2, "gump background")));
    edge->LinkInto(this, 0);   // 0x0043bcc0
    RS_FillRect(0, 0, curW - 1, 1, RS_White, b6_frame_bitmap(edge, 0));   // 0x00432c70, color 0x00436870

    // left edge
    edge = new CDumbGump(0, 0, reinterpret_cast<int>(this), 2, maxH, 4, nullptr, -1);   // 0x0043d0a0
    edge->unk_58 = static_cast<int32_t>(reinterpret_cast<uintptr_t>(gfx::AllocTagged(edge->width * edge->height * 2, "gump background")));
    edge->LinkInto(this, 0);   // 0x0043bcc0
    RS_FillRect(0, 0, 1, curH - 1, RS_White, b6_frame_bitmap(edge, 0));   // 0x00432c70, color 0x00436870

    MouseMove(target->unk_6c, target->unk_70);   // 0x0047b6b0
    g_dragGump = this;
}
HOOK_CTOR(0x0047b230, CResizeGump, (int, int, int, int, int, int, int, int, CGump *));

HOOK_VTABLE(0x004a0a70, CResizeGump);
HOOK_SDD(0x0047b630, CResizeGump);

// 0x0047b660  size 0xb  sets the vtable; the jump to the base destructor is the compiler's.
CResizeGump::~CResizeGump()
{
}
HOOK_DTOR(0x0047b660, CResizeGump);

// 0x0047b670  size 0x3a  RET 8 (arguments not read)
void __thiscall CResizeGump::MouseUp(int a, int b)
{
    (void)a;
    (void)b;
    g_dragGump = nullptr;
    PORT_VSLOT2(target, 0x38, void, int, int)(curW, curH);
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}
HOOK_VMETHOD(0x0047b670, CResizeGump, MouseUp);

// 0x0047b6b0  size 0x1cd  RET 8 (width, height)
void __thiscall CResizeGump::MouseMove(int w, int h)
{
    prevW = curW;
    prevH = curH;
    curW = w;
    curH = h;
    if (w < minW)
        curW = minW;
    if (h < minH)
        curH = minH;
    if (curW > maxW)
        curW = maxW;
    if (curH > maxH)
        curH = maxH;
    if (curW % 2 == 1)
        curW = curW + 1;
    for (CGump *child = lastChild; child != nullptr; child = child->prev) {
        memset(b6_frame_bitmap(child, 0)->px, 0, static_cast<size_t>(child->height * child->width) * 2);
        switch (static_cast<uint32_t>(static_cast<CDumbGump *>(child)->tileArt - 1)) {
        case 0:
            RS_FillRect(0, 0, curW - 1, 1, RS_White, b6_frame_bitmap(child, 0));   // 0x00432c70, color 0x00436870
            break;
        case 1:
            child->x = curW - 2;
            RS_FillRect(0, 0, 1, curH - 1, RS_White, b6_frame_bitmap(child, 0));   // 0x00432c70, color 0x00436870
            break;
        case 2:
            child->y = curH - 2;
            RS_FillRect(0, 0, curW - 1, 1, RS_White, b6_frame_bitmap(child, 0));   // 0x00432c70, color 0x00436870
            break;
        case 3:
            RS_FillRect(0, 0, 1, curH - 1, RS_White, b6_frame_bitmap(child, 0));   // 0x00432c70, color 0x00436870
            break;
        default:
            break;
        }
    }
}
HOOK_VMETHOD(0x0047b6b0, CResizeGump, MouseMove);

// 0x0047b890  size 0x24  vtable slot 0x04, RET 4
void __thiscall CResizeGump::PreDraw(int *clip)
{
    for (CGump *child = lastChild; child != nullptr; child = child->prev)
        PORT_VSLOT1(child, 0x04, void, int *)(clip);
}
HOOK_VMETHOD(0x0047b890, CResizeGump, PreDraw);

// 0x0047b8c0  size 0x24  vtable slot 0x08, RET 4
void __thiscall CResizeGump::Draw(int *clip)
{
    for (CGump *child = lastChild; child != nullptr; child = child->prev)
        PORT_VSLOT1(child, 0x08, void, int *)(clip);
}
HOOK_VMETHOD(0x0047b8c0, CResizeGump, Draw);
