// CFightingGump.
#include "fighting_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch02_helpers.h"
#include "globals.h"
#include "crt.h"
#include "scroll_thumb_gump.h"
#include "button_gump.h"
#include "book_gump.h"
#include "small_methods_u1_ports.h"
#include "char_creation_pages.h"
#include "vtables.h"
#include "class_0049c690.h"


// slider (0xb4 bytes): x, y, 0, length, parent, art
// slider: set value (value, max). ECX = slider.
#define Slider_Set(self, a1, a2) (reinterpret_cast<CScrollThumbGump *>(self)->SetPosition(a1, a2))
// push button (0xcc bytes): x, y, hitX1, hitY1, parent, kind, 0, handler, 0
// button (0xcc bytes): x, y, parent, toggle, state, handler, up, down, 0

#define HANDLER(a) reinterpret_cast<void *>(static_cast<uintptr_t>(a))

// 0x004314c0  size 0x67a
CFightingGump::CFightingGump(int px, int py)
{
    int32_t *fd;
    void *obj;
    uint16_t *bmp;

    x = px;
    y = py;
    GLOBAL_AS(CFightingGump *, g_fightingGump) = this;
    name = const_cast<char *>("fightingstyle gump");
    dirty = 0;
    unk_a0 = 0;
    unk_9c = 0;
    unk_98 = 0;
    unk_94 = 0;
    unk_a8 = 0;
    unk_a4 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    unk_50 = 0x83e;                      // background art id

    GumpArtSize(0x83e, &width, &height);
    GumpArtSize(0x853, &unk_24, &unk_28);       // minimised frame size

    frameIndex = 0;
    frameCount = 2;
    fd = FrameData_Alloc(g_frameDataAllocator, 0x10, "framedata for CFightingGump");
    frameData = fd;
    memset(fd, 0, frameCount * 8);

    frameData[1] = ptr_i32(FrameData_Alloc(g_frameDataAllocator, height * width * 2 + 4, "fightgump frame"));
    fd = frameData;
    memset(static_cast<char *>(i32_ptr(fd[1])) + 4, 0, height * width * 2);
    *static_cast<uint16_t *>(i32_ptr(frameData[1])) = static_cast<uint16_t>(width);
    *(static_cast<uint16_t *>(i32_ptr(frameData[1])) + 1) = static_cast<uint16_t>(height);

    frameData[3] = ptr_i32(FrameData_Alloc(g_frameDataAllocator, unk_24 * unk_28 * 2 + 4, "fightgump min. frame"));
    fd = frameData;
    memset(static_cast<char *>(i32_ptr(fd[3])) + 4, 0, unk_24 * unk_28 * 2);
    *static_cast<uint16_t *>(i32_ptr(frameData[3])) = static_cast<uint16_t>(unk_24);
    *(static_cast<uint16_t *>(i32_ptr(frameData[3])) + 1) = static_cast<uint16_t>(unk_28);

    bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
    DrawGumpArt(ROWBLIT_COPY, unk_50, 0, 0, bmp + 2, bmp[0], bmp[1]);
    bmp = static_cast<uint16_t *>(i32_ptr(frameData[3]));
    DrawGumpArt(ROWBLIT_COPY, 0x853, 0, 0, bmp + 2, bmp[0], bmp[1]);

    // slider A
    obj = new CScrollThumbGump(0x5c, 0x6b, 0, 0xc5, (int)(this), 0x845);
    sliderA = obj;
    RAW_AT(uint16_t, sliderA, 0xac) = 0x846;
    RAW_AT(int32_t, sliderA, 0x68) = (g_fightMode == 1);
    CGump_AddChild(sliderA, this, 1);
    Slider_Set(sliderA, g_fightValueA, 0x64);

    // slider B
    obj = new CScrollThumbGump(0x5c, 0x96, 0, 0xc5, (int)(this), 0x845);
    sliderB = obj;
    RAW_AT(uint16_t, sliderB, 0xac) = 0x846;
    RAW_AT(int32_t, sliderB, 0x68) = (g_fightMode == 1);
    CGump_AddChild(sliderB, this, 1);
    Slider_Set(sliderB, g_fightValueB, 0x64);

    // four small buttons
    obj = new CButtonGump(0x51, 0x6c, 0x5b, 0x79, this, 2, 0, (int)(HANDLER(PORT_ADDR(&CFightingGump::SliderADec))), 0);
    CGump_AddChild(obj, this, 1);

    obj = new CButtonGump(0xc6, 0x6c, 0xce, 0x79, this, 2, 0, (int)(HANDLER(PORT_ADDR(&CFightingGump::SliderAInc))), 0);
    CGump_AddChild(obj, this, 1);

    obj = new CButtonGump(0x51, 0x96, 0x5b, 0xa4, this, 2, 0, (int)(HANDLER(PORT_ADDR(&CFightingGump::SliderBDec))), 0);
    CGump_AddChild(obj, this, 1);

    obj = new CButtonGump(0xc6, 0x96, 0xce, 0xa4, this, 2, 0, (int)(HANDLER(PORT_ADDR(&CFightingGump::SliderBInc))), 0);
    CGump_AddChild(obj, this, 1);

    // toggle button b8 (art 0x83f/0x840, hover 0x841)
    obj = new CButtonGump(0x97, 0x40, (int)(this), 1, (g_fightMode == 1), (int)(nullptr), 0x83f, 0x840, 0);
    toggleB = obj;
    RAW_AT(int32_t, toggleB, 0xa4) = 0x841;
    CGump_AddChild(toggleB, this, 1);

    // toggle button b4 (art 0x842/0x843, hover 0x844)
    obj = new CButtonGump(0x55, 0x40, (int)(this), 1, (g_fightMode != 1), (int)(nullptr), 0x842, 0x843, 0);
    toggleA = obj;
    RAW_AT(int32_t, toggleA, 0xa4) = 0x844;
    CGump_AddChild(toggleA, this, 1);

    // close button
    obj = new CButtonGump(0xfe, 0x14, 0x115, 0x2c, this, 0, 0, (int)(HANDLER(PORT_ADDR(&CBookGump::Iconify))), 0);
    CGump_AddChild(obj, this, 1);

    // four push buttons, hover art id stored at +0xa4 without a null check
    obj = new CButtonGump(0x19, 0xb1, (int)(this), 0, 0, (int)(HANDLER(PORT_ADDR(&DeleteGumpCallback))), 0x847, 0x848, 0);
    RAW_AT(int32_t, obj, 0xa4) = 0x849;
    CGump_AddChild(obj, this, 1);

    obj = new CButtonGump(0x58, 0xb2, (int)(this), 0, 0, (int)(HANDLER(PORT_ADDR(&CFightingGump::SendWarSliders))), 0x84a, 0x84b, 0);
    RAW_AT(int32_t, obj, 0xa4) = 0x84c;
    CGump_AddChild(obj, this, 1);

    obj = new CButtonGump(0x97, 0xb1, (int)(this), 0, 0, (int)(HANDLER(PORT_ADDR(&CFightingGump::PressToggleA))), 0x84d, 0x84e, 0);
    RAW_AT(int32_t, obj, 0xa4) = 0x84f;
    CGump_AddChild(obj, this, 1);

    obj = new CButtonGump(0xd5, 0xb1, (int)(this), 0, 0, (int)(HANDLER(PORT_ADDR(&CGumpHandlers04::HuePickerOk))), 0x850, 0x851, 0);
    RAW_AT(int32_t, obj, 0xa4) = 0x852;
    CGump_AddChild(obj, this, 1);

}
HOOK_CTOR(0x004314c0, CFightingGump, (int, int));
