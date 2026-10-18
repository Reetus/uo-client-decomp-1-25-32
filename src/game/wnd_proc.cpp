// The main window procedure (0x004818a0), registered by address from CMainWindow::CreateMainWindow.
// __stdcall(HWND, UINT, WPARAM, LPARAM), RET 0x10.
#include <cstdint>
#include <cstring>

#include <windows.h>
#include <mmsystem.h>

#include "../runtime/ported.h"
#include "app.h"
#include "ddraw_display.h"
#include "globals.h"
#include "gump_manager.h"
#include "mouse_manager.h"
#include "music_manager.h"
#include "raw_field.h"
#include "sound_manager.h"
#include "text_entry.h"
#include "app_session_ops_ports.h"
#include "hue_gump_ops_ports.h"
#include "app_support_ports.h"
#include "window_helpers_31_ports.h"
#include "sound_manager_ports.h"

#pragma comment(lib, "winmm.lib")

#define wp_mainWnd GLOBAL_AS(HWND, g_mainWindow)
#define wp_presentLock reinterpret_cast<CRITICAL_SECTION *>(static_cast<uintptr_t>(GADDR(g_presentLock_storage)))
#define wp_ddraw GLOBAL_AS(DDrawDisplay, g_ddraw_storage)
#define wp_player GLOBAL_AS(void *, g_player)
#define wp_macroList GLOBAL_AS(void *, g_macroListHead)        // macro nodes: key +0, ctrl +8, alt +0xc, shift +0x10, next +0xe0
#define wp_textEntry GLOBAL_AS(CTextEntry *, g_textEntry)
// war mode: ECX = mode
#define wp_SetWarMode(a1) (SetWarMode(a1))
// help request (0x9b packet), send action (0x43 packet): no arguments
#define wp_HelpRequest() (HelpRequest_Send())
#define wp_SendAction43() (App_SendAction43())
// leave full screen (ported in window_helpers_31.cpp)
#define wp_SwitchToWindowedMode() (SwitchToWindowedMode())
// App key dispatch: ECX = the app (ignored), stack = key
#define wp_HandleKey(a1) (App_HandleKey(a1))

// 0x004818a0  size 0xbae
// Messages handled: WM_CREATE (cursor table, sound timer), WM_DESTROY, WM_SIZE, WM_ACTIVATE, WM_PAINT,
// WM_WINDOWPOSCHANGING (fixed outer size), WM_KEYDOWN / WM_SYSKEYDOWN, WM_KEYUP, WM_CHAR, WM_COMMAND (menu ids
// 0x9c41..0x9ccf), mouse buttons 0x201 / 0x202 / 0x204 / 0x205, 0x222, WM_HOTKEY, MM_MCINOTIFY. Everything else
// goes to DefWindowProcW or DefWindowProcA depending on the unicode flag.
// ? the message names of 0x222 and 0x3b9 are inferred from the numbers
LRESULT __stdcall WndProc_Main(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    if (im_playing == 0) {
        POINT pt;
        bool inside = true;

        GetCursorPos(&pt);
        ScreenToClient(wp_mainWnd, &pt);
        if (g_bFullscreen == 0 && (pt.x < 0 || pt.y < 0 || pt.x >= 0x280 || pt.y >= 0x1e0))
            inside = false;
        else if (GetFocus() != wp_mainWnd)
            inside = false;
        if (!inside) {
            if (g_hideCursorFlag != 1) {
                g_hideCursorFlag = 1;
                ShowCursor(TRUE);
            }
        } else if (g_hideCursorFlag != 0) {
            g_hideCursorFlag = 0;
            ShowCursor(FALSE);
        }
    }

    switch (msg) {
    case WM_CREATE: {   // 0x004819a7
        HCURSOR *cur = wp_cursors;

        memset(cur, 0, 0x40 * sizeof(uint32_t));
        wp_cursors[0] = LoadCursorA(nullptr, MAKEINTRESOURCEA(0x7f00));
        wp_cursors[1] = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x67));
        wp_cursors[2] = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x6b));
        wp_cursors[3] = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x69));
        wp_cursors[4] = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x6e));
        wp_cursors[5] = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x68));
        wp_cursors[6] = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x6d));
        wp_cursors[7] = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x6a));
        wp_cursors[8] = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x6c));
        wp_cursors[9] = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x71));
        wp_cursors[10] = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x76));
        wp_cursors[11] = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x77));
        wp_cursors[12] = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x78));
        wp_cursors[13] = LoadCursorA(nullptr, MAKEINTRESOURCEA(0x7f00));
        wp_cursors[14] = LoadCursorA(nullptr, MAKEINTRESOURCEA(0x7f00));
        wp_cursors[15] = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x79));
        wp_cursors[16] = LoadCursorA(nullptr, MAKEINTRESOURCEA(0x7f00));
        wp_cursors[17] = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x82));
        wp_cursors[18] = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x83));
        g_0075881c = LoadCursorA(nullptr, MAKEINTRESOURCEA(0x7f00));
        g_00758820 = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x7f));
        g_00758824 = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x81));
        g_00758828 = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x7e));
        g_0075882c = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x7c));
        g_00758830 = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x7a));
        g_00758834 = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x7b));
        g_00758838 = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x7d));
        g_0075883c = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0x80));
        g_00758840 = LoadCursorA(im_hInstance, MAKEINTRESOURCEA(0xa7));
        wp_flag7582ac = 0;
        if (g_cursorEnabled2 == 0)
            return 0;
        // 0x00480c40 is the sound timer callback (SoundTimerProc)
        g_timerId = timeSetEvent(0x19, 1, reinterpret_cast<LPTIMECALLBACK>(static_cast<uintptr_t>(PORT_ADDR(&SoundTimerProc))), 1, 1);
        return 0;
    }

    case WM_DESTROY:   // 0x00481bc1
        if (GLOBAL_AS(int, wp_frameBusy) != 0)
            return 0;
        g_gumpMgr.SaveDesktop();   // 0x0043cd60
        g_suppressFlag = 1;
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:   // 0x00481bf1
        if (wp_mainWnd == nullptr) {
            wp_mainWnd = hwnd;
            if (g_paletteOn != 0) {
                HDC dc = GetDC(hwnd);
                SelectPalette(dc, g_palette, 0);
                RealizePalette(dc);
                ReleaseDC(hwnd, dc);
            }
        }
        if (GLOBAL_AS(int, wp_frameBusy) != 0) {
            g_gumpMgr.CreateMemDC();   // 0x0043a040
            return 0;
        }
        if (static_cast<uint16_t>(wParam) == 2) {
            int f;
            int c;
            int cy;
            int cx;

            wh_fullscreenFlag = 1;
            f = GetSystemMetrics(0x21);
            c = GetSystemMetrics(4);
            cy = c + f * 2 + 0x1e0;
            cx = GetSystemMetrics(0x20) * 2 + 0x280;
            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, cx, cy, 0x116);
        } else if (wh_fullscreenFlag != 0) {
            int f;
            int c;
            int cy;
            int cx;

            wh_fullscreenFlag = 0;
            f = GetSystemMetrics(0x21);
            c = GetSystemMetrics(4);
            cy = c + f * 2 + 0x1e0;
            cx = GetSystemMetrics(0x20) * 2 + 0x280;
            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, cx, cy, 0x116);
        }
        g_gumpMgr.CreateMemDC();   // 0x0043a040
        return 0;

    case WM_ACTIVATE:   // 0x0048195d
        if ((wParam & 3) == 0) {
            GLOBAL_AS(int, wp_activeFlag) = 0;
            break;
        }
        GLOBAL_AS(int, wp_activeFlag) = 1;
        if (g_paletteOn != 0) {
            HDC dc = GetDC(hwnd);
            SelectPalette(dc, g_palette, 0);
            RealizePalette(dc);
            ReleaseDC(hwnd, dc);
        }
        if (g_targetCallbackActive != 0)
            g_mouse.SetMouseCursor(0x11);   // 0x00446840
        break;

    case WM_PAINT: {   // 0x00481ceb
        PAINTSTRUCT ps;

        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_WINDOWPOSCHANGING: {   // 0x00481d2a
        WINDOWPOS *pos = reinterpret_cast<WINDOWPOS *>(lParam);

        if (im_playing != 0)
            pos->flags |= 3;
        if ((pos->flags & 1) == 0) {
            pos->cx = GetSystemMetrics(0x20) * 2 + 0x280;
            if (wh_fullscreenFlag != 0) {
                int f = GetSystemMetrics(0x21);
                int c = GetSystemMetrics(4);
                pos->cy = c + f * 2 + 0x1e0;
            } else {
                int f = GetSystemMetrics(0x21);
                int c = GetSystemMetrics(4);
                pos->cy = c + f * 2 + 0x1e0;
            }
        }
        break;
    }

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {   // 0x00481dcd
        void *focus;
        void *node;
        int index;

        if (wParam == 9) {
            if (wp_warModeEnabled != 0) {
                int state = g_pendingWarMode;
                bool doIt;

                if (state == -1)
                    doIt = RAWF(int32_t, wp_player, 0x150) == 0;
                else
                    doIt = (state == 0);
                if (doIt)
                    wp_SetWarMode(1);   // 0x0044dda0
            } else if ((lParam & 0x40000000) == 0) {
                wp_SetWarMode(-1);
            }
        }
        focus = g_focusedInput;
        if (focus != nullptr) {
            void *parent = RAWF(void *, focus, 0x48);
            if (parent != nullptr) {
                if (PORT_VSLOT0(parent, 0xc0, int)() != 0) {
                    focus = g_focusedInput;
                    parent = RAWF(void *, focus, 0x48);
                    if (focus == RAWF(void *, parent, 0xf0)) {
                        g_savedKeyLParam = static_cast<int>(lParam);
                        g_savedKeyScan = static_cast<int>(wParam);
                        parent = RAWF(void *, focus, 0x48);
                        PORT_VSLOT1(parent, 0x44, void, void *)(focus);
                        return 0;
                    }
                }
            }
        }
        wq_runKey = (static_cast<int16_t>(GetKeyState(0x11)) < 0) ? 1 : 0;
        wp_shiftDown = (static_cast<int16_t>(GetKeyState(0x10)) < 0) ? 1 : 0;
        wp_altDown = (static_cast<int16_t>(GetKeyState(0x12)) < 0) ? 1 : 0;
        node = wp_macroList;
        wp_macroCursor = node;
        index = 0;
        if (node != nullptr) {
            do {
                if (RAWF(uint32_t, node, 0) == wParam && RAWF(int32_t, node, 8) == wq_runKey &&
                    RAWF(int32_t, node, 0xc) == wp_altDown && RAWF(int32_t, node, 0x10) == wp_shiftDown) {
                    g_app.RunMacro(index);   // 0x0044c8e0
                    return 0;
                }
                node = RAWF(void *, node, 0xe0);
                index++;
                wp_macroCursor = node;
            } while (node != nullptr);
        }
        focus = g_focusedInput;
        if (focus != nullptr) {
            unsigned key = static_cast<unsigned>(wParam);

            if (!((key >= 0x21 && key <= 0x28) || key == 0x2e))
                break;
            PORT_VSLOT1(focus, 0x3c, void, unsigned)(key + 0xe000);
            return 0;
        }
        if (wp_textEntry->OnKeyDown(static_cast<int>(wParam), static_cast<int>(lParam)) == 0)   // 0x004429f0
            break;
        return 0;
    }

    case WM_CHAR:   // 0x00481f43
        wp_HandleKey(static_cast<int>(wParam & 0xffff));   // 0x0044c4a0
        return 0;

    case WM_KEYUP: {   // 0x00481f62
        if (wParam != 9)
            return 0;
        if (wp_warModeEnabled == 0)
            return 0;
        int state = g_pendingWarMode;
        bool doIt;

        if (state == -1)
            doIt = RAWF(int32_t, wp_player, 0x150) != 0;
        else
            doIt = (state == 1);
        if (!doIt)
            return 0;
        wp_SetWarMode(0);
        return 0;
    }

    case WM_COMMAND:   // 0x00481fad
        switch (static_cast<unsigned>(wParam & 0xffff)) {
        case 0x9c41:
            SendMessageA(hwnd, 0x10, 0, 0);
            return 0;
        case 0x9c44:
        case 0x9cbd:
            if (g_inputActive == 0)
                return 0;
            g_gumpMgr.CreateGump(nullptr, 0x1388, nullptr, 0);   // 0x0043bd20
            return 0;
        case 0x9c6b:
        case 0x9c99:
            if (g_bFullscreen == 0) {
                RECT rc;
                LONG style;
                DWORD exStyle;

                GLOBAL_AS(int, wp_frameBusy) = 1;
                EnterCriticalSection(wp_presentLock);
                wh_fullscreenFlag = 1;
                style = GetWindowLongA(wp_mainWnd, GWL_STYLE);
                SetWindowLongA(wp_mainWnd, GWL_STYLE, static_cast<LONG>((static_cast<uint32_t>(style) & 0xff39ffffu) | 0x80000000u));
                rc.left = 0;
                rc.top = 0;
                rc.right = 0x280;
                rc.bottom = 0x1e0;
                exStyle = static_cast<DWORD>(GetWindowLongA(wp_mainWnd, GWL_EXSTYLE));
                {
                    BOOL hasMenu = (GetMenu(wp_mainWnd) != nullptr) ? TRUE : FALSE;
                    DWORD curStyle = static_cast<DWORD>(GetWindowLongA(wp_mainWnd, GWL_STYLE));
                    AdjustWindowRectEx(&rc, curStyle, hasMenu, exStyle);
                }
                wp_ddraw.Init();   // 0x0047e550
                LeaveCriticalSection(wp_presentLock);
                GLOBAL_AS(int, wp_frameBusy) = 0;
                return 0;
            }
            GLOBAL_AS(int, wp_frameBusy) = 1;
            wp_SwitchToWindowedMode();   // 0x004817a0
            GLOBAL_AS(int, wp_frameBusy) = 0;
            return 0;
        case 0x9c89: {
            int enabled = g_musicManager.enabled;
            HMENU menu = GetMenu(wp_mainWnd);

            CheckMenuItem(menu, 0x9c89, (enabled != 0) ? 0 : 8);
            g_musicManager.SetEnabled(g_musicManager.enabled == 0 ? 1 : 0);   // 0x0047f410
            return 0;
        }
        case 0x9c8a: {
            int enabled = g_ccMusicVal;
            HMENU menu = GetMenu(wp_mainWnd);

            CheckMenuItem(menu, 0x9c8a, (enabled != 0) ? 0 : 8);
            g_soundManager.Enable(g_ccMusicVal == 0 ? 1 : 0);   // 0x0047fa80
            return 0;
        }
        case 0x9cb3:
            if (g_inputActive == 0)
                return 0;
            wp_SetWarMode(-1);   // 0x0044dda0
            return 0;
        case 0x9cb4:
            if (g_inputActive == 0)
                return 0;
            wp_SendAction43();   // 0x0044c300
            return 0;
        case 0x9cb8:
            if (g_inputActive == 0)
                return 0;
            g_gumpMgr.CreateGump(nullptr, 0x1392, nullptr, 0);   // 0x0043bd20
            return 0;
        case 0x9cbc: {
            HMENU menu = GetMenu(wp_mainWnd);

            CheckMenuItem(menu, 0x9cbc, (g_profanityOn != 0) ? 0 : 8);
            g_profanityOn = (g_profanityOn == 0) ? 1 : 0;
            return 0;
        }
        case 0x9ccc:
            wp_HelpRequest();   // 0x004428d0
            return 0;
        case 0x9ccf:
            g_ccVal2cc = (g_ccVal2cc == 0) ? 1 : 0;
            return 0;
        default:
            break;
        }
        break;

    case WM_LBUTTONDOWN:   // 0x0048229e
        SetCapture(hwnd);
        SetFocus(hwnd);
        g_mouse.SetPos(static_cast<int16_t>(lParam & 0xffff), static_cast<int16_t>((lParam >> 16) & 0xffff));   // 0x00446ed0
        return 0;

    case WM_LBUTTONUP:   // 0x004822d4
        ReleaseCapture();
        g_mouse.SetPosSlot14(static_cast<int16_t>(lParam & 0xffff), static_cast<int16_t>((lParam >> 16) & 0xffff));   // 0x00447010
        return 0;

    case WM_RBUTTONDOWN:   // 0x004822ff
        SetCapture(hwnd);
        SetFocus(hwnd);
        g_mouse.SetPosSlot18(static_cast<int16_t>(lParam & 0xffff), static_cast<int16_t>((lParam >> 16) & 0xffff));   // 0x00447210
        return 0;

    case WM_RBUTTONUP:   // 0x00482335
        ReleaseCapture();
        g_mouse.SetPosSlot1c(static_cast<int16_t>(lParam & 0xffff), static_cast<int16_t>((lParam >> 16) & 0xffff));   // 0x00447340
        return 0;

    case 0x222:   // 0x00482360
        if (static_cast<LPARAM>(lParam) != reinterpret_cast<LPARAM>(hwnd)) {
            GLOBAL_AS(int, wp_activeFlag) = 0;
            break;
        }
        GLOBAL_AS(int, wp_activeFlag) = 1;
        if (g_paletteOn != 0) {
            HDC dc = GetDC(hwnd);
            SelectPalette(dc, g_palette, 0);
            RealizePalette(dc);
            ReleaseDC(hwnd, dc);
        }
        if (g_targetCallbackActive != 0)
            g_mouse.SetMouseCursor(0x11);
        break;

    case 0x3b9:   // 0x0048240f
        g_musicManager.PlayNextInCycle();   // 0x0047f450
        return 0;

    case WM_HOTKEY:   // 0x00482424
        if (GetActiveWindow() == wp_mainWnd)
            g_app.RunMacro(static_cast<int>(wParam));   // 0x0044c8e0
        return 0;

    default:
        break;
    }

    if (g_useWideWindow != 0)
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}
HOOK_FUNC(0x004818a0, WndProc_Main);
