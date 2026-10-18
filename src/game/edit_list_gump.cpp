#include "edit_list_gump.h"

#include <windows.h>

#include <cstdlib>
#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "globals.h"
#include "crt.h"
#include "vcall.h"
#include "input_control_gump.h"
#include "char_flow_gumps.h"
#include "gump.h"
#include "gump_base_ops_ports.h"
#include "vtables.h"

// The SEH frames of the original (state counter around every operator new, handlers 0x49915e / 0x49918e / 0x4991a8 /
// 0x4991d6) are omitted. Every "p = operator new(size); obj = p ? ctor(p, ...) : 0" of the original is kept; the code
// after it dereferences the result without a NULL check, as the original does.


// 0x00410510 / 0x004105d0: scroll the list up / down, plain RET, ECX = the edit list. (They are also the button callbacks.)
#define Orig_ScrollUp(self) (reinterpret_cast<CGump_00410510 *>(self)->ScrollBack())
#define Orig_ScrollDown(self) (reinterpret_cast<CGump_00410510 *>(self)->ScrollForward())
// 0x0043a720 / 0x0043a820 on the gump manager at 0x006488c0: commit the current focus / give the focus to a gump (RET 4).
#define Orig_CommitFocus(o) PORT_THISCALL0(&Gump_CommitFocus, void, (o))()
#define Orig_SetFocus(o, a1) PORT_THISCALL1(&Gump_SetFocus, void, (o), CGump *)(a1)

static inline CEditListNode *nodeOf(CInputControlGump *row)
{
    return reinterpret_cast<CEditListNode *>(static_cast<uintptr_t>(static_cast<uint32_t>(row->unk_154)));
}

static int32_t nodeToInt(CEditListNode *node)
{
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(node));
}

// 0x00416740  size 0x27a
// Constructor, non-editable form: frame0 bitmap drawn from art `artId`, scroll up / down buttons (both hidden).
CEditListGump::CEditListGump(int x_, int y_, int, int, int artId, int maxCount_, int font_)
{
    name = const_cast<char *>("edit list gump");
    dirty = 0;
    x = x_;
    owner = nullptr;
    y = y_;
    editable = 0;
    unk_a8 = 0;
    pressed = 0;
    unk_a0 = 0;
    dragging = 0;
    clickState = 0;
    pressTick = 0;
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    gfx::ArtSize(artId, &width, &height);
    frameIndex = 0;
    frameCount = 1;
    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CEditListGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    frameData[1] = static_cast<int32_t>(reinterpret_cast<uintptr_t>(gfx::AllocTagged(height * width * 2 + 4, "cfg frame0")));
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(height * width * 2));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);
    maxCount = maxCount_;
    font = font_;
    visibleRows = 0;
    count = 0;
    count_ca = 10;
    selected = nullptr;
    cur_c0 = nullptr;
    rows_bc = nullptr;
    head = nullptr;
    unk_50 = artId;
    gfx::DrawArt(gfx::artPlain(), unk_50, 0, 0, gfx::bitmapOf(this) + 2, width, height);

        CButtonGump *b;
    b = new CButtonGump(width - 0xe, 8, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 2, 0, PORT_ADDR(&CGump_00410510::ScrollBack), 0x983, 0x983, 0);
    child_94 = b;
    child_94->visible = 0;
    child_94->artHover = 0x984;
    gfx::AddChild(child_94, this);

    b = new CButtonGump(width - 0xe, height - 0x14, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 2, 0, PORT_ADDR(&CGump_00410510::ScrollForward), 0x985, 0x985, 0);
    child_98 = b;
    child_98->visible = 0;
    child_98->artHover = 0x986;
    gfx::AddChild(child_98, this);
}
HOOK_CTOR(0x00416740, CEditListGump, (int, int, int, int, int, int, int));

HOOK_VTABLE(0x0049cef0, CEditListGump);
HOOK_SDD(0x004169c0, CEditListGump);

// 0x004169f0  size 0x27c
// Constructor, editable form (trailing "(new)" row, Delete key): as 0x00416740 with editable = 1 and font 0x1d.
CEditListGump::CEditListGump(int x_, int y_, int, int, int artId, int maxCount_)
{
    name = const_cast<char *>("edit list gump");
    dirty = 0;
    x = x_;
    owner = nullptr;
    editable = 1;
    unk_a8 = 0;
    y = y_;
    pressed = 0;
    unk_a0 = 0;
    dragging = 0;
    clickState = 0;
    pressTick = 0;
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    gfx::ArtSize(artId, &width, &height);
    frameIndex = 0;
    frameCount = 1;
    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CEditListGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    frameData[1] = static_cast<int32_t>(reinterpret_cast<uintptr_t>(gfx::AllocTagged(height * width * 2 + 4, "cfg frame0")));
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(height * width * 2));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);
    maxCount = maxCount_;
    font = 0x1d;
    visibleRows = 0;
    count = 0;
    count_ca = 10;
    selected = nullptr;
    cur_c0 = nullptr;
    rows_bc = nullptr;
    head = nullptr;
    unk_50 = artId;
    gfx::DrawArt(gfx::artPlain(), unk_50, 0, 0, gfx::bitmapOf(this) + 2, width, height);

        CButtonGump *b;
    b = new CButtonGump(width - 0xe, 8, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 2, 0, PORT_ADDR(&CGump_00410510::ScrollBack), 0x983, 0x983, 0);
    child_94 = b;
    child_94->visible = 0;
    child_94->artHover = 0x984;
    gfx::AddChild(child_94, this);

    b = new CButtonGump(width - 0xe, height - 0x14, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 2, 0, PORT_ADDR(&CGump_00410510::ScrollForward), 0x985, 0x985, 0);
    child_98 = b;
    child_98->visible = 0;
    child_98->artHover = 0x986;
    gfx::AddChild(child_98, this);
}
HOOK_CTOR(0x004169f0, CEditListGump, (int, int, int, int, int, int));

// 0x00416c70  size 0xc7
// Destructor body: deletes every row and its node, frees the frame bitmap and the frame table.
CEditListGump::~CEditListGump()
{
    if (head != nullptr) {
        CEditListNode *nextNode;
        do {
            nextNode = head->next;
            if (head->row != nullptr)
                decomp::vdelete(head->row);
            operator delete(head);
            head = nextNode;
        } while (nextNode != nullptr);
    }
    if (frameData[1] != 0) {
        gfx::FreeTagged(gfx::bitmapOf(this), "editlist frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "editlist framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00416c70, CEditListGump);

// 0x00416d40  size 0x3b3
// Appends a row (a 0x88 x 0x11 edit box at y = (count - visibleRows) * 17 + 3, nudged past the art's separator
// lines) holding `text` / `value`, shows the scroll-down button once more than visibleRows + 1 rows exist, and redraws
// the background clipped to the rows.
void __thiscall CEditListGump::AddItem(const char *text, int value)
{
    int cnt = count;
    if (cnt == maxCount)
        return;
    int vis = visibleRows;
    int idx = cnt - vis;
    int rowY = idx * 17 + 3;
    if (rowY == 0x25)
        rowY++;
    if (rowY == 0x36)
        rowY = 0x37;
    if (rowY == 0x47)
        rowY = 0x49;
    count = cnt + 1;
    if (vis + 1 < cnt + 1)
        child_98->visible = 1;

    CEditListNode *node;
    void *mem;
    if (tail() == nullptr) {
        node = static_cast<CEditListNode *>(::operator new(0xc));
        head = node;
        CInputControlGump *row;
        row = new CInputControlGump(static_cast<uint16_t>(count_ca), rowY, this, static_cast<uint16_t>(0x88), static_cast<uint16_t>(0x11), 0, font, 1, 0x386, 1, 0, 0);
        head->row = row;
        if (editable != 0) {
            head->row->unk_ac = 0;
            head->row->unk_d0 = 1;
        }
        head->row->unk_154 = nodeToInt(head);
        if (rowY < 3 || rowY + 0x11 > height)
            head->row->visible = 0;
        gfx::AddChild(head->row, this);
        strcpy(head->row->text, text);
        head->row->value = value;
        head->row->Redraw();
        head->prev = nullptr;
        head->next = nullptr;
        cur_c0 = head;
        rows_bc = reinterpret_cast<CInputControlGump **>(head);
    } else {
        tail()->next = static_cast<CEditListNode *>(::operator new(0xc));
        CInputControlGump *row;
        row = new CInputControlGump(static_cast<uint16_t>(count_ca), rowY, this, static_cast<uint16_t>(0x88), static_cast<uint16_t>(0x11), 0, font, 1, 0x386, 1, 0, 0);
        tail()->next->row = row;
        if (editable != 0) {
            tail()->next->row->unk_ac = 0;
            tail()->next->row->unk_d0 = 1;
        }
        tail()->next->row->unk_154 = nodeToInt(tail()->next);
        if (rowY < 3 || rowY + 0x11 > height)
            tail()->next->row->visible = 0;
        gfx::AddChild(tail()->next->row, this);
        strcpy(tail()->next->row->text, text);
        tail()->next->row->value = value;
        tail()->next->row->Redraw();
        tail()->next->next = nullptr;
        tail()->next->prev = tail();
        rows_bc = reinterpret_cast<CInputControlGump **>(tail()->next);
    }

    int clip[4];
    clip[0] = 0;
    clip[1] = 0;
    clip[2] = width;
    clip[3] = (count - visibleRows) * 17 + 9;
    gfx::DrawArt(gfx::artPlain(), unk_50, 0, 0, gfx::bitmapOf(this) + 2, width, height);
    if (count != 0)
        gfx::DrawArtClipped(gfx::artPlain(), unk_50 + 1, clip, 0, 0, gfx::bitmapOf(this) + 2, width, height);
}
HOOK_METHOD(0x00416d40, CEditListGump, AddItem);

// 0x00417100  size 0x184
// Unlinks a row node, deletes its row, moves the later rows up by one row height (art 0x13a7 height + 1), hides the
// scroll-down button when few rows are left, redraws the background and tells the parent.
void __thiscall CEditListGump::RemoveItem(CEditListNode *node)
{
    int w;
    int h;

    if (selected == node)
        selected = nullptr;
    if (cur_c0 == node && cur_c0->next != nullptr)
        cur_c0 = cur_c0->next;
    if (node->row != nullptr)
        decomp::vdelete(node->row);
    if (head == node)
        head = node->next;
    if (tail() == node)
        rows_bc = reinterpret_cast<CInputControlGump **>(node->prev);
    if (node->next != nullptr)
        node->next->prev = node->prev;
    if (node->prev != nullptr)
        node->prev->next = node->next;
    CEditListNode *n = node->next;
    gfx::ArtSize(0x13a7, &w, &h);
    h = h + 1;
    for (; n != nullptr; n = n->next) {
        n->row->y = n->row->y - h;
        if (n->row->y + h < height)
            n->row->visible = 1;
    }
    operator delete(node);
    count = count - 1;
    if (!(visibleRows + 1 < count))
        child_98->visible = 0;

    int clip[4];
    clip[0] = 0;
    clip[1] = 0;
    clip[2] = width;
    clip[3] = (count - visibleRows) * 17 + 9;
    gfx::DrawArt(gfx::artPlain(), unk_50, 0, 0, gfx::bitmapOf(this) + 2, width, height);
    if (count != 0)
        gfx::DrawArtClipped(gfx::artPlain(), unk_50 + 1, clip, 0, 0, gfx::bitmapOf(this) + 2, width, height);
    if (parent != nullptr)
        PORT_VSLOT1(parent, 0x44, void, CGump *)(this);
}
HOOK_METHOD(0x00417100, CEditListGump, RemoveItem);

// 0x00417290  size 0x36
// Restores the selected row's hue and drops the selection.
void __thiscall CEditListGump::ClearSelection()
{
    if (selected != nullptr) {
        selected->row->hue = static_cast<uint16_t>(savedHue);
        selected->row->Redraw();
        selected = nullptr;
    }
}
HOOK_METHOD(0x00417290, CEditListGump, ClearSelection);

// 0x004172d0  size 0x161
// A row's text changed. Typing in the trailing row appends a fresh trailing row ("(new)" in the editable form); a
// row that became empty is removed after moving the focus to its neighbour. The parent is told in every case.
void __thiscall CEditListGump::OnChildChanged(int rowArg)
{
    CInputControlGump * row = reinterpret_cast<CInputControlGump *>(rowArg);
    CEditListNode *node = nodeOf(row);
    CEditListNode *tl = tail();
    int removed = 0;

    if (node == tl) {
        if (strlen(tl->row->text) != 0) {
            if (editable != 0)
                AddItem("(new)", 0);
            else
                AddItem(g_emptyString, 0);
        }
    } else if (strlen(node->row->text) == 0) {
        CEditListNode *nb = node->prev;
        if (nb != nullptr) {
            Orig_CommitFocus(g_gumpManager);
            Orig_SetFocus(g_gumpManager, nb->row);
            PORT_VSLOT1(nb->row, 0x3c, void, int)(0xe023);
            nb->row->Redraw();
            if (nb->row->y < 8)
                Orig_ScrollUp(this);
        } else {
            nb = node->next;
            if (nb != nullptr) {
                Orig_CommitFocus(g_gumpManager);
                Orig_SetFocus(g_gumpManager, nb->row);
                PORT_VSLOT1(nb->row, 0x3c, void, int)(0xe023);
                nb->row->Redraw();
                if (nb->row->y < 8)
                    Orig_ScrollUp(this);
            }
        }
        RemoveItem(node);
        removed = 1;
    }
    if (removed == 0 && row->hue != 0x386) {
        row->hue = 0x386;
        row->Redraw();
    }
    if (parent != nullptr)
        PORT_VSLOT1(parent, 0x44, void, CGump *)(this);
}
HOOK_VMETHOD(0x004172d0, CEditListGump, OnChildChanged);

// 0x00417440  size 0x9f
// Selects the node of `row`: restores the old selection's hue, highlights the new one (hue 0x58) and takes the focus.
void __thiscall CEditListGump::OnFocus(int rowArg)
{
    CInputControlGump * row = reinterpret_cast<CInputControlGump *>(rowArg);
    if (selected != nullptr) {
        selected->row->hue = static_cast<uint16_t>(savedHue);
        selected->row->Redraw();
    }
    for (selected = cur_c0; selected != nullptr; selected = selected->next) {
        if (selected->row == row)
            break;
    }
    if (selected != nullptr) {
        savedHue = static_cast<int16_t>(selected->row->hue);
        selected->row->hue = 0x58;
        selected->row->Redraw();
        Orig_SetFocus(g_gumpManager, this);
    }
}
HOOK_VMETHOD(0x00417440, CEditListGump, OnFocus);

// 0x004174e0  size 0x32
// Delete (0xe02e) removes the selected row in the editable form, except the trailing one.
void __thiscall CEditListGump::OnKey(int key)
{
    if (editable != 0 && key == 0xe02e && selected != nullptr && selected != tail())
        RemoveItem(selected);
}
HOOK_VMETHOD(0x004174e0, CEditListGump, OnKey);

// 0x00417520  size 0x90
// Moves the focus from the focused row to the next one, scrolling down when that row would fall below the list.
void __thiscall CEditListGump::OnTab()
{
    int w;
    int h;
    CInputControlGump *focus = GLOBAL_AS(CInputControlGump *, g_focusedInput);
    gfx::ArtSize(0x13a7, &w, &h);
    h = h + 1;
    CEditListNode *node = nodeOf(focus);
    if (node != nullptr && node->next != nullptr) {
        Orig_CommitFocus(g_gumpManager);
        Orig_SetFocus(g_gumpManager, nodeOf(focus)->next->row);
        nodeOf(focus)->next->row->Redraw();
        if (nodeOf(focus)->next->row->y + h > height)
            Orig_ScrollDown(this);
    }
}
HOOK_VMETHOD(0x00417520, CEditListGump, OnTab);

// 0x004175b0  size 0x58
// Mouse down: remembers the press, brings this gump to the front, tracks the double-click state.
void __thiscall CEditListGump::MouseDown(int mx, int my)
{
    pressed = 1;
    unk_6c = mx;
    unk_70 = my;
    CGump_BringToFront(this, 0);
    if (clickState == 1) {
        clickState = 2;
        return;
    }
    clickState = 1;
    pressTick = static_cast<int32_t>(GetTickCount());
}
HOOK_VMETHOD(0x004175b0, CEditListGump, MouseDown);

// 0x00417610  size 0x42
// Mouse up: releases; a drag clears the click state.
void __thiscall CEditListGump::MouseUp(int, int)
{
    pressed = 0;
    if (dragging != 0) {
        dragging = 0;
        clickState = 0;
        g_dragGump = nullptr;
        return;
    }
    if (clickState == 2)
        clickState = 0;
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x00417610, CEditListGump, MouseUp);

// 0x00417660  size 0x81
// Mouse move: more than 4 pixels from the press starts a drag; while pressed the parent follows the mouse.
void __thiscall CEditListGump::MouseMove(int mx, int my)
{
    int wasPressed = pressed;
    if (wasPressed != 0 && static_cast<uint32_t>(clickState) > 0) {
        if (abs(mx - unk_6c) > 4 || abs(my - unk_70) > 4) {
            dragging = 1;
            clickState = 0;
        }
    }
    if (wasPressed != 0 && parent != nullptr)
        parent->Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);
}
HOOK_VMETHOD(0x00417660, CEditListGump, MouseMove);

// 0x004176f0  size 0x48
// Timer: a first press older than the double-click time is no longer a pending click (and becomes a drag if held).
void __thiscall CEditListGump::Tick()
{
    DWORD now = GetTickCount();
    DWORD dbl = GetDoubleClickTime();
    if (clickState == 1 && now - static_cast<DWORD>(pressTick) > dbl) {
        clickState = 0;
        if (pressed != 0)
            dragging = 1;
    }
}
HOOK_VMETHOD(0x004176f0, CEditListGump, Tick);

// 0x00417740  size 0x8e
// Redraws the background art and the overlay art (id + 1) clipped to the rows.
void __thiscall CEditListGump::RedrawBackground()
{
    int clip[4];
    clip[0] = 0;
    clip[1] = 0;
    clip[2] = width;
    clip[3] = (count - visibleRows) * 17 + 9;
    gfx::DrawArt(gfx::artPlain(), unk_50, 0, 0, gfx::bitmapOf(this) + 2, width, height);
    if (count != 0)
        gfx::DrawArtClipped(gfx::artPlain(), unk_50 + 1, clip, 0, 0, gfx::bitmapOf(this) + 2, width, height);
}
HOOK_METHOD(0x00417740, CEditListGump, RedrawBackground);

// 0x004177d0  size 0xbe
// Draws frame 0 (or frame 1 when +0x60 is set) at the absolute origin when the clip allows it, then the children.
void __thiscall CEditListGump::Draw(int *clip)
{
    int ox = 0;
    int oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);

    uint16_t *bmp;
    if (unk_60 == 0)
        bmp = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[1])));
    else
        bmp = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[3])));
    int rect[4];
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = bmp[0] + ox;
    rect[3] = oy + bmp[1];

    // 0x0043b540: thiscall on the gump manager object, (clip, &rect), RET 8
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, void *, int *)(clip, rect) != 0) {
        PORT_FUNC10(&GumpBlitBitmapClipped, void, __fastcall, void *, void *, void *, int, int, int, int, void *, int, int)(gfx::artPlain(), bmp + 2, clip, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
        for (CGump *c = firstChild; c != nullptr; c = c->next)
            PORT_VSLOT1(c, kGumpSlot_Draw, void, void *)(clip);
    }
}
HOOK_VMETHOD(0x004177d0, CEditListGump, Draw);
