#include <windows.h>

#include <cstdlib>
#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "batch01_helpers.h"
#include "client_cfg_gump.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "sound_manager.h"
#include "vcall.h"
#include "macro_gump.h"
#include "gump_manager.h"
#include "music_manager.h"
#include "book_gump.h"
#include "window_helpers_31_ports.h"
#include "gump_base_ops_ports.h"
#include "vtables.h"

// The SEH frame of the original destructor (try level around the base destructor) is omitted.


// 0x00418830: add a row to the list box, RET 0x14, (a, b, c, 4, 0xe).
#define ListBox_AddRow(self, a1, a2, a3, a4, a5) (reinterpret_cast<CMacroListGump *>(self)->AddRow(a1, a2, (const char *)(a3), a4, a5))

HOOK_VTABLE(0x0049ccc0, CClientCfgGump);
HOOK_SDD(0x004157e0, CClientCfgGump);

// 0x00415810  size 0x102
// Closes the sub gumps (hue sample gumps and the like) that belong to this one, then frees the frames.
CClientCfgGump::~CClientCfgGump()
{
    CGump *g = g_gumpListTail;
    CGump *savedCursor = g_gumpDeleteCursor;
    g_clientCfgGump = nullptr;
    while (g != nullptr) {
        g_gumpDeleteCursor = g->prev;
        if (PORT_VSLOT0(g, 0xfc, int)() != 0) {
            if (RAWF(int32_t, g, 0xb4) != 0 || RAWF(int32_t, g, 0xb8) != 0)
                PORT_VSLOT2(g, 0x1c, void, int, int)(0, 0);
        }
        g = g_gumpDeleteCursor;
    }
    g_gumpDeleteCursor = savedCursor;
    if (frameData[1] != 0) {
        gfx::FreeTagged(reinterpret_cast<void *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[1]))), "cfg frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData[3] != 0) {
        gfx::FreeTagged(reinterpret_cast<void *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[3]))), "cfg frame1 pBMP");
        frameData[3] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "cfg framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00415810, CClientCfgGump);

// 0x00412a90  size 0x9e
void __thiscall CClientCfgGump::CheckPassword()
{
    if (strcmp(input_100->text, g_0069e630) != 0) {
        // 0x00439f60 on the gump manager: alert gump, RET 0x10, (text, 0, 0, 0)
        PORT_THISCALL4(&CreateGump2ac, void, g_gumpManager, const char *, int, int, int)("Password incorrect!", 0, 0, 0);
        return;
    }
    layer = 3;
    word_c6 = 3;
    word_c4 = 0x963;
    uint8_t *bmp = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[1])));
    gfx::DrawArt(gfx::artPlain(), 0x963, 0, 0, bmp + 4, width, height);
}
HOOK_METHOD(0x00412a90, CClientCfgGump, CheckPassword);

// 0x00415920  size 0x1b6
// ? name from the first pass. listCopyCur is a node of the key binding list: +0x04 key lParam, +0x08 / +0x0c /
// +0x10 modifier flags, +0x14 row count, rows of 0x14 bytes from +0x18 (a, b, heap text at +0x20).
void __thiscall CClientCfgGump::UpdateKeyBindingDisplay()
{
    char keyName[0x18];
    if (listCopyCur != nullptr) {
        GetKeyNameTextA(static_cast<LONG>(RAWF(uint32_t, listCopyCur, 4) & 0xbfffffffu), keyName, 0x14);
        strcpy(input_f0->text, keyName);
    } else {
        strcpy(input_f0->text, g_emptyString);
    }
    input_f0->Redraw();

    void *cur = listCopyCur;
    if (cur != nullptr && RAWF(int32_t, cur, 8) != 0) {
        toggle_e4->checked = 1;
        toggle_e4->Redraw();
    } else {
        toggle_e4->checked = 0;
        toggle_e4->Redraw();
    }
    cur = listCopyCur;
    if (cur != nullptr && RAWF(int32_t, cur, 0xc) != 0) {
        toggle_e8->checked = 1;
        toggle_e8->Redraw();
    } else {
        toggle_e8->checked = 0;
        toggle_e8->Redraw();
    }
    cur = listCopyCur;
    if (cur != nullptr && RAWF(int32_t, cur, 0x10) != 0) {
        toggle_ec->checked = 1;
        toggle_ec->Redraw();
    } else {
        toggle_ec->checked = 0;
        toggle_ec->Redraw();
    }

    // delete the rows of the list box, last to first
    CGump *row = listBox_16c->lastChild;
    while (row != nullptr) {
        CGump *prevRow = row->prev;
        if (PORT_VSLOT0(row, 0xd8, int)() != 0 && row != nullptr)
            PORT_VSLOT1(row, 0x00, void, int)(1);   // scalar deleting destructor
        row = prevRow;
    }
    // refill it from the node's rows, then add the blank row
    cur = listCopyCur;
    if (cur != nullptr) {
        for (int i = 0; i < RAWF(int32_t, listCopyCur, 0x14); i++) {
            unsigned off = static_cast<unsigned>(i) * 0x14;
            ListBox_AddRow(listBox_16c, RAWF(int32_t, listCopyCur, 0x18 + off), RAWF(int32_t, listCopyCur, 0x1c + off),
                           RAWF(int32_t, listCopyCur, 0x20 + off), 4, 0xe);
        }
    }
    ListBox_AddRow(listBox_16c, 0, 0, 0, 4, 0xe);
}
HOOK_METHOD(0x00415920, CClientCfgGump, UpdateKeyBindingDisplay);

// 0x00415ae0  size 0x24c
// A child control changed (ctrl). Keeps the slider visibility, the key binding node and the list in step.
void __thiscall CClientCfgGump::OnChildChanged(int ctrlArg)
{
    CGump * ctrl = reinterpret_cast<CGump *>(ctrlArg);
    if (layer == 4)
        unk_a4 = 1;
    if (ctrl == toggle_c8) {
        if (toggle_c8->checked != 0)
            thumb_124->visible = 1;
        else
            thumb_124->visible = 0;
    }
    if (ctrl == toggle_cc) {
        if (toggle_cc->checked != 0)
            thumb_128->visible = 1;
        else
            thumb_128->visible = 0;
    }
    if (listCopyCur != nullptr && ctrl == input_f0) {
        RAWF(int32_t, listCopyCur, 0) = g_savedKeyScan;
        RAWF(int32_t, listCopyCur, 4) = g_savedKeyLParam;
        UpdateKeyBindingDisplay();   // 0x00415920
    }
    if (listCopyCur != nullptr && ctrl == toggle_e4)
        RAWF(int32_t, listCopyCur, 8) = toggle_e4->checked;
    if (listCopyCur != nullptr && ctrl == toggle_ec)
        RAWF(int32_t, listCopyCur, 0x10) = toggle_ec->checked;
    if (listCopyCur != nullptr && ctrl == toggle_e8)
        RAWF(int32_t, listCopyCur, 0xc) = toggle_e8->checked;
    if (ctrl == listBox_16c) {
        // free the old row texts
        int rows = 0;
        for (int i = 0; i < RAWF(int32_t, listCopyCur, 0x14); i++) {
            void *text = RAWF(void *, listCopyCur, static_cast<unsigned>(i) * 0x14 + 0x20);
            if (text != nullptr)
                operator delete(text);
        }
        // rebuild the rows from the list box children that hold a value
        CGump *child = listBox_16c->firstChild;
        if (child != nullptr) {
            unsigned off = 0;
            for (; child != nullptr; child = child->next) {
                if (PORT_VSLOT0(child, 0xd8, int)() == 0)
                    continue;
                int32_t value = RAWF(CInputControlGump *, child, 0x98)->value;
                if (value == 0)
                    continue;
                RAWF(int32_t, listCopyCur, off + 0x18) = value;
                CInputControlGump *second = RAWF(CInputControlGump *, child, 0x9c);
                if (second != nullptr)
                    RAWF(int32_t, listCopyCur, off + 0x1c) = second->value;
                else
                    RAWF(int32_t, listCopyCur, off + 0x1c) = 0;
                CInputControlGump *third = RAWF(CInputControlGump *, child, 0xa0);
                if (third != nullptr) {
                    size_t len = strlen(third->text) + 1;
                    RAWF(void *, listCopyCur, off + 0x20) = ::operator new(len);
                    strcpy(RAWF(char *, listCopyCur, off + 0x20), RAWF(CInputControlGump *, child, 0xa0)->text);
                } else {
                    RAWF(int32_t, listCopyCur, off + 0x20) = 0;
                }
                rows++;
                off += 0x14;
            }
        }
        RAWF(int32_t, listCopyCur, 0x14) = rows;
    }
}
HOOK_VMETHOD(0x00415ae0, CClientCfgGump, OnChildChanged);

// 0x00415d30  size 0x62
void __thiscall CClientCfgGump::OnScroll(int pos, int range, int thumbArg)
{
    CGump * thumb = reinterpret_cast<CGump *>(thumbArg);
    if (thumb == thumb_124) {
        int v = (pos << 8) / range;
        if (v >= 0x100)
            v = 0xff;
        g_soundManager.SetVolume(static_cast<unsigned char>(v));
        return;
    }
    if (thumb == thumb_128) {
        int v = (pos << 8) / range;
        if (v >= 0x100)
            v = 0xff;
        // 0x0047f3b0: music manager volume, RET 4
        reinterpret_cast<MusicManager *>(GLOBAL_PTR(void, g_musicManager_storage))->SetVolume(v);
    }
}
HOOK_VMETHOD(0x00415d30, CClientCfgGump, OnScroll);

// 0x00415da0  size 0x4b
// The sound volume thumb was released: set the volume and play a sample sound.
void __thiscall CClientCfgGump::OnScrollEnd(int pos, int range, int thumbArg)
{
    CGump * thumb = reinterpret_cast<CGump *>(thumbArg);
    if (thumb == thumb_124) {
        int v = (pos << 8) / range;
        if (v >= 0x100)
            v = 0xff;
        g_soundManager.SetVolume(static_cast<unsigned char>(v));
        g_soundManager.PlaySound(0x51, 0, 0, 0, 0, 1);
    }
}
HOOK_VMETHOD(0x00415da0, CClientCfgGump, OnScrollEnd);

// 0x00415df0  size 0xa7
void __thiscall CClientCfgGump::MouseDown(int mx, int my)
{
    int lx = mx;
    int ly = my;
    gfx::GumpOrigin(this, &lx, &ly);   // 0x0043b4e0: screen -> local
    if (gfx::ScrollThumbTryGrab(thumb_124, lx, ly) != 0)   // 0x0041aac0
        return;
    if (gfx::ScrollThumbTryGrab(thumb_128, lx, ly) != 0)
        return;
    unk_9c = 1;
    unk_6c = mx;
    unk_70 = my;
    CGump_BringToFront(this, 0);   // 0x0043ba70
    if (unk_ac == 1) {
        unk_ac = 2;
        return;
    }
    unk_ac = 1;
    unk_a8 = static_cast<int32_t>(GetTickCount());
}
HOOK_VMETHOD(0x00415df0, CClientCfgGump, MouseDown);

// 0x00415ea0  size 0x53
void __thiscall CClientCfgGump::MouseUp(int mx, int my)
{
    (void)mx;
    (void)my;
    unk_9c = 0;
    if (unk_94 != 0) {
        unk_94 = 0;
        unk_ac = 0;
        g_dragGump = nullptr;
        return;
    }
    if (unk_ac == 2) {
        int32_t notify = unk_60;
        unk_ac = 0;
        if (notify != 0)
            reinterpret_cast<CBookGump *>(this)->Uniconify();
    }
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x00415ea0, CClientCfgGump, MouseUp);

// 0x00415f00  size 0x88
void __thiscall CClientCfgGump::MouseMove(int mx, int my)
{
    (g_app.*PORT_METHOD1(CApp, void, &SetCursorForRegion, int))(0);   // set the default mouse cursor
    int pressed = unk_9c;
    if (pressed != 0 && static_cast<uint32_t>(unk_ac) > 0) {
        if (abs(mx - unk_6c) > 4 || abs(my - unk_70) > 4) {
            unk_94 = 1;
            unk_ac = 0;
        }
    }
    if (pressed != 0)
        Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);   // 0x0043b140
}
HOOK_VMETHOD(0x00415f00, CClientCfgGump, MouseMove);

// 0x00415f90  size 0x26
void __thiscall CClientCfgGump::OnFocus(int ctrlArg)
{
    CGump * ctrl = reinterpret_cast<CGump *>(ctrlArg);
    if (ctrl == input_f0) {
        input_f0->unk_50 = 0x98c;
        input_f0->Redraw();
    }
}
HOOK_VMETHOD(0x00415f90, CClientCfgGump, OnFocus);

// 0x00415fc0  size 0x48
void __thiscall CClientCfgGump::Tick()
{
    DWORD now = GetTickCount();
    DWORD dbl = GetDoubleClickTime();
    if (unk_ac == 1 && now - static_cast<DWORD>(unk_a8) > dbl) {
        int32_t pressed = unk_9c;
        unk_ac = 0;
        if (pressed != 0)
            unk_94 = 1;
    }
}
HOOK_VMETHOD(0x00415fc0, CClientCfgGump, Tick);

// 0x00416010  size 0x106
// Draws the frame for the current page (frame0 when unk_60 is 0, else frame1), then the children that belong to
// the page: a child that answers slot 0xa8 is drawn when its +0xb4 equals unk_60, others only on page 0;
// children with a non-zero layer byte only on the matching tab.
void __thiscall CClientCfgGump::Draw(int *clip)
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
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, void *, int *)(clip, rect) == 0)
        return;
    PORT_FUNC10(&GumpBlitBitmapClipped, void, __fastcall, void *, void *, void *, int, int, int, int, void *, int, int)(gfx::artPlain(), bmp + 2, clip, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
    for (CGump *c = firstChild; c != nullptr; c = c->next) {
        if (PORT_VSLOT0(c, 0xa8, int)() != 0) {
            if (RAWF(int32_t, c, 0xb4) == unk_60) {
                if (c->layer == 0 || c->layer == layer)
                    PORT_VSLOT1(c, kGumpSlot_Draw, void, void *)(clip);
            }
        } else if (unk_60 == 0) {
            if (c->layer == 0 || c->layer == layer)
                PORT_VSLOT1(c, kGumpSlot_Draw, void, void *)(clip);
        }
    }
}
HOOK_VMETHOD(0x00416010, CClientCfgGump, Draw);
