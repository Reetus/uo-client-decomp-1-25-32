// CCityChoiceGump constructor, destructor and scalar deleting destructor. (Porting pass 2, batch 08.)
// The SEH frames of the original (try levels around operator new) are omitted.
#include "city_choice_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "button_gump.h"
#include "input_control_gump.h"
#include "crt.h"
#include "dumb_gump.h"
#include "vtables.h"
#include "globals.h"


// 0x0043d0a0: CDumbGump constructor, RET 0x20, (x, y, a3, w, h, -1, 0, -1). Not ported.
#define Orig_DumbGumpCtor(self, a1, a2, a3, a4, a5, a6, a7, a8) ((CGump *)reinterpret_cast<CDumbGump *>(self)->Ctor(a1, a2, a3, a4, a5, a6, (const void *)(a7), a8))

static inline int32_t selfArg(const void *p) { return static_cast<int32_t>(reinterpret_cast<uintptr_t>(p)); }

// 0x00423830  size 0x5c1
// "Choose your city" page: server and city drop-down fields, four buttons and two decorations.
CCityChoiceGump::CCityChoiceGump()
{
    CButtonGump *btn;
    CInputControlGump *field;
    int w;
    int h;
    int i;
    const char *src;

    name = const_cast<char *>("CityChoice gump");
    dirty = 0;
    unk_50 = 0x514;
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    gfx::ArtSize(0x514, &width, &height);
    x = 0x77;
    y = 0x71;
    frameIndex = 0;
    frameCount = 1;
    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CCityChoiceGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    gfx::setBitmap(this, gfx::AllocTagged(width * height * 2 + 4, "dumb frame0"));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(width * height * 2));
    gfx::DrawArt(gfx::artPlain(), unk_50, 0, 0, gfx::bitmapOf(this) + 2, width, height);

    // Server field (+0x94)
    field = new CInputControlGump(0xe0 - x, 0xab - y, this, static_cast<uint16_t>(0xb4), static_cast<uint16_t>(0x10), 2, 0x1e, 1, 0x386, 1, 1, 0);
    serverField = field;
    field->dropDown = 1;
    field = static_cast<CInputControlGump *>(serverField);
    field->value = 0;
    {
        // index of the current server value in the table of 0x40 entries
        uint16_t *vals = g_serverValues;
        int sel = GLOBAL_AS(int8_t, g_selectedServer);
        i = 0;
        do {
            if (*vals == sel)
                break;
            i++;
            vals++;
        } while (i < 0x40);
    }
    if (g_loginByAddr != 0) {
        src = "Standalone Server";
    } else {
        char *s = g_serverNames[i];
        if (s != nullptr)
            src = s;
        else
            src = "Unknown Server";
    }
    strcpy(static_cast<CInputControlGump *>(serverField)->text, src);
    static_cast<CInputControlGump *>(serverField)->Redraw();
    gfx::AddChild(static_cast<CInputControlGump *>(serverField), this);

    // City field (+0x98)
    field = new CInputControlGump(0xe0 - x, 0xf7 - y, this, static_cast<uint16_t>(0xb4), static_cast<uint16_t>(0x10), 2, 0x1e, 1, 0x386, 1, 1, 0);
    cityField = field;
    field->dropDown = 1;
    field = static_cast<CInputControlGump *>(cityField);
    field->value = 0;
    {
        // first non-empty start location name
        char **locs = g_locNames;
        for (;;) {
            src = *locs;
            if (src != nullptr && *src != 0)
                break;
            locs++;
        }
    }
    strcpy(static_cast<CInputControlGump *>(cityField)->text, src);
    static_cast<CInputControlGump *>(cityField)->Redraw();
    gfx::AddChild(static_cast<CInputControlGump *>(cityField), this);

    // Button: server list (handler 0x00411ec0)
    btn = new CButtonGump(0xfd - x, 0x86 - y, selfArg(this), 0, 0, PORT_ADDR(&CCityChoiceGump::OnServerListButton), 0x51b, 0x51c, 0);
    btn->flag_b0 = 1;
    gfx::AddChild(btn, this);

    // Button: city list (handler 0x00411f90)
    btn = new CButtonGump(0x112 - x, 0xd0 - y, selfArg(this), 0, 0, PORT_ADDR(&CCityChoiceGump::OnCityListButton), 0x51e, 0x51f, 0);
    btn->artHover = 0x520;
    gfx::AddChild(btn, this);

    // Button: finish (handler 0x004114c0)
    btn = new CButtonGump(0x187 - x, 0x119 - y, selfArg(this), 0, 0, PORT_ADDR(&CCityChoiceGump::Finish), 0x518, 0x519, 0);
    btn->artHover = 0x51a;
    gfx::AddChild(btn, this);

    // Button: back (handler 0x00411470)
    btn = new CButtonGump(0xb4 - x, 0x119 - y, selfArg(this), 0, 0, PORT_ADDR(&CCityChoiceGump::BackToCreation), 0x515, 0x516, 0);
    btn->artHover = 0x517;
    gfx::AddChild(btn, this);

    // Decoration 1 (+0x9c): art 0x4ed at (0x55, 0x79)
    gfx::ArtSize(0x4ed, &w, &h);
    deco1 = new CDumbGump(0x55, 0x79, 0, w, h, -1, (const void *)(0), -1);
    gfx::DrawArt(gfx::artPlain(), 0x4ed, 0, 0, gfx::bitmapOf(deco1) + 2, deco1->width, deco1->height);
    gfx::AddChild(deco1, nullptr);

    // Decoration 2 (+0xa0): art 0x4ee at (0x1f7, 0x74)
    gfx::ArtSize(0x4ee, &w, &h);
    deco2 = new CDumbGump(0x1f7, 0x74, 0, w, h, -1, (const void *)(0), -1);
    gfx::DrawArt(gfx::artPlain(), 0x4ee, 0, 0, gfx::bitmapOf(deco2) + 2, deco2->width, deco2->height);
    gfx::AddChild(deco2, nullptr);
}
HOOK_CTOR(0x00423830, CCityChoiceGump, ());

// 0x00423e30  size 0xb5
// Clears the live-instance global, deletes the two fields (not the decorations), frees frame0 and the frame data.
CCityChoiceGump::~CCityChoiceGump()
{
    g_cityChoiceGump = nullptr;
    if (serverField != nullptr) {
        PORT_VSLOT1(serverField, kGumpSlot_DeletingDtor, void, int)(1);
        serverField = nullptr;
    }
    if (cityField != nullptr) {
        PORT_VSLOT1(cityField, kGumpSlot_DeletingDtor, void, int)(1);
        cityField = nullptr;
    }
    if (gfx::bitmapOf(this) != nullptr) {
        gfx::FreeTagged(gfx::bitmapOf(this), "CityChoice frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "CityChoice framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00423e30, CCityChoiceGump);

HOOK_VTABLE(0x0049e390, CCityChoiceGump);
HOOK_SDD(0x00423e00, CCityChoiceGump);
