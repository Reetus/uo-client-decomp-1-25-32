// Main window creation, window title and the intro movie frame callback.
#include "main_window.h"

#include <cstring>

#include "../runtime/ported.h"
#include "com_call.h"
#include "wnd_proc_ports.h"
#include "globals.h"

#define g_mainWindowHandle GLOBAL_AS(HWND, g_mainWindow)
#define g_mainWndProc PORT_ADDR(&WndProc_Main)   // window procedure, not ported yet

// 0x00480d60  size 0xb1
// Set the main window title to "<class name>[ - suffix]". ECX = suffix (may be null), plain RET.
void __fastcall SetMainWindowTitle(const char *suffix)
{
    char buf[0x1f4];

    strcpy(buf, g_classNamePtr);
    if (suffix != nullptr) {
        strcat(buf, g_titleSeparator);
        strcat(buf, suffix);
    }
    SetWindowTextA(g_mainWindowHandle, buf);
}
HOOK_FUNC(0x00480d60, SetMainWindowTitle);

// 0x00481370  size 0x369
// Create/register the main window class, set the title, show the window.
// ANSI path = RegisterClassA/CreateWindowExA into this+0x410; wide path (0x004b7234 != 0) = RegisterClassW /
// CreateWindowExW on a stack WNDCLASSW.
void __thiscall CMainWindow::CreateMainWindow(HINSTANCE hInst, int skipCreate, int arg3, int nCmdShow)
{
    (void)arg3;
    char title[0x1f4];
    char verbuf[0x80];
    WNDCLASSW wcw;
    int width, height;

    hInstance = hInst;
    width = GetSystemMetrics(0x20) * 2 + 0x280;   // SM_CXFRAME * 2 + 640
    int cyFrame = GetSystemMetrics(0x21);         // SM_CYFRAME
    int cyCaption = GetSystemMetrics(4);          // SM_CYCAPTION
    height = cyCaption + cyFrame * 2 + 0x1e0;     // + 480
    hMenu = LoadMenuA(hInstance, MAKEINTRESOURCEA(0x6a));

    if (skipCreate == 0) {
        HWND created;
        if (g_useWideWindow != 0) {
            memset(&wcw, 0, sizeof(wcw));
            wcw.style = 0x20;
            wcw.lpfnWndProc = (WNDPROC)(uintptr_t)g_mainWndProc;
            wcw.hInstance = hInstance;
            wcw.hIcon = LoadIconA(hInstance, MAKEINTRESOURCEA(0xcf));
            wcw.hCursor = nullptr;
            wcw.hbrBackground = (HBRUSH)(uintptr_t)0xd;
            wcw.lpszClassName = g_classNameWide;
            RegisterClassW(&wcw);
            created = CreateWindowExW(0, g_classNameWide, g_classNameWide, 0xcf0000, (int)0x80000000, (int)0x80000000,
                                      width, height, nullptr, hMenu, hInstance, nullptr);
        } else {
            memset(&wndClass, 0, sizeof(wndClass));
            wndClass.style = 0x20;   // CS_DBLCLKS
            wndClass.lpfnWndProc = (WNDPROC)(uintptr_t)g_mainWndProc;
            wndClass.hInstance = hInstance;
            wndClass.hIcon = LoadIconA(hInstance, MAKEINTRESOURCEA(0xcf));
            wndClass.hCursor = nullptr;
            wndClass.hbrBackground = (HBRUSH)(uintptr_t)0xd;
            wndClass.lpszClassName = g_classNamePtr;
            RegisterClassA(&wndClass);
            created = CreateWindowExA(0, g_classNamePtr, g_classNamePtr, 0xcf0000, (int)0x80000000, (int)0x80000000,
                                      width, height, nullptr, hMenu, hInstance, nullptr);
        }
        hwnd = created;
    }

    if (g_acctName[0] != 0 && g_titlePart2[0] != 0) {
        strcpy(verbuf, g_acctName);
        strcat(verbuf, g_titlePart1);
        strcat(verbuf, g_titlePart2);
        strcpy(title, g_classNamePtr);
        if (verbuf != nullptr) {   // an address, always true
            strcat(title, g_titleSeparator);
            strcat(title, verbuf);
        }
        SetWindowTextA(g_mainWindowHandle, title);
    } else {
        strcpy(title, g_classNamePtr);
        SetWindowTextA(g_mainWindowHandle, title);
    }
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    if (g_ccVal62c != 0)
        SendMessageA(g_mainWindowHandle, WM_COMMAND, 0x9c99, 0);
}
HOOK_METHOD(0x00481370, CMainWindow, CreateMainWindow);


// 0x00482550  size 0xed
// Per-frame callback while the intro movie plays: pumps messages and repaints the black fill.
// Returns 1 when the movie should be aborted (key / mouse button / SC_RESTORE), else 0.
int __cdecl IntroMovieFrame()
{
    uint32_t fx[25];   // DDBLTFX: dwSize (+0) = 100, dwFillColor (+0x50) = 0
    MSG msg;

    g_introFrameCounter = g_introFrameCounter + 1;
    if (g_introFrameCounter == 2) {
        fx[0x00 / 4] = 100;
        fx[0x50 / 4] = 0;
        // IDirectDrawSurface::Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT = 0x1000400, &fx)
        com_call(PORT_RET(long), g_introSurface, 0x14, (void *)nullptr, (void *)nullptr, (void *)nullptr, (DWORD)0x1000400, (void *)fx);
    }
    if (PeekMessageA(&msg, nullptr, 0, 0, 0) != 0 && GetMessageA(&msg, nullptr, 0, 0) != 0) {
        if (msg.message == WM_KEYDOWN || msg.message == WM_LBUTTONDOWN || msg.message == WM_RBUTTONDOWN
            || (msg.message == WM_SYSCOMMAND && msg.wParam == 0xf120))   // SC_RESTORE
            return 1;
        if (msg.message == WM_PAINT) {
            fx[0x00 / 4] = 100;
            fx[0x50 / 4] = 0;
            com_call(PORT_RET(long), g_introSurface, 0x14, (void *)nullptr, (void *)nullptr, (void *)nullptr, (DWORD)0x1000400, (void *)fx);
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return 0;
}
HOOK_FUNC(0x00482550, IntroMovieFrame);
