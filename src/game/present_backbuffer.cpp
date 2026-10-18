// Present_BackBuffer: copy the 640x480 back buffer (DIB selected into a memory DC, or the raw 16-bit pixels) to the
// screen, honouring the scroll offset (the buffer is a ring that wraps at X / Y).
#include <cstdint>

#include <windows.h>

#include "../runtime/ported.h"
#include "com_call.h"
#include "ddraw_display.h"
#include "globals.h"
#include "mouse_manager.h"
#include "display_ops_ports.h"

#define pb_presentLock reinterpret_cast<CRITICAL_SECTION *>(static_cast<uintptr_t>(GADDR(g_presentLock_storage)))
#define pb_mainWnd GLOBAL_AS(HWND, g_mainWindow)
#define pb_display GLOBAL_AS(DDrawDisplay, g_ddraw_storage)
#define pb_pixels (reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(desc[9])))   // lpSurface of the locked surface; read from desc each time, as the original
// 555 -> 565: ECX = dest, EDX = src, stack = pixel count
#define pb_Convert555To565(a1, a2, a3) (Convert555To565(a1, a2, a3))
// copy: ECX = dest, EDX = src, stack = pixel count and an unused second argument
#define pb_CopyPixels16(a1, a2, a3, a4) (CopyPixels16(a1, a2, a3, a4))

static const long kSurfaceLost = static_cast<long>(0x887601c2);   // DDERR_SURFACELOST

// 0x00482ca0  size 0x3f7
// Under the present lock. Full screen: the back surface is filled either through its GDI DC (when 0x00758a0c is
// set; four BitBlts that unwrap the scroll ring) or by Lock + pixel copy (555 -> 565 conversion when the
// display is 565 but the art is 555), then Unlock and Flip. Windowed: the same four BitBlts to the window DC.
// With no back buffer the function returns without leaving the lock (the original does the same).
// ? the BitBlts after the first use source y = Y + 1 and height 0x1df - Y; kept as the original has them
// ? the ReleaseDC on the back surface is passed the address of the HDC variable, not the HDC (as the original)
void __cdecl Present_BackBuffer()
{
    HDC hdc;
    long hr;
    uint32_t desc[27];

    EnterCriticalSection(pb_presentLock);
    g_mouse.PollCursorPos();   // 0x004482a0
    if (g_backBuffer == nullptr)
        return;

    if (g_bFullscreen != 0) {
        if (pb_gdiMode != 0) {
            hr = com_call(PORT_RET(long), pb_display.backBuffer, 0x44, &hdc);   // GetDC
            if (hr == kSurfaceLost) {
                com_call(PORT_RET(long), pb_display.backBuffer, 0x6c);           // Restore
            } else if (hr == 0) {
                if (g_scrollOffset != 0) {
                    BitBlt(hdc, 0, 0, 0x280 - g_scrollOffsetX, 0x1e0 - g_scrollOffsetY, g_memDC, g_scrollOffsetX, g_scrollOffsetY, SRCCOPY);
                    if (g_scrollOffsetX != 0) {
                        BitBlt(hdc, 0x280 - g_scrollOffsetX, 0, g_scrollOffsetX, 0x1df - g_scrollOffsetY, g_memDC, 0,
                               g_scrollOffsetY + 1, SRCCOPY);
                    }
                    if (g_scrollOffsetY != 0) {
                        BitBlt(hdc, 0, 0x1e0 - g_scrollOffsetY, 0x280 - g_scrollOffsetX, g_scrollOffsetY, g_memDC, g_scrollOffsetX, 0,
                               SRCCOPY);
                        if (g_scrollOffsetX != 0) {
                            BitBlt(hdc, 0x280 - g_scrollOffsetX, 0x1df - g_scrollOffsetY, g_scrollOffsetX, g_scrollOffsetY + 1, g_memDC, 0,
                                   0, SRCCOPY);
                        }
                    }
                } else {
                    BitBlt(hdc, 0, 0, 0x280, 0x1e0, g_memDC, 0, 0, SRCCOPY);
                }
                com_call(PORT_RET(long), pb_display.backBuffer, 0x68, reinterpret_cast<HDC>(&hdc));   // ReleaseDC
            }
        } else {
            desc[0] = 0x6c;
            hr = com_call(PORT_RET(long), pb_display.backBuffer, 0x64, static_cast<void *>(nullptr), desc, static_cast<DWORD>(0x21), static_cast<void *>(nullptr));   // Lock(NULL, &desc, DDLOCK_WAIT | DDLOCK_WRITEONLY..., NULL)
            if (hr == kSurfaceLost) {
                com_call(PORT_RET(long), pb_display.backBuffer, 0x6c);   // Restore
            } else if (hr == 0) {
                if (pb_display.is565 != 0 && g_is565 == 0) {
                    pb_Convert555To565(pb_pixels, g_backBuffer + g_scrollOffset, 0x4b000 - g_scrollOffset);   // 0x0047e450
                    if (g_scrollOffset != 0)
                        pb_Convert555To565(pb_pixels + (0x4b000 - g_scrollOffset), g_backBuffer, g_scrollOffset);
                } else {
                    pb_CopyPixels16(pb_pixels, g_backBuffer + g_scrollOffset, 0x4b000 - g_scrollOffset, 0x3fff3fff);   // 0x0047e4b0
                    if (g_scrollOffset != 0)
                        pb_CopyPixels16(pb_pixels + (0x4b000 - g_scrollOffset), g_backBuffer, g_scrollOffset, 0x3fff3fff);
                }
                com_call(PORT_RET(long), pb_display.backBuffer, 0x80, static_cast<void *>(nullptr));   // Unlock
                if (pb_display.flipping != 0)
                    com_call(PORT_RET(long), pb_display.primary, 0x2c, static_cast<void *>(nullptr), static_cast<DWORD>(1));   // Flip
            }
        }
    } else {
        hdc = GetDC(pb_mainWnd);
        if (g_scrollOffset != 0) {
            BitBlt(hdc, 0, 0, 0x280 - g_scrollOffsetX, 0x1e0 - g_scrollOffsetY, g_memDC, g_scrollOffsetX, g_scrollOffsetY, SRCCOPY);
            if (g_scrollOffsetX != 0) {
                BitBlt(hdc, 0x280 - g_scrollOffsetX, 0, g_scrollOffsetX, 0x1df - g_scrollOffsetY, g_memDC, 0, g_scrollOffsetY + 1,
                       SRCCOPY);
            }
            if (g_scrollOffsetY != 0) {
                BitBlt(hdc, 0, 0x1e0 - g_scrollOffsetY, 0x280 - g_scrollOffsetX, g_scrollOffsetY, g_memDC, g_scrollOffsetX, 0, SRCCOPY);
                if (g_scrollOffsetX != 0) {
                    BitBlt(hdc, 0x280 - g_scrollOffsetX, 0x1df - g_scrollOffsetY, g_scrollOffsetX, g_scrollOffsetY + 1, g_memDC, 0, 0,
                           SRCCOPY);
                }
            }
        } else {
            BitBlt(hdc, 0, 0, 0x280, 0x1e0, g_memDC, 0, 0, SRCCOPY);
        }
        ReleaseDC(pb_mainWnd, hdc);
    }
    g_mouse.RefreshFromSaved();   // 0x004484f0
    LeaveCriticalSection(pb_presentLock);
}
HOOK_FUNC(0x00482ca0, Present_BackBuffer);
