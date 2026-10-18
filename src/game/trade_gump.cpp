// CTradeGump second pass (batch 30). The SEH frames (try levels around operator new) are omitted.
#include "trade_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "batch06_helpers.h"
#include "font_manager.h"
#include "globals.h"
#include "object_manager.h"
#include "crt.h"
#include "packet_cursor.h"
#include "raw_field.h"
#include "packet_builders_b25_ports.h"
#include "gump_base_ops_ports.h"
#include "vtables.h"

#define TR_FontMgr GLOBAL_AS(CFontManager, g_fontMgr_storage)
#define TR_Self static_cast<int>(reinterpret_cast<uintptr_t>(this))
// 0x00464900 on the packet builder object: (buf, action, id1, id2, id3), RET 0x14.
#define TR_Build6F(o, a1, a2, a3, a4, a5) PORT_THISCALL5(&Pkt_Build_0x6F_SecureTrade, void, (o), uint8_t *, int, uint32_t, int, int)(a1, a2, a3, a4, a5)
#define TR_PacketBuilder GLOBAL_PTR(void, g_packetBuilder_storage)
// 0x0043b540: thiscall on the gump manager, (clip, rect), RET 8.
#define TR_RectIntersects(o, a1, a2) PORT_THISCALL2(&GumpRectIntersects, int, (o), int *, int *)(a1, a2)
// 0x0043b590: ECX = clip, EDX = &rect, stack = &out, RET 4.
#define TR_RectClip(a1, a2, a3) (GumpRectClip((const int *)(a1), (const int *)(a2), a3))
// The string measured to place the names (at 0x004a6214).

// 0x0047b8f0  size 0x444
CTradeGump::CTradeGump(CObj *partner, uint32_t serial1, uint32_t serial2, const char *partnerName_)
{
    int artW;
    int artH;
    int tw;
    int th;
    int tb;
    int clip[4];   // ebp-0x3c: left, top, right, bottom
    void *mem;
    CDynamicObj *obj;
    CContainerGump *cg;
    CToggleControl *tog;

    name = const_cast<char *>("trade gump");
    dirty = 0;
    unk_94 = 0;
    owner = partner;
    if (partner != nullptr) {
        if (PORT_VSLOT0(partner, 0x24, int)() != 0)
            RAWF(CTradeGump *, owner, 0x1d4) = this;
    }
    strcpy(partnerName, partnerName_);
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    unk_50 = 0x866;
    gfx::ArtSize(0x866, &width, &height);   // 0x00433e70
    gfx::ArtSize(0x52, &artW, &artH);   // 0x00433e70
    frameIndex = 0;
    frameCount = 1;
    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CTradeGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    b6_set_frame_bitmap(this, 0, gfx::AllocTagged(width * height * 2 + 4, "container frame"));
    memset(b6_frame_bitmap(this, 0)->px, 0, static_cast<size_t>(width * height) * 2);
    b6_frame_bitmap(this, 0)->w = static_cast<uint16_t>(width);
    b6_frame_bitmap(this, 0)->h = static_cast<uint16_t>(height);
    {
        B6Bitmap *bmp = b6_frame_bitmap(this, 0);
        gfx::DrawArt(gfx::artPlain(), unk_50, 0, 0, bmp->px, bmp->w, bmp->h);   // 0x00433ed0
    }
    // (the original also zeroes three words of a local here; nothing reads them)

    obj = g_objMgr.FindBySerial(serial1);   // 0x00456d70
    if (obj != nullptr && PORT_VSLOT0(obj, 0x30, int)() != 0) {
        cg = new CContainerGump(0x2e, 0x58, this, obj, 1, artW, artH, 0x52);   // 0x00425e30
        container1 = cg;
        container1->unk_a0 = this;
        RAWF(CContainerGump *, obj, 0xac) = container1;
        gfx::AddChild(container1, this);   // 0x0043bcc0
    }
    obj = g_objMgr.FindBySerial(serial2);   // 0x00456d70
    if (obj != nullptr && PORT_VSLOT0(obj, 0x30, int)() != 0) {
        cg = new CContainerGump(0xc0, 0x47, this, obj, 1, artW, artH, 0x52);   // 0x00425e30
        container2 = cg;
        container2->unk_a0 = this;
        RAWF(CContainerGump *, obj, 0xac) = container2;
        gfx::AddChild(container2, this);   // 0x0043bcc0
    }

    tog = new CToggleControl(0x34, 0x1d, TR_Self, 0x867, 0x869, 1, 0x386, g_emptyString, 0, 0x867, 0x869);   // 0x00416120
    acceptOwn = tog;
    acceptOwn->artHover = 0x868;
    acceptOwn->artHoverChecked = 0x86a;
    gfx::AddChild(acceptOwn, this);   // 0x0043bcc0

    tog = new CToggleControl(0x10a, 0xa0, TR_Self, 0x867, 0x869, 1, 0x386, g_emptyString, 0, 0x867, 0x869);   // 0x00416120
    acceptOther = tog;
    acceptOther->disabled = 1;
    acceptOther->Redraw();   // 0x004163f0
    gfx::AddChild(acceptOther, this);   // 0x0043bcc0

    TR_FontMgr.MeasureText(1, g_refText, &tw, &th, &tb);   // 0x004327b0
    clip[3] = height;
    clip[2] = width;
    clip[0] = 0;
    clip[1] = 0;
    TR_FontMgr.DrawTextPlain(0x54, tb + 0x28, 1, 0x386, g_playerName, b6_frame_bitmap(this, 0)->px, width, clip);   // 0x00432130
    TR_FontMgr.MeasureText(1, partnerName, &tw, &th, &tb);   // 0x004327b0
    TR_FontMgr.DrawTextPlain(0x104 - tw, tb + 0xaa, 1, 0x386, partnerName, b6_frame_bitmap(this, 0)->px, width, clip);   // 0x00432130
}
HOOK_CTOR(0x0047b8f0, CTradeGump, (CObj *, uint32_t, uint32_t, const char *));

HOOK_VTABLE(0x004a0b88, CTradeGump);
HOOK_SDD(0x0047bd40, CTradeGump);

// 0x0047bd70  size 0xf8  destructor body: deletes the two container gumps (after releasing their container
// objects through vtable slot 0x04), clears the partner's back pointer, frees frame 0 and the frame table.
CTradeGump::~CTradeGump()
{
    if (container1 != nullptr) {
        CObj *o = container1->owner;
        if (o != nullptr)
            PORT_VSLOT0(o, 0x04, void)();
        if (container1 != nullptr)
            PORT_VSLOT1(container1, 0x00, void, int)(1);
        container1 = nullptr;
    }
    if (container2 != nullptr) {
        CObj *o = container2->owner;
        if (o != nullptr)
            PORT_VSLOT0(o, 0x04, void)();
        if (container2 != nullptr)
            PORT_VSLOT1(container2, 0x00, void, int)(1);
        container2 = nullptr;
    }
    if (owner != nullptr) {
        if (PORT_VSLOT0(owner, 0x24, int)() != 0)
            RAWF(void *, owner, 0x1d4) = nullptr;
    }
    if (frameData[1] != 0) {
        gfx::FreeTagged(b6_ptr(frameData[1]), "Trade frame1 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "Trade framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x0047bd70, CTradeGump);

// 0x0047be70  size 0x5e  RET 8 (arguments not read)
void __thiscall CTradeGump::RButtonUp(int a, int b)
{
    (void)a;
    (void)b;
    uint8_t packet[0x30];

    g_dragGump = nullptr;
    if (container1 != nullptr && container1->owner != nullptr) {
        uint32_t serial = RAWF(uint32_t, container1->owner, 0x78);
        TR_Build6F(TR_PacketBuilder, packet, 1, serial, 0, 0);   // 0x00464900
        pkt::Send(packet);   // 0x00478610
        return;
    }
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}
HOOK_VMETHOD(0x0047be70, CTradeGump, RButtonUp);

// 0x0047bed0  size 0xba  RET 4 (control)
void __thiscall CTradeGump::OnChildChanged(int child)
{
    CGump *control = reinterpret_cast<CGump *>(child);
    uint8_t packet[0x30];

    if (container1 != nullptr && acceptOwn != nullptr && container1->owner != nullptr && g_pktSender != nullptr &&
        control == acceptOwn) {
        if (acceptOwn->checked != 0) {
            TR_Build6F(TR_PacketBuilder, packet, 2, RAWF(uint32_t, container1->owner, 0x78), 1, 0);   // 0x00464900
            acceptOwn->checked = 0;
        } else {
            TR_Build6F(TR_PacketBuilder, packet, 2, RAWF(uint32_t, container1->owner, 0x78), 0, 0);   // 0x00464900
            acceptOwn->checked = 1;
        }
        acceptOwn->Redraw();   // 0x004163f0
        pkt::Send(packet);   // 0x00478610
    }
}
HOOK_VMETHOD(0x0047bed0, CTradeGump, OnChildChanged);

// 0x0047bf90  size 0xcf  vtable slot 0x08, RET 4
void __thiscall CTradeGump::Draw(int *clip)
{
    int sx = 0;
    int sy = 0;
    int rect[4];
    int clipped[4];

    LocalToScreen(&sx, &sy);   // 0x0043b4e0
    rect[0] = sx;
    rect[1] = sy;
    rect[2] = sx + b6_frame_bitmap(this, 0)->w;
    rect[3] = sy + b6_frame_bitmap(this, 0)->h;
    if (TR_RectIntersects(g_gumpManager, clip, rect) != 0) {   // 0x0043b540
        TR_RectClip(clip, rect, clipped);   // 0x0043b590
        GFX_BlitFrame(gfx::artPlain(), b6_frame_bitmap(this, 0)->px, clipped, b6_frame_bitmap(this, 0)->w, b6_frame_bitmap(this, 0)->h, sx, sy, g_backBuffer, 0x280, 0x1e0);   // 0x004342e0
        for (CGump *c = firstChild; c != nullptr; c = c->next)
            PORT_VSLOT1(c, 0x08, void, int *)(clipped);
    }
}
HOOK_VMETHOD(0x0047bf90, CTradeGump, Draw);

// Own copies of bodies shared with other classes in the original (the linker folds them).
int __thiscall CTradeGump::HitTest(int px, int py)
{
    if (px < 0 || py < 0 || px >= width || py >= height)
        return 0;
    const uint16_t *pixels = reinterpret_cast<const uint16_t *>(static_cast<uintptr_t>(frameData[1]));
    return pixels[py * width + px] != 0;
}

void __thiscall CTradeGump::MouseDown(int mx, int my)
{
    BringToFront(0);   // 0x0043ba70
    RAWF(int32_t, this, 0x94) = 1;
    unk_6c = mx;
    unk_70 = my;
}

void __thiscall CTradeGump::MouseUp(int, int)
{
    RAWF(int32_t, this, 0x94) = 0;
    g_dragGump = nullptr;
}

void __thiscall CTradeGump::MouseMove(int mx, int my)
{
    if (RAWF(int32_t, this, 0x94) != 0)
        Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);   // 0x0043b140
}

