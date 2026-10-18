// Small window-level helpers: clipboard paste into the focused input, switch back to a windowed display,
// launch the web browser, cursor choice by id, and a message box that can exit.
#include <cstdint>
#include <cstring>

#include <windows.h>

#include "../runtime/ported.h"
#include "ddraw_display.h"
#include "globals.h"
#include "mouse_manager.h"
#include "crt.h"
#include "app_support_ports.h"
#include "shell_launch_ports.h"
#include "gump_manager.h"

#define wh_mainWnd GLOBAL_AS(HWND, g_mainWindow)
#define wh_presentLock reinterpret_cast<CRITICAL_SECTION *>(static_cast<uintptr_t>(GADDR(g_presentLock_storage)))
#define wh_ddraw GLOBAL_AS(DDrawDisplay, g_ddraw_storage)
// App key dispatch: ECX = the app (ignored), stack = key (sign-extended char), RET 4
#define wh_HandleKey(a1) PORT_THISCALL1(&App_HandleKey, void, GLOBAL_PTR(void, g_app_storage), int)(a1)
// URL opener: ECX passed through, EDX = URL, stack = working directory; returns a process handle or -1
#define wh_OpenUrl(a1, a2, a3) (OpenUrlWithBrowser(a1, (char *)(a2), (char *)(a3)))
// message gump: stack (text, 0, 0, 0), ECX = gump manager (ignored)
#define wh_CreateMessageGump(a1, a2, a3, a4) (CreateGump2ac((int)(a1), a2, a3, a4))

// 0x004816e0  size 0xbe
// Paste the CF_TEXT clipboard data into the focused input, one key at a time. The limit is 0xf0 characters
// with no focused control, 0x50 when the control's slot 0xc8 answers true, 0x3e8 otherwise; for the first two a
// carriage return ends the paste.
// ? GetClipboardData's handle is used directly as a string pointer (no GlobalLock), as the original does
void __cdecl PasteClipboardText()
{
    HANDLE h;
    const char *data;
    int stopAtCr;
    int len;
    int i;
    void *ctl;

    if (OpenClipboard(wh_mainWnd)) {
        h = GetClipboardData(1);
        i = 0;
        if (h != nullptr) {
            data = static_cast<const char *>(h);
            stopAtCr = 0;
            len = static_cast<int>(strlen(data));
            ctl = g_focusedInput;
            if (ctl == nullptr) {
                if (len > 0xf0)
                    len = 0xf0;
                stopAtCr = 1;
            } else if (PORT_VSLOT0(ctl, 0xc8, int)() != 0) {
                if (len > 0x50)
                    len = 0x50;
                stopAtCr = 1;
            } else if (len > 0x3e8) {
                len = 0x3e8;
            }
            if (len > 0) {
                do {
                    if (stopAtCr != 0 && data[i] == 0xd)
                        break;
                    wh_HandleKey(static_cast<int>(data[i]));   // 0x0044c4a0
                    i++;
                } while (i < len);
            }
        }
        CloseClipboard();
    }
}
HOOK_FUNC(0x004816e0, PasteClipboardText);

// 0x004817a0  size 0xf6
// Leave full screen: restyle the main window as a captioned window of 640x480 plus frame metrics, drop the menu,
// shut the DirectDraw display down and resize. All under the present lock.
// ? the AdjustWindowRectEx result is never read; the size uses the frame metrics directly
void __cdecl SwitchToWindowedMode()
{
    RECT rc;
    LONG style;
    int width;
    int height;

    EnterCriticalSection(wh_presentLock);
    wh_fullscreenFlag = 0;
    style = GetWindowLongA(wh_mainWnd, GWL_STYLE);
    SetWindowLongA(wh_mainWnd, GWL_STYLE, static_cast<LONG>((static_cast<uint32_t>(style) & 0x7f39ffffu) | 0xc60000u));
    width = GetSystemMetrics(SM_CXSIZEFRAME) * 2 + 0x280;
    height = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYSIZEFRAME) * 2 + 0x1e0;
    rc.top = 0;
    rc.left = 0;
    rc.right = width;
    rc.bottom = height;
    SetMenu(wh_mainWnd, nullptr);
    AdjustWindowRectEx(&rc, GetWindowLongA(wh_mainWnd, GWL_STYLE), FALSE, GetWindowLongA(wh_mainWnd, GWL_EXSTYLE));
    wh_ddraw.Shutdown();   // 0x0047e6b0
    SetWindowPos(wh_mainWnd, HWND_NOTOPMOST, 0, 0, width, height, 0x112);
    InvalidateRect(wh_mainWnd, nullptr, FALSE);
    LeaveCriticalSection(wh_presentLock);
}
HOOK_FUNC(0x004817a0, SwitchToWindowedMode);

// 0x004830a0  size 0x3a
// ECX = URL. Opens it; 1 on success. On failure shows "Web browser not found!" unless 0x00758268 is set; returns 0.
int __fastcall LaunchWebBrowser(const char *url)
{
    if (wh_OpenUrl(0, url, g_emptyString) == reinterpret_cast<HANDLE>(-1)) {   // 0x0047e020
        if (g_launchDirError == 0)
            PORT_THISCALL4(&CreateGump2ac, void *, GLOBAL_PTR(void, g_gumpMgr_storage), const char *, void *, int, int)("Web browser not found!", nullptr, 0, 0);   // 0x00439f60
        return 0;
    }
    return 1;
}
HOOK_FUNC(0x004830a0, LaunchWebBrowser);

// 0x00482be0  size 0x3f
// Cursor for a hit-test region id (stack, RET 4): ids 1..8 and 0x1f..0x26 give cursor 0, everything else 0x11.
void __stdcall SetCursorForRegion(unsigned id)
{
    if ((id >= 1 && id <= 8) || (id >= 0x1f && id <= 0x26))
        g_mouse.SetMouseCursor(0);   // 0x00446840
    else
        g_mouse.SetMouseCursor(0x11);
}
HOOK_FUNC(0x00482be0, SetCursorForRegion);

// 0x00482c70  size 0x24
// ECX = text, EDX = caption, stack = exit flag (RET 4). Message box; exits the process when the flag is set.
int __fastcall ShowMessageBoxAndExit(const char *text, const char *caption, int exitFlag)
{
    MessageBoxA(nullptr, text, caption, 0);
    if (exitFlag != 0)
        exit(1);   // 0x0048b610
    return exitFlag;
}
HOOK_FUNC(0x00482c70, ShowMessageBoxAndExit);
