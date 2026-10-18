#include "stack_gump.h"

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "batch06_helpers.h"
#include "globals.h"
#include "scroll_thumb_gump.h"
#include "button_gump.h"
#include "gump.h"
#include "ui_callbacks_ports.h"
#include "vtables.h"
#include "crt.h"

// 0x00413740 scroll thumb ctor: (mem, w?, h?, ..., parent, art) - 6 stack args, returns the control.
// 0x00413c00 scrollbar update on the thumb: (value, max)
#define ScrollThumb_Update(self, a1, a2) (reinterpret_cast<CScrollThumbGump *>(self)->SetPosition(a1, a2))
// 0x00412fb0 button ctor: (x, y, parent, 0, 0, callback, upArt, downArt, 0)
// 0x0043a820 on the gump manager: register a gump.
#define GumpManager_Register(o, a1) PORT_THISCALL1(&Gump_SetFocus, void, (o), CGump *)(a1)

// 0x00469890  size 0x242  "stack gump" constructor (vtable 0x0049fe58), RET 0xc
// Sizes the gump from art 0x85c, allocates its frame, seeds the amount from the owner,
// adds the scroll thumb and the OK button.
CStackGump::CStackGump(int x_, int y_, void *owner_)
{
    x = x_;
    owner = static_cast<CObj *>(owner_);
    name = const_cast<char *>("stack gump");
    dirty = 0;
    unk_94 = 0;
    y = y_;
    unk_60 = 0;
    if (owner_ != nullptr)
        *reinterpret_cast<CGump **>(static_cast<char *>(owner_) + 0x90) = this;
    unk_50 = 0x85c;   // art id
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    b6_art_size(0x85c, &width, &height);
    rowStride = (width + 7) & ~7;   // asm: ADD 7 / AND AL,0xf8
    width = rowStride;
    frameIndex = 0;
    frameCount = 1;
    int32_t *fd = static_cast<int32_t *>(b6_alloc(g_frameDataAllocator, 8, "framedata for CStatusGump"));
    memset(fd, 0, frameCount * 8);
    frameData = fd;
    void *bmp = b6_alloc(g_frameDataAllocator, width * height * 2 + 4, "status frame0");
    b6_set_frame_bitmap(this, 0, bmp);
    memset(b6_frame_bitmap(this, 0)->px, 0, width * height * 2);
    b6_frame_bitmap(this, 0)->w = static_cast<uint16_t>(width);
    b6_frame_bitmap(this, 0)->h = static_cast<uint16_t>(height);

    unk_98 = 0;
    int v = *reinterpret_cast<uint16_t *>(reinterpret_cast<char *>(owner) + 0x3c);
    maxValue = v;
    value = v;
    sprintf(text, "%d", v);

    GumpManager_Register(g_gumpManager, this);
    this->CStackGump::RedrawFrame(1);

    uint8_t *thumb;
    thumb = reinterpret_cast<uint8_t *>(new CScrollThumbGump(0x1c, 0x10, 0, 0x88, (int)(this), 0x845));
    scrollThumb = thumb;
    *reinterpret_cast<int16_t *>(thumb + 0xac) = 0x846;   // ? no null check in the original
    CGump_AddChild(scrollThumb, this, 1);
    ScrollThumb_Update(scrollThumb, value, maxValue);

    uint8_t *btn;
    btn = (uint8_t *)new CButtonGump(0x66, 0x25, (int)(this), 0, 0, (int)(reinterpret_cast<void *>(PORT_ADDR(&UiCb_PickUpOwner))), 0x85d, 0x85e, 0);
    *reinterpret_cast<int32_t *>(btn + 0xa4) = 0x81c;   // ? no null check in the original
    CGump_AddChild(btn, this, 1);
}
HOOK_CTOR(0x00469890, CStackGump, (int, int, void *));

// 0x00469b10  size 0x98  "stack gump" destructor (vtable 0x0049fe58)
// Frees the frame bitmap and frame data, clears the owner's gump slot, runs the CGump destructor.
CStackGump::~CStackGump()
{
    void *bmp = b6_frame_bitmap(this, 0);
    if (bmp != nullptr) {
        b6_free(g_frameDataAllocator, bmp, "stack frame0 pBMP");
        b6_set_frame_bitmap(this, 0, nullptr);
    }
    if (frameData != nullptr) {
        b6_free(g_frameDataAllocator, frameData, "stack framedata");
        frameData = nullptr;
    }
    char *o = reinterpret_cast<char *>(owner);
    if (o != nullptr) {
        if (*reinterpret_cast<int32_t *>(o + 0x90) != 0)
            *reinterpret_cast<int32_t *>(o + 0x90) = 0;
    }
}
HOOK_DTOR(0x00469b10, CStackGump);
