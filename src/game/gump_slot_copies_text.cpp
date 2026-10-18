// Slot overrides of the text, board and book gumps that share a body with another class's slot function.
// In the original one function fills the slot of several classes (its linker folded them, or they inherit it);
// the gumps are moving to real virtual functions, where every class declares its own override. Each function
// below is a copy of the body named in its comment; the linker folds the identical copies back to one function,
// so the original address stays paired through the HOOK on the canonical copy.
// Gump virtual slots, prep piece P3 (docs/matching.md).
#include <cstring>
#include <windows.h>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "batch06_helpers.h"
#include "globals.h"
#include "crt.h"
#include "vcall.h"
#include "bb_gump.h"
#include "bb_line_gump.h"
#include "bb_msg_gump.h"
#include "book_gump.h"
#include "spell_icon_gump.h"
#include "button_gump.h"
#include "scroll_thumb_gump.h"
#include "sign_gump.h"
#include "dumb_gump.h"
#include "obj_picker_gump.h"
#include "string_query_gump.h"
#include "edit_group_gump.h"
#include "fighting_gump.h"
#include "text_gump.h"
#include "tips_gump.h"
#include "resize_gump.h"
#include "gump_base_ops_ports.h"
#include "window_helpers_31_ports.h"
#include "vtables.h"
#include "macro_gump.h"
#include "global_views.h"
#include "art_row_blits_ports.h"
#include "gump_art_ops_ports.h"

#define Gump_Uniconify(self) (reinterpret_cast<CBookGump *>(self)->Uniconify())
#define TG_RectIntersects(o, a1, a2) PORT_THISCALL2(&GumpRectIntersects, int, (o), int *, int *)(a1, a2)
#define TP_SetCursor(a1) (SetCursorForRegion((unsigned)(a1)))

// ---- from bb_gump.cpp
// Slot override: same body as CCourseGump::CloseAndDelete (0x00427b90); the linker folds the copies.
void __thiscall CBBGump::RButtonUp(int px, int py)
{
    (void)px;
    (void)py;
    g_dragGump = nullptr;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}

void __thiscall CFightingGump::Draw(int *clip)
{
    int sx = 0;
    int sy = 0;
    int rect[4];
    B6Bitmap *bmp;

    LocalToScreen(&sx, &sy);   // 0x0043b4e0
    if (unk_60 == 0)
        bmp = b6_frame_bitmap(this, 0);
    else
        bmp = b6_frame_bitmap(this, 1);
    rect[0] = sx;
    rect[1] = sy;
    rect[2] = sx + bmp->w;
    rect[3] = sy + bmp->h;
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, int *, int *)(clip, rect) != 0) {   // 0x0043b540
        GFX_BlitFrame(gfx::artPlain(), bmp->px, clip, bmp->w, bmp->h, sx, sy, g_backBuffer, 0x280, 0x1e0);   // 0x004342e0
        if (unk_60 == 0) {
            for (CGump *c = firstChild; c != nullptr; c = c->next)
                PORT_VSLOT1(c, 0x08, void, int *)(clip);
        }
    }
}

// Slot override: same body as CTextGump::Draw (0x0047a3d0); the linker folds the copies.
void __thiscall CTipsGump::Draw(int *clip)
{
    int sx = 0;
    int sy = 0;
    int rect[4];
    B6Bitmap *bmp;

    LocalToScreen(&sx, &sy);   // 0x0043b4e0
    if (unk_60 == 0)
        bmp = b6_frame_bitmap(this, 0);
    else
        bmp = b6_frame_bitmap(this, 1);
    rect[0] = sx;
    rect[1] = sy;
    rect[2] = sx + bmp->w;
    rect[3] = sy + bmp->h;
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, int *, int *)(clip, rect) != 0) {   // 0x0043b540
        GFX_BlitFrame(gfx::artPlain(), bmp->px, clip, bmp->w, bmp->h, sx, sy, g_backBuffer, 0x280, 0x1e0);   // 0x004342e0
        if (unk_60 == 0) {
            for (CGump *c = firstChild; c != nullptr; c = c->next)
                PORT_VSLOT1(c, 0x08, void, int *)(clip);
        }
    }
}

// Slot override: same body as CMacroCmdGump::ForwardToParent (0x00417d30); the linker folds the copies.
void __thiscall CBBLineGump::RButtonUp(int, int)
{
    CGump *par = parent;
    if (par != nullptr)
        PORT_VSLOT2(par, 0x1c, void, int, int)(0, 0);
}

// ---- from bb_msg_gump.cpp
// Slot override: same body as CCourseGump::CloseAndDelete (0x00427b90); the linker folds the copies.
void __thiscall CBBMsgGump::RButtonUp(int px, int py)
{
    (void)px;
    (void)py;
    g_dragGump = nullptr;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}

// 0x00406510  size 0x2a
// Slot HitTest (x, y) inside the gump, ignoring visibility. The original's one copy (formerly CGump::ContainsLocalPoint);
// CScrollThumbGump's copy folds onto it.
int __thiscall CBBMsgGump::HitTest(int px, int py)
{
    if (px >= 0 && py >= 0 && px < width && py < height)
        return 1;
    return 0;
}
HOOK_VMETHOD(0x00406510, CBBMsgGump, HitTest);

// ---- from spell_icon_gump.cpp
// Slot override: same body as CDeleteCharGump::HitTestFrame (0x004229c0); the linker folds the copies.
int __thiscall CSpellIconGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

// Slot override: same body as CCourseGump::CloseAndDelete (0x00427b90); the linker folds the copies.
void __thiscall CSpellIconGump::RButtonUp(int px, int py)
{
    (void)px;
    (void)py;
    g_dragGump = nullptr;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}

// ---- from button_gump_ops.cpp
// Slot override: same body as CButtonGump::RButtonDown (0x00413300); the linker folds the copies.
void __thiscall CScrollThumbGump::RButtonDown(int a, int b)
{
    PORT_VSLOT2(parent, 0x18, void, int, int)(a, b);
}

// Slot override: same body as CButtonGump::RButtonUp (0x00413320); the linker folds the copies.
void __thiscall CScrollThumbGump::RButtonUp(int a, int b)
{
    PORT_VSLOT2(parent, 0x1c, void, int, int)(a, b);
}

// Slot override: same body as CGump::ContainsLocalPoint (0x00406510); the linker folds the copies.
int __thiscall CScrollThumbGump::HitTest(int px, int py)
{
    if (px >= 0 && py >= 0 && px < width && py < height)
        return 1;
    return 0;
}

// ---- from sign_gump.cpp
// Slot override: same body as CStringPickerGump::HitTest (0x0046fd50); the linker folds the copies.
int __thiscall CSignGump::HitTest(int px, int py)
{
    B6Bitmap *bmp = b6_frame_bitmap(this, 0);
    int w = bmp->w;
    int h = bmp->h;
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return bmp->px[w * py + px] != 0 ? 1 : 0;
}

// Slot override: same body as CMacroCmdGump::OnMouseUp (0x00417d70); the linker folds the copies.
void __thiscall CSignGump::MouseUp(int, int)
{
    dragging = 0;
    g_dragGump = nullptr;
}

// Slot override: same body as CCourseGump::CloseAndDelete (0x00427b90); the linker folds the copies.
void __thiscall CSignGump::RButtonUp(int px, int py)
{
    (void)px;
    (void)py;
    g_dragGump = nullptr;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}

// ---- from dumb_gump_ops.cpp
// Slot override: same body as CMissileGump::HitTestNever (0x00439e80); the linker folds the copies.
int __thiscall CDumbGump::HitTest(int, int)
{
    return 0;
}

// ---- from obj_picker_gump_ops.cpp
// Slot override: same body as CStringPickerGump::HitTest (0x0046fd50); the linker folds the copies.
int __thiscall CObjPickerGump::HitTest(int px, int py)
{
    B6Bitmap *bmp = b6_frame_bitmap(this, 0);
    int w = bmp->w;
    int h = bmp->h;
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return bmp->px[w * py + px] != 0 ? 1 : 0;
}

// Slot override: same body as CFightingGump::BeginDrag (0x00431c20); the linker folds the copies.
void __thiscall CObjPickerGump::MouseDown(int grabX, int grabY)
{
    CGump_BringToFront(this, 0);   // 0x0043ba70
    unk_9c = 1;
    unk_6c = grabX;
    unk_70 = grabY;
    if (unk_a8 == 1) {
        unk_a8 = 2;
        return;
    }
    unk_a8 = 1;
    unk_a4 = static_cast<int32_t>(GetTickCount());
}

// Slot override: same body as CCourseGump::CloseAndDelete (0x00427b90); the linker folds the copies.
void __thiscall CObjPickerGump::RButtonUp(int px, int py)
{
    (void)px;
    (void)py;
    g_dragGump = nullptr;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}

// Slot override: same body as CSpellIconGump::MouseMove (0x0040c9b0); the linker folds the copies.
void __thiscall CObjPickerGump::MouseMove(int mouseX, int mouseY)
{
    if (unk_9c != 0)
        Move(mouseX - unk_6c, mouseY - unk_70, 1, nullptr, 1);
}

// Slot override: same body as CInputControlGump::UpdateClickState (0x0041ee00); the linker folds the copies.
void __thiscall CObjPickerGump::Tick()
{
    DWORD now = GetTickCount();
    UINT dct = GetDoubleClickTime();
    if (unk_a8 == 1 && (now - static_cast<DWORD>(unk_a4)) > dct) {
        unk_a8 = 0;
        if (unk_9c != 0)
            unk_94 = 1;
    }
}

// ---- from string_picker_gump.cpp
// Slot override: same body as CFightingGump::BeginDrag (0x00431c20); the linker folds the copies.
void __thiscall CStringPickerGump::MouseDown(int grabX, int grabY)
{
    CGump_BringToFront(this, 0);   // 0x0043ba70
    unk_9c = 1;
    unk_6c = grabX;
    unk_70 = grabY;
    if (unk_a8 == 1) {
        unk_a8 = 2;
        return;
    }
    unk_a8 = 1;
    unk_a4 = static_cast<int32_t>(GetTickCount());
}

// Slot override: same body as CInputControlGump::UpdateClickState (0x0041ee00); the linker folds the copies.
void __thiscall CStringPickerGump::Tick()
{
    DWORD now = GetTickCount();
    UINT dct = GetDoubleClickTime();
    if (unk_a8 == 1 && (now - static_cast<DWORD>(unk_a4)) > dct) {
        unk_a8 = 0;
        if (unk_9c != 0)
            unk_94 = 1;
    }
}

// Slot override: same body as CDeleteCharGump::HitTestFrame (0x004229c0); the linker folds the copies.
int __thiscall CStringQueryGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

// Slot override: same body as CSignGump::MouseDown (0x0043d6e0); the linker folds the copies.
void __thiscall CStringQueryGump::MouseDown(int mx, int my)
{
    BringToFront(0);   // 0x0043ba70
    unk_94 = 1;
    unk_6c = mx;
    unk_70 = my;
}

// Slot override: same body as CMacroCmdGump::OnMouseUp (0x00417d70); the linker folds the copies.
void __thiscall CStringQueryGump::MouseUp(int, int)
{
    unk_94 = 0;
    g_dragGump = nullptr;
}

// Slot override: same body as CDragReleaseGump::ClearDragCapture (0x0041c2f0); the linker folds the copies.
void __thiscall CStringQueryGump::RButtonUp(int mx, int my)
{
    (void)mx;
    (void)my;
    g_dragGump = nullptr;
}

// ---- from edit_group_gump.cpp
// Slot override: same body as CDeleteCharGump::HitTestFrame (0x004229c0); the linker folds the copies.
int __thiscall CEditGroupGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

// Slot override: same body as CSignGump::MouseDown (0x0043d6e0); the linker folds the copies.
void __thiscall CEditGroupGump::MouseDown(int mx, int my)
{
    BringToFront(0);   // 0x0043ba70
    unk_94 = 1;
    unk_6c = mx;
    unk_70 = my;
}

// Slot override: same body as CMacroCmdGump::OnMouseUp (0x00417d70); the linker folds the copies.
void __thiscall CEditGroupGump::MouseUp(int, int)
{
    unk_94 = 0;
    g_dragGump = nullptr;
}

// Slot override: same body as CCourseGump::CloseAndDelete (0x00427b90); the linker folds the copies.
void __thiscall CEditGroupGump::RButtonUp(int px, int py)
{
    (void)px;
    (void)py;
    g_dragGump = nullptr;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}

// Slot override: same body as CSignGump::MouseMove (0x0043d710); the linker folds the copies.
void __thiscall CEditGroupGump::MouseMove(int mx, int my)
{
    if (unk_94 != 0)
        Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);   // 0x0043b140
}

// Slot override: empty, as CLoginGump::Stub_00421390 (0x00421390); the linker folds the copies.
void __thiscall CEditGroupGump::OnChildChanged(int child)
{
}

// ---- from text_gump_ops.cpp
// Slot override: same body as CSkillListGump::HitTestPoint (0x004754d0); the linker folds the copies.
int __thiscall CTextGump::HitTest(int mx, int my)
{
    uint16_t *bmp;
    if (unk_60 == 0)
        bmp = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[1])));
    else
        bmp = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[3])));
    int w = bmp[0];
    int h = bmp[1];
    bmp += 2;
    if (mx < 0 || my < 0 || mx >= w || my >= h)
        return 0;
    if (bmp[w * my + mx] != 0)
        return 1;
    LocalToScreen(&mx, &my);
    for (CGump *c = lastChild; c != nullptr; c = c->prev) {
        int cx = mx;
        int cy = my;
        c->ScreenToLocal(&cx, &cy);
        if (PORT_VSLOT2(c, 0x0c, int, int, int)(cx, cy) != 0)
            return 1;
    }
    return 0;
}

// Slot override: same body as CFightingGump::EndDrag (0x00431c80); the linker folds the copies.
void __thiscall CTextGump::MouseUp(int unusedA, int unusedB)
{
    (void)unusedA;
    (void)unusedB;
    int32_t prevUnk94 = unk_94;
    held = 0;
    if (prevUnk94 != 0) {
        unk_94 = 0;
        pressState = 0;
        g_dragGump = nullptr;
        return;
    }
    if (pressState == 2) {
        int32_t notify = unk_60;
        pressState = 0;
        if (notify != 0)
            Gump_Uniconify(this);   // 0x0040d410
    }
    g_dragGump = nullptr;
}

// Slot override: same body as CCourseGump::CloseAndDelete (0x00427b90); the linker folds the copies.
void __thiscall CTextGump::RButtonUp(int px, int py)
{
    (void)px;
    (void)py;
    g_dragGump = nullptr;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}

// Slot override: same body as CTipsGump::MouseMove (0x0047ad00); the linker folds the copies.
void __thiscall CTextGump::MouseMove(int mx, int my)
{
    TP_SetCursor(0);   // 0x00482be0
    int wasHeld = held;
    if (wasHeld != 0 && pressState != 0) {
        int dx = mx - unk_6c;
        if (dx < 0)
            dx = -dx;
        int dy = my - unk_70;
        if (dy < 0)
            dy = -dy;
        if (dx > 4 || dy > 4) {
            unk_94 = 1;
            pressState = 0;
        }
    }
    if (wasHeld != 0)
        Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);   // 0x0043b140
}

// ---- from resize_gump.cpp
// Slot override: same body as CMissileGump::HitTestNever (0x00439e80); the linker folds the copies.
int __thiscall CResizeGump::HitTest(int, int)
{
    return 0;
}

// Slot override: empty, as CircleTrans_Nullsub (0x004242b0); the linker folds the copies.
void __thiscall CResizeGump::MouseOver(int x, int y)
{
}

// ---- from tips_gump.cpp
// Slot override: same body as CFightingGump::EndDrag (0x00431c80); the linker folds the copies.
void __thiscall CTipsGump::MouseUp(int unusedA, int unusedB)
{
    (void)unusedA;
    (void)unusedB;
    int32_t prevUnk94 = unk_94;
    held = 0;
    if (prevUnk94 != 0) {
        unk_94 = 0;
        pressState = 0;
        g_dragGump = nullptr;
        return;
    }
    if (pressState == 2) {
        int32_t notify = unk_60;
        pressState = 0;
        if (notify != 0)
            Gump_Uniconify(this);   // 0x0040d410
    }
    g_dragGump = nullptr;
}

// Slot override: same body as CCourseGump::CloseAndDelete (0x00427b90); the linker folds the copies.
void __thiscall CTipsGump::RButtonUp(int px, int py)
{
    (void)px;
    (void)py;
    g_dragGump = nullptr;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}

// ---- Gump virtual slots, prep piece P5: Draw copies for classes whose table pointed at another class's member.
// Slot override: same body as CBBMsgGump::Draw (0x00406450); the linker folds the copies.
void __thiscall CBBGump::Draw(int *clip)
{
    int ox = 0;
    int oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);

    int rect[4];
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = ox + gfx::bitmapOf(this)[0];
    rect[3] = gfx::bitmapOf(this)[1] + oy;
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, void *, int *)(clip, rect) == 0)
        return;
    uint16_t *bmp = gfx::bitmapOf(this);
    GFX_BlitFrame(gfx::artPlain(), bmp + 2, clip, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
    for (CGump *c = firstChild; c != nullptr; c = c->next)
        PORT_VSLOT1(c, kGumpSlot_Draw, void, void *)(clip);
}
