#include "char_stat_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "button_gump.h"
#include "init_skill_gump.h"
#include "crt.h"
#include "input_control_gump.h"
#include "dumb_gump.h"
#include "app_support_ports.h"
#include "char_creation_pages.h"
#include "vtables.h"
#include "globals.h"

// The SEH frame of the original (try-level bookkeeping around each operator new) is omitted; every
// "operator new returned NULL" branch of the original is kept (the object pointer becomes 0).

#define g_gumpManager GLOBAL_PTR(void, g_gumpMgr_storage)               // this for 0x0043a720 / 0x0043a820
#define g_focusedInputCS GLOBAL_AS(CGump *, g_focusedInput)             // focused input control


// 0x0043d0a0: art display child gump (operator new(0xb0)), RET 0x20, (x, y, parent, w, h, a6, a7, a8). Not ported.
#define Orig_NewArtChild(self, a1, a2, a3, a4, a5, a6, a7, a8) ((CGump *)reinterpret_cast<CDumbGump *>(self)->Ctor(a1, a2, (int)(a3), a4, a5, a6, (const void *)(a7), a8))

// 0x0044b650: rand() % modulus. ECX = modulus, plain RET.
#define Orig_RandMod(a1) (App_RandomMod(a1))

// 0x0041a040  size 0x9c0
// Constructor of the character creation stat page: frame0, two name boxes, profession box, three stat boxes with
// thumbs and mirror boxes, three buttons, an art child; a random profession is then applied.
CCharStatGump::CCharStatGump()
{
    name = const_cast<char *>("CharStat gump");
    dirty = 0;
    unk_50 = 0x4b0;   // art id of the background
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;

    gfx::ArtSize(unk_50, &width, &height);
    x = 0x78;
    y = 0x66;
    frameIndex = 0;
    frameCount = 1;

    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CCharStatGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    gfx::setBitmap(this, gfx::AllocTagged(width * height * 2 + 4, "charstat frame0"));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(width * height * 2));
    gfx::DrawArt(gfx::artPlain(), unk_50, 0, 0, gfx::bitmapOf(this) + 2, width, height);

    // name box +0x98
    edit_98 = new CInputControlGump(0xf1 - x, 0x83 - y, this, 0x4b1, 0x4b1, 0, 0x10, 1, 0x386, 1, 0, 0);
    edit_98->artDe = 0x4b2;
    edit_98->artFocused = 0x4b4;
    edit_98->artEmpty = 0x4b3;
    edit_98->marginX = 7;
    edit_98->marginY = 2;
    edit_98->Redraw();
    gfx::AddChild(edit_98, this);

    // second name box +0x9c (password style)
    edit_9c = new CInputControlGump(0xf2 - x, 0x97 - y, this, 0x4b5, 0x4b5, 0, 0x10, 1, 0x386, 1, 0, 0);
    edit_9c->artDe = 0x4b6;
    edit_9c->artFocused = 0x4b8;
    edit_9c->artEmpty = 0x4b7;
    edit_9c->password = 1;
    edit_9c->marginX = 7;
    edit_9c->marginY = 0;
    edit_9c->Redraw();
    gfx::AddChild(edit_9c, this);

    // profession box +0xc4
    profession_c4 = new CInputControlGump(0, 0, this, 0, 0, 2, 8, 1, 0x386, 0, 1, 0);

    // mirror boxes +0xb8 / +0xbc / +0xc0
    mirror_b8 = new CInputControlGump(0x156 - x, 0xd6 - y, this, static_cast<uint16_t>(0x1b), static_cast<uint16_t>(0xc), 1, 0x3e7, 1, 0x386, 1, 1, 0);
    mirror_b8->marginY = 0;
    mirror_b8->dropDown = 1;
    gfx::AddChild(mirror_b8, this);

    mirror_bc = new CInputControlGump(0x156 - x, 0xf4 - y, this, static_cast<uint16_t>(0x1b), static_cast<uint16_t>(0xc), 1, 0x3e7, 1, 0x386, 1, 1, 0);
    mirror_bc->marginY = 0;
    mirror_bc->dropDown = 1;
    gfx::AddChild(mirror_bc, this);

    mirror_c0 = new CInputControlGump(0x156 - x, 0x113 - y, this, static_cast<uint16_t>(0x1b), static_cast<uint16_t>(0xc), 1, 0x3e7, 1, 0x386, 1, 1, 0);
    mirror_c0->marginY = 0;
    mirror_c0->dropDown = 1;
    gfx::AddChild(mirror_c0, this);

    // stat boxes +0xa0 / +0xa4 / +0xa8
    edit_a0 = new CInputControlGump(0x112 - x, 0xd6 - y, this, static_cast<uint16_t>(0x1b), static_cast<uint16_t>(0xc), 1, 0x23, 1, 0x386, 1, 1, 0);
    edit_a0->value = 0x16;
    edit_a0->marginY = 0;
    edit_a0->dropDown = 1;
    edit_a0->Redraw();
    gfx::AddChild(edit_a0, this);

    edit_a4 = new CInputControlGump(0x111 - x, 0xf4 - y, this, static_cast<uint16_t>(0x1b), static_cast<uint16_t>(0xc), 1, 0x23, 1, 0x386, 1, 1, 0);
    edit_a4->value = 0x15;
    edit_a4->marginY = 0;
    edit_a4->dropDown = 1;
    edit_a4->Redraw();
    gfx::AddChild(edit_a4, this);

    edit_a8 = new CInputControlGump(0x111 - x, 0x113 - y, this, static_cast<uint16_t>(0x1b), static_cast<uint16_t>(0xc), 1, 0x23, 1, 0x386, 1, 1, 0);
    edit_a8->value = 0x16;
    edit_a8->marginY = 0;
    edit_a8->dropDown = 1;
    edit_a8->Redraw();
    gfx::AddChild(edit_a8, this);

    // scroll thumbs +0xac / +0xb0 / +0xb4
    thumb_ac = new CScrollThumbGump(0xb3 - x, 0xdc - y, 0, 0x10d - x, (int)(this), 0x4b9);
    thumb_ac->artAlt = 0x4ba;
    gfx::ScrollThumbSet(thumb_ac, edit_a0->value - 10, 0x23);
    gfx::AddChild(thumb_ac, this);

    thumb_b0 = new CScrollThumbGump(0xb3 - x, 0xfa - y, 0, 0x10d - x, (int)(this), 0x4b9);
    thumb_b0->artAlt = 0x4ba;
    gfx::ScrollThumbSet(thumb_b0, edit_a4->value - 10, 0x23);
    gfx::AddChild(thumb_b0, this);

    thumb_b4 = new CScrollThumbGump(0xb3 - x, 0x118 - y, 0, 0x10d - x, (int)(this), 0x4b9);
    thumb_b4->artAlt = 0x4ba;
    gfx::ScrollThumbSet(thumb_b4, edit_a8->value - 10, 0x23);
    gfx::AddChild(thumb_b4, this);

    // buttons: (x, y, parent, callback, up art, down art); artHover is set after construction
    {
        CButtonGump *b = new CButtonGump(0x11b - x, 0xb1 - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CGumpHandlers04::OpenProfessionPicker), 0x4bc, 0x4bd, 0);
        b->artHover = 0x4be;
        gfx::AddChild(b, this);
    }
    {
        CButtonGump *b = new CButtonGump(0xb6 - x, 0x136 - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CCharStatGump::CancelCreation), 0x4c2, 0x4c3, 0);
        b->artHover = 0x4c4;
        gfx::AddChild(b, this);
    }
    {
        CButtonGump *b = new CButtonGump(0x196 - x, 0x136 - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CCharStatGump::NextFromName), 0x4bf, 0x4c0, 0);
        b->artHover = 0x4c1;
        gfx::AddChild(b, this);
    }

    mirror_b8->value = edit_a0->value;
    mirror_b8->Redraw();
    mirror_bc->value = edit_a8->value;
    mirror_bc->Redraw();
    mirror_c0->value = edit_a4->value;
    mirror_c0->Redraw();

    // art child (size 0xb0) painted with art 0x4bb
    int w1, h1;
    gfx::ArtSize(0x4bb, &w1, &h1);
    {
        CGump *p;
        p = new CDumbGump(0x200, 0x79, (int)(nullptr), w1, h1, -1, (const void *)(0), -1);
        art_c8 = p;
    }
    gfx::DrawArt(gfx::artPlain(), 0x4bb, 0, 0, gfx::bitmapOf(art_c8) + 2, art_c8->width, art_c8->height);
    gfx::AddChild(art_c8, nullptr);   // parent 0 = top level

    // random starting profession
    int r = Orig_RandMod(5);
    profession_c4->value = g_professionTable[r];
    OnChildChanged(reinterpret_cast<int>(profession_c4));
}
HOOK_CTOR(0x0041a040, CCharStatGump, ());

// 0x0041aa30  size 0x89
// Destructor body: clears the page global, frees frame0 and the frame data, base dtor.
CCharStatGump::~CCharStatGump()
{
    g_pCharStatGump = nullptr;
    if (frameData[1] != 0) {
        gfx::FreeTagged(gfx::bitmapOf(this), "CharStat frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "CharStat framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x0041aa30, CCharStatGump);

HOOK_VTABLE(0x0049d670, CCharStatGump);
HOOK_SDD(0x0041aa00, CCharStatGump);

// 0x0041ab70  size 0x82
// Mouse down: offer the press to the three stat thumbs, else start dragging the window.
void __thiscall CCharStatGump::MouseDown(int mx, int my)
{
    int sx = mx;
    int sy = my;
    gfx::GumpOrigin(this, &sx, &sy);   // 0x0043b4e0: local -> screen
    if (gfx::ScrollThumbTryGrab(thumb_ac, sx, sy) == 0 && gfx::ScrollThumbTryGrab(thumb_b0, sx, sy) == 0 &&
        gfx::ScrollThumbTryGrab(thumb_b4, sx, sy) == 0) {
        dragging = 1;
        unk_6c = mx;
        unk_70 = my;
    }
}
HOOK_VMETHOD(0x0041ab70, CCharStatGump, MouseDown);

// 0x0041b280  size 0x1a5
// The profession box changed: look up the preset for its value (1..14) and apply it.
void __thiscall CCharStatGump::OnChildChanged(int ctlArg)
{
    CInputControlGump * ctl = reinterpret_cast<CInputControlGump *>(ctlArg);
    if (ctl != profession_c4)
        return;
    unsigned int idx = static_cast<unsigned int>(profession_c4->value - 1);
    if (idx > 13)
        return;
    // stats (str, dex, int), skill ids, skill values
    switch (idx) {
    case 0:  ApplyPreset(0x14, 0x1e, 0x32, 0x00, 0x1a, 0x24, 0x22, 0x21, 0x21); break;
    case 1:  ApplyPreset(0x19, 0x19, 0x32, 0x02, 0x23, 0x27, 0x21, 0x22, 0x21); break;
    case 2:  ApplyPreset(0x14, 0x23, 0x2d, 0x15, 0x1e, 0x24, 0x21, 0x22, 0x21); break;
    case 3:  ApplyPreset(0x14, 0x2d, 0x23, 0x09, 0x16, 0x1d, 0x21, 0x21, 0x22); break;
    case 4:  ApplyPreset(0x32, 0x19, 0x19, 0x04, 0x05, 0x07, 0x21, 0x21, 0x22); break;
    case 5:  ApplyPreset(0x19, 0x32, 0x19, 0x05, 0x08, 0x1f, 0x21, 0x22, 0x21); break;
    case 6:  ApplyPreset(0x14, 0x1e, 0x32, 0x01, 0x11, 0x13, 0x21, 0x22, 0x21); break;
    case 7:  ApplyPreset(0x21, 0x21, 0x22, 0x0a, 0x1f, 0x26, 0x21, 0x21, 0x22); break;
    case 8:  ApplyPreset(0x14, 0x28, 0x28, 0x02, 0x14, 0x27, 0x21, 0x22, 0x21); break;
    case 9:  ApplyPreset(0x14, 0x32, 0x1e, 0x1c, 0x18, 0x21, 0x21, 0x21, 0x22); break;
    case 10: ApplyPreset(0x14, 0x28, 0x28, 0x03, 0x18, 0x25, 0x21, 0x21, 0x22); break;
    case 11: ApplyPreset(0x14, 0x1e, 0x32, 0x03, 0x04, 0x0c, 0x21, 0x21, 0x22); break;
    case 12: ApplyPreset(0x26, 0x26, 0x18, 0x04, 0x05, 0x1b, 0x21, 0x21, 0x22); break;
    case 13: ApplyPreset(0x19, 0x19, 0x32, 0x17, 0x19, 0x1a, 0x21, 0x22, 0x21); break;
    }
}
HOOK_VMETHOD(0x0041b280, CCharStatGump, OnChildChanged);

// 0x0041b460  size 0xa5  vtable slot 0x48
// Tab between the two name boxes: the page comes to the front, the focused box commits, and when it was one of
// the two name boxes the focus mark and the keyboard focus move to the other. The box that ends up focused is
// redrawn.
void __thiscall CCharStatGump::OnTab()
{
    CGump *had = g_focusedInputCS;   // EDI, read before the commit
    BringToFront(0);
    PORT_THISCALL0(&Gump_CommitFocus, void, g_gumpManager)();
    if (had == edit_98) {
        edit_98->focused = 0;
        edit_98->Redraw();
        edit_9c->focused = 1;
        PORT_THISCALL1(&Gump_SetFocus, void, g_gumpManager, CGump *)(edit_9c);
    } else if (had == edit_9c) {
        edit_9c->focused = 0;
        edit_9c->Redraw();
        edit_98->focused = 1;
        PORT_THISCALL1(&Gump_SetFocus, void, g_gumpManager, CGump *)(edit_98);
    }
    CGump *now = g_focusedInputCS;
    if (now != nullptr)
        static_cast<CInputControlGump *>(now)->Redraw();
}
HOOK_VMETHOD(0x0041b460, CCharStatGump, OnTab);

// 0x0041b510  size 0x8f
// Focus moved to one of the two name boxes: mark it focused and the other not.
void __thiscall CCharStatGump::OnFocus(int ctlArg)
{
    CInputControlGump * ctl = reinterpret_cast<CInputControlGump *>(ctlArg);
    if (ctl->focused != 0)
        return;
    if (ctl == edit_98) {
        edit_98->focused = 1;
        edit_98->Redraw();
        edit_9c->focused = 0;
        edit_9c->Redraw();
        return;
    }
    if (ctl == edit_9c) {
        edit_98->focused = 0;
        edit_98->Redraw();
        edit_9c->focused = 1;
        edit_9c->Redraw();
    }
}
HOOK_VMETHOD(0x0041b510, CCharStatGump, OnFocus);

// 0x0041b5d0  size 0x344
// Show a preset: stats a1..a3 are scaled down to total 0x41 (each at least 10) when they exceed it, written to the
// three stat boxes and thumbs, then the skills (ids a4..a6, values a7..a9) are written to the skill page if it
// exists, and the mirror boxes are refreshed.
void __thiscall CCharStatGump::ApplyPreset(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)
{
    int s0 = a1;   // ESI
    int s1 = a2;   // EDI
    int s2 = a3;   // EBX
    int sum = s2 + s1 + s0;   // ECX
    if (sum > 0x41) {
        s0 = (s0 * 0x41) / sum;
        sum = sum - s0;
        if (s0 < 10)
            s0 = 10;
        s2 = 0x41 - s0;
        s1 = (s1 * s2) / sum;
        if (s1 < 10)
            s1 = 10;
        s2 = s2 - s1;
        if (s2 < 10) {
            do {
                if (s0 == 10 && s1 == s0)
                    break;
                if (s0 > 10) {
                    s0--;
                    s2++;
                }
                if (s1 > 10) {
                    s1--;
                    s2++;
                }
            } while (s2 < 10);
        }
    }

    edit_a0->value = s0;
    edit_a0->Redraw();
    edit_a4->value = s1;
    edit_a4->Redraw();
    edit_a8->value = s2;
    edit_a8->Redraw();
    gfx::ScrollThumbSet(thumb_ac, s0 - 10, 0x23);
    gfx::ScrollThumbSet(thumb_b0, s1 - 10, 0x23);
    gfx::ScrollThumbSet(thumb_b4, s2 - 10, 0x23);

    CInitSkillGump *sk = GLOBAL_AS(CInitSkillGump *, g_pInitSkillGump);
    if (sk != nullptr) {
        sk->name_9c->value = a4;
        strcpy(sk->name_9c->text, g_skillNames + a4 * 0x50);
        sk->name_9c->Redraw();
        sk->name_a0->value = a5;
        strcpy(sk->name_a0->text, g_skillNames + a5 * 0x50);
        sk->name_a0->Redraw();
        sk->name_a4->value = a6;
        strcpy(sk->name_a4->text, g_skillNames + a6 * 0x50);
        sk->name_a4->Redraw();
        sk->skill_a8->value = a7;
        sk->skill_a8->Redraw();
        sk->skill_ac->value = a8;
        sk->skill_ac->Redraw();
        sk->skill_b0->value = a9;
        sk->skill_b0->Redraw();
        gfx::ScrollThumbSet(sk->thumb_b4, a7, 0x32);
        gfx::ScrollThumbSet(sk->thumb_b8, a8, 0x32);
        gfx::ScrollThumbSet(sk->thumb_bc, a9, 0x32);
    }

    mirror_b8->value = edit_a0->value;
    mirror_b8->Redraw();
    mirror_bc->value = edit_a8->value;
    mirror_bc->Redraw();
    mirror_c0->value = edit_a4->value;
    mirror_c0->Redraw();
}
HOOK_METHOD(0x0041b5d0, CCharStatGump, ApplyPreset);
