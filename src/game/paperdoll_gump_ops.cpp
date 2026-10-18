// CPaperdollGump: scalar deleting destructor, mouse and tick handlers, draw
// pass, layer hit test, double click, and the mouse-over cursor handler
// 0x00467470; plus the layer-covered helper 0x00466b60.
// (Porting pass 2, batch 25.)
// 0x00466b60, 0x00467250, 0x004673a0, 0x00467410, 0x00467470, 0x004674a0,
// 0x004675b0, 0x00467c10, 0x00467cf0, 0x00467f30.
#include <windows.h>

#include "../runtime/ported.h"
#include "globals.h"
#include "mouse_manager.h"
#include "obj.h"
#include "object_manager.h"
#include "crt.h"
#include "paperdoll_gump.h"
#include "raw_field.h"
#include "vcall.h"
#include "client_misc_ops_ports.h"
#include "gump_art_ops_ports.h"
#include "gump_base_ops_ports.h"
#include "window_helpers_31_ports.h"
#include "book_gump.h"
#include "object_manager_ops_ports.h"
#include "art_row_blits_ports.h"

#define g_rowPlain PTR_AT(void, PORT_ADDR(&RowBlitCopyNonZero))

static inline int b25_IsGameMaster() { return Misc_Return0(); }
static inline void b25_ArtSize(int artId, int *w, int *h) { GumpArtSizeImpl(artId, w, h); }
static inline int b25_RectIntersects(const int *a, const int *b) { return PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, const int *, const int *)(a, b); }
static inline void b25_RectClip(const int *in, const int *clip, int *out) { GumpRectClip(in, clip, out); }
static inline void b25_BlitClipped(const void *src, const int *clip, int w, int h, int x, int y, const void *dest, int stride, int destH)
{
    PORT_FUNC10(&GumpBlitBitmapClipped, void, __fastcall, void *, const void *, const int *, int, int, int, int, const void *, int, int)(
        g_rowPlain, src, clip, w, h, x, y, dest, stride, destH);
}
static inline uint16_t b25_ArtPixel(int artId, int x, int y) { return GumpArtPixel(artId, x, y); }

// Pick up the item in equipment slot `slot` of the paperdoll's mobile when the player may do so
// (own paperdoll while alive, or the game master check), then redraw and clear the drag state.
// Returns true when the pick-up ran.
static bool b25_TryPickUp(CPaperdollGump *self, int slot)
{
    void *item = RAWF(void *, self->owner, 0xc4 + slot * 4);
    if (item == nullptr)
        return false;
    bool allowed = false;
    if (self->owner == g_player) {
        if (decomp::vcall(PORT_RET(int), g_player, 0x8c) == 0)
            allowed = true;
    }
    if (!allowed && b25_IsGameMaster() == 0)   // 0x00478cd0
        return false;
    g_objMgr.BeginPickUp(static_cast<CDynamicObj *>(item), 0);   // 0x00456b80
    CGump *held = g_objMgr.dragGump;                             // 0x0068ce8c
    if (held != nullptr)
        RAWF(CPaperdollGump *, held, 0x94) = self;
    self->RedrawFrame();   // 0x00467680
    self->unk_e8 = 0;
    self->unk_e0 = 0;
    self->unk_f4 = 0;
    return true;
}

// ---------------------------------------------------------------------------
// 0x00466b60  size 0x49
// ECX = layer, EDX = equipment array (mobile + 0xc4). Is the layer hidden by what is worn on top?
//   layers 3 and 4: slot 0x18 (+0x60) occupied; layer 0xb: slot 0x16 (+0x58) holds graphic 0x204e;
//   layers 0xd and 0x13: slot 0x16 occupied.
// ---------------------------------------------------------------------------
int __fastcall Paperdoll_LayerIsCovered(int layer, uint8_t *equip)
{
    switch (layer) {
    case 3:
    case 4:
        if (RAWF(void *, equip, 0x60) != nullptr)
            return 1;
        break;
    case 0xd:
    case 0x13:
        if (RAWF(void *, equip, 0x58) != nullptr)
            return 1;
        break;
    case 0xb: {
        void *robe = RAWF(void *, equip, 0x58);
        if (robe != nullptr && RAWF(uint16_t, robe, 0x38) == 0x204e)
            return 1;
        break;
    }
    default:
        break;
    }
    return 0;
}
HOOK_FUNC(0x00466b60, Paperdoll_LayerIsCovered);

HOOK_VTABLE(0x0049fc28, CPaperdollGump);
HOOK_SDD(0x00467250, CPaperdollGump);

// ---------------------------------------------------------------------------
// 0x004673a0  size 0x6a
// Left button down: remember the position, raise, count the click, find the layer under the cursor.
// unk_e8 = button is down, unk_f4 = click state (1 first click, 2 second), unk_f0 = tick of the first click.
// ---------------------------------------------------------------------------
void __thiscall CPaperdollGump::MouseDown(int mx, int my)
{
    unk_e8 = 1;
    unk_6c = mx;
    unk_70 = my;
    BringToFront(0);   // 0x0043ba70
    if (unk_f4 == 1) {
        unk_f4 = 2;
    } else {
        unk_f4 = 1;
        unk_f0 = static_cast<int32_t>(GetTickCount());
    }
    heldLayer = HitTestLayer(mx, my, 0);   // 0x00467cf0
}
HOOK_VMETHOD(0x004673a0, CPaperdollGump, MouseDown);

// ---------------------------------------------------------------------------
// 0x00467410  size 0x5b
// Left button up. unk_e0 = a drag started.
// ---------------------------------------------------------------------------
void __thiscall CPaperdollGump::MouseUp(int mx, int my)
{
    int32_t dragged = unk_e0;
    unk_e8 = 0;
    if (dragged != 0) {
        unk_e0 = 0;
        unk_f4 = 0;
    } else if (unk_f4 == 2) {
        unk_f4 = 0;
        OnDoubleClick(mx, my);   // 0x00467f30
    }
    heldLayer = -1;
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x00467410, CPaperdollGump, MouseUp);

// ---------------------------------------------------------------------------
// 0x00467470  size 0x2c
// RET 8, both arguments unused and `this` not read (ECX is replaced). Mouse-over handler:
// cursor 0x13 while a target callback is pending, otherwise 0x00482be0(0) (default cursor).
// ? the class this belongs to is inferred from its neighbours; the vtable was not checked.
// ---------------------------------------------------------------------------
void __thiscall CPaperdollGump::MouseOver(int, int)
{
    if (g_targetCallbackActive != 0) {
        g_mouse.SetMouseCursor(0x13);   // 0x00446840
        return;
    }
    PORT_THISCALL1(&SetCursorForRegion, void, GLOBAL_PTR(void, g_app_storage), int)(0);   // 0x00482be0, ECX is the app object (not read)
}
HOOK_VMETHOD(0x00467470, CPaperdollGump, MouseOver);

// ---------------------------------------------------------------------------
// 0x004674a0  size 0x107
// Mouse move. With the button down and a click pending, moving more than 4 pixels either way starts
// a drag: over an equipment layer it picks the item up (when allowed), otherwise (layer -1) the
// gump itself is moved.
// ---------------------------------------------------------------------------
void __thiscall CPaperdollGump::MouseMove(int mx, int my)
{
    if (unk_e8 != 0 && static_cast<uint32_t>(unk_f4) > 0) {
        int dx = mx - unk_6c;
        int dy = my - unk_70;
        int adx = (dx ^ (dx >> 31)) - (dx >> 31);
        int ady = (dy ^ (dy >> 31)) - (dy >> 31);
        if (adx > 4 || ady > 4) {
            int pickSlot = heldLayer;
            unk_e0 = 1;
            unk_f4 = 0;
            if (pickSlot != -1)
                b25_TryPickUp(this, pickSlot);   // 0x00478cd0 0x00456b80 0x00467680
        }
    }
    if (unk_e8 != 0 && heldLayer == -1)
        Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);   // 0x0043b140
}
HOOK_VMETHOD(0x004674a0, CPaperdollGump, MouseMove);

// ---------------------------------------------------------------------------
// 0x004675b0  size 0xcf
// Per-tick update. A single click that is older than the double-click time is delivered: without the
// button held it is a click (OnClick at the remembered position), with it held it becomes a drag.
// ---------------------------------------------------------------------------
void __thiscall CPaperdollGump::Tick()
{
    uint32_t now = GetTickCount();
    uint32_t dct = GetDoubleClickTime();
    if (unk_f4 != 1)
        return;
    if (now - static_cast<uint32_t>(unk_f0) <= dct)
        return;
    int32_t down = unk_e8;
    unk_f4 = 0;
    if (down == 0) {
        OnClick(unk_6c, unk_70);   // 0x00467dd0
        return;
    }
    unk_e0 = 1;
    int pickSlot = heldLayer;
    if (pickSlot != -1)
        b25_TryPickUp(this, pickSlot);   // 0x00478cd0 0x00456b80 0x00467680
}
HOOK_VMETHOD(0x004675b0, CPaperdollGump, Tick);

// ---------------------------------------------------------------------------
// 0x00467c10  size 0xd8
// Draw pass 2 (vtable slot 8), RET 4: blit frame bitmap 3 (iconified, unk_60 set) or 1 at the
// gump's screen position clipped to `clip`, then draw the children (slot 8 of each).
// ---------------------------------------------------------------------------
void __thiscall CPaperdollGump::Draw(int *clip)
{
    uint16_t *bmp;
    int rect[4];
    int out[4];
    int sx;
    int sy;

    if (unk_60 != 0)
        bmp = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[3])));
    else
        bmp = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[1])));
    g_paperdollRedrawFlag = 0;
    sx = 0;
    sy = 0;
    LocalToScreen(&sx, &sy);   // 0x0043b4e0
    rect[0] = sx;
    rect[1] = sy;
    rect[2] = bmp[0] + sx;
    rect[3] = bmp[1] + sy;
    if (b25_RectIntersects(clip, rect) == 0)   // 0x0043b540
        return;
    b25_RectClip(clip, rect, out);   // 0x0043b590
    PORT_FUNC10(&GumpBlitBitmapClipped, void, __fastcall, void *, const void *, const int *, int, int, int, int, const void *, int, int)(
        g_rowPlain, bmp + 2, out, bmp[0], bmp[1], sx, sy, g_backBuffer, 0x280, 0x1e0);   // 0x004342e0, row blitter 0x004764c0
    for (CGump *child = firstChild; child != nullptr; child = child->next)
        decomp::vcall_void(child, 8, out);
}
HOOK_VMETHOD(0x00467c10, CPaperdollGump, Draw);

// ---------------------------------------------------------------------------
// 0x00467cf0  size 0xd7
// Equipment layer under (x, y), or -1 (also always -1 while iconified). Walks the 25 layers of
// g_layerOrder from the last, skipping layers 0xb and 0x10 (and 0x15 when `flag` is 0) unless the game
// master check passes, and tests the pixel of the item's gump art (tiledata word +0xc, art id
// 0xc350 + that) at (x - 8, y - 0x13). RET 0xc.
// ---------------------------------------------------------------------------
int __thiscall CPaperdollGump::HitTestLayer(int mx, int my, int flag)
{
    if (unk_60 != 0)
        return -1;
    void *mob = owner;
    for (int i = 0x18; i >= 0; i--) {
        int equipLayer = g_equipLayerOrder[i];
        int art;
        if (RAWF(void *, mob, 0xc4 + equipLayer * 4) != nullptr) {
            bool gated = equipLayer == 0xb || equipLayer == 0x10 || (flag == 0 && equipLayer == 0x15);
            if (!gated || b25_IsGameMaster() != 0) {   // 0x00478cd0
                void *item = RAWF(void *, mob, 0xc4 + equipLayer * 4);
                art = RAWF(uint16_t, g_tileData + RAWF(uint16_t, item, 0x38) * 0x28, 0xc);
            } else
                art = 0;
        } else
            art = 0;
        if (art > 0 && art < 0xfff6) {
            art += 0xc350;
            if (mx >= 8 && my >= 0x13) {
                if (b25_ArtPixel(art, mx - 8, my - 0x13) != 0)   // 0x00434410
                    return equipLayer;
            }
        }
    }
    return -1;
}
HOOK_METHOD(0x00467cf0, CPaperdollGump, HitTestLayer);

// ---------------------------------------------------------------------------
// 0x00467f30  size 0xf9
// Double click, RET 8. Ignored while a target callback is pending. Iconified: restore. On an item:
// refuse with a bark when the player is dead (unless the item is graphic 0x33), otherwise use it when
// its serial has bit 0x40000000. On the paperdoll's guide button (art 0x7d2 at (0x1b, 0xc4), own
// paperdoll only): open the web page.
// ---------------------------------------------------------------------------
void __thiscall CPaperdollGump::OnDoubleClick(int mx, int my)
{
    int w;
    int h;

    if (g_targetCallbackActive != 0)
        return;
    if (unk_60 != 0) {
        reinterpret_cast<CBookGump *>(this)->Uniconify();   // restore
        return;
    }
    int slot = HitTestLayer(mx, my, 1);   // 0x00467cf0
    if (slot != -1) {
        void *item = RAWF(void *, owner, 0xc4 + slot * 4);
        if (item == nullptr)
            return;
        if (decomp::vcall(PORT_RET(int), g_player, 0x8c) != 0 && RAWF(uint16_t, item, 0x38) != 0x33) {
            g_player->CreateBarkKeepTwo(g_004ae3fc, 0x3b2, 3, nullptr);   // "I am dead and cannot do that."
            return;
        }
        if ((RAWF(uint32_t, item, 0x78) & 0x40000000u) == 0)
            return;
        PORT_THISCALL1(&Object_Use, void, &g_objMgr, void *)(item);   // Object_Use
        return;
    }
    b25_ArtSize(0x7d2, &w, &h);   // 0x00433e70
    if (isPlayer != 0 && mx >= 0x1b && my >= 0xc4 && mx <= w + 0x1b && my <= h + 0xc4) {
        Misc_ToggleIfFullscreen();
        LaunchWebBrowser(g_004ae3e8);   // "ultimaonline.com"
    }
}
HOOK_METHOD(0x00467f30, CPaperdollGump, OnDoubleClick);

// Own copies of bodies shared with other classes in the original (the linker folds them).
int __thiscall CPaperdollGump::HitTest(int px, int py)
{
    uint16_t *bmp;
    if (unk_60 != 0)
        bmp = static_cast<uint16_t *>(i32_ptr(frameData[3]));
    else
        bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
    int w = bmp[0];
    int h = bmp[1];
    bmp += 2;
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return bmp[w * py + px] != 0;
}

void __thiscall CPaperdollGump::RButtonUp(int px, int py)
{
    (void)px;
    (void)py;
    g_dragGump = nullptr;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}

