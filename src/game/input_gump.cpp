// CInputGump: the typed-text strip at the bottom of the screen.
#include "input_gump.h"

#include <cstring>
#include <windows.h>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "globals.h"
#include "crt.h"
#include "font_manager.h"
#include "gump_base_ops_ports.h"
#include "unicode_font_ports.h"
#include "bark_gump_ui_ports.h"
#include "vtables.h"


// 0x004320a0: thiscall on the text object, (font): non-zero when the font exists.
#define Text_HasFont(self, a1) (reinterpret_cast<CFontManager *>(self)->HasFont(a1))
// 0x0043a990: thiscall on the gump manager, (&dst, &src), RET 8: grow dst to contain src.
#define Mgr_RectUnion(o, a1, a2) PORT_THISCALL2(&GumpRectUnion, void, (o), int *, int *)(a1, a2)
// 0x0047cc80: ECX = 16-bit string, EDX = &width, stack (&height, &yOffset), RET 8.
#define Unicode_Measure(a1, a2, a3, a4) (MeasureUnicodeString((const uint16_t *)(a1), a2, a3, a4))
// 0x0047cae0: ECX = dest pixels, EDX = dest width, stack (text, 1, y, hue, clip), RET 0x14.
#define Unicode_Draw(a1, a2, a3, a4, a5, a6, a7) (DrawUnicodeString((uint16_t *)(a1), a2, (const uint16_t *)(a3), a4, a5, (uint16_t)(a6), (const int *)(a7)))
// 0x00401a90 (bark_gump_ui.cpp): ECX = bitmap, EDX = width, stack (height, colour), RET 8.
#define Bitmap_Outline(a1, a2, a3, a4) (CBark_BitmapOutline((uint16_t *)(a1), a2, a3, (uint32_t)(a4)))

// Allocate the pixel buffer (4-byte size header + width * height 16-bit pixels) for the current size,
// store the size in the header and clear the pixels. Shared tail of both redraws.
static inline void InputGump_AllocBitmap(CInputGump *g)
{
    g->frameData[1] = static_cast<int32_t>(reinterpret_cast<uintptr_t>(
        ::operator new(static_cast<size_t>(g->height * g->width) * 2 + 4)));
    *reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(g->frameData[1])) = static_cast<uint16_t>(g->width);
    *reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(g->frameData[1]) + 2) = static_cast<uint16_t>(g->height);
    memset(reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(g->frameData[1])) + 4, 0, static_cast<size_t>(g->rowStride * g->height) * 2);
}

// 0x00402360  size 0xc1
// Constructor (stack args, RET 0x10; the SEH frame is omitted). Docks the gump to the bottom of the
// screen with an empty one-slot frame table, links it in and draws the text.
CInputGump::CInputGump(void *text_, uint32_t hue, uint32_t font, int unicode)
{
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    owner = nullptr;
    x = 0;
    y = 0x1e0;
    frameIndex = 0;
    frameCount = 1;
    frameData = FrameData_Alloc(g_frameDataAllocator, 8, "framedata for CInputGump");
    frameData[0] = 0;
    frameData[1] = 0;
    CGump_AddChild(this, nullptr, 0);
    if (unicode != 0)
        RedrawUnicode(static_cast<uint16_t *>(text_), static_cast<uint16_t>(hue), static_cast<uint16_t>(font));   // 0x004026b0
    else
        Redraw(static_cast<char *>(text_), static_cast<uint16_t>(hue), static_cast<uint16_t>(font));   // 0x004024d0
}
HOOK_CTOR(0x00402360, CInputGump, (void *, uint32_t, uint32_t, int));

HOOK_VTABLE(0x0049c118, CInputGump);
HOOK_SDD(0x00402430, CInputGump);

// 0x00402460  size 0x62
// Destructor body: frees the pixel buffer (unconditionally, as the original does) and runs the base
// destructor. The SEH frame is omitted.
CInputGump::~CInputGump()
{
    operator delete(reinterpret_cast<void *>(static_cast<uintptr_t>(frameData[1])));
    frameData[1] = 0;
}
HOOK_DTOR(0x00402460, CInputGump);

// 0x004024d0  size 0x1db
// Redraw from an 8-bit string. With the frame-rate display on, the text becomes "%3d fps <text>". The
// gump is raised, its old pixel buffer freed, the text measured with `font` and drawn into a new buffer,
// and the union of the old and new screen rect is added to the dirty list.
void __thiscall CInputGump::Redraw(char *text_, uint16_t hue, uint16_t font)
{
    char fpsText[1024];
    int rectOld[4];
    int rectDraw[4];
    int out1;   // the original leaves these uninitialised when the font is missing
    int out2;
    int out3;

    CGump_BringToFront(this, 1);
    if (frameData[1] != 0) {
        operator delete(reinterpret_cast<void *>(static_cast<uintptr_t>(frameData[1])));
        frameData[1] = 0;
    }
    if (g_showFps != 0) {
        uint32_t now = GetTickCount();
        uint32_t delta = now - g_lastFpsTick;
        g_lastFpsTick = now;
        uint32_t fps = (delta == 0) ? 0 : 1000 / delta;
        sprintf(fpsText, "%3d fps %s", fps, text_);
        text_ = fpsText;
    }
    rectOld[0] = 0;
    rectOld[1] = y;
    rectOld[2] = width;
    rectOld[3] = height + y;
    int fontId = static_cast<int>(font);
    rectDraw[0] = 0;
    rectDraw[2] = 0x280;
    rectDraw[1] = 0;
    rectDraw[3] = 0;
    if (Text_HasFont(gfx::textObj(), fontId) != 0) {
        gfx::TextMeasure(fontId, text_, &out1, &out2, &out3);
        int w = out1;
        if (w < 2)
            w = 2;
        int h = out2;
        width = w;
        if (h < 1)
            h = 1;
        height = h;
        rectDraw[2] = w - 1;
        rectDraw[3] = h - 1;
    }
    x = 0;
    y = 0x1e0 - height;
    rowStride = width;
    InputGump_AllocBitmap(this);   // operator new 0x0048ad70, header, clear
    g_inputRedrawing = 1;
    gfx::TextDraw(0, out3, fontId, static_cast<int>(hue), text_,
                  reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(frameData[1])) + 4, rowStride, rectDraw);
    g_inputRedrawing = 0;
    if (rectDraw[1] < 0)
        rectDraw[1] = 0;
    rectDraw[3] = height + y;
    Mgr_RectUnion(g_gumpManager, rectDraw, rectOld);
}
HOOK_METHOD(0x004024d0, CInputGump, Redraw);

// 0x004026b0  size 0x192
// Redraw from a 16-bit string: measured and drawn with the unifont routines, colour taken from the
// font hue table entry `hueIndex`, outlined with colour 1. The third stack arg is never read.
void __thiscall CInputGump::RedrawUnicode(uint16_t *text_, uint16_t hueIndex, uint16_t)
{
    int rectOld[4];
    int rectDraw[4];
    int mw;
    int mh;
    int yOff;

    CGump_BringToFront(this, 1);
    if (frameData[1] != 0) {
        operator delete(reinterpret_cast<void *>(static_cast<uintptr_t>(frameData[1])));
        frameData[1] = 0;
    }
    rectOld[0] = 0;
    rectOld[1] = y;
    rectOld[2] = width;
    rectOld[3] = y + height;
    rectDraw[0] = 0;
    rectDraw[2] = 0x280;
    rectDraw[1] = 0;
    rectDraw[3] = 0;
    Unicode_Measure(text_, &mw, &mh, &yOff);
    mw += 2;
    mh += 2;
    int w = mw;
    if (w < 2)
        w = 2;
    width = w;
    int h = mh;
    if (h < 1)
        h = 1;
    height = h;
    rectDraw[2] = w - 1;
    rectDraw[3] = h - 1;
    x = 0;
    y = 0x1e0 - h;
    rowStride = w;
    frameData[1] = static_cast<int32_t>(reinterpret_cast<uintptr_t>(
        ::operator new(static_cast<size_t>(height * width) * 2 + 4)));
    *reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(frameData[1]) + 0) = static_cast<uint16_t>(width);
    *reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(frameData[1]) + 2) = static_cast<uint16_t>(height);
    memset(reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(frameData[1])) + 4, 0, static_cast<size_t>(rowStride * height) * 2);
    g_inputRedrawing = 1;
    Unicode_Draw(reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(frameData[1])) + 4, rowStride, text_, 1, yOff + 1,
                 *reinterpret_cast<uint16_t *>(g_fontHueTable + static_cast<unsigned>(hueIndex) * 0x58), rectDraw);
    Bitmap_Outline(reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(frameData[1])) + 4, width, height, 1);   // 0x00401a90
    g_inputRedrawing = 0;
    if (rectDraw[1] < 0)
        rectDraw[1] = 0;
    rectDraw[3] = height + y;
    Mgr_RectUnion(g_gumpManager, rectDraw, rectOld);
}
HOOK_METHOD(0x004026b0, CInputGump, RedrawUnicode);
