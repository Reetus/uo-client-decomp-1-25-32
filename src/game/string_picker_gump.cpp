// CStringPickerGump ("stringpicker gump") and the CStringQueryGump destructors: constructor, destructors,
// hit test, mouse up and the radio group handler (0x0046f7e0 .. 0x0046fe10, 0x004702a0, 0x004702d0).
#include "string_query_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch06_helpers.h"
#include "button_gump.h"
#include "font_manager.h"
#include "globals.h"
#include "gump_frame.h"
#include "toggle_control.h"
#include "char_creation_pages.h"
#include "vtables.h"
#include "crt.h"

#define Sp_cancelHandler PORT_ADDR(&CGumpHandlers04::MenuCancel)
#define Sp_okHandler PORT_ADDR(&CGumpHandlers04::MenuOk)

// 0x0046f7e0  size 0x3d8  CStringPickerGump::CStringPickerGump(title, serial, type, count, ids, strings)
// Measures the title and every string (wrapped to width - 0x46) to size the gump, centres it on the 640x480
// screen, paints the frame art 0x13ec and the title into a new frame bitmap, adds one radio toggle per
// string with its text drawn beside it, then a cancel (0x00412740) and an OK (0x00412790) button.
// The allocation-failure paths still pass the null control on, as the original does.
CStringPickerGump::CStringPickerGump(char *title, unsigned int serial_, uint16_t type_, int count, uint16_t *ids_,
                                     char **strings)
{
    int tw, th, ty;
    int rect[4];
    int wrapW;
    int rowY;

    serial = serial_;
    type = type_;
    name = const_cast<char *>("stringpicker gump");
    dirty = 0;
    unk_50 = 0x13ec;   // gump art id
    unk_60 = 0;
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
    width = 0x190;
    g_fontMgr.MeasureTextWrapped(1, title, 0x168, &tw, &th, &ty, 0);   // 0x004328e0
    wrapW = width - 0x46;
    rowY = th + 0x24;
    height = th + 0x6a;
    if (count > 0) {
        uint16_t *src = ids_;
        uint16_t *dst = ids;
        char **str = strings;
        int left = count;
        do {
            g_fontMgr.MeasureTextWrapped(1, *str, wrapW, &tw, &th, &ty, 0);   // 0x004328e0
            height = height + (th + 6);
            str++;
            *dst = *src;
            src++;
            dst++;
            left--;
        } while (left != 0);
    }
    x = (0x280 - width) / 2;
    y = (0x1e0 - height) / 2;
    frameIndex = 0;
    frameCount = 1;
    frameData = static_cast<int32_t *>(b6_alloc(g_frameDataAllocator, 8, "framedata for CDumbGump"));   // 0x004462f0
    memset(frameData, 0, frameCount * 8);
    b6_set_frame_bitmap(this, 0, b6_alloc(g_frameDataAllocator, height * width * 2 + 4, "dumb frame0"));   // 0x004462f0
    memset(b6_frame_bitmap(this, 0)->px, 0, (height * width) * 2);
    b6_frame_bitmap(this, 0)->w = static_cast<uint16_t>(width);
    b6_frame_bitmap(this, 0)->h = static_cast<uint16_t>(height);
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = width;
    rect[3] = height;
    PaintGumpFrame(unk_50, reinterpret_cast<uint16_t *>(b6_frame_bitmap(this, 0)), 0);   // 0x0041f540
    g_fontMgr.DrawTextWrapped(0x14, 0x10, 1, 0x386, title, width - 0x28, b6_frame_bitmap(this, 0)->px, width, rect, 0,
                              0);   // 0x00432260

    if (count > 0) {
        char **str = strings;
        int left = count;
        do {
            CToggleControl *ctl = new CToggleControl(0x14, rowY, reinterpret_cast<int>(this), 0x138a, 0x138b, 1, 0x386,
                                                        reinterpret_cast<const char *>(GADDR(g_emptyString)), 0, 0, 0);   // 0x00416120
            ctl->LinkInto(this, 1);   // 0x0043bcc0
            g_fontMgr.MeasureTextWrapped(1, *str, wrapW, &tw, &th, &ty, 0);   // 0x004328e0
            g_fontMgr.DrawTextWrapped(0x32, rowY + 2, 1, 0x386, *str, wrapW, b6_frame_bitmap(this, 0)->px, width, rect,
                                      0, 0);   // 0x00432260
            str++;
            left--;
            rowY = rowY + th + 6;
        } while (left != 0);
    }

    CButtonGump *made = new CButtonGump(0x28, height - 0x2c, reinterpret_cast<int>(this), 0, 0, Sp_cancelHandler, 0x1450, 0x1451, 0);   // 0x00412fb0
    made->LinkInto(this, 1);   // 0x0043bcc0
    made = new CButtonGump(0xa0, height - 0x2c, reinterpret_cast<int>(this), 0, 0, Sp_okHandler, 0x13b2, 0x13b3, 0);   // 0x00412fb0
    made->LinkInto(this, 1);   // 0x0043bcc0
}
HOOK_CTOR(0x0046f7e0, CStringPickerGump, (char *, unsigned int, uint16_t, int, uint16_t *, char **));

HOOK_VTABLE(0x004a03d0, CStringPickerGump);
HOOK_SDD(0x0046fbc0, CStringPickerGump);

// 0x0046fbf0  size 0x92  CStringPickerGump::~CStringPickerGump
// Clears 0x006488d4 when it is this gump, frees the frame bitmap and frame data, runs the CGump destructor.
// The SEH frame is omitted.
CStringPickerGump::~CStringPickerGump()
{
    if (g_scrollChoiceGump == this)
        g_scrollChoiceGump = nullptr;
    if (b6_frame_bitmap(this, 0) != nullptr) {
        b6_free(g_frameDataAllocator, b6_frame_bitmap(this, 0), "stringpicker frame0 pBMP");   // 0x00446310
        b6_set_frame_bitmap(this, 0, nullptr);
    }
    if (frameData != nullptr) {
        b6_free(g_frameDataAllocator, frameData, "stringpicker framedata");   // 0x00446310
        frameData = nullptr;
    }
}
HOOK_DTOR(0x0046fbf0, CStringPickerGump);

// 0x0046fd50  size 0x4d  vtable slot 0x0c, RET 8
// 1 when (px, py) is inside the frame bitmap and its pixel is non-zero, else 0.
int __thiscall CStringPickerGump::HitTest(int px, int py)
{
    B6Bitmap *bmp = b6_frame_bitmap(this, 0);
    int w = bmp->w;
    int h = bmp->h;
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return bmp->px[w * py + px] != 0 ? 1 : 0;
}
HOOK_VMETHOD(0x0046fd50, CStringPickerGump, HitTest);

// 0x0046fda0  size 0x16  RET 8 (arguments not read)
// Runs the cancel handler 0x00412740 with this (it sends the reply and deletes the gump), then clears the
// drag gump.
void __thiscall CStringPickerGump::RButtonUp(int a, int b)
{
    (void)a;
    (void)b;
    PORT_THISCALL0(&CGumpHandlers04::MenuCancel, void, this)();   // 0x00412740
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x0046fda0, CStringPickerGump, RButtonUp);

// 0x0046fdc0  size 0x42  RET 8 (arguments not read)
void __thiscall CStringPickerGump::MouseUp(int a, int b)
{
    (void)a;
    (void)b;
    unk_9c = 0;
    if (unk_94 != 0) {
        unk_94 = 0;
        unk_a8 = 0;
        g_dragGump = nullptr;
        return;
    }
    if (unk_a8 == 2)
        unk_a8 = 0;
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x0046fdc0, CStringPickerGump, MouseUp);

// 0x0046fe10  size 0x77  RET 4
// `clicked` is a child that answers true to vtable slot 0xc4 (a radio toggle): when it is unchecked, check it
// and redraw it; otherwise uncheck and redraw every other such child that is checked.
void __thiscall CStringPickerGump::OnChildChanged(int child)
{
    CGump *clicked = reinterpret_cast<CGump *>(child);
    CToggleControl *tog = static_cast<CToggleControl *>(clicked);
    if (PORT_VSLOT0(clicked, 0xc4, int)() != 0 && tog->checked == 0) {
        tog->checked = 1;
        tog->Redraw();   // 0x004163f0
        return;
    }
    for (CGump *c = lastChild; c != nullptr; c = c->prev) {
        if (PORT_VSLOT0(c, 0xc4, int)() != 0 && c != clicked && static_cast<CToggleControl *>(c)->checked != 0) {
            static_cast<CToggleControl *>(c)->checked = 0;
            static_cast<CToggleControl *>(c)->Redraw();   // 0x004163f0
        }
    }
}
HOOK_VMETHOD(0x0046fe10, CStringPickerGump, OnChildChanged);

HOOK_VTABLE(0x004a04e8, CStringQueryGump);
HOOK_SDD(0x004702a0, CStringQueryGump);

// 0x004702d0  size 0xa8  CStringQueryGump::~CStringQueryGump
// Clears 0x006488d4 when it is this gump, deletes the edit control, frees the frame bitmap and frame data,
// runs the CGump destructor. The SEH frame is omitted.
CStringQueryGump::~CStringQueryGump()
{
    if (g_scrollChoiceGump == this)
        g_scrollChoiceGump = nullptr;
    if (editControl != nullptr) {
        PORT_VSLOT1(editControl, 0x00, void, int)(1);
        editControl = nullptr;
    }
    if (b6_frame_bitmap(this, 0) != nullptr) {
        b6_free(g_frameDataAllocator, b6_frame_bitmap(this, 0), "StringQuery frame0 pBMP");   // 0x00446310
        b6_set_frame_bitmap(this, 0, nullptr);
    }
    if (frameData != nullptr) {
        b6_free(g_frameDataAllocator, frameData, "StringQuery framedata");   // 0x00446310
        frameData = nullptr;
    }
}
HOOK_DTOR(0x004702d0, CStringQueryGump);
