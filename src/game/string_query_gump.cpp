#include "string_query_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch06_helpers.h"
#include "globals.h"
#include "input_control_gump.h"
#include "button_gump.h"
#include "font_manager.h"
#include "gump.h"
#include "gump_base_ops_ports.h"
#include "gump_art_ops_ports.h"
#include "char_creation_pages.h"
#include "vtables.h"
#include "crt.h"

// 0x0041e0f0 on the control, no args.
#define EditControl_Init(self) (reinterpret_cast<CInputControlGump *>(self)->Redraw())
// 0x00412fb0 button ctor: (x, y, parent, 0, 0, callback, upArt, downArt, 0)
// 0x00432260 wrapped text: (x, y, font, color, text, wrapWidth, pixels, stride, clip, 0, 0)
#define b6_text_draw_wrapped(self, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) (reinterpret_cast<CFontManager *>(self)->DrawTextWrapped(a1, a2, a3, a4, (char *)(a5), a6, (uint16_t *)(a7), a8, (const int *)(a9), a10, a11))
// 0x0043b4e0 on a gump: screen origin -> (&x, &y).
#define Gump_GetOrigin(self, a1, a2) (reinterpret_cast<CGump *>(self)->LocalToScreen(a1, a2))
// 0x0043b540 on the gump manager: (clip, rect) -> non-zero when the rect intersects the clip.
#define GumpManager_Intersects(o, a1, a2) PORT_THISCALL2(&GumpRectIntersects, int, (o), void *, int *)(a1, a2)
// 0x004342e0 pixel copy with clip: ECX = row blit, EDX = pixels, stack = (clip, w, h, x, y, dest, destStride, destHeight).
#define b6_blit_clipped PORT_FUNC10(&GumpBlitBitmapClipped, void, __fastcall, B6RowBlit, uint16_t *, void *, int, int, int, int, uint16_t *, int, int)

#define g_activeStringEdit GLOBAL_AS(uint8_t *, g_focusedInput)
#define g_stringQueryGump GLOBAL_AS(CStringQueryGump *, g_scrollChoiceGump)

// 0x0046fe90  size 0x402  CStringQueryGump::CStringQueryGump(serial, type, title, flags, kind, arg6, text)
// kind 1 and kind 2 build the same edit control, differing only in the sixth constructor argument (0 / 1).
// Allocation-failure paths keep the original's unconditional stores. The SEH frame is omitted.
CStringQueryGump::CStringQueryGump(unsigned int serial_, uint16_t type_, char *title, uint8_t flags_,
                                   uint8_t kind_, unsigned int arg6, char *text)
{
    int rect[4];
    unsigned char bl = 5;

    unk_94 = 0;
    name = const_cast<char *>("Stringquery gump");
    dirty = 0;
    unk_50 = 0x474;   // gump art id
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    b6_art_size(0x474, &width, &height);
    x = 0x79;
    y = 0x66;
    frameIndex = 0;
    frameCount = 1;
    frameData = static_cast<int32_t *>(b6_alloc(g_frameDataAllocator, 8, "framedata for CStringQueryGump"));
    memset(frameData, 0, frameCount * 8);
    b6_set_frame_bitmap(this, 0, b6_alloc(g_frameDataAllocator, height * width * 2 + 4, "dumb frame0"));
    b6_frame_bitmap(this, 0)->w = static_cast<uint16_t>(width);
    b6_frame_bitmap(this, 0)->h = static_cast<uint16_t>(height);
    type = type_;
    serial = serial_;
    kind = kind_;
    flags = flags_;
    memset(b6_frame_bitmap(this, 0)->px, 0, (height * width) * 2);
    b6_draw_art(B6_ROWBLIT_COPY, unk_50, 0, 0, b6_frame_bitmap(this, 0)->px, width, height);

    rect[0] = 0;
    rect[1] = 0;
    rect[2] = width;
    rect[3] = height;
    b6_text_draw_wrapped(b6_font, 0x3c, 0x32, 2, 0x386, title, width - 0x78, b6_frame_bitmap(this, 0)->px, width, rect, 0, 0);
    b6_text_draw(b6_font, 0x3c, 0x7d, 2, 0x386, text, b6_frame_bitmap(this, 0)->px, width, rect);

    if (kind == 1 || kind == 2) {
        uint8_t *ctl;
        if (kind == 1)
            ctl = reinterpret_cast<uint8_t *>(new CInputControlGump(0x3c, 0x82, this, 0x475, 0x475, 0, arg6, 1, 0x386, 1, 0, 0));
        else
            ctl = reinterpret_cast<uint8_t *>(new CInputControlGump(0x3c, 0x82, this, 0x475, 0x475, 1, arg6, 1, 0x386, 1, 0, 0));
        editControl = ctl;
        *reinterpret_cast<uint16_t *>(ctl + 0xde) = 0x476;   // ? null deref if the allocation failed, as in the original
        *reinterpret_cast<uint16_t *>(editControl + 0xe0) = 0x477;
        *(editControl + 0xe2) = 0xc;
        *(editControl + 0xe3) = bl;
        CGump_AddChild(editControl, this, 1);
        *reinterpret_cast<int32_t *>(editControl + 0xd8) = 1;
        g_activeStringEdit = editControl;
        EditControl_Init(editControl);
    }

    uint8_t *btn;
    if ((flags & 1) != 0) {
        // OK + Cancel
void * p;
                btn = (uint8_t *)new CButtonGump(0xee - x, 0x124 - y, (int)(this), 0, 0, (int)(reinterpret_cast<void *>(PORT_ADDR(&CGumpHandlers04::TextEntryOk))), 0x47b, 0x47c, 0);
        *reinterpret_cast<int32_t *>(btn + 0xa4) = 0x47d;   // ? null deref if the allocation failed
        CGump_AddChild(btn, this, 1);
        btn = (uint8_t *)new CButtonGump(0x145 - x, 0x124 - y, (int)(this), 0, 0, (int)(reinterpret_cast<void *>(PORT_ADDR(&CGumpHandlers04::TextEntryCancel))), 0x478, 0x479, 0);
        *reinterpret_cast<int32_t *>(btn + 0xa4) = 0x47a;
    } else {
        // single OK button
        btn = (uint8_t *)new CButtonGump(0x11a - x, 0x124 - y, (int)(this), 0, 0, (int)(reinterpret_cast<void *>(PORT_ADDR(&CGumpHandlers04::TextEntryOk))), 0x47b, 0x47c, 0);
        *reinterpret_cast<int32_t *>(btn + 0xa4) = 0x47d;
    }
    CGump_AddChild(btn, this, 1);
    g_stringQueryGump = this;
}
HOOK_CTOR(0x0046fe90, CStringQueryGump, (unsigned int, uint16_t, char *, uint8_t, uint8_t, unsigned int, char *));

// 0x0046fc90  size 0xbc  CStringPickerGump draw(clip) (same shape as 0x0046f3a0)
// Copies the frame bitmap to the back buffer where the gump rect intersects the clip, then draws the children.
void __thiscall CStringPickerGump::Draw(int *clip)
{
    int ox, oy;
    int rc[4];

    if (visible == 0)
        return;
    ox = 0;
    oy = 0;
    Gump_GetOrigin(this, &ox, &oy);   // gump screen origin
    B6Bitmap *bmp = b6_frame_bitmap(this, 0);
    rc[0] = ox;
    rc[1] = oy;
    rc[2] = bmp->w + ox;
    rc[3] = oy + bmp->h;
    if (GumpManager_Intersects(g_gumpManager, clip, rc) != 0) {
        b6_blit_clipped(B6_ROWBLIT_COPY, bmp->px, clip, bmp->w, bmp->h, ox, oy, g_backBuffer, 0x280, 0x1e0);
        CGump *child = firstChild;
        if (child != nullptr) {
            do {
                PORT_VSLOT1(child, kGumpSlot_Draw, void, void *)(clip);
                child = child->next;
            } while (child != nullptr);
        }
    }
}
HOOK_VMETHOD(0x0046fc90, CStringPickerGump, Draw);
