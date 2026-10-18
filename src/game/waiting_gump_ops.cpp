// The "waiting gump" (vtable 0x0049f0a0, 0x2ac bytes; class CGump_0043de30 in waiting_gump.h):
// constructor, destructors and the text renderer. gumps.cxx, 0x0043d740..0x0043dd00.
// The SEH frames of the original (around operator new and the base constructor) are omitted.
#include "waiting_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "batch02_helpers.h"
#include "button_gump.h"
#include "font_manager.h"
#include "globals.h"
#include "batch06_helpers.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "text_entry.h"
#include "mouse_manager.h"
#include "gump_art_ops_ports.h"
#include "input_control_gump.h"
#include "char_flow_gumps.h"
#include "art_row_blits_ports.h"
#include "vtables.h"

#define kWaitingGumpVtable reinterpret_cast<uintptr_t>(vtbl_CGump_0043de30)
#define g_focusGump GLOBAL_AS(CGump *, g_focusedInput)
#define g_mouseMgr GLOBAL_PTR(void, g_mouse_storage)
#define Wg_TextRefresh(self) (reinterpret_cast<CTextEntry *>(self)->RefreshLine())
#define Wg_MouseHold(self) (reinterpret_cast<CMouseManager *>(self)->IncBusy())        // counter at +4 incremented
#define Wg_MouseRelease(self) (reinterpret_cast<CMouseManager *>(self)->DecBusy())     // counter at +4 decremented, floor 0
#define Wg_SetCursor(self, a1) (reinterpret_cast<CMouseManager *>(self)->SetMouseCursor(a1))   // CMouseManager::SetMouseCursor
#define kWaitCloseHandler PORT_ADDR(&CGump_004111c0::Close)

// 0x0043d740  size 0x427
// Constructor, RET 0x1c: (x, y, text, closeTarget, withButtons, smallStyle, p7). The gump takes
// the keyboard focus (remembering the previous focus gump), sizes itself from gump art 0x474 (small
// style) or 0x816, builds two frames (frame0 shown, frame1 the pristine art), renders the text and
// adds a button child. Position -1 centres the gump on 640x480; the small style sits at (0x79, 0x66).
CGump_0043de30::CGump_0043de30(int px, int py, const char *txt, int closeTgt, int withButtons, int smallStyle, int val9c)
{
    x = px;
    y = py;
    name = const_cast<char *>("waiting gump");   // 0x004a847c
    dirty = 0;
    unk_60 = 0;
    noDrag = smallStyle;
    unk_9c = val9c;
    if (smallStyle != 0)
        unk_50 = 0x474;
    else
        unk_50 = 0x816;
    strcpy(text, txt);
    closeTarget = closeTgt;
    prevFocus = g_focusGump;
    if (g_focusGump != this) {
        Gump_CommitFocus();   // 0x0043a720
        void *te = g_textEntry;
        g_focusGump = this;
        if (te != nullptr)
            Wg_TextRefresh(te);   // 0x00443590
    }
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    GumpArtSizeImpl(unk_50, &width, &height);   // 0x00433e70
    if (noDrag != 0) {
        x = 0x79;
        y = 0x66;
    }
    if (x == -1) {
        x = 0x140 - width / 2;
        y = 0xf0 - height / 2;
    }

    frameIndex = 0;
    frameCount = 2;
    int32_t *fd = static_cast<int32_t *>(gfx::AllocTagged(0x10, "framedata for CDumbGump"));
    memset(fd, 0, static_cast<size_t>(frameCount) << 3);
    frameData = fd;

    uint8_t *bmp0 = static_cast<uint8_t *>(gfx::AllocTagged(width * height * 2 + 4, "dumb frame0"));
    frameData[1] = ptr_i32(bmp0);
    memset(bmp0 + 4, 0, static_cast<size_t>(width * height) * 2);
    *reinterpret_cast<uint16_t *>(bmp0) = static_cast<uint16_t>(width);
    *reinterpret_cast<uint16_t *>(bmp0 + 2) = static_cast<uint16_t>(height);

    uint8_t *bmp1 = static_cast<uint8_t *>(gfx::AllocTagged(width * height * 2 + 4, "dumb frame0"));
    frameData[3] = ptr_i32(bmp1);
    memset(bmp1 + 4, 0, static_cast<size_t>(width * height) * 2);
    *reinterpret_cast<uint16_t *>(bmp1) = static_cast<uint16_t>(width);
    *reinterpret_cast<uint16_t *>(bmp1 + 2) = static_cast<uint16_t>(height);

    uint8_t *b1 = static_cast<uint8_t *>(i32_ptr(frameData[3]));
    DrawGumpArt(reinterpret_cast<RowBlitFn>(static_cast<uintptr_t>(PORT_ADDR(&RowBlitCopyNonZero))), unk_50, 0, 0,
                reinterpret_cast<uint16_t *>(b1 + 4), width, height);   // 0x00433ed0
    RenderText();   // 0x0043dd00
    cursorHeld = 0;

    CButtonGump *btn;
    if (noDrag != 0) {
        if (withButtons != 0) {
            cursorHeld = 1;
            Wg_MouseHold(g_mouseMgr);   // 0x004468b0
            int savedInput = g_inputActive;
            g_inputActive = 1;
            Wg_SetCursor(g_mouseMgr, 0x11);   // 0x00446840
            g_inputActive = savedInput;
            btn = new CButtonGump(0x11a - x, 0x124 - y, ptr_i32(this), 0, 0, kWaitCloseHandler, 0x478, 0x479, 0);   // 0x00412fb0
            btn->artHover = 0x47a;   // ? not null-checked in the original either
        } else {
            btn = new CButtonGump(0x11a - x, 0x124 - y, ptr_i32(this), 0, 0, kWaitCloseHandler, 0x47b, 0x47c, 0);   // 0x00412fb0
            btn->artHover = 0x47d;
        }
    } else {
        if (withButtons != 0) {
            btn = new CButtonGump(0x41, 0x4b, ptr_i32(this), 0, 0, kWaitCloseHandler, 0x817, 0x818, 0);   // 0x00412fb0
            btn->artHover = 0x819;
        } else {
            btn = new CButtonGump(0x41, 0x4b, ptr_i32(this), 0, 0, kWaitCloseHandler, 0x81a, 0x81b, 0);   // 0x00412fb0
            btn->artHover = 0x81c;
        }
    }
    btn->LinkInto(this, 1);   // 0x0043bcc0
}
HOOK_CTOR(0x0043d740, CGump_0043de30, (int, int, const char *, int, int, int, int));

HOOK_VTABLE(0x0049f0a0, CGump_0043de30);
HOOK_SDD(0x0043db70, CGump_0043de30);

// 0x0043dba0  size 0x151
// Destructor body: release the wait cursor, clear the globals that point at this gump, hand the
// keyboard focus back to the remembered gump (or commit it), free both frame bitmaps and the frame
// table, then the CGump base. SEH frame omitted.
CGump_0043de30::~CGump_0043de30()
{
    if (cursorHeld != 0) {
        Wg_MouseRelease(g_mouseMgr);   // 0x004468c0
        int savedInput = g_inputActive;
        g_inputActive = 1;
        Wg_SetCursor(g_mouseMgr, 0x11);   // 0x00446840
        g_inputActive = savedInput;
    }
    if (g_scrollChoiceGump == this)
        g_scrollChoiceGump = nullptr;
    if (g_loginObj648914 == this)
        g_loginObj648914 = nullptr;

    CGump *pf = prevFocus;
    if (pf != nullptr) {
        if (g_focusGump != pf) {
            Gump_CommitFocus();   // 0x0043a720
            void *te = g_textEntry;
            g_focusGump = pf;
            if (te != nullptr)
                Wg_TextRefresh(te);   // 0x00443590
        }
        if (decomp::vcall(PORT_RET(int), prevFocus, 0xc8) != 0)
            reinterpret_cast<CInputControlGump *>(prevFocus)->Redraw();   // CInputControlGump::Redraw
    } else {
        Gump_CommitFocus();   // 0x0043a720
    }

    int32_t b0 = frameData[1];
    if (b0 != 0) {
        gfx::FreeTagged(i32_ptr(b0), "waiting frame0 pBMP");   // 0x00446310
        frameData[1] = 0;
    }
    int32_t b1 = frameData[3];
    if (b1 != 0) {
        gfx::FreeTagged(i32_ptr(b1), "waiting frame1 pBMP");   // 0x00446310
        frameData[3] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "waiting framedata");   // 0x00446310
        frameData = nullptr;
    }
}
HOOK_DTOR(0x0043dba0, CGump_0043de30);

// 0x0043dd00  size 0x12b
// Copy frame1 (the bare art) over frame0, then draw the text into frame0: wrapped into
// {0x3e, 0x2e, 0x14a, 0xaa} (after measuring it at width 0x10c) in the large style, or into
// {0x21, 0x1e, 0x94, 0x3f} at wrap width 0x73 in the small style. Font id from 0x004a8360 (2 in the image), hue 0x386.
void __thiscall CGump_0043de30::RenderText()
{
    uint8_t *f0 = static_cast<uint8_t *>(i32_ptr(frameData[1]));
    uint8_t *f1 = static_cast<uint8_t *>(i32_ptr(frameData[3]));
    memcpy(f0 + 4, f1 + 4, static_cast<size_t>(height * width) * 2);

    int rc[4];
    if (noDrag != 0) {
        int outBase, outH, outW;
        rc[0] = 0x3e;
        rc[2] = 0x14a;
        rc[1] = 0x2e;
        rc[3] = 0xaa;
        g_fontMgr.MeasureTextWrapped(static_cast<int>(g_waitFont & 0xffff), text, 0x10c, &outW, &outH, &outBase, 0);   // 0x004328e0
        gfx::TextDrawWrapped(rc[0], rc[1], static_cast<int>(g_waitFont & 0xffff), 0x386, text, rc[2] - rc[0],
                             static_cast<uint8_t *>(i32_ptr(frameData[1])) + 4, width, rc, 0, 0);   // 0x00432260
        return;
    }
    rc[0] = 0x21;
    rc[2] = 0x94;
    rc[1] = 0x1e;
    rc[3] = 0x3f;
    gfx::TextDrawWrapped(0x21, 0x1e, 1, 0x386, text, 0x73, static_cast<uint8_t *>(i32_ptr(frameData[1])) + 4, width, rc, 0, 0);   // 0x00432260
}
HOOK_METHOD(0x0043dd00, CGump_0043de30, RenderText);

// Own copies of bodies shared with other classes in the original (the linker folds them).
int __thiscall CGump_0043de30::HitTest(int px, int py)
{
    B6Bitmap *bmp = b6_frame_bitmap(this, 0);
    int w = bmp->w;
    int h = bmp->h;
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return bmp->px[w * py + px] != 0 ? 1 : 0;
}

void __thiscall CGump_0043de30::MouseUp(int, int)
{
    RAWF(int32_t, this, 0x94) = 0;
    g_dragGump = nullptr;
}

