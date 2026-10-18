// Dirty rectangles of the 640x480 back buffer (second pass, batch 19): queue a rectangle, invalidate one
// with scroll wrap, scroll the buffer, flush the queue through the world renderer and the light pass,
// and invalidate everything.
//
// State: scroll offset into the buffer (pixels, wraps at 0x4b000 = 640 * 480) with its x / y split, the
// queue of up to 0x100 rectangles {left, top, right, bottom} at 0x006f5148, and the full-redraw flag.
// The SEH frames of the original are omitted.
#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"
#include "gump_manager.h"
#include "light.h"
#include "multi_placer.h"
#include "world_renderer.h"
#include "mouse_manager.h"
#include "multi_placer_ports.h"

#define dr_mouse GLOBAL_PTR(void, g_mouse_storage)

// 0x0044f6e0, 0x0044f8b0: ECX = rectangle {left, top, right, bottom}.
static void __fastcall AddDirtyRect(int *r);
void __fastcall DirtyRects_Invalidate(int *r);

// 0x0044f6e0  size 0x1c3
// Queue a rectangle. Ignored when everything is already dirty or the rectangle is empty or off screen.
// With a full queue and no suppression the whole screen is invalidated and the full-redraw flag set. A
// rectangle that overlaps a queued one is cut into up to four strips outside it (each added the same way)
// and not added itself.
static void __fastcall AddDirtyRect(int *r)
{
    int x0;
    int y0;
    int x1;
    int y1;
    int count;
    int i;
    int *e;
    int t[4];

    if (dr_fullRedraw != 0)
        return;
    x0 = r[0];
    if (x0 >= 0x280)
        return;
    x1 = r[2];
    if (x1 <= 0)
        return;
    y0 = r[1];
    if (y0 >= 0x1e0)
        return;
    y1 = r[3];
    if (y1 <= 0)
        return;
    if (x1 <= x0)
        return;
    if (y1 <= y0)
        return;
    count = dr_count;
    if (count >= 0x100) {
        if (g_suppressSpeech == 0) {
            g_scrollOffsetY = 0;
            g_scrollOffsetX = 0;
            g_scrollOffset = 0;
            dr_count = 0;
            DirtyRects_Invalidate(dr_screenRect);
            dr_fullRedraw = 1;
        }
        return;
    }
    for (i = 0; i < count; i++) {
        e = &dr_rects[i * 4];
        if (x1 > e[0] && x0 < e[2] && y1 > e[1] && y0 < e[3]) {
            if (e[0] > x0) {
                t[0] = x0;
                t[1] = (y0 > e[1]) ? y0 : e[1];
                t[3] = (y1 < e[3]) ? y1 : e[3];
                t[2] = e[0];
                AddDirtyRect(t);
            }
            if (e[2] < r[2]) {
                t[0] = e[2];
                t[1] = (r[1] > e[1]) ? r[1] : e[1];
                t[3] = (r[3] < e[3]) ? r[3] : e[3];
                t[2] = r[2];
                AddDirtyRect(t);
            }
            if (e[1] > r[1]) {
                t[0] = r[0];
                t[1] = r[1];
                t[2] = r[2];
                t[3] = e[1];
                AddDirtyRect(t);
            }
            if (e[3] < r[3]) {
                t[0] = r[0];
                t[1] = e[3];
                t[2] = r[2];
                t[3] = r[3];
                AddDirtyRect(t);
            }
            return;
        }
    }
    e = &dr_rects[count * 4];
    count++;
    e[0] = r[0];
    e[1] = r[1];
    e[2] = r[2];
    dr_count = count;
    e[3] = r[3];
}
HOOK_FUNC(0x0044f6e0, AddDirtyRect);

// 0x0044f8b0  size 0x191
// Invalidate a screen rectangle: clamp it to the screen and queue it. The back buffer is a ring that starts
// at the scroll offset, so a rectangle whose first and last pixel lie on different sides of the wrap point
// (0x4b000 - scroll offset) is queued in two or, when the wrap also falls inside its columns, four pieces.
void __fastcall DirtyRects_Invalidate(int *r)
{
    int x0;
    int x1;
    int y0;
    int y1;
    int q[4];
    int left;
    int top;
    int right;
    int bottom;
    int first;
    int last;
    int wrap;
    int wrapX;
    int wrapY;

    if (dr_fullRedraw != 0)
        return;
    if (g_suppressSpeech != 0)
        return;
    x0 = r[0];
    if (x0 >= 0x280)
        return;
    x1 = r[2];
    if (x1 <= 0)
        return;
    y0 = r[1];
    if (y0 >= 0x1e0)
        return;
    y1 = r[3];
    if (y1 <= 0)
        return;
    left = x0;
    if (x0 < 0) {
        left = 0;
        x0 = 0;
    }
    right = x1;
    if (x1 > 0x280) {
        right = 0x280;
        x1 = 0x280;
    }
    top = y0;
    if (y0 < 0) {
        y0 = 0;
        top = 0;
    }
    bottom = y1;
    if (y1 > 0x1e0) {
        bottom = 0x1e0;
        y1 = 0x1e0;
    }
    first = y0 * 640 + x0;
    last = y1 * 640 + x1 - 0x281;
    wrap = 0x4b000 - g_scrollOffset;
    if ((first < wrap) == (last < wrap)) {
        q[0] = left;
        q[1] = top;
        q[2] = right;
        q[3] = bottom;
        AddDirtyRect(q);
        return;
    }
    wrapX = wrap % 640;
    wrapY = wrap / 640;
    if (wrapX > left) {
        if (wrapX < right) {
            q[1] = top;
            q[0] = left;
            q[2] = wrapX;
            q[3] = wrapY + 1;
            AddDirtyRect(q);
            q[0] = wrapX;
            q[2] = right;
            q[3] = wrapY;
            AddDirtyRect(q);
            q[1] = wrapY;
            q[3] = bottom;
            AddDirtyRect(q);
            q[0] = left;
            q[2] = wrapX;
            q[1] = wrapY + 1;
            AddDirtyRect(q);
            return;
        }
        wrapY++;
    } else if (wrapX >= right) {
        wrapY++;
    }
    q[2] = right;
    q[1] = top;
    q[0] = left;
    q[3] = wrapY;
    AddDirtyRect(q);
    q[1] = wrapY;
    q[3] = bottom;
    AddDirtyRect(q);
}
HOOK_FUNC(0x0044f8b0, DirtyRects_Invalidate);

// 0x0044fa50  size 0x203
// Scroll the back buffer by (dx = ECX, dy = EDX) pixels: move the scroll offset (wrapped into
// [0, 0x4b000]), shift and clamp every queued rectangle, then invalidate the strips the scroll uncovered.
void __fastcall ScrollBackBuffer(int dx, int dy)
{
    int off;
    int count;
    int *e;
    int y0;
    int q[4];

    if (dr_fullRedraw != 0)
        return;
    off = g_scrollOffset + (-dx - dy * 640);
    g_scrollOffset = off;
    if (off > 0x4b000) {
        off -= 0x4b000;
        g_scrollOffset = off;
    }
    if (off < 0) {
        off += 0x4b000;
        g_scrollOffset = off;
    }
    g_scrollOffsetX = off % 640;
    g_scrollOffsetY = off / 640;
    count = dr_count;
    if (count > 0) {
        e = &dr_rects[0];
        do {
            y0 = e[1];
            e[3] += dy;
            e[0] += dx;
            e[2] += dx;
            y0 += dy;
            e[1] = y0;
            if (y0 < 0)
                e[1] = 0;
            if (e[0] < 0)
                e[0] = 0;
            if (e[3] > 0x1e0)
                e[3] = 0x1e0;
            if (e[2] > 0x280)
                e[2] = 0x280;
            e += 4;
            count--;
        } while (count != 0);
    }
    if (dr_fullRedraw != 0)
        return;
    if (dx < 0) {
        q[1] = 0;
        if (dy < 0) {
            q[0] = dx + 0x280;
            q[3] = 0x1e0;
            q[2] = 0x280;
            DirtyRects_Invalidate(q);
            q[1] = dy + 0x1e0;
            q[0] = 0;
            q[3] = 0x1e0;
            q[2] = dx + 0x280;
            DirtyRects_Invalidate(q);
            return;
        }
        if (dy > 0) {
            q[0] = dx + 0x280;
            q[3] = 0x1e0;
            q[2] = 0x280;
            DirtyRects_Invalidate(q);
            q[1] = 0;
            q[0] = 0;
            q[3] = dy;
            q[2] = dx + 0x280;
            DirtyRects_Invalidate(q);
            return;
        }
        q[3] = 0x1e0;
        q[0] = dx + 0x280;
        q[2] = 0x280;
        DirtyRects_Invalidate(q);
        return;
    }
    if (dx > 0) {
        q[1] = 0;
        q[0] = 0;
        if (dy < 0) {
            q[3] = 0x1e0;
            q[2] = dx;
            DirtyRects_Invalidate(q);
            q[0] = dx;
            q[1] = dy + 0x1e0;
            q[3] = 0x1e0;
            q[2] = 0x280;
            DirtyRects_Invalidate(q);
            return;
        }
        q[3] = 0x1e0;
        q[2] = dx;
        if (dy == 0) {
            DirtyRects_Invalidate(q);
            return;
        }
        DirtyRects_Invalidate(q);
        q[1] = 0;
        q[0] = dx;
        q[3] = dy;
        q[2] = 0x280;
        DirtyRects_Invalidate(q);
        return;
    }
    if (dy < 0) {
        q[0] = 0;
        q[1] = dy + 0x1e0;
        q[3] = 0x1e0;
        q[2] = 0x280;
        DirtyRects_Invalidate(q);
        return;
    }
    if (dy == 0)
        return;
    q[1] = 0;
    q[0] = 0;
    q[3] = dy;
    q[2] = 0x280;
    DirtyRects_Invalidate(q);
}
HOOK_FUNC(0x0044fa50, ScrollBackBuffer);

// 0x0044fc60  size 0x184
// Redraw the queue: first take the multi placement preview off the world (when a multi is loaded and the
// mouse is over the world view), then for every non-empty rectangle (clamped to the screen) draw the world
// into it and run the light pass, with the back buffer pointer moved to the ring position of its first
// pixel. The queue is then published (copied to 0x0071c818, count in 0x00721040 / 0x00648ea8) and cleared.
void __cdecl FlushDirtyRects()
{
    int mouseX;
    int mouseY;
    int cellX;
    int cellY;
    int count;
    int i;
    int off;
    int *e;
    uint16_t *saved;

    if (g_loadedMulti != nullptr) {
        reinterpret_cast<CMouseManager *>(dr_mouse)->GetMousePos(&mouseX, &mouseY);   // CMouseManager::GetMousePos
        if (g_gumpMgr.GumpAtPoint(mouseX, mouseY, nullptr, 0) == g_screenGump) {
            PORT_THISCALL4(&Mouse_TerrainTileAt, void, dr_mouse, int, int, int *, int *)(mouseX, mouseY, &cellX, &cellY);
            g_multiPlacer->Place(cellX, cellY);
        }
    }
    count = dr_count;
    i = 0;
    if (count > 0) {
        saved = g_backBuffer;
        do {
            e = &dr_rects[i * 4];
            if (e[1] < 0)
                e[1] = 0;
            if (e[0] < 0)
                e[0] = 0;
            if (e[3] > 0x1e0)
                e[3] = 0x1e0;
            if (e[2] > 0x280)
                e[2] = 0x280;
            if (e[2] > e[0] && e[3] > e[1]) {
                off = g_scrollOffset;
                if (e[1] * 640 + e[0] >= 0x4b000 - off)
                    off = off - 0x4b000;
                g_backBuffer = g_backBuffer + off;
                g_worldRenderer.RedrawRect(e);
                reinterpret_cast<LightPass *>(e)->Apply();
                count = dr_count;
                g_backBuffer = saved;
            }
            i++;
        } while (i < count);
    }
    memcpy(dr_published, dr_rects, static_cast<size_t>(count) * 16);
    dr_publishedCount = count;
    dr_fullRedraw = 0;
    dr_publishedCount2 = count;
    dr_count = 0;
    if (g_loadedMulti != nullptr)
        MultiObjects_DeleteAll();   // MultiObjects_DeleteAll
}
HOOK_FUNC(0x0044fc60, FlushDirtyRects);

// 0x0044fdf0  size 0x3d
// Invalidate the whole screen and mark everything dirty.
void __cdecl InvalidateAll()
{
    if (dr_fullRedraw == 0 && g_suppressSpeech == 0) {
        g_scrollOffsetY = 0;
        g_scrollOffsetX = 0;
        g_scrollOffset = 0;
        dr_count = 0;
        DirtyRects_Invalidate(dr_screenRect);
        dr_fullRedraw = 1;
    }
}
HOOK_FUNC(0x0044fdf0, InvalidateAll);

// 0x0044fe30  size 0x2b
// ECX -> four signed bytes {x0, y0, x1, y1}: returns 1 when |x0 - x1| <= |y0 - y1|.
int __fastcall RectIsTall(const int8_t *r)
{
    int dx = static_cast<int>(r[0]) - static_cast<int>(r[2]);
    int dy = static_cast<int>(r[1]) - static_cast<int>(r[3]);
    int ax = (dx < 0) ? -dx : dx;
    int ay = (dy < 0) ? -dy : dy;
    return ax <= ay ? 1 : 0;
}
HOOK_FUNC(0x0044fe30, RectIsTall);
