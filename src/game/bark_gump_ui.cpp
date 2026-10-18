// CBarkGump: destructor, text rendering, mouse handling, drawing and stacking of the floating speech
// text. The constructor is in bark_gump.cpp. Also the bitmap outline helper used by the bark gumps and
// CGump::AnyChildOverlapsSelf, which CBarkGump::Draw calls.
#include <cstring>
#include <windows.h>

#include "../runtime/ported.h"
#include "bark_gump.h"
#include "batch01_helpers.h"
#include "globals.h"
#include "map_gump.h"
#include "obj.h"
#include "crt.h"
#include "raw_field.h"
#include "gump.h"
#include "gump_base_ops_ports.h"
#include "art_row_blits_ports.h"
#include "window_helpers_31_ports.h"
#include "mobile_draw_ops_ports.h"
#include "app_support_ports.h"
#include "map_world_ops_ports.h"
#include "bark_gump_ui_ports.h"
#include "vtables.h"


// 0x0043a1e0: thiscall on the bark, no args: non-zero when the speaker is set and slot 0x24 or 0x54 is true.
#define Bark_HasSpeaker(self) (reinterpret_cast<CBarkGump *>(self)->SpeakerIsSpecial())
// 0x0043b4e0: thiscall on a gump, (&x, &y): screen origin.
#define Gump_ToScreen(self, a1, a2) (reinterpret_cast<CGump *>(self)->LocalToScreen(a1, a2))
// 0x0043b540: thiscall on the gump manager, (rectA, rectB), RET 8: non-zero when they intersect.
#define Mgr_Intersects(o, a1, a2) PORT_THISCALL2(&GumpRectIntersects, int, (o), int *, int *)(a1, a2)
// 0x0043b590: ECX = clip, EDX = &rect, stack = &out, RET 4: rect clipped to clip.
#define Rect_Clip(a1, a2, a3) (GumpRectClip((const int *)(a1), (const int *)(a2), a3))
// 0x0043a990: thiscall on the gump manager, (&dst, &src), RET 8: grow dst to contain src.
#define Mgr_RectUnion(o, a1, a2) PORT_THISCALL2(&GumpRectUnion, void, (o), int *, int *)(a1, a2)
// 0x004767c0: ECX = buffer, EDX = width.
#define Tile_SetTarget(a1, a2) (SetBlitSurface((uint16_t *)(a1), a2))
// 0x0043b760: ECX = rows, EDX = width, stack (dst, src, dstStride, srcStride), RET 0x10.
#define Blit_Rows(a1, a2, a3, a4, a5, a6) (GumpBlitRows(a1, a2, (uint16_t *)(a3), (uint16_t *)(a4), a5, a6))
// 0x00476550: ECX = dst, EDX = src, stack = count, RET 4: blend one row.
#define Blit_BlendRow(a1, a2, a3) (RowBlitHalfMaskNonZero((uint16_t *)(a1), (uint16_t *)(a2), a3))
// 0x00482c70: ECX = text, EDX = caption, stack = exit flag, RET 4: message box.
#define Fatal_Box(a1, a2, a3) (ShowMessageBoxAndExit(a1, a2, a3))
// 0x0042dbc0: thiscall on a mobile, no args: result 0x59 / 0x22 / 0x3b2.
#define Mobile_Class(o) PORT_THISCALL0(&Mobile_NameHue, int, (o))()
// 0x0044b260: no args.
#define Target_Cancel() (App_SetCursor11())
// 0x00482be0: (cursor id), RET 4; ECX is not read.
#define Cursor_Set(a1) PORT_THISCALL1(&SetCursorForRegion, void, GLOBAL_PTR(void, g_app_storage), int)(a1)
// 0x00444d50: thiscall (ECX = g_screenGump, not read), (object), RET 4: double-click the object.
#define Obj_DoubleClick(self, a1) (reinterpret_cast<CMapGump *>(self)->OnDoubleClick(a1))
// 0x00446310: thiscall on the frame allocator, (ptr, tag).
#define FrameData_Free(o, a1, a2) PORT_THISCALL2(&Map_Free, void, (o), void *, const char *)(a1, a2)

// 0x004011c0  size 0x89
// True when any later sibling (walking +0x40) that answers virtual slot 0x84 has a rect that intersects
// this gump's rect.
int __thiscall CGump::AnyChildOverlapsSelf()
{
    int mine[4];
    int other[4];

    mine[0] = x;
    mine[1] = y;
    mine[2] = width + x;
    mine[3] = height + y;
    for (CGump *g = next; g != nullptr; g = g->next) {
        if (PORT_VSLOT0(g, 0x84, int)() == 0)
            continue;
        other[0] = g->x;
        other[1] = g->y;
        other[2] = g->width + other[0];
        other[3] = g->height + other[1];
        if (Mgr_Intersects(g_gumpManager, mine, other) != 0)
            return 1;
    }
    return 0;
}
HOOK_METHOD(0x004011c0, CGump, AnyChildOverlapsSelf);

// 0x00401250  size 0x7
// Empty procedure, two stack args (RET 8); ECX and EDX are not read.
void __fastcall Stub_00401250(void *, void *, int, int)
{
}
HOOK_FUNC(0x00401250, Stub_00401250);

// 0x00401260  size 0x7
// Empty procedure, one stack arg (RET 4); ECX and EDX are not read.
void __fastcall Stub_00401260(void *, void *, int)
{
}
HOOK_FUNC(0x00401260, Stub_00401260);

// 0x00421390  size 0x7
// Empty procedure, one stack arg (RET 4). The default of the gump slot OnKey; also called as a plain function by packet 0x73,
// by the drag code and by packet 0x43 on non-gump objects (a pointer to a virtual member would be its call thunk).
void __fastcall Stub_00421390(void *, void *, int)
{
}
HOOK_FUNC(0x00421390, Stub_00421390);

HOOK_VTABLE(0x0049c000, CBarkGump);
HOOK_SDD(0x00401270, CBarkGump);

// 0x00401950  size 0x135
// Destructor body: unlinks from the speaker's bark list (or the top-level one), re-stacks the speaker's
// remaining barks, frees the bitmap and the text. The SEH frame is omitted.
CBarkGump::~CBarkGump()
{
    CObj *owner_ = speaker;
    if (owner_ != nullptr && owner_->barkList == this) {
        owner_->barkList = older;
    } else if (g_barkListHead == this) {
        g_barkListHead = older;
    }
    if (older != nullptr)
        older->newer = newer;
    if (newer != nullptr)
        newer->older = older;
    if (g_childDeleteFlag == 0 && g_suppressFlag == 0 && speaker != nullptr && speaker->barkList != nullptr) {
        for (CBarkGump *s = static_cast<CBarkGump *>(speaker->barkList); s != nullptr; s = s->older)
            s->Reposition();   // 0x00402090
        static_cast<CBarkGump *>(speaker->barkList)->StackAndClamp();   // 0x00402230
    }
    if (frameData != nullptr && frameData[1] != 0) {
        operator delete(reinterpret_cast<void *>(static_cast<uintptr_t>(frameData[1])));
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        FrameData_Free(g_frameDataAllocator, frameData, "draggump framedata");
        frameData = nullptr;
    }
    operator delete(text);
}
HOOK_DTOR(0x00401950, CBarkGump);

// 0x00401a90  size 0xaa
// 8-neighbour outline: every non-zero pixel that is not already `color` paints `color` into those of its
// eight neighbours that are zero. The outermost pixel ring is never scanned. ECX = 16-bit bitmap,
// EDX = width, stack (height, color), RET 8.
void __fastcall CBark_BitmapOutline(uint16_t *bmp, int width, int height, uint32_t color)
{
    int offs[8];
    for (int k = 0; k < 8; k++)
        offs[k] = g_dirDy[k] * width + g_dirDx[k];
    int idx = width + 1;
    if (height - 1 > 1) {
        int rows = height - 2;
        do {
            if (width - 1 > 1) {
                const uint16_t *p = bmp + idx;
                int cols = width - 2;
                do {
                    if (*p != 0 && *p != color) {
                        for (int k = 0; k < 8; k++) {
                            int n = idx + offs[k];
                            if (bmp[n] == 0)
                                bmp[n] = static_cast<uint16_t>(color);
                        }
                    }
                    idx++;
                    p++;
                    cols--;
                } while (cols != 0);
            }
            idx += 2;
            rows--;
        } while (rows != 0);
    }
}
HOOK_FUNC(0x00401a90, CBark_BitmapOutline);

// 0x00401b40  size 0x55
// Bring the gump to the top, then (when the speaker qualifies) send the 0x114 slot event 0x35 with the
// full-bitmap rect. Both stack args are unread.
void __thiscall CBarkGump::MouseOver(int, int)
{
    int rect[4];

    if (g_gumpListTail != this)
        CGump_BringToFront(this, 0);
    if (Bark_HasSpeaker(this) != 0) {
        rect[0] = 0;
        rect[1] = 0;
        rect[2] = rowStride;
        rect[3] = height;
        PORT_VSLOT2(this, 0x114, void, int, int *)(0x35, rect);
    }
}
HOOK_VMETHOD(0x00401b40, CBarkGump, MouseOver);

// 0x00401ba0  size 0x43
// As Refresh35 without the raise, and with the event id taken from the hue field.
void __thiscall CBarkGump::MouseLeave()
{
    int rect[4];

    if (Bark_HasSpeaker(this) != 0) {
        rect[3] = height;
        rect[2] = rowStride;
        rect[0] = 0;
        rect[1] = 0;
        PORT_VSLOT2(this, 0x114, void, int, int *)(hue, rect);
    }
}
HOOK_VMETHOD(0x00401ba0, CBarkGump, MouseLeave);

// 0x00401bf0  size 0x41
// Draw the wrapped text into the bitmap (past its 4-byte header) with the given hue and clip rect.
void __thiscall CBarkGump::RenderText(int hue_, int *rect)
{
    gfx::TextDrawWrapped(0, 0, static_cast<int>(font), hue_, text, rowStride,
                         reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(frameData[1])) + 4, rowStride, rect, 0, 0);
}
HOOK_VMETHOD(0x00401bf0, CBarkGump, RenderText);

// 0x00401c40  size 0x5a
// Per-tick: when the first click is older than the double-click time, fire the single click (button
// already released, unk_c4 == 0) or mark a drag (unk_bc = 1).
void __thiscall CBarkGump::Tick()
{
    DWORD now = GetTickCount();
    DWORD dbl = GetDoubleClickTime();
    if (unk_d0 == 1 && static_cast<DWORD>(now - static_cast<DWORD>(unk_cc)) > dbl) {
        unk_d0 = 0;
        if (unk_c4 == 0) {
            SingleClick(unk_6c, unk_70);   // 0x00401d70
            return;
        }
        unk_bc = 1;
    }
}
HOOK_VMETHOD(0x00401c40, CBarkGump, Tick);

// 0x00401ca0  size 0x4d
// Left button down: remember the position (in the base fields +0x6c / +0x70); a second click inside the
// window sets state 2, otherwise state 1 and the time.
void __thiscall CBarkGump::MouseDown(int mouseX, int mouseY)
{
    unk_c4 = 1;
    unk_6c = mouseX;
    unk_70 = mouseY;
    if (unk_d0 == 1) {
        unk_d0 = 2;
        return;
    }
    unk_d0 = 1;
    unk_cc = static_cast<int32_t>(GetTickCount());
}
HOOK_VMETHOD(0x00401ca0, CBarkGump, MouseDown);

// 0x00401cf0  size 0x80
// Left button up. A pending drag just resets the state; a second click double-clicks the speaker.
void __thiscall CBarkGump::MouseUp(int, int)
{
    unk_c4 = 0;
    if (unk_bc != 0) {
        unk_bc = 0;
        unk_d0 = 0;
        g_dragGump = nullptr;
        return;
    }
    if (unk_d0 == 2) {
        unk_d0 = 0;
        if (speaker != nullptr) {
            if (PORT_VSLOT0(speaker, 0x24, int)() != 0 || PORT_VSLOT0(speaker, 0x54, int)() != 0)
                Obj_DoubleClick(g_screenGump, speaker);
        }
    }
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x00401cf0, CBarkGump, MouseUp);

// 0x00401d70  size 0xe2
// Single click on the speaker while a target cursor is pending: either cancel the cursor and ask for
// confirmation (notoriety warning) or call the pending target callback with ECX = the speaker.
void __thiscall CBarkGump::SingleClick(int, int)
{
    if (speaker == nullptr)
        return;
    if (PORT_VSLOT0(speaker, 0x24, int)() == 0 && PORT_VSLOT0(speaker, 0x54, int)() == 0)
        return;
    if (g_targetCallbackActive == 0)
        return;
    if (g_targetCallback == 0)
        return;
    if (g_modalActive != 0)
        return;

    CObj *target = nullptr;
    if (PORT_VSLOT0(speaker, 0x24, int)() != 0)
        target = speaker;
    if (g_warnOnAttack != 0 && target != nullptr && g_targetFlags != 0) {
        if ((g_targetFlags == 1 && Mobile_Class(target) == 0x59) || (g_targetFlags == 2 && Mobile_Class(target) == 0x22)) {
            Target_Cancel();
            g_targetFlags = 0;
            Cursor_Set(0);
            ConfirmLowerNotorietyA(RAWF(void *, target, 0x78));
            return;
        }
    }
    PORT_THISCALL0((uint32_t)(g_targetCallback), void, speaker)();
}
HOOK_METHOD(0x00401d70, CBarkGump, SingleClick);

// 0x00401e60  size 0x230
// Blit the bark bitmap to the 640-wide 16-bit back buffer, clipped to `clip`. Where a later sibling
// overlaps, non-zero pixels are blended 50% with the back buffer instead of copied.
void __thiscall CBarkGump::Draw(int *clip)
{
    int sx = 0;
    int sy = 0;
    int mine[4];
    int vis[4];

    Gump_ToScreen(this, &sx, &sy);
    if (visible == 0)
        return;
    mine[0] = sx;
    mine[1] = sy;
    mine[2] = width + sx;
    mine[3] = height + sy;
    Tile_SetTarget(g_backBuffer, 0x280);
    if (Mgr_Intersects(g_gumpManager, clip, mine) == 0)
        return;
    Rect_Clip(clip, mine, vis);

    int dx = 0;
    int dy = 0;
    if (mine[0] != vis[0])
        dx = vis[0] - mine[0];
    if (mine[1] != vis[1])
        dy = vis[1] - mine[1];
    const uint16_t *src = reinterpret_cast<const uint16_t *>(
        reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(frameData[frameIndex * 2 + 1])) + 4) + (dy * rowStride + dx);
    uint16_t *dst = g_backBuffer + ((dy + sy) * 0x280 + dx + sx);
    uint16_t *dst0 = dst;

    if (AnyChildOverlapsSelf() != 0) {   // 0x004011c0
        int w = vis[2] - vis[0];
        int h = vis[3] - vis[1];
        int stride = rowStride;
        if (g_is565 != 0) {
            if (h <= 0)
                return;
            do {
                if (w != 0) {
                    for (int i = 0; i < w; i++) {
                        uint16_t p = src[i];
                        if (p != 0)
                            dst[i] = static_cast<uint16_t>((((dst[i] >> 1) & 0x7bef) + ((p >> 1) & 0x7bef)) | 1);
                    }
                }
                dst += 0x280;
                src += stride;
                h--;
            } while (h != 0);
        } else {
            if (h <= 0)
                return;
            do {
                Blit_BlendRow(dst, const_cast<uint16_t *>(src), w);
                dst += 0x280;
                src += stride;
                h--;
            } while (h != 0);
        }
        return;
    }
    if (PORT_VSLOT0(this, 0x7c, int)() != 0) {
        Blit_Rows(vis[3] - vis[1], vis[2] - vis[0], dst0, const_cast<uint16_t *>(src), 0x280, rowStride);
        return;
    }
    for (int row = vis[1]; row < vis[3]; row++) {
        memcpy(dst, src, static_cast<size_t>(vis[2] - vis[0]) * 2);
        dst += 0x280;
        src += rowStride;
    }
}
HOOK_VMETHOD(0x00401e60, CBarkGump, Draw);

// 0x00402090  size 0x192
// Re-anchor above the speaker after it moved: shift homeX / homeY by the change of the anchor, clamp to
// the screen and queue the union of the old and new rect as a dirty rect.
void __thiscall CBarkGump::Reposition()
{
    int oldRect[4];
    int newRect[4];

    oldRect[0] = x;
    oldRect[2] = width + x;
    oldRect[1] = y;
    oldRect[3] = height + y;
    CObj *who = speaker;
    if (who == nullptr)
        return;
    if (who == g_player)
        return;
    if (PORT_VSLOT0(who, 0x2c, int)() != 0 && RAWF(int32_t, speaker, 0x7c) != 0)
        return;

    int anchorX = ((RAWF(int32_t, speaker, 0x0c) + RAWF(int32_t, speaker, 0x04)) >> 1) - g_scrollX;
    int anchorY = (RAWF(int32_t, speaker, 0x08) - g_scrollY) + 4;
    int oldAnchorX = unk_9c;
    unk_9c = anchorX;
    homeX = homeX + (anchorX - oldAnchorX);
    int oldAnchorY = unk_a0;
    unk_a0 = anchorY;
    x = homeX;
    homeY += anchorY - oldAnchorY;
    y = homeY;
    int w = width;
    if (x + w > 0x27f)
        x = 0x27f - w;
    if (x < 0)
        x = 0;
    int h = height;
    if (y + h > 0x1df)
        y = 0x1df - h;
    if (y < 0)
        y = 0;
    newRect[0] = x;
    newRect[1] = y;
    newRect[2] = w + x;
    newRect[3] = h + y;
    Mgr_RectUnion(g_gumpManager, oldRect, newRect);
    if (g_dirtyCount < 1000) {
        int32_t *slot = g_00648eb0 + g_dirtyCount * 4;
        slot[0] = oldRect[0];
        slot[1] = oldRect[1];
        slot[2] = oldRect[2];
        slot[3] = oldRect[3];
        g_dirtyCount = g_dirtyCount + 1;
    } else {
        Fatal_Box("TOO MANY GUMP DIRTY RECTS!", "Nicked Gut", 0);
        g_dirtyCount = 0;
    }
}
HOOK_METHOD(0x00402090, CBarkGump, Reposition);

// 0x00402230  size 0x123
// Walk the speaker's bark chain (this = any member): sum the heights; while the total reaches 0x1e0 delete
// the oldest barks (with g_deletingBarks set so their destructors do not re-stack); then, if the oldest
// remaining bark starts above the screen, stack everything down from y = 0, otherwise if the newest
// ends below 0x1e0, stack everything up from the bottom.
void __thiscall CBarkGump::StackAndClamp()
{
    int total = height;
    CBarkGump *newest = this;
    for (CBarkGump *p = newer; p != nullptr; p = p->newer) {
        newest = p;
        total += p->height;
    }
    CBarkGump *oldest = this;
    for (CBarkGump *p = older; p != nullptr; p = p->older) {
        oldest = p;
        total += p->height;
    }
    while (total >= 0x1e0 && oldest != this) {
        total -= oldest->height;
        CBarkGump *victim = oldest;
        oldest = oldest->newer;
        g_childDeleteFlag = 1;
        if (victim != nullptr)
            PORT_VSLOT1(victim, 0x00, void, int)(1);
        g_childDeleteFlag = 0;
    }
    if (oldest != nullptr && oldest->homeY < 0) {
        CBarkGump *p = oldest->newer;
        oldest->y = 0;
        for (; p != nullptr; p = p->newer)
            p->y = p->older->y + p->older->height;
        return;
    }
    if (newest != nullptr && newest->homeY + newest->height > 0x1e0) {
        newest->y = 0x1e0 - newest->height;
        for (CBarkGump *p = newest->older; p != nullptr; p = p->older)
            p->y = p->newer->y - p->height;
    }
}
HOOK_METHOD(0x00402230, CBarkGump, StackAndClamp);
