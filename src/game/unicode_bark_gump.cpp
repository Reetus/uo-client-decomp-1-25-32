// CUnicodeBarkGump: floating speech text for 16-bit (unifont) strings.
#include "unicode_bark_gump.h"

#include <cstring>
#include <windows.h>

#include "../runtime/ported.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "unicode_speech_ops_ports.h"
#include "text_list.h"
#include "unicode_text_ports.h"
#include "bark_gump_ui_ports.h"
#include "gump.h"
#include "vtables.h"

// Chat-line history state shared with bark_gump.cpp.
#define g_journal GLOBAL_PTR(void, g_msgTarget_storage)

// 16-bit string helpers of the original (ECX / EDX, no stack args).
#define Wide_Strlen PORT_FUNC1(&WcsLenThunk, int, __fastcall, const uint16_t *)                   // length in characters
#define Wide_Strcpy(a1, a2) (WcsCpy(a1, a2))      // (dst, src)
#define Wide_Widen(a1, a2) (WidenStrCpy(a1, a2))           // (dst, narrow src): copy
#define Wide_Strcat(a1, a2) (WcsCat(a1, a2))      // (dst, src)
#define Wide_WidenCat(a1, a2) (WidenStrCat(a1, a2))        // (dst, narrow src): append
// 0x00479380: thiscall on the journal, (text, hue, flag), RET 0xc.
#define Journal_AddLineUnicode(self, a1, a2, a3) (reinterpret_cast<CTextList *>(self)->AddLineUnicodeAndLog(a1, a2, a3))
// 0x0047cd70: ECX = string, EDX = max width, stack (&width, &height, &zero), RET 0xc.
#define Wide_MeasureWrapped(a1, a2, a3, a4, a5) (MeasureWrappedUnicode((uint16_t *)(a1), a2, a3, a4, a5))
// 0x0047d0b0: ECX = x, EDX = y, stack (hue, text, maxWidth, dest, pitch, clip), RET 0x18.
#define Wide_DrawWrapped(a1, a2, a3, a4, a5, a6, a7, a8) (DrawWrappedUnicode(a1, a2, a3, (uint16_t *)(a4), a5, (uint16_t *)(a6), a7, a8))
// 0x00401a90 (bark_gump_ui.cpp): ECX = bitmap, EDX = width, stack (height, colour), RET 8.
#define Bitmap_Outline(a1, a2, a3, a4) (CBark_BitmapOutline((uint16_t *)(a1), a2, a3, (uint32_t)(a4)))
// 0x0043b4e0: thiscall on a gump, (&x, &y): screen origin.
#define Gump_ToScreen(self, a1, a2) (reinterpret_cast<CGump *>(self)->LocalToScreen((int *)(a1), (int *)(a2)))

// 0x00402850  size 0x61c
// Constructor (stack args, RET 0x18; the SEH frame is omitted). Same placement and stacking logic as
// CBarkGump::Ctor, with the text kept as a 16-bit string and measured / drawn by the unifont routines
// (lines are 0x12 high; the bitmap is outlined). The line count for the display time is height / 15.
CUnicodeBarkGump::CUnicodeBarkGump(CObj *speaker_, uint16_t *text_, uint16_t hue_, uint16_t font_, int32_t extra,
                                   CGump *param5_)
{
    uint16_t buf[0x400];
    int youSee;
    int zeroOut;
    int rect[4];
    int maxWidth;
    int lines;

    youSee = 0;
    text = static_cast<char *>(::operator new(static_cast<size_t>(Wide_Strlen(text_)) * 2 + 2));
    Wide_Strcpy(reinterpret_cast<uint16_t *>(text), text_);
    hue = hue_;
    font = font_;
    unk_dc = extra;
    unk_c8 = 0;
    unk_c4 = 0;
    unk_c0 = 0;
    unk_bc = 0;
    unk_d0 = 0;
    unk_cc = 0;

    buf[0] = 0;
    if (g_barkState == 2) {
        Wide_Widen(buf, g_barkPrefix);
        Wide_WidenCat(buf, g_barkPrefixSep);
    } else if (g_barkState == 3) {
        Wide_Widen(buf, "You see: ");
        youSee = 1;
    }
    Wide_Strcat(buf, text_);
    int hueInt = static_cast<int>(hue_);
    Journal_AddLineUnicode(g_journal, buf, hueInt, g_barkState == 1);
    g_barkState = 1;
    strcpy(g_barkPrefix, g_emptyString);

    name = const_cast<char *>("unicode bark gump");
    Wide_Strlen(text_);   // result unused
    param5 = param5_;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    owner = nullptr;

    if (speaker_ == nullptr) {
        maxWidth = 0x140;
        older = g_barkListHead;
        newer = nullptr;
        if (g_barkListHead != nullptr)
            g_barkListHead->newer = this;
        speaker = nullptr;
        g_barkListHead = this;
        x = 0;
        y = 0x1cc;
    } else {
        maxWidth = 200;
        older = RAWF(CBarkGump *, speaker_, 0x68);
        newer = nullptr;
        if (RAWF(CBarkGump *, speaker_, 0x68) != nullptr)
            RAWF(CBarkGump *, speaker_, 0x68)->newer = this;
        speaker = speaker_;
        RAWF(CBarkGump *, speaker_, 0x68) = this;

        if (PORT_VSLOT0(speaker_, 0x2c, int)() != 0 && RAWF(void *, speaker_, 0x7c) != nullptr) {
            CGump *sib = RAWF(CGump *, RAWF(void *, speaker_, 0x7c), 0xac);
            if (sib == nullptr) {
                // No anchor gump for this speaker: delete the half-built bark.
                PORT_VSLOT1(this, 0x00, void, int)(1);
                return;
            }
            unk_a0 = 0;
            unk_9c = 0;
            Gump_ToScreen(sib, &unk_9c, &unk_a0);
            unk_9c += sib->unk_6c;
            unk_a0 += sib->unk_70;
            x = unk_9c;
            y = unk_a0;
        } else if (PORT_VSLOT0(speaker_, 0x24, int)() != 0) {
            unk_9c = ((RAWF(int32_t, speaker_, 0x0c) + RAWF(int32_t, speaker_, 0x04)) >> 1) - g_scrollX;
            unk_a0 = (RAWF(int32_t, speaker_, 0x08) - g_scrollY) + 4;
            unk_a0 = RAWF(int32_t, speaker_, 0x1cc) + unk_a0;
            x = unk_9c;
            y = unk_a0;
        } else {
            unk_9c = ((RAWF(int32_t, speaker_, 0x0c) + RAWF(int32_t, speaker_, 0x04)) >> 1) - g_scrollX;
            unk_a0 = (RAWF(int32_t, speaker_, 0x08) - g_scrollY) + 4;
            if (youSee != 0 && PORT_VSLOT0(speaker_, 0x54, int)() == 0) {
                x = g_screenGump->unk_6c;
                y = g_screenGump->unk_70;
            } else {
                x = unk_9c;
                y = unk_a0;
            }
        }
    }

    frameIndex = 0;
    frameCount = 1;
    frameData = FrameData_Alloc(g_frameDataAllocator, 8, "framedata for CBarkGump");
    frameData[0] = 0;
    frameData[1] = 0;

    // The original also builds a rect {x, 0, x + maxWidth, 0} here and shifts it by one; it is rebuilt
    // before its only use below, so it is not reproduced.
    Wide_MeasureWrapped(text_, maxWidth, &width, &height, &zeroOut);

    width = width + 2;
    height = height + 2;
    lines = height / 15;
    if (speaker_ != nullptr)
        x = x + -(width / 2);
    homeX = x;
    if (x + width > 0x27f)
        x = 0x27f - width;
    if (x < 0)
        x = 0;
    y = y - height;
    homeY = y;
    if (y + height > 0x1df)
        y = 0x1df - height;
    if (y < 0)
        y = 0;

    rowStride = width;
    uint8_t *bitmap = static_cast<uint8_t *>(::operator new(static_cast<size_t>(height) * width * 2 + 4));
    frameData[1] = static_cast<int32_t>(reinterpret_cast<uintptr_t>(bitmap));
    *reinterpret_cast<uint16_t *>(bitmap + 0) = static_cast<uint16_t>(width);
    *reinterpret_cast<uint16_t *>(bitmap + 2) = static_cast<uint16_t>(height);
    memset(bitmap + 4, 0, static_cast<size_t>(rowStride) * height * 2);

    rect[0] = 0;
    rect[1] = 0;
    rect[2] = rowStride;
    rect[3] = height;
    Wide_DrawWrapped(1, 1, hueInt, reinterpret_cast<uint16_t *>(text), rowStride, bitmap + 4, rowStride, rect);
    Bitmap_Outline(bitmap + 4, width, height, 1);   // 0x00401a90

    // Push every older bark of this speaker up by this bark's height.
    for (CBarkGump *s = older; s != nullptr; s = s->older) {
        s->homeY -= height;
        s->y = s->homeY;
        if (s->homeY < 0)
            s->y = 0;
        if (s->height + s->y > 0x1e0)
            s->y = 0x1e0 - s->height;
    }

    StackAndClamp();   // 0x00402230

    // Union rectangle of this bark and the older ones. It is computed but not used further.
    rect[0] = x;
    rect[1] = y;
    rect[2] = width + x;
    rect[3] = height + y;
    for (CBarkGump *s = older; s != nullptr; s = s->older) {
        if (s->x < rect[0])
            rect[0] = s->x;
        if (s->width + s->x > rect[2])
            rect[2] = s->width + s->x;
        if (s->y < rect[1])
            rect[1] = s->y;
        if (s->height + s->y > rect[3])
            rect[3] = s->height + s->y;
    }

    tickStart = static_cast<int32_t>(GetTickCount());
    if (g_barkMultiLine != 0)
        duration = (lines * g_barkSecondsPerLine * 4000) / 100;
    else
        duration = (g_barkSecondsPerLine * 4000) / 100;
    CGump_AddChild(this, nullptr, 0);
}
HOOK_CTOR(0x00402850, CUnicodeBarkGump, (CObj *, uint16_t *, uint16_t, uint16_t, int32_t, CGump *));

HOOK_VTABLE(0x0049c230, CUnicodeBarkGump);
HOOK_SDD(0x00402e70, CUnicodeBarkGump);

// 0x00402ea0  size 0xb
// Sets the vtable, then jumps into ~CBarkGump (0x00401950): an empty body.
CUnicodeBarkGump::~CUnicodeBarkGump()
{
}
HOOK_DTOR(0x00402ea0, CUnicodeBarkGump);

// 0x00402eb0  size 0x4c
// Redraw the wrapped 16-bit text into the bitmap with the given colour index and clip rect, then outline it.
void __thiscall CUnicodeBarkGump::RenderText(int hue_, int *rect)
{
    uint8_t *pixels = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(frameData[1])) + 4;
    Wide_DrawWrapped(1, 1, hue_, reinterpret_cast<uint16_t *>(text), rowStride, pixels, rowStride, rect);
    Bitmap_Outline(reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(frameData[1])) + 4, width, height, 1);   // 0x00401a90
}
HOOK_VMETHOD(0x00402eb0, CUnicodeBarkGump, RenderText);
