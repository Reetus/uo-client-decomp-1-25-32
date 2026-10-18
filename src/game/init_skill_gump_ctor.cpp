#include "init_skill_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "button_gump.h"
#include "crt.h"
#include "input_control_gump.h"
#include "dumb_gump.h"
#include "char_creation_events_ports.h"
#include "char_creation_pages.h"
#include "vtables.h"
#include "globals.h"

// The SEH frame of the original (try-level bookkeeping around each operator new) is omitted; every
// "operator new returned NULL" branch of the original is kept (the object pointer becomes 0).



// 0x0043d0a0: art display child gump (operator new(0xb0)), RET 0x20, (x, y, parent, w, h, a6, a7, a8). Not ported.
#define Orig_NewArtChild(self, a1, a2, a3, a4, a5, a6, a7, a8) ((CGump *)reinterpret_cast<CDumbGump *>(self)->Ctor(a1, a2, (int)(a3), a4, a5, a6, (const void *)(a7), a8))

// 0x00418bd0: plain RET, does not read ECX; called right after the base constructor.
// ? purpose not settled (its name in the merged first pass, LoadSkillNameTable, is a guess).
#define Orig_Fn418bd0() (LoadSkillNameTable())

// 0x0041b920  size 0x909
// Constructor of the character creation skill page: frame0, three skill name boxes, three skill value boxes with
// thumbs, five buttons and an art child. The caller stores the result in 0x006488fc.
CInitSkillGump::CInitSkillGump()
{
    name = const_cast<char *>("InitSkill gump");
    dirty = 0;
    unk_50 = 0x4ce;   // art id of the background
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;

    gfx::ArtSize(unk_50, &width, &height);
    x = 0x62;
    y = 0x65;
    frameIndex = 0;
    frameCount = 1;

    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CInitSkillGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    gfx::setBitmap(this, gfx::AllocTagged(width * height * 2 + 4, "initskill frame0"));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(width * height * 2));
    gfx::DrawArt(gfx::artPlain(), unk_50, 0, 0, gfx::bitmapOf(this) + 2, width, height);

    Orig_Fn418bd0();

    // skill name box +0x9c (skill 0)
    name_9c = new CInputControlGump(0xe5 - x, 0x96 - y, this, static_cast<uint16_t>(0xa6), static_cast<uint16_t>(0x14), 2, 0x18, 1, 0x386, 1, 1, 0);
    name_9c->dropDown = 1;
    gfx::AddChild(name_9c, this);
    name_9c->value = 0;
    strcpy(name_9c->text, g_skillNames);
    name_9c->Redraw();

    // skill value box +0xa8
    skill_a8 = new CInputControlGump(0x12d, 0x9f - y, this, static_cast<uint16_t>(0x24), static_cast<uint16_t>(0xe), 1, 0x32, 1, 0x386, 1, 1, 0);
    skill_a8->value = 0x22;
    skill_a8->dropDown = 1;
    skill_a8->Redraw();
    gfx::AddChild(skill_a8, this);

    // skill name box +0xa0 (skill 1)
    name_a0 = new CInputControlGump(0xe5 - x, 0xc2 - y, this, static_cast<uint16_t>(0xa6), static_cast<uint16_t>(0x14), 2, 0x18, 1, 0x386, 1, 1, 0);
    name_a0->dropDown = 1;
    gfx::AddChild(name_a0, this);
    name_a0->value = 1;
    strcpy(name_a0->text, g_skillNames + 0x50);
    name_a0->Redraw();

    // skill value box +0xac
    skill_ac = new CInputControlGump(0x12e, 0xcc - y, this, static_cast<uint16_t>(0x24), static_cast<uint16_t>(0xe), 1, 0x32, 1, 0x386, 1, 1, 0);
    skill_ac->value = 0x21;
    skill_ac->dropDown = 1;
    skill_ac->Redraw();
    gfx::AddChild(skill_ac, this);

    // skill name box +0xa4 (skill 2)
    name_a4 = new CInputControlGump(0xe5 - x, 0xef - y, this, static_cast<uint16_t>(0xa6), static_cast<uint16_t>(0x14), 2, 0x18, 1, 0x386, 1, 1, 0);
    name_a4->dropDown = 1;
    gfx::AddChild(name_a4, this);
    name_a4->value = 2;
    strcpy(name_a4->text, g_skillNames + 0xa0);
    name_a4->Redraw();

    // skill value box +0xb0
    skill_b0 = new CInputControlGump(0x12f, 0xfa - y, this, static_cast<uint16_t>(0x24), static_cast<uint16_t>(0xe), 1, 0x32, 1, 0x386, 1, 1, 0);
    skill_b0->value = 0x21;
    skill_b0->dropDown = 1;
    skill_b0->Redraw();
    gfx::AddChild(skill_b0, this);

    // scroll thumbs +0xb4 / +0xb8 / +0xbc
    thumb_b4 = new CScrollThumbGump(0xe4 - x, 0xa9 - y, 0, 0x18a - x, (int)(this), 0x4b9);
    thumb_b4->artAlt = 0x4ba;
    gfx::ScrollThumbSet(thumb_b4, skill_a8->value, 0x32);
    gfx::AddChild(thumb_b4, this);

    thumb_b8 = new CScrollThumbGump(0xe4 - x, 0xd6 - y, 0, 0x18a - x, (int)(this), 0x4b9);
    thumb_b8->artAlt = 0x4ba;
    gfx::ScrollThumbSet(thumb_b8, skill_ac->value, 0x32);
    gfx::AddChild(thumb_b8, this);

    thumb_bc = new CScrollThumbGump(0xe4 - x, 0x104 - y, 0, 0x18a - x, (int)(this), 0x4b9);
    thumb_bc->artAlt = 0x4ba;
    gfx::ScrollThumbSet(thumb_bc, skill_b0->value, 0x32);
    gfx::AddChild(thumb_bc, this);

    // buttons: (x, y, parent, callback, up art, down art); artHover is set after construction
    {
        CButtonGump *b = new CButtonGump(0xbe - x, 0x99 - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CInitSkillGump::OpenSkillChoice0), 0x4d6, 0x4d7, 0);
        b->artHover = 0x4d8;
        gfx::AddChild(b, this);
    }
    {
        CButtonGump *b = new CButtonGump(0xbc - x, 0xc6 - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CInitSkillGump::OpenSkillChoice1), 0x4d9, 0x4da, 0);
        b->artHover = 0x4db;
        gfx::AddChild(b, this);
    }
    {
        CButtonGump *b = new CButtonGump(0xbc - x, 0xf2 - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CGumpHandlers04::OpenSkillPicker), 0x4dc, 0x4dd, 0);
        b->artHover = 0x4de;
        gfx::AddChild(b, this);
    }
    {
        CButtonGump *b = new CButtonGump(0xbf - x, 0x130 - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CInitSkillGump::ShowStatPage), 0x4d3, 0x4d4, 0);
        b->artHover = 0x4d5;
        gfx::AddChild(b, this);
    }
    {
        CButtonGump *b = new CButtonGump(0x17a - x, 0x12f - y, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&CInitSkillGump::NextFromSkills), 0x4d0, 0x4d1, 0);
        b->artHover = 0x4d2;
        gfx::AddChild(b, this);
    }

    // art child (size 0xb0) painted with art 0x4cf
    int w1, h1;
    gfx::ArtSize(0x4cf, &w1, &h1);
    {
        CGump *p;
        p = new CDumbGump(0x205, 0x78, (int)(nullptr), w1, h1, -1, (const void *)(0), -1);
        art_98 = p;
    }
    gfx::DrawArt(gfx::artPlain(), 0x4cf, 0, 0, gfx::bitmapOf(art_98) + 2, art_98->width, art_98->height);
    gfx::AddChild(art_98, nullptr);   // parent 0 = top level
}
HOOK_CTOR(0x0041b920, CInitSkillGump, ());

// 0x0041c260  size 0x89
// Destructor body: clears the page global, frees frame0 and the frame data, base dtor.
CInitSkillGump::~CInitSkillGump()
{
    GLOBAL_AS(CInitSkillGump *, g_pInitSkillGump) = nullptr;
    if (frameData[1] != 0) {
        gfx::FreeTagged(gfx::bitmapOf(this), "InitSkill frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "InitSkill framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x0041c260, CInitSkillGump);

HOOK_VTABLE(0x0049d788, CInitSkillGump);
HOOK_SDD(0x0041c230, CInitSkillGump);

// 0x0041c310  size 0x82
// Mouse down: offer the press to the three skill thumbs, else start dragging the window.
void __thiscall CInitSkillGump::MouseDown(int mx, int my)
{
    int sx = mx;
    int sy = my;
    gfx::GumpOrigin(this, &sx, &sy);   // 0x0043b4e0: local -> screen
    if (gfx::ScrollThumbTryGrab(thumb_b4, sx, sy) == 0 && gfx::ScrollThumbTryGrab(thumb_b8, sx, sy) == 0 &&
        gfx::ScrollThumbTryGrab(thumb_bc, sx, sy) == 0) {
        dragging = 1;
        unk_6c = mx;
        unk_70 = my;
    }
}
HOOK_VMETHOD(0x0041c310, CInitSkillGump, MouseDown);
