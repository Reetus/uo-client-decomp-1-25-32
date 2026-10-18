// CHueGump, second pass: the centred constructor, destructor, redraw and mouse handling.
// 0x00441bd0 .. 0x004428d0 (the hue picker gump, vtable 0x0049f4c8). The SEH frames of the original are omitted.
#include "hue_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "font_manager.h"
#include "globals.h"
#include "hue_table.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "packet_builders_b25_ports.h"
#include "client_socket.h"
#include "art_row_blits_ports.h"
#include "art_record_ops_ports.h"
#include "mouse_manager.h"
#include "class_0049c690.h"
#include "scroll_thumb_gump.h"
#include "gump_base_ops_ports.h"
#include "packet_builders_batch24_ports.h"
#include "art_blit_ports.h"
#include "art_row_blits_hue_ports.h"
#include "char_flow_gumps.h"
#include "small_methods_u1_ports.h"
#include "vtables.h"

#define HG_ConfigGump GLOBAL_AS(void *, g_clientCfgGump)       // client config gump
#define HG_DragGump GLOBAL_AS(CGump *, g_dragGump)
#define HG_Hues GLOBAL_PTR(HueEntry, g_hueTable_storage)
#define HG_ArtCache GLOBAL_PTR(void, g_artCache_storage)
#define HG_MouseMgr GLOBAL_PTR(void, g_mouse_storage)
#define HG_NetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)

// 0x00465360: dye window reply 0x95 builder on the packet builder: (buffer, serial, word, word), RET 0x10.
#define HG_BuildDyeReply(o, a1, a2, a3, a4) PORT_THISCALL4(&Pkt_Build_0x95_DyeWindowReply, void, (o), uint8_t *, int, uint32_t, uint32_t)(a1, a2, a3, a4)
// 0x00478610: send a packet on the connection object, RET 4.
#define HG_SendPacket(self, a1) (reinterpret_cast<CClientSocket *>(self)->SendPacket((const uint8_t *)(a1)))
// 0x004767c0: set the RLE walker's destination (ECX = buffer, EDX = row width in pixels).
#define HG_SetBlitTarget(a1, a2) (SetBlitSurface((uint16_t *)(a1), a2))
// 0x004767d0: RLE art walk (ECX = row blitter, EDX = art record, stack = clip, x, y), RET 0xc.
#define HG_ArtWalk PORT_FUNC5(&RleArtWalk, void, __fastcall, void *, uint8_t *, int *, int, int)
// 0x00476130: art record lookup on the object at 0x0068d2d8 (item art index), RET 4.
#define HG_ArtLookup(o, a1) PORT_THISCALL1(&ArtRecord_Get, uint8_t *, (o), int)(a1)
// 0x00446840: set the mouse cursor on the mouse manager, RET 4.
#define HG_SetCursor(self, a1) (reinterpret_cast<CMouseManager *>(self)->SetMouseCursor(a1))
// 0x00406720: empty procedure.
#define HG_Nop(self) (reinterpret_cast<CWorldObj *>(self)->CWorldObj::ToVoid())

// Row blitters passed as the "art object" of the art drawers.
#define HG_BlitPlain PTR_AT(void, PORT_ADDR(&RowBlitCopyNonZero))
#define HG_BlitHued PTR_AT(void, PORT_ADDR(&RowBlitLookupNonZero))
#define HG_BlitItem PTR_AT(void, PORT_ADDR(&RowBlitLookupAll))
#define HG_BlitItemTrans PTR_AT(void, PORT_ADDR(&RowBlitKeyedLookupAll))

HOOK_VTABLE(0x0049f4c8, CHueGump);
HOOK_SDD(0x00441bd0, CHueGump);

// 0x00441c00  size 0x341
// Constructor of the hue gump centred on the 640x480 screen. The frame data is one frame whose bitmap is the
// width x height 16-bit area of art 0x906 ("framedata for CDumbGump" / "dumb frame0").
CHueGump::CHueGump(int serial, uint16_t arg0, uint16_t arg1)
{
    name = const_cast<char *>("hue gump");
    dirty = 0;
    unk_50 = 0;
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    gfx::ArtSize(0x906, &width, &height);   // 0x00433e70
    x = (0x280 - width) / 2;
    y = (0x1e0 - height) / 2;
    unk_bc = serial;
    RAWF(uint16_t, this, 0xc2) = arg1;
    unk_c8 = 0x22;
    unk_cc = 0x22;
    unk_d0 = 0xc1;
    unk_d4 = 0x71;
    hue = 3;
    page = 1;
    mode = 0;
    isFirst = 0;
    unk_ac = 1;
    unk_b0 = 1;
    unk_b8 = 0;
    unk_b4 = 0;
    RAWF(uint16_t, this, 0xc0) = arg0;
    title = nullptr;
    frameIndex = 0;
    frameCount = 1;

    int32_t *fd = FrameData_Alloc(g_frameDataAllocator, 8, "framedata for CDumbGump");   // 0x004462f0
    frameData = fd;
    memset(fd, 0, static_cast<size_t>(frameCount) * 8);
    void *bmp = gfx::AllocTagged(width * height * 2 + 4, "dumb frame0");
    frameData[1] = static_cast<int32_t>(reinterpret_cast<uintptr_t>(bmp));
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(width * height) * 2);
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);
    uint16_t *hdr = gfx::bitmapOf(this);
    gfx::DrawArt(HG_BlitPlain, 0x906, 0, 0, hdr + 2, hdr[0], hdr[1]);   // 0x00433ed0
    Redraw();   // 0x00442080

    CScrollThumbGump *th = new CScrollThumbGump(0x26, 0x8e, 0, 0xba, reinterpret_cast<int>(this), 0x845);   // 0x00413740
    thumb = th;
    th->artAlt = 0x846;
    thumb->LinkInto(this, 1);   // 0x0043bcc0
    thumb->CScrollThumbGump::SetPosition(page, 4);   // CScrollThumbGump::SetPosition

    CButtonGump *btn = new CButtonGump(0x1a, 0x8e, 0x27, 0x9e, this, 2, 0, PORT_ADDR(&CHueGump_00410d30::StepDown), 0);   // 0x00413110
    btnDown = btn;
    btn->LinkInto(this, 1);

    btn = new CButtonGump(0xb8, 0x8e, 0xc5, 0x9e, this, 2, 0, PORT_ADDR(&CHueGump_00410d30::StepUp), 0);
    btnUp = btn;
    btn->LinkInto(this, 1);

    btn = new CButtonGump(0xd0, 0x8a, reinterpret_cast<int>(this), 0, 0, PORT_ADDR(&DeleteGumpCallback), 0x907, 0x908, 0);   // 0x00412fb0
    btn->artHover = 0x909;
    btn->LinkInto(this, 1);
}
HOOK_CTOR(0x00441c00, CHueGump, (int, uint16_t, uint16_t));

// 0x00441f50  size 0x12f
// Destructor body. When +0xb0 is set the chosen hue is sent back (packet 0x95); +0xb4 / +0xb8 store it into the
// client config gump (+0xc0 / +0xc2) unless the gump is closing through the right button.
CHueGump::~CHueGump()
{
    if (unk_b0 != 0) {
        uint8_t pkt[0x10];
        HG_BuildDyeReply(HG_NetBuilder, pkt, unk_bc, RAWF(uint16_t, this, 0xc0), hue);   // 0x00465360
        HG_SendPacket(g_pktSender, pkt);   // 0x00478610
    }
    if (unk_b4 != 0 && HG_Closing == 0) {
        RAWF(uint16_t, HG_ConfigGump, 0xc0) = hue;
        HG_Nop(HG_ConfigGump);   // 0x00406720
    } else if (unk_b8 != 0 && HG_Closing == 0) {
        RAWF(uint16_t, HG_ConfigGump, 0xc2) = hue;
        HG_Nop(HG_ConfigGump);
    }
    if (title != nullptr)
        operator delete(title);   // 0x0048ad60
    if (gfx::bitmapOf(this) != nullptr) {
        gfx::FreeTagged(gfx::bitmapOf(this), "hue frame0 pBMP");   // 0x00446310
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "hue framedata");
        frameData = nullptr;
    }
    HG_Closing = 0;
}
HOOK_DTOR(0x00441f50, CHueGump);

// 0x00442080  size 0x57d
// Redraw: the grid of 8x8 hue cells (20 columns, row r / column c is hue 5 * (20 r + c) + page + 2), the white
// selection mark, then the preview of the chosen hue drawn by mode: gump art (+0xa4), the title text (+0xa8) or the
// item art of graphic +0xc2 (+0xac).
void __thiscall CHueGump::Redraw()
{
    uint32_t idx = static_cast<uint16_t>(page + 1);
    for (int row = unk_cc; row <= unk_d4; row += 8) {
        for (int col = unk_c8; col <= unk_d0; col += 8) {
            uint16_t color = HG_Hues[idx & 0xffff].colors[0x10];
            if (g_is565 != 0) {
                uint16_t c565 = static_cast<uint16_t>((color & 0x1f) | ((color & 0xffe0) << 1));
                for (int py = row; py < row + 8; py++)
                    for (int px = col; px < col + 8; px++)
                        gfx::bitmapOf(this)[2 + py * width + px] = c565;
            } else {
                for (int py = row; py < row + 8; py++)
                    for (int px = col; px < col + 8; px++)
                        gfx::bitmapOf(this)[2 + py * width + px] = color;
            }
            idx += 5;
        }
    }

    // selection mark
    int sel = hue - 2;
    int selX = ((sel % 100) / 5) * 8 + 3 + unk_c8;
    int selY = (sel / 100) * 8 + 3 + unk_cc;
    for (int px = selX; px < selX + 3; px++)
        gfx::bitmapOf(this)[2 + (selY + 1) * width + px] = 0x7fff;
    for (int py = selY; py < selY + 3; py++)
        gfx::bitmapOf(this)[2 + py * width + selX + 1] = 0x7fff;

    int rect[4] = {0xc8, 0x32, 0xfe, 0x64};
    if (isFirst != 0) {
        uint16_t *hdr = gfx::bitmapOf(this);
        if (hue != 0) {
            g_hueOverride = hue - 1;
            gfx::DrawArt(HG_BlitHued, unk_50, unk_d0 - 0x28, unk_cc - 0x46, hdr + 2, hdr[0], hdr[1]);   // 0x00433ed0
            return;
        }
        gfx::DrawArt(HG_BlitPlain, unk_50, unk_d0 - 0x28, unk_cc - 0x46, hdr + 2, hdr[0], hdr[1]);
        return;
    }
    if (mode != 0) {
        int textW = 0;
        int textH = 0;
        int textBase = 0;
        g_fontMgr.MeasureTextWrapped(3, title, 0x36, &textW, &textH, &textBase, 0);   // 0x004328e0
        int ty = ((rect[3] - textH) - rect[1]) / 2 + rect[1];
        gfx::TextDrawWrapped(rect[0], ty, 3, hue, title, rect[2] - rect[0], gfx::bitmapOf(this) + 2, width, rect, 0, 0);   // 0x00432260
        return;
    }
    if (unk_ac == 0)
        return;

    if (g_is565 != 0) {
        uint8_t *art = HG_ArtLookup(HG_ArtCache, RAWF(uint16_t, this, 0xc2) + 0x4000);   // 0x00476130
        if (art == nullptr) {
            art = HG_ArtLookup(HG_ArtCache, 0x4000);
            if (art == nullptr)
                return;
        }
        size_t bytes = static_cast<size_t>(height * width) * 2;
        uint16_t *tmp = static_cast<uint16_t *>(::operator new(bytes));   // 0x0048ad70
        memset(tmp, 0, bytes);
        HG_SetBlitTarget(tmp, width);   // 0x004767c0
        int artW = *reinterpret_cast<uint16_t *>(art + 4);
        int artH = *reinterpret_cast<uint16_t *>(art + 6);
        int sx = static_cast<uint16_t>(static_cast<int16_t>((width - artW - unk_d0) / 2) + static_cast<int16_t>(unk_d0));
        int sy = static_cast<uint16_t>((height / 2) - (artH / 2));
        g_hueOverride = hue - 1;
        HG_ArtWalk(HG_BlitItem, art, rect, sx, sy);   // 0x004767d0
        uint16_t *dst = gfx::bitmapOf(this) + 2;
        uint16_t *src = tmp;
        for (int py = 0; py < height; py++) {
            for (int px = 0; px < width; px++) {
                uint16_t p = *src;
                if (p != 0)
                    *dst = static_cast<uint16_t>(((p & 0xffe0) << 1) | (p & 0x1f));
                src++;
                dst++;
            }
        }
        operator delete(tmp);
        return;
    }

    HG_SetBlitTarget(gfx::bitmapOf(this) + 2, width);
    uint8_t *art = HG_ArtLookup(HG_ArtCache, RAWF(uint16_t, this, 0xc2) + 0x4000);
    if (art == nullptr) {
        art = HG_ArtLookup(HG_ArtCache, 0x4000);
        if (art == nullptr)
            return;
    }
    int artW = *reinterpret_cast<uint16_t *>(art + 4);
    int artH = *reinterpret_cast<uint16_t *>(art + 6);
    int sx = static_cast<uint16_t>(static_cast<int16_t>((width - artW - unk_d0) / 2) + static_cast<int16_t>(unk_d0) - 10);
    int sy = static_cast<uint16_t>((height / 2) - (artH / 2));
    g_hueOverride = hue - 1;
    if ((*reinterpret_cast<uint32_t *>(g_tileData + RAWF(uint16_t, this, 0xc2) * 0x28) & 0x40000) != 0)
        HG_ArtWalk(HG_BlitItemTrans, art, rect, sx, sy);
    else
        HG_ArtWalk(HG_BlitItem, art, rect, sx, sy);
}
HOOK_METHOD(0x00442080, CHueGump, Redraw);

// 0x00442600  size 0xbd
// Vtable slot 0x08: blit the frame bitmap clipped to the screen, then draw the children.
void __thiscall CHueGump::Draw(int *clip)
{
    if (visible == 0)
        return;
    int ox = 0;
    int oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);   // 0x0043b4e0
    uint16_t *bmp = gfx::bitmapOf(this);
    int rect[4];
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = ox + bmp[0];
    rect[3] = oy + bmp[1];
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, const int *, int *)(clip, rect) != 0)
        GFX_BlitFrame(HG_BlitPlain, bmp + 2, const_cast<int *>(clip), bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);   // 0x004342e0
    for (CGump *child = firstChild; child != nullptr; child = child->next)
        PORT_VSLOT1(child, kGumpSlot_Draw, void, const int *)(clip);
}
HOOK_VMETHOD(0x00442600, CHueGump, Draw);

// 0x004426c0  size 0x2f
// Vtable slot 0x1c (right button up; arguments not read): unless +0xb0 is set, close the gump.
void __thiscall CHueGump::RButtonUp(int a, int b)
{
    (void)a;
    (void)b;
    HG_DragGump = nullptr;
    if (unk_b0 == 0) {
        HG_Closing = 1;
        if (this != nullptr)
            PORT_VSLOT1(this, kGumpSlot_DeletingDtor, void, int)(1);
    }
}
HOOK_VMETHOD(0x004426c0, CHueGump, RButtonUp);

// 0x004426f0  size 0xb3
// Vtable slot 0x10 (button down): raise, remember the position, pick the hue of the grid cell under the mouse.
void __thiscall CHueGump::MouseDown(int mx, int my)
{
    BringToFront(0);   // 0x0043ba70
    unk_a0 = 1;
    unk_6c = mx;
    unk_70 = my;
    if (mx >= unk_c8 && mx <= unk_d0 && my >= unk_cc && my <= unk_d4) {
        int col = (mx - unk_c8) / 8;
        int rowIdx = (my - unk_cc) / 8;
        hue = static_cast<uint16_t>((col + rowIdx * 20) * 5 + page + 2);
        Redraw();   // 0x00442080
        HG_GridHit = 1;
        return;
    }
    HG_GridHit = 0;
    HG_SetCursor(HG_MouseMgr, 0x11);   // 0x00446840
}
HOOK_VMETHOD(0x004426f0, CHueGump, MouseDown);

// 0x004427b0  size 0x14
// Vtable slot 0x14 (button up).
void __thiscall CHueGump::MouseUp(int mx, int my)
{
    (void)mx;
    (void)my;
    unk_a0 = 0;
    HG_DragGump = nullptr;
}
HOOK_VMETHOD(0x004427b0, CHueGump, MouseUp);

// 0x004427d0  size 0xbd
// Vtable slot 0x28 (mouse move). While the button is held: drag the gump when the press was outside the grid,
// otherwise pick the hue of the cell under the mouse.
void __thiscall CHueGump::MouseMove(int mx, int my)
{
    if (unk_a0 == 0)
        return;
    if (HG_GridHit == 0) {
        HG_SetCursor(HG_MouseMgr, 0x11);
        Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);   // 0x0043b140
        return;
    }
    if (mx >= unk_c8 && mx <= unk_d0 && my >= unk_cc && my <= unk_d4) {
        int col = (mx - unk_c8) / 8;
        int rowIdx = (my - unk_cc) / 8;
        hue = static_cast<uint16_t>((col + rowIdx * 20) * 5 + page + 2);
        Redraw();
    }
}
HOOK_VMETHOD(0x004427d0, CHueGump, MouseMove);

// 0x00442890  size 0x3f
// Vtable slot 0x30: the page slider (+0x94) moved to value / range; the page is value * 4 / range.
void __thiscall CHueGump::OnScroll(int value, int range, int sourceArg)
{
    CGump *source = reinterpret_cast<CGump *>(sourceArg);
    if (reinterpret_cast<CGump *>(source) != reinterpret_cast<CGump *>(thumb))
        return;
    hue = static_cast<uint16_t>(hue - page);
    uint16_t newPage = static_cast<uint16_t>((value << 2) / range);
    hue = static_cast<uint16_t>(hue + newPage);
    page = newPage;
    Redraw();
}
HOOK_VMETHOD(0x00442890, CHueGump, OnScroll);

// 0x004428d0  size 0x37
// Send the help request packet 0x9b with an empty text.
void __cdecl HelpRequest_Send()
{
    uint8_t pkt[0x104];
    PORT_THISCALL3(&Build_Packet_0x9B, void, HG_NetBuilder, uint8_t *, int, const char *)(pkt, 0, g_emptyString);   // Build_Packet_0x9B
    HG_SendPacket(g_pktSender, pkt);   // 0x00478610
}
HOOK_FUNC(0x004428d0, HelpRequest_Send);

// Own copies of bodies shared with other classes in the original (the linker folds them).
int __thiscall CHueGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

