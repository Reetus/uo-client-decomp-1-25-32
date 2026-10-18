// CMainWindow: the main window object. One static instance lives at 0x00758368 in the
// original image; its +0x400 field is the main HWND (0x00758768), +0x544 the menu
// (0x007588ac). WinMain's shutdown sends WM_CLOSE to every non-null dword in the first
// 0x400 bytes (0x00758368 .. 0x00758768), so that range is an array of window handles.
// Layout from decomp/pass2/0x00481370.cpp and 0x00480e80.cpp.
//
// ? pass2 calls this class "CApp"; that name already belongs to the application object at
// ? 0x0069e0e8 (app.h), so this one is called CMainWindow here.
#pragma once

#include <cstddef>
#include <cstdint>

#include <windows.h>

#include "../runtime/ported.h"
#include "globals.h"

class CMainWindow {
public:
    // 0x00481370: register the "Ultima Online" window class, create the window (unless skipCreate),
    // set the title, show it. arg3 is unused.
    void __thiscall CreateMainWindow(HINSTANCE hInstance, int skipCreate, int arg3, int nCmdShow);

    HWND windows[0x100];    // +0x000  window handles, closed by WinMain on shutdown
    HWND hwnd;              // +0x400  main window (same address as DAT_00758768)
    uint8_t pad_404[0xc];
    WNDCLASSA wndClass;     // +0x410  ANSI path only
    HINSTANCE hInstance;    // +0x438
    uint8_t pad_43c[0x544 - 0x43c];
    HMENU hMenu;            // +0x544  (same address as DAT_007588ac)
    // ? size of the object is unknown
};

static_assert(sizeof(WNDCLASSA) == 0x28, "WNDCLASSA size");
static_assert(offsetof(CMainWindow, hwnd) == 0x400, "CMainWindow::hwnd");
static_assert(offsetof(CMainWindow, wndClass) == 0x410, "CMainWindow::wndClass");
static_assert(offsetof(CMainWindow, hInstance) == 0x438, "CMainWindow::hInstance");
static_assert(offsetof(CMainWindow, hMenu) == 0x544, "CMainWindow::hMenu");

#define g_mainWindowObject GLOBAL_AS(CMainWindow, g_mainWindowObject_storage)
