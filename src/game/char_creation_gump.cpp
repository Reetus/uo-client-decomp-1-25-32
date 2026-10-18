#include "char_creation_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "globals.h"
#include "crt.h"
#include "input_control_gump.h"
#include "dumb_gump.h"
#include "vtables.h"

// The SEH frame of the original (try-level bookkeeping around each operator new) is omitted; every
// "operator new NULL check" of the original is kept.


// 0x0043d0a0: art display child gump (operator new(0xb0)), RET 0x20, (x, y, parent, w, h, a6, a7, a8). Not ported.
#define Orig_NewArtChild(self, a1, a2, a3, a4, a5, a6, a7, a8) ((CGump *)reinterpret_cast<CDumbGump *>(self)->Ctor(a1, a2, (int)(a3), a4, a5, a6, (const void *)(a7), a8))

// The constructor allocates its children in place (the original inlines every allocation).
#define newEditBox(x, y, parent, a, b, c, d, e, f, g, h, i)     new CInputControlGump(x, y, parent, static_cast<uint16_t>(a), static_cast<uint16_t>(b), c, d, e, f, g, h, i)
#define newThumb(x, y, x2, parent) new CScrollThumbGump(x, y, 0, x2, (int)(parent), 0x4b9)
#define newButton(x, y, parent, callback, artUp, artDown)     new CButtonGump(x, y, static_cast<int>(reinterpret_cast<uintptr_t>(parent)), 0, 0, callback, artUp, artDown, 0)

// 0x00418d60  size 0x8d3
// Constructor: frame0 bitmap, name/gender/hair/beard edit boxes, six scroll thumbs, five buttons, two art children.
CCharCreationGump::CCharCreationGump()
{
    name = const_cast<char *>("CharCreation gump");
    dirty = 0;
    unk_50 = 0x4ec;   // art id of the background
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;

    gfx::ArtSize(unk_50, &width, &height);
    rowStride = width;
    x = 0x6c;
    y = 0;
    frameIndex = 0;
    frameCount = 1;

    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CDumbGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    gfx::setBitmap(this, gfx::AllocTagged(width * height * 2 + 4, "dumb frame0"));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(width * height * 2));

    // name edit box at +0xa8 (hidden)
    genderBox = newEditBox(0, 0, this, 0x50, 0xe, 2, 10, 1, 0x386, 0, 1, 0);
    genderBox->value = 0;
    genderBox->visible = 0;
    gfx::AddChild(genderBox, this);

    // scroll thumb +0xac
    thumb_ac = newThumb(0x10b - x, 0x10e - y, 0x167 - x, this);
    thumb_ac->artAlt = 0x4ba;
    gfx::AddChild(thumb_ac, this);

    // scroll thumb +0xb0
    thumb_b0 = newThumb(0x106 - x, 0x122 - y, 0x16c - x, this);
    thumb_b0->artAlt = 0x4ba;
    gfx::AddChild(thumb_b0, this);

    skinHueIndex = 0x3e9;
    unk_cc = 0;
    unk_d0 = 0;
    hairHue = 0x44d;
    beardHue = 0x44d;
    unk_e0 = 0;
    unk_dc = 0;
    unk_d8 = 0;
    unk_d4 = 0;

    // first edit box +0xa0 (hair style name)
    hairBox = newEditBox(0x9a - x, 0xe6 - y, this, 0x6a, 0xe, 2, 10, 1, 0x386, 1, 1, 0);
    hairBox->dropDown = 1;
    gfx::AddChild(hairBox, this);
    hairBox->value = 0x203b;
    strcpy(hairBox->text, "Short");
    hairBox->Redraw();

    // scroll thumb +0xb4
    thumb_b4 = newThumb(0x9f - x, 0x10f - y, 0xfb - x, this);
    thumb_b4->artAlt = 0x4ba;
    gfx::AddChild(thumb_b4, this);

    // scroll thumb +0xbc
    thumb_bc = newThumb(0x9a - x, 0xfb - y, 0x100 - x, this);
    thumb_bc->artAlt = 0x4ba;
    gfx::AddChild(thumb_bc, this);

    // second edit box +0xa4 (beard style name)
    beardBox = newEditBox(0x170 - x, 0xe6 - y, this, 0x6a, 0xe, 2, 10, 1, 0x386, 1, 1, 0);
    beardBox->dropDown = 1;
    gfx::AddChild(beardBox, this);
    beardBox->value = 0;
    strcpy(beardBox->text, "NONE");
    beardBox->Redraw();

    // scroll thumb +0xb8 (hidden initially)
    thumb_b8 = newThumb(0x178 - x, 0x10f - y, 0x1d3 - x, this);
    thumb_b8->artAlt = 0x4ba;
    thumb_b8->visible = 0;
    gfx::AddChild(thumb_b8, this);

    // scroll thumb +0xc0 (hidden initially)
    thumb_c0 = newThumb(0x172 - x, 0xfb - y, 0x1d9 - x, this);
    thumb_c0->artAlt = 0x4ba;
    thumb_c0->visible = 0;
    gfx::AddChild(thumb_c0, this);

    // buttons: (x, y, parent, callback, up art, down art); artHover is set after construction
    {
        CButtonGump *b = newButton(0x11c - x, 200 - y, this, PORT_ADDR(&CCharCreationGump::ToggleGender), 0x4f7, 0x4f8);
        b->artHover = 0x4f9;
        gfx::AddChild(b, this);   // not stored
    }
    {
        CButtonGump *b = newButton(0xa0 - x, 0xcc - y, this, PORT_ADDR(&CCharCreationGump::OpenHairPicker), 0x4fa, 0x4fb);
        button_98 = b;
        button_98->artHover = 0x4fc;
        gfx::AddChild(button_98, this);
    }
    {
        CButtonGump *b = newButton(0x176 - x, 0xcc - y, this, PORT_ADDR(&CCharCreationGump::OpenBeardPicker), 0x4fd, 0x4fe);
        button_9c = b;
        button_9c->artHover = 0x4ff;
        gfx::AddChild(button_9c, this);
    }
    {
        CButtonGump *b = newButton(0x195 - x, 0x96 - y, this, PORT_ADDR(&CCharCreationGump::ShowCityChoice), 0x4f4, 0x4f5);
        b->artHover = 0x4f6;
        gfx::AddChild(b, this);   // not stored
    }
    {
        CButtonGump *b = newButton(0xa8 - x, 0x97 - y, this, PORT_ADDR(&CCharCreationGump::ShowInitSkill), 0x4f1, 0x4f2);
        b->artHover = 0x4f3;
        gfx::AddChild(b, this);   // not stored
    }

    // two art-display children (size 0xb0), then paint art into their frame0 bitmap
    int w1, h1;
    gfx::ArtSize(0x4ed, &w1, &h1);
    {
                CGump *p;
        p = new CDumbGump(0x50, 0x85, (int)(nullptr), w1, h1, -1, (const void *)(0), -1);
        art_c4 = p;
        gfx::DrawArt(gfx::artPlain(), 0x4ed, 0, 0, gfx::bitmapOf(art_c4) + 2, art_c4->width, art_c4->height);
        gfx::AddChild(art_c4, nullptr);   // parent 0 = top level
    }

    gfx::ArtSize(0x4ee, &w1, &h1);
    {
        CGump *p;
        p = new CDumbGump(0x1f9, 0x81, (int)(nullptr), w1, h1, -1, (const void *)(0), -1);
        art_c8 = p;
        gfx::DrawArt(gfx::artPlain(), 0x4ee, 0, 0, gfx::bitmapOf(art_c8) + 2, art_c8->width, art_c8->height);
        gfx::AddChild(art_c8, nullptr);
    }

    RedrawPreview();
}
HOOK_CTOR(0x00418d60, CCharCreationGump, ());

// 0x004197d0  size 0xcb
// Mouse-down handler. The press is converted to the gump's local space; each thumb gets a chance to grab it,
// otherwise a window drag starts.
void __thiscall CCharCreationGump::MouseDown(int mx, int my)
{
    int lx = mx;
    int ly = my;
    gfx::GumpOrigin(this, &lx, &ly);   // ? screen -> local per pass2; direction not checked

    if (gfx::ScrollThumbTryGrab(thumb_ac, lx, ly)) return;
    if (gfx::ScrollThumbTryGrab(thumb_b0, lx, ly)) return;
    if (gfx::ScrollThumbTryGrab(thumb_b4, lx, ly)) return;
    if (gfx::ScrollThumbTryGrab(thumb_b8, lx, ly)) return;
    if (gfx::ScrollThumbTryGrab(thumb_bc, lx, ly)) return;
    if (gfx::ScrollThumbTryGrab(thumb_c0, lx, ly)) return;

    dragging = 1;   // nothing hit: start window drag
    unk_6c = mx;
    unk_70 = my;
}
HOOK_VMETHOD(0x004197d0, CCharCreationGump, MouseDown);

// 0x00419cb0  size 0x38b
// Composes background, body, head, hair, beard and clothing layers of the preview into frame0.
void __thiscall CCharCreationGump::RedrawPreview()
{
    int rect[4];
    int found, aux;
    int skinHue = skinHueIndex + 1;
    int bodyArt = genderBox->value + 0xc;

    gfx::ArtSize(bodyArt, &found, &aux);
    if (found == 0)
        return;

    rect[0] = 0;
    rect[1] = 0;
    rect[2] = width;
    rect[3] = height;
    int xoff = 0xe8 - x;

    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(height * width * 2));

    gfx::DrawArt(gfx::artPlain(), unk_50, 0, 0, gfx::bitmapOf(this) + 2, width, height);
    gfx::DrawArt(gfx::artPlain(), genderBox->value + 0x4ef, 0x11d - x, 0xdd - y, gfx::bitmapOf(this) + 2, width, height);

    uint16_t *bmp;

    // body (skin tone through the hued renderer when skinHue != 0)
    bmp = gfx::bitmapOf(this);
    if (skinHue == 0) {
        gfx::DrawArtClipped(gfx::artPlain(), bodyArt, rect, xoff, -0x28, bmp + 2, bmp[0], bmp[1]);
    } else {
        g_hueOverride = skinHue - 1;
        gfx::DrawArtClipped(gfx::artHued(), bodyArt, rect, xoff, -0x28, bmp + 2, bmp[0], bmp[1]);
    }

    // head
    bmp = gfx::bitmapOf(this);
    gfx::DrawArtClipped(gfx::artPlain(), genderBox->value * 10000 + 0xc502, rect, xoff, -0x28, bmp + 2, bmp[0], bmp[1]);

    // hair
    if (hairBox->value != 0) {
        int hairArt = *reinterpret_cast<uint16_t *>(g_tileData + hairBox->value * 0x28 + 0xc);
        if (genderBox->value == 0)
            hairArt += 0xc350;
        else
            hairArt += 0xea60;
        bmp = gfx::bitmapOf(this);
        if (hairHue + 1 == 0) {   // hairHue == -1
            gfx::DrawArtClipped(gfx::artPlain(), hairArt, rect, xoff, -0x28, bmp + 2, bmp[0], bmp[1]);
        } else {
            g_hueOverride = hairHue;
            gfx::DrawArtClipped(gfx::artHued(), hairArt, rect, xoff, -0x28, bmp + 2, bmp[0], bmp[1]);
        }
    }

    // beard
    if (beardBox->value != 0) {
        int beardArt = *reinterpret_cast<uint16_t *>(g_tileData + beardBox->value * 0x28 + 0xc) + 0xc350;
        bmp = gfx::bitmapOf(this);
        if (beardHue + 1 == 0) {   // beardHue == -1
            gfx::DrawArtClipped(gfx::artPlain(), beardArt, rect, xoff, -0x28, bmp + 2, bmp[0], bmp[1]);
        } else {
            g_hueOverride = beardHue;
            gfx::DrawArtClipped(gfx::artHued(), beardArt, rect, xoff, -0x28, bmp + 2, bmp[0], bmp[1]);
        }
    }

    // fixed layer
    bmp = gfx::bitmapOf(this);
    gfx::DrawArtClipped(gfx::artPlain(), 0xc5be, rect, xoff, -0x28, bmp + 2, bmp[0], bmp[1]);

    // gender-dependent layer
    bmp = gfx::bitmapOf(this);
    if (genderBox->value == 0)
        gfx::DrawArtClipped(gfx::artPlain(), 0xc4ff, rect, xoff, -0x28, bmp + 2, bmp[0], bmp[1]);
    else
        gfx::DrawArtClipped(gfx::artPlain(), 0xec21, rect, xoff, -0x28, bmp + 2, bmp[0], bmp[1]);

    // last layer
    bmp = gfx::bitmapOf(this);
    gfx::DrawArtClipped(gfx::artPlain(), genderBox->value * 10000 + 0xc530, rect, xoff, -0x28, bmp + 2, bmp[0], bmp[1]);
}
HOOK_METHOD(0x00419cb0, CCharCreationGump, RedrawPreview);
