// CListBoxControlGump: repaint, mouse handling and AddItem. (Porting pass 2, batch 07.)
// Also the selection highlight helper 0x0041fef0, used only by Redraw.
#include "list_box_control_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "input_control_gump.h"
#include "crt.h"
#include "gump_art_ops_ports.h"
#include "skill_list_gump_ops_ports.h"
#include "mouse_manager.h"
#include "gump.h"
#include "globals.h"

#define g_mouseManager GLOBAL_PTR(void, g_mouse_storage)

// 0x00434410: ECX = art id, EDX = x, stack = y, RET 4. Colour of one art pixel, 0 when transparent / outside.
static uint16_t ArtPixel(int artId, int px, int py)
{
    return GumpArtPixel(artId, px, py);
}

// 0x0041fef0  size 0x19f
// ECX = x, EDX = y, stack (totalWidth, artId, bitmap); RET 0xc.
// Darkens the pixels of a bar `totalWidth` wide and as tall as art `artId`, starting at (x, y) of the bitmap:
// the left cap (art `artId`) and right cap (art `artId + 1`) only where the cap art has a pixel, the middle everywhere.
static void __fastcall ListBox_DrawHighlight(int hx, int hy, int totalWidth, int artId, uint16_t *bmp)
{
    int w0, h0;
    int w1, h1;
    int midW;
    int rows;
    uint16_t *base;
    int r;
    int c;

    gfx::ArtSize(artId, &w0, &h0);
    gfx::ArtSize(artId + 1, &w1, &h1);
    midW = totalWidth - w0 - w1;
    rows = h0;
    base = bmp + 2 + (hy * static_cast<int>(bmp[0]) + hx);

    // left cap
    for (r = 0; r < rows; r++) {
        for (c = 0; c < w0; c++) {
            if (ArtPixel(artId, c, r) != 0) {
                uint16_t *p = &base[static_cast<uint16_t>(r * static_cast<int>(bmp[0]) + c)];
                *p = static_cast<uint16_t>(((g_blendMaskB >> 1) & (*p >> 1)) + g_shadeAdd);
            }
        }
    }
    // middle
    for (r = 0; r < rows; r++) {
        for (c = 0; c < midW; c++) {
            uint16_t *p = &base[static_cast<uint16_t>(r * static_cast<int>(bmp[0]) + w0 + c)];
            *p = static_cast<uint16_t>(((g_blendMaskB >> 1) & (*p >> 1)) + g_shadeAdd);
        }
    }
    // right cap (the -1 is in the original)
    for (r = 0; r < rows; r++) {
        for (c = 0; c < w1; c++) {
            if (ArtPixel(artId + 1, c, r) != 0) {
                uint16_t *p = &base[static_cast<uint16_t>(r * static_cast<int>(bmp[0]) + w0 + midW + c - 1)];
                *p = static_cast<uint16_t>(((g_blendMaskB >> 1) & (*p >> 1)) + g_shadeAdd);
            }
        }
    }
}
HOOK_FUNC(0x0041fef0, ListBox_DrawHighlight);

HOOK_VTABLE(0x0049dad0, CListBoxControlGump);
HOOK_SDD(0x004201d0, CListBoxControlGump);

// 0x00420290  size 0x5c6
// Repaints frame0. `row == -1`: frame art, every visible item and the scroll arrows; otherwise only the
// strip of the frame behind item `row`, its highlight and its text (and the arrows).
void __thiscall CListBoxControlGump::Redraw(int row)
{
    int xoff = 0;
    int w;
    int h0;
    int h1;
    int h4;
    int aw;
    int ah;
    int junk1;
    int junk2;
    int lineOff;
    int clip[4];
    int textX;
    int i;
    int yy;

    if (frameData[1] == 0) {
        gfx::setBitmap(this, gfx::AllocTagged(height * width * 2 + 4, "listboxctrl frame0"));
        gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
        gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);
        memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(height * width * 2));
    }

    gfx::ArtSize(unk_50, &w, &h0);
    gfx::ArtSize(unk_50 + 1, &w, &h1);
    if (w < width)
        xoff = (width - w) / 2;
    textX = xoff + 6;
    clip[0] = textX;
    clip[1] = h0;
    clip[2] = width - 6;
    gfx::ArtSize(unk_50 + 4, &w, &h4);
    clip[3] = height - h4;

    if (row == -1) {
        SkillList_DrawTiledFrame(unk_50, 3, gfx::bitmapOf(this), 0, 0);
        if (selected != -1)
            ListBox_DrawHighlight(textX, h0 + 15 * selected, width - textX * 2, 0x13f5, gfx::bitmapOf(this));
        if (hasScroll != 0) {
            yy = 0;
            for (i = 0; i < rows; i++) {
                gfx::TextMeasure(font, names[unk_a8 + i], &junk1, &junk2, &lineOff);
                gfx::TextDrawAlt(textX, lineOff + h0 + yy + 1, font, hue, names[unk_a8 + i], gfx::bitmapOf(this) + 2, width,
                                 clip);
                yy += 15;
            }
            if (unk_a8 > 0) {
                gfx::ArtSize(0x983, &aw, &ah);
                gfx::DrawArt(gfx::artPlain(), 0x983, clip[2] - aw - 6, h0, gfx::bitmapOf(this) + 2, width, height);
            }
            if (unk_a8 + rows < unk_9c) {
                gfx::ArtSize(0x985, &aw, &ah);
                gfx::DrawArt(gfx::artPlain(), 0x985, clip[2] - aw - 6, (height - h4) - ah, gfx::bitmapOf(this) + 2, width,
                             height);
            }
        } else {
            yy = 0;
            for (i = 0; i < unk_9c; i++) {
                gfx::TextMeasure(font, names[i], &junk1, &junk2, &lineOff);
                gfx::TextDrawAlt(textX, lineOff + h0 + yy + 1, font, hue, names[i], gfx::bitmapOf(this) + 2, width, clip);
                yy += 15;
            }
        }
        return;
    }

    // Single row.
    int rel = row - unk_a8;
    int rowTop = h0 + 15 * rel;
    int rowBottom = h0 + 15 * (rel + 1);
    int tileTop = h0 - 1;
    int piece = 0;
    int th;

    gfx::ArtSize(unk_50 + 1, &w, &th);
    int tileEnd = th + tileTop;
    if (tileEnd <= rowTop) {
        do {
            piece++;
            tileTop = tileEnd;
            if (piece == 3)
                piece = 0;
            gfx::ArtSize(unk_50 + piece + 1, &w, &th);
            tileEnd = th + tileTop;
        } while (tileEnd <= rowTop);
    }
    clip[1] = rowTop;
    clip[3] = rowBottom + 1;
    if (tileTop < rowBottom) {
        do {
            gfx::ArtSize(unk_50 + piece + 1, &w, &h1);
            gfx::DrawArtClipped(gfx::artPlain(), unk_50 + piece + 1, clip, xoff, tileTop, gfx::bitmapOf(this) + 2,
                                gfx::bitmapOf(this)[0], gfx::bitmapOf(this)[1]);
            tileTop += h1;
            piece++;
            if (piece == 3)
                piece = 0;
        } while (tileTop < rowBottom);
    }
    if (row == selected)
        ListBox_DrawHighlight(textX, h0 + 15 * (selected - unk_a8), width - textX * 2, 0x13f5, gfx::bitmapOf(this));
    gfx::TextMeasure(font, names[row], &junk1, &junk2, &lineOff);
    gfx::TextDrawAlt(textX, lineOff + 15 * (row - unk_a8) + h0 + 1, font, hue, names[row], gfx::bitmapOf(this) + 2, width, clip);
    if (hasScroll == 0)
        return;
    if (unk_a8 > 0) {
        gfx::ArtSize(unk_50, &w, &h1);
        gfx::ArtSize(0x983, &aw, &ah);
        gfx::DrawArt(gfx::artPlain(), 0x983, clip[2] - aw - 6, h1, gfx::bitmapOf(this) + 2, width, height);
    }
    if (rows + unk_a8 < unk_9c) {
        gfx::ArtSize(unk_50 + 4, &w, &h1);
        int bottom = height - h1;
        gfx::ArtSize(0x985, &aw, &ah);
        gfx::DrawArt(gfx::artPlain(), 0x985, clip[2] - aw - 6, bottom - ah, gfx::bitmapOf(this) + 2, width, height);
    }
}
HOOK_METHOD(0x00420290, CListBoxControlGump, Redraw);

// 0x00420860  size 0xa8
// Left button up: when an item is selected and the linked input control holds another value, copies the
// item's value and text into it and tells the control's parent (vtable slot 0x44); then deletes the list.
void __thiscall CListBoxControlGump::MouseUp(int px, int py)
{
    (void)px;
    (void)py;
    g_dragGump = nullptr;
    CInputControlGump *ctl = reinterpret_cast<CInputControlGump *>(static_cast<uintptr_t>(static_cast<uint32_t>(unk_94)));
    if (selected != -1 && ctl != nullptr) {
        int v = values[selected];
        if (ctl->value != v) {
            ctl->value = v;
            strcpy(ctl->text, names[selected]);
            ctl->Redraw();
            PORT_VSLOT1(ctl->parent, 0x44, void, CGump *)(ctl);
        }
    }
    PORT_VSLOT1(this, 0x00, void, int)(1);   // delete this
}
HOOK_VMETHOD(0x00420860, CListBoxControlGump, MouseUp);

// 0x00420910  size 0xc5
// Mouse move: maps the position to a row (15 pixels each) and repaints the old and new rows.
void __thiscall CListBoxControlGump::MouseMove(int px, int py)
{
    int w;
    int h0;
    int h4;
    int oldSel = selected;

    gfx::ArtSize(unk_50, &w, &h0);
    gfx::ArtSize(unk_50 + 4, &w, &h4);
    h4 = height - h4;
    if (px >= 0 && py >= h0 && px < width && py < h4) {
        selected = (py - h0) / 15 + unk_a8;
        if (oldSel == selected)
            return;
        if (oldSel != -1)
            Redraw(oldSel);
        Redraw(selected);
        return;
    }
    selected = -1;
    if (oldSel != -1)
        Redraw(oldSel);
}
HOOK_VMETHOD(0x00420910, CListBoxControlGump, MouseMove);

// 0x004209e0  size 0x8c
// While the mouse is above (below) the list, scrolls one row up (down) and repaints.
void __thiscall CListBoxControlGump::Tick()
{
    int mp[2];   // [ebp-8] x, [ebp-4] y

    if (hasScroll == 0)
        return;
    reinterpret_cast<CMouseManager *>(g_mouseManager)->GetMousePos(&mp[0], &mp[1]);
    this->CGump::ScreenToLocal(&mp[0], &mp[1]);   // CGump::ScreenToLocal
    if (mp[1] < 0) {
        if (unk_a8 > 0) {
            unk_a8 = unk_a8 - 1;
            Redraw(-1);
            return;
        }
    }
    if (mp[1] > height) {
        if (unk_a8 + rows < unk_9c) {
            unk_a8 = unk_a8 + 1;
            Redraw(-1);
        }
    }
}
HOOK_VMETHOD(0x004209e0, CListBoxControlGump, Tick);

// 0x00420a70  size 0x11a
// Appends an item (up to 100). Grows the gump by one row while there is no scroll bar or the rows are not yet
// all shown, keeps it on the 640x480 screen, and widens / recentres it for a text wider than the box.
void __thiscall CListBoxControlGump::AddItem(int itemValue, const char *itemName)
{
    int limitW = width - 12;
    int tw;
    int tx;
    int th;

    if (unk_9c >= 100)
        return;
    values[unk_9c] = itemValue;
    strcpy(names[unk_9c], itemName);
    unk_9c = unk_9c + 1;
    int scrolling = hasScroll;
    if (scrolling == 0 || unk_9c <= rows)
        height = height + 15;
    if (y + height > 0x1e0)
        y = 0x1e0 - height;
    if (scrolling != 0) {
        if (y + height > 0x1cc)
            y = 0x1cc - height;
    }
    if (y < 0)
        y = 0;
    gfx::TextMeasure(font, itemName, &tw, &tx, &th);
    if (tw > limitW) {
        width = tw + 12;
        x = x + -((tw - limitW) / 2);
    }
}
HOOK_METHOD(0x00420a70, CListBoxControlGump, AddItem);
