// CInputControlGump: variant B constructor, mouse and keyboard handling, and the art strip helper
// 0x0041fdf0. (Porting pass 2, batch 07.) The SEH frame of the original constructor is omitted.
#include "input_control_gump.h"

#include <windows.h>

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "font_manager.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "text_gump_ctor_ports.h"
#include "window_helpers_31_ports.h"
#include "gump.h"
#include "app_support_ports.h"
#include "packet_builders_b25_ports.h"
#include "client_socket.h"
#include "input_control_gump_input_ports.h"
#include "vtables.h"

#define g_journal GLOBAL_PTR(void, g_msgTarget_storage)
#define g_appObject GLOBAL_PTR(void, g_app_storage)
#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)                            // this for 0x00465890

// 0x00432550 etc. are wrapped in batch01_helpers.h. The hit-test helpers are local.
// 0x00433a50: ECX = font object, (font, str, pixelWidth) RET 0xc: number of characters of str that fit.
static inline int IndexAtWidth(int font, const char *str, int px)
{
    return reinterpret_cast<CFontManager *>(gfx::textObj())->CharsFitting(font, str, px);
}
// 0x00433b30: ECX = font object, (font, str, wrapWidth, px, py) RET 0x14: character index at a point in wrapped text.
static inline int IndexAtPoint(int font, const char *str, int wrapWidth, int px, int py)
{
    return reinterpret_cast<CFontManager *>(gfx::textObj())->CharIndexAtPoint(font, str, wrapWidth, px, py);
}
// 0x004335d0: ECX = font object, (font, ch) RET 8.
static int GlyphWidth(int font, int ch)
{
    return reinterpret_cast<CFontManager *>(gfx::textObj())->GetGlyphWidth(font, ch);
}
// 0x004320d0: ECX = font object, (font, ch) RET 8: non-zero when the font has the glyph.
static int HasGlyph(int font, int ch)
{
    return reinterpret_cast<CFontManager *>(gfx::textObj())->HasGlyph(font, ch);
}
static inline void AddSystemMessage(const char *text)
{
    PORT_THISCALL3(&CreateBarkGump, void, g_journal, int, int, const char *)(0, 3, text);
}
static inline void SetCursor(int id)
{
    PORT_THISCALL1(&SetCursorForRegion, void, g_appObject, int)(id);
}
static void NotifyParentChanged(CInputControlGump *self)
{
    PORT_VSLOT1(self->parent, 0x44, void, CGump *)(self);
}

HOOK_VTABLE(0x0049d9b8, CInputControlGump);
HOOK_SDD(0x0041ddf0, CInputControlGump);

// 0x0041de20  size 0x226
// Constructor variant B: width and height are given, the background is transparent (frame0 starts cleared).
CInputControlGump::CInputControlGump(int x_, int y_, void *parentUnused, uint16_t w, uint16_t h, uint8_t mode_,
                                      int maxLen, int16_t font_, int16_t hue_, int a10, int a11, int sizedFlag)
{
    (void)parentUnused;
    x = x_;
    y = y_;
    name = const_cast<char *>("InputControl gump");
    dirty = 0;
    unk_50 = 0;
    artDropDown = 0;
    artEmpty = 0;
    artFocused = 0;
    artDe = 0;
    unk_60 = 0;
    height = h;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    width = w;
    transparent = 1;
    frameIndex = 0;
    frameCount = 1;

    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CInputControlGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    gfx::setBitmap(this, gfx::AllocTagged(width * height * 2 + 4, "inpctrl frame0"));
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(width * height * 2));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);

    unk_ac = a10;
    maxLength = maxLen;
    mode = mode_;
    centered = a11;
    sizedByArg = (sizedFlag != 0);
    dropDown = 0;
    password = 0;
    unk_bc = 0;
    focused = 0;
    flag_d4 = 0;
    unk_d0 = 0;
    unk_cc = 0;
    multiline = 0;
    marginX = 3;
    marginY = 2;
    strcpy(text, g_emptyString);
    value = 0;
    if (mode == 1)
        caret = 1;
    else
        caret = 0;
    font = static_cast<uint16_t>(font_);
    scroll = 0;
    hue = static_cast<uint16_t>(hue_);
    unk_154 = 0;

    Redraw();
}
HOOK_CTOR(0x0041de20, CInputControlGump, (int, int, void *, uint16_t, uint16_t, uint8_t, int, int16_t, int16_t, int, int, int));

// 0x0041e9f0  size 0x25
// Clears the drag gump and forwards (px, py) to the parent's vtable slot 0x1c.
void __thiscall CInputControlGump::RButtonUp(int px, int py)
{
    g_dragGump = nullptr;
    if (parent != nullptr)
        PORT_VSLOT2(parent, 0x1c, void, int, int)(px, py);
}
HOOK_VMETHOD(0x0041e9f0, CInputControlGump, RButtonUp);

// 0x0041ea20  size 0x1e1
// Left button down: raises the parent, takes the keyboard focus, places the caret under the click and starts
// the double-click timer.
void __thiscall CInputControlGump::MouseDown(int px, int py)
{
    char buf[0x54];
    int n;
    int idx;

    if (parent != nullptr) {
        CGump_BringToFront(parent, 0);
        PORT_VSLOT1(parent, 0x54, void, CGump *)(this);
    }
    if (unk_ac != 0 && dropDown == 0) {
        PORT_THISCALL0(&Gump_CommitFocus, void, g_gumpManager)();                      // CommitFocus
        PORT_THISCALL1(&Gump_SetFocus, void, g_gumpManager, CGump *)(this);       // SetFocus
        if (mode != 0 && mode != 2) {
            sprintf(buf, g_fmtNumber, value);
            caret = static_cast<int32_t>(strlen(buf));
            Redraw();
        } else {
            if (password != 0) {
                n = static_cast<int>(strlen(text));
                if (n > 0)
                    memset(buf, '*', static_cast<size_t>(n));
                buf[n] = 0;
                idx = IndexAtWidth(font, buf + scroll, px - 1);
            } else if (multiline != 0) {
                idx = IndexAtPoint(font, text + scroll, width - 6, px - 1, py);
            } else {
                idx = IndexAtWidth(font, text + scroll, px - 1);
            }
            caret = idx + scroll;
            Redraw();
        }
    } else {
        if (mode == 2 && dropDown == 0)
            PORT_VSLOT1(parent, 0x64, void, CGump *)(this);
    }
    int oldState = unk_a8;
    unk_9c = 1;
    if (oldState == 1) {
        unk_a8 = 2;
        unk_6c = px;
        unk_70 = py;
        return;
    }
    unk_a8 = 1;
    unk_a4 = static_cast<int32_t>(GetTickCount());
    unk_6c = px;
    unk_70 = py;
}
HOOK_VMETHOD(0x0041ea20, CInputControlGump, MouseDown);

// 0x0041ec10  size 0x9c
// Left button up: on a double click with a target hook (+0xd0) arms the target cursor for a player.
void __thiscall CInputControlGump::MouseUp(int px, int py)
{
    (void)px;
    (void)py;
    unk_9c = 0;
    if (unk_94 != 0) {
        unk_94 = 0;
        unk_a8 = 0;
        g_dragGump = nullptr;
        return;
    }
    if (unk_a8 == 2) {
        unk_a8 = 0;
        if (unk_d0 != 0 && g_targetCallbackActive == 0) {
            AddSystemMessage("Select a player.");
            g_awaitingTargetControl = this;
            App_SetCursor13();
            g_targetCursorType = 2;
            g_targetCallback = PORT_ADDR(&InputControl_OnTargetSelected);   // InputControl_OnTargetSelected
        }
        if (parent != nullptr)
            PORT_VSLOT1(parent, 0x58, void, CGump *)(this);
    }
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x0041ec10, CInputControlGump, MouseUp);

// 0x0041ecb0  size 0x3c
// Mouse over: resets the cursor and marks the control hot (redraws when it has a hot art).
void __thiscall CInputControlGump::MouseOver(int px, int py)
{
    (void)px;
    (void)py;
    SetCursor(0);
    if (flag_d4 == 0) {
        flag_d4 = 1;
        if (artDe != 0)
            Redraw();
    }
}
HOOK_VMETHOD(0x0041ecb0, CInputControlGump, MouseOver);

// 0x0041ed20  size 0xdc
// Target cursor callback, ECX = clicked world object (no stack arguments). Stores the serial in the
// waiting control as "(mobile)" and sends the 0x98 name query; otherwise reports "Not a mobile.".
void __fastcall InputControl_OnTargetSelected(void *obj)
{
    char pkt[0x28];

    if (obj != nullptr && decomp::vcall(PORT_RET(int), obj, 0x24) != 0 && g_awaitingTargetControl != nullptr) {
        g_awaitingTargetControl->value = RAWF(int32_t, obj, 0x78);   // serial
        strcpy(g_awaitingTargetControl->text, "(mobile)");   // 0x004a71c0
        g_awaitingTargetControl->Redraw();
        PORT_VSLOT1(g_awaitingTargetControl->parent, 0x44, void, CGump *)(
                                                                         g_awaitingTargetControl);
        PORT_THISCALL2(&Pkt_Build_0x98_AllNamesQuery, void, g_packetBuilder, char *, int)(pkt, g_awaitingTargetControl->value);   // 0x98 name query
        reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(pkt));
        App_SetCursor11();
        g_targetFlags = 0;
        return;
    }
    AddSystemMessage("Not a mobile.");   // 0x004a71b0
    App_SetCursor11();
    g_targetFlags = 0;
}
HOOK_FUNC(0x0041ed20, InputControl_OnTargetSelected);

// 0x0041ee00  size 0x48
// Double-click timeout: when the double-click time has passed since the first click the wait ends.
void __thiscall CInputControlGump::Tick()
{
    DWORD now = GetTickCount();
    UINT dct = GetDoubleClickTime();
    if (unk_a8 == 1 && (now - static_cast<DWORD>(unk_a4)) > dct) {
        unk_a8 = 0;
        if (unk_9c != 0)
            unk_94 = 1;
    }
}
HOOK_VMETHOD(0x0041ee00, CInputControlGump, Tick);

// 0x0041ee50  size 0x109
// Scrolls the first visible character forward until the caret fits in the control width. Always returns 1.
int __thiscall CInputControlGump::ScrollToCaret()
{
    char buf[0x14];
    const char *p;
    const char *base;
    int fit;
    int w;
    int len;

    if (unk_cc != 0)
        return 1;
    if (mode != 0 && mode != 2) {
        sprintf(buf, g_fmtNumber, value);
        p = buf + scroll;
    } else {
        p = text + scroll;
    }
    fit = IndexAtWidth(font, p, width - 12) + scroll;
    base = p - scroll;
    if (fit > caret)
        return 1;
    w = gfx::TextWidth(font, caret - fit, base + fit);
    len = static_cast<int>(strlen(base)) - 1;
    if (w <= 0)
        return 1;
    do {
        if (scroll >= len)
            break;
        w -= GlyphWidth(font, static_cast<signed char>(base[scroll]));
        scroll = scroll + 1;
    } while (w > 0);
    return 1;
}
HOOK_METHOD(0x0041ee50, CInputControlGump, ScrollToCaret);

// 0x0041ef60  size 0x5d5
// Key press. Keys: 9 / 0xd / 0x1b go to the parent's slots 0x48 / 0x4c / 0x50; 0xe023 End, 0xe024 Home,
// 0xe025 Left, 0xe027 Right, 0xe026 / 0xe028 ignored, 8 Backspace, 0xe02e Delete; anything else is inserted.
// After a change the control redraws and tells the parent (slot 0x44).
void __thiscall CInputControlGump::OnKey(int key)
{
    char buf[0x40];
    char tmp[0x70];
    int w;
    int h;
    int d;
    int len;
    const char *s;

    if (dropDown != 0)
        return;
    if (key == 9) {
        decomp::vcall_void(parent, 0x48);
        return;
    }
    if (key == 0xd) {
        decomp::vcall_void(parent, 0x4c);
        return;
    }
    if (key == 0x1b) {
        decomp::vcall_void(parent, 0x50);
        return;
    }
    if (key == 0xe024) {   // Home
        if (mode != 0)
            return;
        if (caret <= 0)
            return;
        caret = 0;
        scroll = 0;
        Redraw();
        return;
    }
    if (key == 0xe023) {   // End
        if (mode == 0 || mode == 2) {
            s = text;
        } else {
            if (mode != 1)
                return;
            sprintf(buf, g_fmtNumber, value);
            s = buf;
        }
        len = static_cast<int>(strlen(s));
        if (caret >= len)
            return;
        caret = len;
        ScrollToCaret();
        Redraw();
        return;
    }
    if (key == 0xe026 || key == 0xe028)
        return;
    if (key == 0xe025 || key == 0xe027) {   // Left / Right
        if (mode != 0)
            return;
        if (key == 0xe025 && caret > 0) {
            caret = caret - 1;
            if (caret >= scroll) {
                Redraw();
                return;
            }
            int s8 = scroll - 8;
            scroll = (s8 < 0) ? 0 : s8;
            Redraw();
            return;
        }
        if (key != 0xe027)
            return;
        len = static_cast<int>(strlen(text));
        if (caret >= len)
            return;
        caret = caret + 1;
        ScrollToCaret();
        Redraw();
        return;
    }
    if (key == 8 || key == 0xe02e) {   // Backspace / Delete
        if (mode != 0 && mode != 2) {
            uint32_t v = static_cast<uint32_t>(value);
            if (v == 0)
                return;
            value = static_cast<int32_t>(v / 10);
            if (caret > 1)
                caret = caret - 1;
            if (caret < scroll) {
                int s8 = scroll - 8;
                scroll = (s8 < 0) ? 0 : s8;
            }
            Redraw();
            if (parent != nullptr)
                NotifyParentChanged(this);
            return;
        }
        len = static_cast<int>(strlen(text));
        if (len <= 0)
            return;
        if (key == 0xe02e) {
            if (caret >= len)
                return;
            caret = caret + 1;
        }
        bool remove = true;
        if (caret < len) {
            if (caret <= 0) {
                remove = false;
            } else {
                int i = caret - 1;
                int last = len - 1;
                while (i < last) {
                    text[i] = text[i + 1];
                    i++;
                }
            }
        }
        if (remove) {
            text[len - 1] = 0;
            caret = caret - 1;
        }
        if (caret < scroll) {
            int s8 = scroll - 8;
            scroll = (s8 < 0) ? 0 : s8;
        }
        Redraw();
        if (parent != nullptr)
            NotifyParentChanged(this);
        return;
    }

    // Any other key: insert a character (text modes) or a digit (numeric mode).
    if (HasGlyph(font, key) == 0 || (mode != 0 && mode != 2)) {
        if (mode != 1)
            return;
        if (key < 0x30 || key > 0x39)
            return;
        int v = value * 10 + key - 0x30;
        if (v > maxLength)
            return;
        value = v;
        caret = caret + 1;
        Redraw();
        if (parent != nullptr)
            NotifyParentChanged(this);
        return;
    }
    if (unk_bc != 0) {   // alphanumeric only
        if (!((key >= 0x30 && key <= 0x39) || (key >= 0x61 && key <= 0x7a) || (key >= 0x41 && key <= 0x5a)))
            return;
    }
    len = static_cast<int>(strlen(text));
    if (len >= maxLength)
        return;
    if (len > caret) {
        int i = len;
        do {
            text[i] = text[i - 1];
            i--;
        } while (i > caret);
        text[caret] = static_cast<char>(key);
    } else {
        text[len] = static_cast<char>(key);
    }
    text[len + 1] = 0;
    caret = caret + 1;
    ScrollToCaret();
    if (unk_cc == 0) {
        if (multiline == 0)
            goto changed;
    } else if (multiline == 0) {
        // single line with a limit: reject the character when the text plus the caret glyph no longer fits
        strcpy(tmp, text);
        strcat(tmp, g_caretGlyph);
        gfx::TextMeasure(font, tmp, &w, &h, &d);
        if (w >= width - 6) {
            PORT_VSLOT1(this, 0x3c, void, int)(8);
            return;
        }
        goto changed;
    }
    // multi-line: reject the character when the wrapped text grows taller than the control
    strcpy(tmp, text);
    strcat(tmp, g_caretGlyph);
    g_fontMgr.MeasureTextWrapped(font, tmp, width - 6, &w, &h, &d, 0);
    if (h >= height) {
        PORT_VSLOT1(this, 0x3c, void, int)(8);
        return;
    }
changed:
    Redraw();
    if (parent != nullptr)
        NotifyParentChanged(this);
}
HOOK_VMETHOD(0x0041ef60, CInputControlGump, OnKey);

// 0x0041fdf0  size 0xf8
// ECX = base art id, EDX = destination bitmap ({u16 w, u16 h, pixels}); plain RET.
// Draws a three-piece horizontal strip: art `artId` as the left cap, `artId + 1` tiled across the middle
// (clipped to the bitmap), `artId + 2` as the right cap. Does nothing when any of the three is missing.
void __fastcall InputControl_DrawSizedArt(int artId, uint16_t *bmp)
{
    int w0, h0;
    int w1, h1;
    int w2, h2;
    int clip[4];

    gfx::ArtSize(artId, &w0, &h0);
    gfx::ArtSize(artId + 1, &w1, &h1);
    gfx::ArtSize(artId + 2, &w2, &h2);
    if (w0 == 0 || w1 == 0 || w2 == 0)
        return;
    uint16_t *pix = bmp + 2;
    int bw = bmp[0];
    int bh = bmp[1];
    clip[0] = 0;
    clip[1] = 0;
    clip[2] = bw;
    clip[3] = bh;
    gfx::DrawArt(gfx::artPlain(), artId, 0, 0, pix, bw, bh);
    int xx = w0;
    if (xx < bw - w2) {
        do {
            gfx::DrawArtClipped(gfx::artPlain(), artId + 1, clip, xx, 0, pix, bmp[0], bmp[1]);
            xx += w1;
        } while (xx < bmp[0] - w2);
    }
    gfx::DrawArt(gfx::artPlain(), artId + 2, bmp[0] - w2, 0, pix, bmp[0], bmp[1]);
}
HOOK_FUNC(0x0041fdf0, InputControl_DrawSizedArt);
