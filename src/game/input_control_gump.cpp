#include "input_control_gump.h"

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "globals.h"
#include "input_control_gump_input_ports.h"
#include "vtables.h"

// The SEH frames of the original (try levels around the base constructor / destructor) are omitted.


// 0x0041fdf0: ECX = art id, EDX = destination bitmap; plain RET. Draws the art into a bitmap of a different size.
// ? the exact behaviour is not read from pass2.
#define Orig_DrawArtSized(a1, a2) (InputControl_DrawSizedArt(a1, (uint16_t *)(a2)))

// 0x0041db90  size 0x25c
// Input control constructor, variant A (sized from art).
CInputControlGump::CInputControlGump(int x_, int y_, void *parentUnused, int artId, int a5, uint8_t mode_,
                                      int a7, int16_t font_, int16_t hue_, int a10, int a11, int widthOverride)
{
    (void)parentUnused;
    x = x_;
    y = y_;
    name = const_cast<char *>("InputControl gump");
    dirty = 0;
    unk_50 = artId;
    artDropDown = a5;
    artEmpty = 0;
    artFocused = 0;
    artDe = 0;
    unk_60 = 0;
    transparent = 0;
    unk_a0 = 0;
    unk_9c = 0;
    unk_98 = 0;
    unk_94 = 0;
    unk_a8 = 0;
    unk_a4 = 0;
    unk_bc = 0;
    focused = 0;
    flag_d4 = 0;
    unk_d0 = 0;
    unk_cc = 0;
    multiline = 0;
    marginX = 3;   // text x margin
    marginY = 2;   // text y margin
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;

    gfx::ArtSize(unk_50, &width, &height);
    height = height + 1;
    if (widthOverride != 0)
        width = widthOverride;
    rowStride = width;
    frameIndex = 0;
    frameCount = 1;

    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CInputControlGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    gfx::setBitmap(this, gfx::AllocTagged(height * width * 2 + 4, "inpctrl frame0"));
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(height * width * 2));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);

    unk_ac = a10;
    maxLength = a7;
    mode = mode_;
    centered = a11;
    sizedByArg = (widthOverride != 0);
    dropDown = 0;
    password = 0;
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
HOOK_CTOR(0x0041db90, CInputControlGump, (int, int, void *, int, int, uint8_t, int, int16_t, int16_t, int, int, int));

// 0x0041e050  size 0xa0
// Destructor body: clears the "focused" and "awaiting target" globals, frees frame0 and the frame data, base dtor.
CInputControlGump::~CInputControlGump()
{
    if (g_awaitingTargetControl == this)
        g_awaitingTargetControl = nullptr;   // control awaiting a target

    if (frameData[1] != 0) {
        gfx::FreeTagged(gfx::bitmapOf(this), "InputControl frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "InputControl framedata");
        frameData = nullptr;
    }
    if (GLOBAL_AS(CInputControlGump *, g_focusedInput) == this)
        GLOBAL_AS(CInputControlGump *, g_focusedInput) = nullptr;   // focused input control

}
HOOK_DTOR(0x0041e050, CInputControlGump);

// 0x0041e0f0  size 0x8f9
// Repaints the control: background art, then the text (plain, password mask, numeric, caption or wrapped) and the caret.
void __thiscall CInputControlGump::Redraw()
{
    char buf[0x78];
    int textW;
    int junk1;
    int junk2;
    int textH;
    int curX = 0;
    int curYextra = 0;
    int clip[4];
    int xm, ym, n, w, px, py, fnt, hu;
    uint16_t artId;
    const char *str;
    void *dst;
    uint16_t *bm;

    g_inputRedrawing = 1;

    // choose background art id
    if (focused != 0) {
        artId = artFocused;
        if (artId != 0)
            goto haveArt;
    }
    artId = artDe;
    if (artId != 0 && flag_d4 != 0)
        goto haveArt;
    if (strlen(text) == 0) {
        artId = artEmpty;
        if (artId != 0)
            goto haveArt;
    }
    if (dropDown != 0)
        artId = static_cast<uint16_t>(artDropDown);
    else
        artId = static_cast<uint16_t>(unk_50);
haveArt:

    gfx::TextMeasure(font, g_refText, &textW, &junk1, &textH);

    if (transparent == 0) {
        if (sizedByArg == 0) {
            bm = gfx::bitmapOf(this);
            gfx::DrawArt(gfx::artPlain(), artId, 0, 0, bm + 2, bm[0], bm[1]);
        } else {
            Orig_DrawArtSized(artId, gfx::bitmapOf(this));
        }
    } else {
        // clear bitmap pixels
        memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(width * height * 2));
    }

    xm = marginX;
    ym = marginY;
    clip[0] = xm;
    clip[1] = ym;
    clip[2] = width - xm;
    clip[3] = height - ym;

    if (mode != 0 && mode != 2) {
        // numeric mode
        sprintf(buf, "%d", value);
        if (GLOBAL_AS(CInputControlGump *, g_focusedInput) == this) {
            curX = gfx::TextWidth(font, caret - scroll, buf + scroll);
            gfx::TextDraw(marginX, textH + marginY, font, hue, buf + scroll, gfx::bitmapOf(this) + 2, width, clip);
            w = width;
            fnt = font;
            hu = 0x386;
            str = g_caretGlyph;
            dst = gfx::bitmapOf(this) + 2;
            py = textH + marginY + 2;
            px = curX + marginX;
            goto draw;
        }
        if (centered != 0) {
            curX = gfx::TextWidth(font, static_cast<int>(strlen(buf)), buf);
            w = width;
            px = (w - curX) / 2;
            py = textH + marginY;
            str = buf + scroll;
            fnt = font;
            hu = hue;
            dst = gfx::bitmapOf(this) + 2;
            goto draw;
        }
        w = width;
        px = marginX;
        py = textH + marginY;
        str = buf + scroll;
        fnt = font;
        hu = hue;
        dst = gfx::bitmapOf(this) + 2;
        goto draw;
    }

    // text mode (0 or 2)
    w = width;
    if (GLOBAL_AS(CInputControlGump *, g_focusedInput) != this) {
        // not focused
        if (password != 0) {
            // password mask
            n = static_cast<int>(strlen(text));
            if (n > 0)
                memset(buf, '*', static_cast<size_t>(n));
            buf[n] = 0;
            px = xm;
            py = textH + ym;
            str = buf + scroll;
            fnt = font;
            hu = hue;
            dst = gfx::bitmapOf(this) + 2;
            goto draw;
        }
        if (centered != 0) {
            str = text;
            if (dropDown == 0) {
                gfx::TextMeasure(font, str, &textW, &junk1, &junk2);
                w = width;
                if (textW + marginX * 2 < w) {
                    gfx::TextDraw((w - textW) / 2, textH + marginY, font, hue, str, gfx::bitmapOf(this) + 2, w, clip);
                } else {
                    gfx::TextDrawAlt(marginX, textH + marginY, font, hue, str, gfx::bitmapOf(this) + 2, w, clip);
                }
                // drop-down arrow on right edge
                bm = gfx::bitmapOf(this);
                if (unk_50 == 0x98d) {
                    gfx::DrawArt(gfx::artPlain(), 0x985, (width - marginX) - 0xb, marginY + 2, bm + 2, bm[0], bm[1]);
                } else if (unk_50 == 0x98e) {
                    gfx::DrawArt(gfx::artPlain(), 0x985, (width - marginX) - 0xf, marginY + 2, bm + 2, bm[0], bm[1]);
                } else {
                    gfx::DrawArt(gfx::artPlain(), 0x1391, (width - marginX) - 9, 1, bm + 2, bm[0], bm[1]);
                }
                g_inputRedrawing = 0;
                return;
            }
            gfx::TextMeasure(font, str, &textW, &junk1, &junk2);
            if (textW >= clip[2] - clip[0]) {
                gfx::TextDrawAlt(clip[0], textH + marginY, font, hue, str, gfx::bitmapOf(this) + 2, width, clip);
                g_inputRedrawing = 0;
                return;
            }
            w = width;
            px = (w - textW) / 2;
            py = textH + marginY;
            fnt = font;
            hu = hue;
            dst = gfx::bitmapOf(this) + 2;
            goto draw;
        }
        if (multiline != 0) {
            gfx::TextDrawWrapped(xm, ym, font, hue, text + scroll, w - xm * 2, gfx::bitmapOf(this) + 2, w, clip, 0, 0);
            g_inputRedrawing = 0;
            return;
        }
        gfx::TextDraw(xm, textH + ym, font, hue, text + scroll, gfx::bitmapOf(this) + 2, w, clip);
        g_inputRedrawing = 0;
        return;
    }

    // focused
    if (password == 0) {
        if (multiline == 0) {
            gfx::TextDraw(xm, textH + ym, font, hue, text + scroll, gfx::bitmapOf(this) + 2, w, clip);
            curX = gfx::TextWidth(font, caret - scroll, text + scroll);
        } else {
            gfx::TextDrawWrapped(xm, ym, font, hue, text + scroll, w - xm * 2, gfx::bitmapOf(this) + 2, w, clip, 0, 0);
            gfx::TextCaretLocate(font, caret - scroll, text + scroll, width - marginX * 2, &curX, &curYextra);
        }
    } else {
        n = static_cast<int>(strlen(text));
        if (n > 0)
            memset(buf, '*', static_cast<size_t>(n));
        buf[n] = 0;
        gfx::TextDraw(xm, textH + ym, font, hue, buf + scroll, gfx::bitmapOf(this) + 2, w, clip);
        curX = gfx::TextWidth(font, caret - scroll, buf + scroll);
    }
    w = width;
    fnt = font;
    dst = gfx::bitmapOf(this) + 2;
    str = g_caretGlyph;   // caret glyph
    hu = 0x386;
    py = textH + curYextra + marginY + 2;
    px = curX + marginX;
draw:
    gfx::TextDraw(px, py, fnt, hu, str, dst, w, clip);
    g_inputRedrawing = 0;
}
HOOK_METHOD(0x0041e0f0, CInputControlGump, Redraw);
