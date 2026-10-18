// CHueGump constructor. (Porting pass 2, batch 15.)
#include "hue_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "crt.h"
#include "char_flow_gumps.h"
#include "small_methods_u1_ports.h"
#include "vtables.h"
#include "class_0049c690.h"

// 0x00441850  size 0x372
// Frame0 is art 0x906 drawn once; 0x00442080 (CHueGump::Redraw) draws the hue grid into it. The page slider sits at
// (0x26, 0x8e), the two arrow buttons at (0x1a, 0x8e) and (0xb8, 0x8e), the OK button (art 0x907 / 0x908, hover
// 0x909) at (0xd0, 0x8a).
CHueGump::CHueGump(int x_, int y_, int artArg, int mode_, const char *titleText, uint16_t hue_)
{
    x = x_;
    y = y_;
    unk_50 = artArg;
    name = const_cast<char *>("hue gump");
    dirty = 0;
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    gfx::ArtSize(0x906, &width, &height);
    unk_c8 = 0x22;
    unk_cc = 0x22;
    unk_ac = 0;
    hue = hue_;
    int rem = (static_cast<int>(hue_) - 2) % 5;
    unk_b0 = 0;
    unk_b8 = 0;
    unk_b4 = 0;
    mode = mode_;
    unk_d0 = 0xc1;
    unk_d4 = 0x71;
    page = static_cast<uint16_t>(rem);
    isFirst = mode_ == 0;
    size_t titleLen = strlen(titleText) + 1;
    title = static_cast<char *>(::operator new(titleLen));
    strcpy(title, titleText);
    frameIndex = 0;
    frameCount = 1;
    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CDumbGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    gfx::setBitmap(this, gfx::AllocTagged(width * height * 2 + 4, "dumb frame0"));
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(width * height * 2));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);
    gfx::DrawArt(gfx::artPlain(), 0x906, 0, 0, gfx::bitmapOf(this) + 2, gfx::bitmapOf(this)[0], gfx::bitmapOf(this)[1]);
    this->CHueGump::Redraw();   // CHueGump::Redraw

    CScrollThumbGump *th;
    th = new CScrollThumbGump(0x26, 0x8e, 0, 0xba, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0x845);
    thumb = th;
    th->artAlt = 0x846;
    gfx::AddChild(thumb, this);
    thumb->SetPosition(page, 4);

    CButtonGump *btn;
    btn = new CButtonGump(0x1a, 0x8e, 0x27, 0x9e, this, 2, 0, PORT_ADDR(&CHueGump_00410d30::StepDown), 0);
    btnDown = btn;
    gfx::AddChild(btn, this);

    btn = new CButtonGump(0xb8, 0x8e, 0xc5, 0x9e, this, 2, 0, PORT_ADDR(&CHueGump_00410d30::StepUp), 0);
    btnUp = btn;
    gfx::AddChild(btn, this);

    btn = new CButtonGump(0xd0, 0x8a, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 0, 0, PORT_ADDR(&DeleteGumpCallback), 0x907, 0x908, 0);
    btn->artHover = 0x909;
    gfx::AddChild(btn, this);
}
HOOK_CTOR(0x00441850, CHueGump, (int, int, int, int, const char *, uint16_t));
