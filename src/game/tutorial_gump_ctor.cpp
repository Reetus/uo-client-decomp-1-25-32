// CTutorialGump constructor, destructor and scalar deleting destructor. (Porting pass 2, batch 08.)
// The SEH frames of the original (try levels around operator new) are omitted.
#include "tutorial_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "batch01_helpers.h"
#include "button_gump.h"
#include "crt.h"
#include "tga_load_ports.h"
#include "palette_manager_ports.h"
#include "small_methods_u1_ports.h"
#include "vtables.h"
#include "globals.h"

// Table of page file names at 0x004a6eb0 (see tutorial_gump.cpp).
// Object whose children at +0x9c / +0xa0 / +0xa4 hold the three chosen skill indexes (value at +0xe8).
// 0x0068dd98: pixel-format object; 0x00436870 (r, g, b) -> 16-bit pixel, RET 0xc.
#define g_colorConv GLOBAL_PTR(void, g_clientState_storage)
// 0x0043ebe0: load a .tga into a frame bitmap. ECX = path, EDX = bitmap; plain RET.
#define Orig_LoadTga(a1, a2) (LoadTGA16(a1, (uint16_t *)(a2)))

// 0x00423f10  size 0x2d3
// Full-screen page: frame0 filled with a solid colour, the page's .tga loaded into it, and three rectangle buttons
// (next 0x00412490, previous 0x004125e0, 0x00412730).
CTutorialGump::CTutorialGump(int page)
{
    char skillFile[0x28];
    char *path;
    void *bmp;
    CButtonGump *btn;

    pageIndex = page;
    x = 0;
    y = 0;
    name = const_cast<char *>("tutorial gump");
    dirty = 0;
    unk_60 = 0;
    unk_50 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    width = 0x280;
    height = 0x1e0;
    frameIndex = 0;
    frameCount = 1;
    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CDumbGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    gfx::setBitmap(this, gfx::AllocTagged(width * height * 2 + 4, "dumb frame0"));
    int fillBytes = width * height * 2;
    uint32_t color = PORT_THISCALL3(&PackRGB16, uint32_t, g_colorConv, int, int, int)(8, 8, 8) & 0xffff;
    // The original fills with the low byte of the colour in every byte (mov bh, bl; stosd).
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, static_cast<int>(color & 0xff), static_cast<size_t>(fillBytes));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);

    char *entry = g_tutorialTable[pageIndex];
    if (strlen(entry) == 1) {
        char c = entry[0];
        int n;
        if (c == '1')
            n = *reinterpret_cast<int *>(*reinterpret_cast<uint8_t **>(g_pInitSkillGump + 0x9c) + 0xe8) + 1;
        else if (c == '2')
            n = *reinterpret_cast<int *>(*reinterpret_cast<uint8_t **>(g_pInitSkillGump + 0xa0) + 0xe8) + 1;
        else
            n = *reinterpret_cast<int *>(*reinterpret_cast<uint8_t **>(g_pInitSkillGump + 0xa4) + 0xe8) + 1;
        sprintf(skillFile, "tutorial\\skill%02d.tga", n);
        bmp = gfx::bitmapOf(this);
        path = skillFile;
    } else {
        bmp = gfx::bitmapOf(this);
        path = entry;
    }
    Orig_LoadTga(g_app.ResolveDataPath2(path), bmp);

    // Button 1 (0x24e, 0x195)-(0x279, 0x1b7): next page
    btn = new CButtonGump(0x24e, 0x195, 0x279, 0x1b7, this, 0, 0, PORT_ADDR(&CTutorialGump::NextPage), 0);
    gfx::AddChild(btn, this);

    // Button 2 (0x21a, 0x195)-(0x245, 0x1b7): previous page
    btn = new CButtonGump(0x21a, 0x195, 0x245, 0x1b7, this, 0, 0, PORT_ADDR(&CTutorialGump::PrevPage), 0);
    gfx::AddChild(btn, this);

    // Button 3 (0x21b, 0x1be)-(0x278, 0x1d6): handler 0x00412730
    btn = new CButtonGump(0x21b, 0x1be, 0x278, 0x1d6, this, 0, 0, PORT_ADDR(&U1_TutorialFinish), 0);
    gfx::AddChild(btn, this);
}
HOOK_CTOR(0x00423f10, CTutorialGump, (int));

// 0x00424220  size 0x8b
CTutorialGump::~CTutorialGump()
{
    if (gfx::bitmapOf(this) != nullptr) {
        gfx::FreeTagged(gfx::bitmapOf(this), "tutorial frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "tutorial framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00424220, CTutorialGump);

HOOK_VTABLE(0x0049e4a8, CTutorialGump);
HOOK_SDD(0x004241f0, CTutorialGump);
