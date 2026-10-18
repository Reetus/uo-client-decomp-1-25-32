// CMouseManager: hover gump updates, cursor shape selection and the cursor drawing / background saving
// code. Procedures 0x004474f0 .. 0x004484f0 of chunk_14_00445800.
#include "mouse_manager.h"

#include <windows.h>

#include <cstring>

#include "../runtime/ported.h"
#include "cell_grid.h"
#include "com_call.h"
#include "ddraw_display.h"
#include "globals.h"
#include "gump_manager.h"
#include "obj.h"
#include "raw_field.h"
#include "gump_art_ops_ports.h"
#include "art_row_blits_ports.h"

// 0x0043b540 (gump_base_ops.cpp): the rectangles {l, t, r, b} overlap.
int __stdcall GumpRectIntersects(const int *a, const int *b);

#define CS_CURSOR GLOBAL_PTR(CRITICAL_SECTION, g_mouseCursorLock_storage)    // guards curRecord
#define CS_POS GLOBAL_PTR(CRITICAL_SECTION, g_mousePosLock_storage)       // guards posX / posY
#define CS_SIZE GLOBAL_PTR(CRITICAL_SECTION, g_mouseSizeLock_storage)      // guards curW / curH
#define CS_PRESENT GLOBAL_PTR(CRITICAL_SECTION, g_presentLock_storage)   // guards the whole draw

#define g_hMainWnd GLOBAL_AS(HWND, g_mainWindow)
#define g_displayPtr GLOBAL_PTR(void, g_clientState_storage)

// 0x0047e250 on the display object: BitBlt of a DIB section to a DC.
// (hdc, x, y, w, h, hbitmap, bits (not read), srcX, srcY), RET 0x24.
#define Mouse_BlitBitmapToDC(self, a1, a2, a3, a4, a5, a6, a7, a8, a9) (reinterpret_cast<DDrawDisplay *>(self)->BlitBitmapToDC(a1, a2, a3, a4, a5, (HGDIOBJ)(a6), (int)(a7), a8, a9))
// 0x00434260: ECX = row blit, EDX = src; stack (w, h, x, y, dest, stride, destH).
#define Mouse_BlitBitmap PORT_FUNC9(&GumpBlitBitmap, void, __fastcall, void *, const void *, int, int, int, int, void *, int, int)

// 0x004474f0  size 0x116
// Find the gump under the hotspot. The old hover gump gets slot 0x28 with its local position, the gump under
// the point gets slot 0x20 (position) and 0x2c, and the previous "enter" gump (0x006488d8) gets slot 0x24 when
// it changed. Finally the hover gump gets slot 0x2c.
void __thiscall CMouseManager::UpdateHover()
{
    int x;
    int y;
    EnterCriticalSection(CS_CURSOR);
    int r = curRecord;
    LeaveCriticalSection(CS_CURSOR);
    EnterCriticalSection(CS_POS);
    if (r >= 0 && rec[r].loaded != 0) {
        x = posX + rec[r].hotX;
        y = posY + rec[r].hotY;
    } else {
        x = posX;
        y = posY;
    }
    LeaveCriticalSection(CS_POS);
    int lx = x;
    int ly = y;
    if (hover != nullptr) {
        hover->ScreenToLocal(&lx, &ly);   // 0x0043b510
        PORT_VSLOT2(hover, 0x28, void, int, int)(lx, ly);
        lx = x;
        ly = y;
    }
    CGump *g = g_gumpMgr.GumpAtPoint(x, y, nullptr, 0);   // 0x0043a220
    CGump *old = g_pointerGump;
    if (old != nullptr && g != old)
        PORT_VSLOT0(old, 0x24, void)();
    g_pointerGump = g;
    if (g != nullptr) {
        g->ScreenToLocal(&lx, &ly);   // 0x0043b510
        PORT_VSLOT2(g, 0x20, void, int, int)(lx, ly);
        PORT_VSLOT0(g, 0x2c, void)();
        lx = x;
        ly = y;
    }
    CGump *h = hover;
    if (h != nullptr)
        PORT_VSLOT0(h, 0x2c, void)();
}
HOOK_METHOD(0x004474f0, CMouseManager, UpdateHover);

// 0x00447610  size 0x45
// Map coordinates of the terrain tile under a screen point, (-1, -1) when there is none. ECX is not read.
void __stdcall Mouse_TerrainTileAt(int x, int y, int *outX, int *outY)
{
    CWorldObj *o = PORT_THISCALL2(&CellGrid_GetNearestTerrainAt, CWorldObj *, &g_worldPicker, int, int)(x, y);   // 0x00430680; ECX is the picker, not read by the callee
    if (o != nullptr) {
        *outX = o->x;
        *outY = o->y;
        return;
    }
    *outX = -1;
    *outY = -1;
}
HOOK_FUNC(0x00447610, Mouse_TerrainTileAt);

// 0x00447660  size 0xd
void __thiscall CMouseManager::SetEnabled(int value)
{
    enabled = value;
}
HOOK_METHOD(0x00447660, CMouseManager, SetEnabled);

// 0x00447670  size 0x16e
// Pick the cursor record: with no explicit cursor (cursorId == 0) it is the walk direction, plus 8 near the
// screen centre; +0x17 when the player's +0x150 field is zero (or there is no player). A new record publishes
// the changed flag, its size, its index and its first art id.
void __thiscall CMouseManager::UpdateCursorShape()
{
    int dir = GetWalkDirection();   // 0x00446d20
    int flag;
    if (g_player != nullptr)
        flag = RAWF(int, g_player, 0x150) == 0;
    else
        flag = 1;
    int x;
    int y;
    EnterCriticalSection(CS_CURSOR);
    int r = curRecord;
    LeaveCriticalSection(CS_CURSOR);
    EnterCriticalSection(CS_POS);
    if (r >= 0 && rec[r].loaded != 0) {
        x = posX + rec[r].hotX;
        y = posY + rec[r].hotY;
    } else {
        x = posX;
        y = posY;
    }
    LeaveCriticalSection(CS_POS);
    x = x - 0x140;
    y = y - 0xf0;
    int centered = x * x * 9 + (y * y << 4) <= 0x64000;
    EnterCriticalSection(CS_CURSOR);
    int idx = cursorId;
    if (idx == 0) {
        if (centered != 0)
            idx = 8;
        idx = idx + dir;
    }
    if (flag != 0)
        idx = idx + 0x17;
    if (idx >= -1 && idx < 0x2e && idx != curRecord) {
        InterlockedExchange(reinterpret_cast<LONG *>(&changed), 1);
        // Records are read through raw offsets: idx == -1 reads the 0x18 bytes before rec[0].
        int h = RAWF(int, this, (idx + 2) * 0x18);
        int w = RAWF(int, this, idx * 0x18 + 0x2c);
        uint8_t *base = reinterpret_cast<uint8_t *>(this) + idx * 0x18;
        EnterCriticalSection(CS_SIZE);
        curW = w;
        curH = h;
        LeaveCriticalSection(CS_SIZE);
        curRecord = idx;
        unk_1c = 0;
        curArt = RAWF(int, base, 0x20);
    }
    LeaveCriticalSection(CS_CURSOR);
}
HOOK_METHOD(0x00447670, CMouseManager, UpdateCursorShape);

// 0x004477e0  size 0x104
// Draw the cursor: snapshot the position, record and size under the locks, then (when the software cursor
// is on) put back the old background, save the new one and, unless 0x00528670 is set, compose the cursor and
// clear the changed flag. `lockPresent` takes the present critical section around all of it.
void __thiscall CMouseManager::DrawCursor(int lockPresent)
{
    if (g_suppressFlag != 0)
        return;
    if (curRecord == -1)
        return;
    if (enabled == 0)
        return;
    if (lockPresent != 0)
        EnterCriticalSection(CS_PRESENT);
    EnterCriticalSection(CS_POS);
    EnterCriticalSection(CS_CURSOR);
    EnterCriticalSection(CS_SIZE);
    drawX = posX;
    drawY = posY;
    drawRecord = curRecord;
    drawArt = curArt;
    drawW = artW[curArt - 0x6053];
    drawH = artH[curArt - 0x6053];
    LeaveCriticalSection(CS_POS);
    LeaveCriticalSection(CS_CURSOR);
    LeaveCriticalSection(CS_SIZE);
    if (g_cursorEnabled2 != 0) {
        if (savedValid != 0)
            RestoreBackground();   // 0x00447c20
        SaveBackground();   // 0x004478f0
        if (g_cursorSkipImage == 0) {
            DrawCursorImage();   // 0x00447a90
            InterlockedExchange(reinterpret_cast<LONG *>(&changed), 0);
        }
    }
    if (lockPresent != 0)
        LeaveCriticalSection(CS_PRESENT);
}
HOOK_METHOD(0x004477e0, CMouseManager, DrawCursor);

// 0x004478f0  size 0x195
// Save the screen rectangle under the cursor (drawX, drawY, drawW, drawH) into DIB 1 when it touches the
// 640x480 screen, remember it in savedX .. savedH and mark the saved background valid.
void __thiscall CMouseManager::SaveBackground()
{
    g_cursorSkipImage = 0;
    int screen[4];
    int box[4];
    screen[0] = 0;
    screen[1] = 0;
    screen[2] = 0x280;
    screen[3] = 0x1e0;
    int x = drawX;
    int y = drawY;
    box[0] = x;
    box[1] = y;
    box[2] = x + drawW;
    box[3] = y + drawH;
    if (GumpRectIntersects(screen, box)) {   // 0x0043b540
        HDC hdc = nullptr;
        if (g_bFullscreen != 0) {
            void *surf = g_ddSurface;
            long hr = com_call(PORT_RET(long), surf, 0x44, &hdc);   // IDirectDrawSurface::GetDC
            if (hr == static_cast<long>(0x887601c2)) {
                com_call(PORT_RET(long), surf, 0x6c);   // Restore
                hdc = nullptr;
            }
        } else {
            hdc = GetDC(g_hMainWnd);
        }
        if (hdc != nullptr) {
            HDC mem = CreateCompatibleDC(hdc);
            if (g_paletteOn != 0)
                SelectPalette(hdc, g_palette, 0);
            HGDIOBJ oldObj = SelectObject(mem, hbm1);
            BitBlt(mem, 0, 0, drawW, drawH, hdc, x, y, 0xcc0020);
            if (g_bFullscreen != 0)
                com_call(PORT_RET(long), g_ddSurface, 0x68, hdc);   // ReleaseDC
            else
                ReleaseDC(g_hMainWnd, hdc);
            SelectObject(mem, oldObj);
            DeleteDC(mem);
        }
    }
    savedY = y;
    savedX = x;
    savedW = drawW;
    savedH = drawH;
    savedValid = 1;
}
HOOK_METHOD(0x004478f0, CMouseManager, SaveBackground);

// 0x00447a90  size 0x188
// Copy the saved background (DIB 1) into DIB 2, draw the cursor bitmap over it with the plain row blit and
// put DIB 2 on the screen at the cursor position.
void __thiscall CMouseManager::DrawCursorImage()
{
    int x = drawX;
    int y = drawY;
    uint16_t *img = art[drawArt - 0x6053];
    if (img == nullptr)
        return;
    if (bits1 != nullptr) {
        uint8_t *dst = static_cast<uint8_t *>(bits2);
        const uint8_t *src = static_cast<const uint8_t *>(bits1);
        int row = 0;
        if (drawH > 0) {
            do {
                memcpy(dst, src, static_cast<size_t>(drawW) * 2);
                int stride = dibW * 2;
                dst += stride;
                src += stride;
                row++;
            } while (row < drawH);
        }
    }
    Mouse_BlitBitmap(PTR_AT(void, PORT_ADDR(&RowBlitCopyNonZero)), img, drawW, drawH, 0, 0, bits2, dibW, dibH);   // 0x00434260
    HDC hdc = nullptr;
    if (g_bFullscreen != 0) {
        long hr = com_call(PORT_RET(long), g_ddSurface, 0x44, &hdc);   // IDirectDrawSurface::GetDC
        if (hr == static_cast<long>(0x887601c2)) {
            com_call(PORT_RET(long), g_ddSurface, 0x6c);   // Restore
            hdc = nullptr;
        }
    } else {
        hdc = GetDC(g_hMainWnd);
    }
    if (hdc == nullptr)
        return;
    Mouse_BlitBitmapToDC(g_displayPtr, hdc, x, y, drawW, drawH, hbm2, bits2, 0, 0);   // 0x0047e250
    if (g_bFullscreen != 0) {
        com_call(PORT_RET(long), g_ddSurface, 0x68, hdc);   // ReleaseDC
        return;
    }
    ReleaseDC(g_hMainWnd, hdc);
}
HOOK_METHOD(0x00447a90, CMouseManager, DrawCursorImage);

// 0x00447c20  size 0xd2
// Put the saved background (DIB 1, rectangle savedX .. savedH) back on the screen.
void __thiscall CMouseManager::RestoreBackground()
{
    if (bits1 == nullptr)
        return;
    HDC hdc = nullptr;
    if (g_bFullscreen != 0) {
        long hr = com_call(PORT_RET(long), g_ddSurface, 0x44, &hdc);   // IDirectDrawSurface::GetDC
        if (hr == static_cast<long>(0x887601c2)) {
            com_call(PORT_RET(long), g_ddSurface, 0x6c);   // Restore
            hdc = nullptr;
        }
    } else {
        hdc = GetDC(g_hMainWnd);
    }
    if (hdc == nullptr)
        return;
    Mouse_BlitBitmapToDC(g_displayPtr, hdc, savedX, savedY, savedW, savedH, hbm1, bits1, 0, 0);   // 0x0047e250
    if (g_bFullscreen != 0) {
        com_call(PORT_RET(long), g_ddSurface, 0x68, hdc);   // ReleaseDC
        return;
    }
    ReleaseDC(g_hMainWnd, hdc);
}
HOOK_METHOD(0x00447c20, CMouseManager, RestoreBackground);

// 0x00447d00  size 0x211
// Copy the w x h back-buffer pixels at the cursor (drawX, drawY) into DIB 1 (row stride dibW), skipping the
// rows and columns outside the 640x480 screen. The back buffer is a ring of 0x4b000 pixels starting at
// g_scrollOffset, so an index at or past 0x4b000 - g_scrollOffset wraps back by 0x4b000.
void __thiscall CMouseManager::CaptureBackBufferRect()
{
    int s = drawY * 640 + drawX;
    int scroll = g_scrollOffset;
    int end = drawH * 640 + s + drawW - 0x280;
    int limit = 0x4b000 - scroll;
    uint16_t *dib = static_cast<uint16_t *>(bits1);
    if (s <= limit && end > limit) {
        // The rectangle straddles the wrap point.
        uint16_t *base = g_backBuffer + g_scrollOffset;
        int dstOff = 0;
        int row = 0;
        if (drawH > 0) {
            uint16_t *srcRow = base + s;
            do {
                if (drawY + row >= 0 && drawY + row < 0x1e0 && drawW > 0) {
                    uint16_t *src = srcRow;
                    uint16_t *dst = dib + dstOff;
                    for (int col = 0; col < drawW; col++) {
                        if (drawX + col >= 0 && drawX + col < 0x280) {
                            if (s + col < 0x4b000 - g_scrollOffset)
                                *dst = *src;
                            else
                                *dst = src[-0x4b000];
                        }
                        src++;
                        dst++;
                    }
                }
                dstOff = dstOff + dibW;
                s = s + 0x280;
                srcRow = srcRow + 0x280;
                row++;
            } while (row < drawH);
        }
        savedValid = 1;
        return;
    }
    int off = g_scrollOffset;
    if (s >= limit)
        off = off - 0x4b000;
    uint16_t *base = g_backBuffer + off;
    int dstOff = 0;
    int row = 0;
    if (drawH > 0) {
        uint16_t *srcRow = base + s;
        do {
            if (drawY + row >= 0 && drawY + row < 0x1e0 && drawW > 0) {
                uint16_t *src = srcRow;
                uint16_t *dst = dib + dstOff;
                for (int col = 0; col < drawW; col++) {
                    if (drawX + col >= 0 && drawX + col < 0x280)
                        *dst = *src;
                    src++;
                    dst++;
                }
            }
            dstOff = dstOff + dibW;
            srcRow = srcRow + 0x280;
            row++;
        } while (row < drawH);
    }
    savedValid = 1;
}
HOOK_METHOD(0x00447d00, CMouseManager, CaptureBackBufferRect);

// 0x00447f20  size 0x377
// Draw the drawW x drawH bitmap `src` (row stride srcStride pixels) at (drawX, drawY) into the back buffer ring
// (see CaptureBackBufferRect for the wrap rule), clipped to the 640x480 screen; with skipZero the zero pixels
// are transparent.
void __thiscall CMouseManager::BlitToBackBuffer(const uint16_t *src, int srcStride, int skipZero)
{
    int s = drawY * 640 + drawX;
    int scroll = g_scrollOffset;
    int end = drawH * 640 + s + drawW - 0x280;
    int limit = 0x4b000 - scroll;
    if (s <= limit && end > limit) {
        // The rectangle straddles the wrap point.
        uint16_t *base = g_backBuffer + g_scrollOffset;
        if (skipZero != 0) {
            int row = 0;
            if (drawH > 0) {
                uint16_t *dstRow = base + s;
                const uint16_t *srcRow = src;
                do {
                    if (drawY + row >= 0 && drawY + row < 0x1e0 && drawW > 0) {
                        uint16_t *dst = dstRow;
                        const uint16_t *p = srcRow;
                        for (int col = 0; col < drawW; col++) {
                            if (drawX + col >= 0 && drawX + col < 0x280 && *p != 0) {
                                if (s + col < 0x4b000 - g_scrollOffset)
                                    *dst = *p;
                                else
                                    dst[-0x4b000] = *p;
                            }
                            p++;
                            dst++;
                        }
                    }
                    srcRow = srcRow + srcStride;
                    s = s + 0x280;
                    dstRow = dstRow + 0x280;
                    row++;
                } while (row < drawH);
            }
            return;
        }
        int row = 0;
        if (drawH > 0) {
            uint16_t *dstRow = base + s;
            const uint16_t *srcRow = src;
            do {
                if (drawY + row >= 0 && drawY + row < 0x1e0 && drawW > 0) {
                    uint16_t *dst = dstRow;
                    const uint16_t *p = srcRow;
                    for (int col = 0; col < drawW; col++) {
                        if (drawX + col >= 0 && drawX + col < 0x280) {
                            if (s + col < 0x4b000 - g_scrollOffset)
                                *dst = *p;
                            else
                                dst[-0x4b000] = *p;
                        }
                        p++;
                        dst++;
                    }
                }
                srcRow = srcRow + srcStride;
                s = s + 0x280;
                dstRow = dstRow + 0x280;
                row++;
            } while (row < drawH);
        }
        return;
    }
    int off = g_scrollOffset;
    if (s >= limit)
        off = off - 0x4b000;
    uint16_t *base = g_backBuffer + off;
    if (skipZero != 0) {
        int row = 0;
        if (drawH > 0) {
            uint16_t *dstRow = base + s;
            const uint16_t *srcRow = src;
            do {
                if (drawY + row >= 0 && drawY + row < 0x1e0 && drawW > 0) {
                    uint16_t *dst = dstRow;
                    const uint16_t *p = srcRow;
                    for (int col = 0; col < drawW; col++) {
                        if (drawX + col >= 0 && drawX + col < 0x280 && *p != 0)
                            *dst = *p;
                        p++;
                        dst++;
                    }
                }
                srcRow = srcRow + srcStride;
                dstRow = dstRow + 0x280;
                row++;
            } while (row < drawH);
        }
        return;
    }
    int row = 0;
    if (drawH > 0) {
        uint16_t *dstRow = base + s;
        const uint16_t *srcRow = src;
        do {
            if (drawY + row >= 0 && drawY + row < 0x1e0 && drawW > 0) {
                uint16_t *dst = dstRow;
                const uint16_t *p = srcRow;
                for (int col = 0; col < drawW; col++) {
                    if (drawX + col >= 0 && drawX + col < 0x280)
                        *dst = *p;
                    p++;
                    dst++;
                }
            }
            srcRow = srcRow + srcStride;
            dstRow = dstRow + 0x280;
            row++;
        } while (row < drawH);
    }
}
HOOK_METHOD(0x00447f20, CMouseManager, BlitToBackBuffer);

// 0x004482a0  size 0x249
// Poll the OS cursor (GetCursorPos / ScreenToClient). When it differs from the stored hotspot position, store
// it, refresh the shape and draw the cursor (present lock taken). Then snapshot position, record, art and size
// and, when the cursor rectangle touches the screen, capture the back-buffer pixels and draw the cursor
// bitmap into the back buffer with zero pixels transparent. Works on the static manager at 0x0068d2e0.
void __thiscall CMouseManager::PollCursorPos()
{
    if (enabled == 0)
        return;
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(g_hMainWnd, &pt);
    int cx;
    int cy;
    EnterCriticalSection(CS_CURSOR);
    int r = g_mouse.curRecord;
    LeaveCriticalSection(CS_CURSOR);
    EnterCriticalSection(CS_POS);
    if (r >= 0 && g_mouse.rec[r].loaded != 0) {
        cx = g_mouse.posX + g_mouse.rec[r].hotX;
        cy = g_mouse.posY + g_mouse.rec[r].hotY;
    } else {
        cx = g_mouse.posX;
        cy = g_mouse.posY;
    }
    LeaveCriticalSection(CS_POS);
    if (pt.x != cx || pt.y != cy) {
        int px = pt.x;
        int py = pt.y;
        EnterCriticalSection(CS_CURSOR);
        r = g_mouse.curRecord;
        LeaveCriticalSection(CS_CURSOR);
        EnterCriticalSection(CS_POS);
        if (r >= 0 && g_mouse.rec[r].loaded != 0) {
            g_mouse.posX = px - g_mouse.rec[r].hotX;
            g_mouse.posY = py - g_mouse.rec[r].hotY;
        } else {
            g_mouse.posX = px;
            g_mouse.posY = py;
        }
        LeaveCriticalSection(CS_POS);
        g_mouse.UpdateCursorShape();   // 0x00447670
        g_mouse.DrawCursor(1);   // 0x004477e0
    }
    EnterCriticalSection(CS_POS);
    int sx = posX;
    int sy = posY;
    drawX = sx;
    drawY = sy;
    LeaveCriticalSection(CS_POS);
    EnterCriticalSection(CS_CURSOR);
    int art_id = curArt;
    drawRecord = curRecord;
    drawArt = art_id;
    LeaveCriticalSection(CS_CURSOR);
    EnterCriticalSection(CS_SIZE);
    drawW = artW[art_id - 0x6053];
    drawH = artH[art_id - 0x6053];
    LeaveCriticalSection(CS_SIZE);
    int screen[4];
    int box[4];
    screen[0] = 0;
    screen[1] = 0;
    screen[2] = 0x280;
    screen[3] = 0x1e0;
    box[0] = drawX;
    box[1] = drawY;
    box[2] = drawX + drawW;
    box[3] = drawY + drawH;
    if (GumpRectIntersects(screen, box)) {   // 0x0043b540
        CaptureBackBufferRect();   // 0x00447d00
        BlitToBackBuffer(art[drawArt - 0x6053], artW[drawArt - 0x6053], 1);   // 0x00447f20
    }
}
HOOK_METHOD(0x004482a0, CMouseManager, PollCursorPos);

// 0x004484f0  size 0x84
// When a background is saved, put DIB 1 into the back buffer (no transparency), remember the cursor rectangle
// as the saved one and clear the changed flag; otherwise just clear the saved flag and the changed flag.
void __thiscall CMouseManager::RefreshFromSaved()
{
    if (savedValid != 0) {
        BlitToBackBuffer(static_cast<const uint16_t *>(bits1), dibW, 0);   // 0x00447f20
        savedX = drawX;
        savedY = drawY;
        savedW = drawW;
        savedH = drawH;
        savedValid = 1;
        InterlockedExchange(reinterpret_cast<LONG *>(&changed), 0);
        return;
    }
    savedValid = 0;
    InterlockedExchange(reinterpret_cast<LONG *>(&changed), 0);
}
HOOK_METHOD(0x004484f0, CMouseManager, RefreshFromSaved);
