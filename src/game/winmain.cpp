// WinMain (application main), as in decomp/pass2/0x00480e80.cpp. The entry point (standalone/main.cpp)
// calls it after the static initialisers.
#include <cstdio>

#include <windows.h>
#include <commctrl.h>
#include <mmsystem.h>

#include "../runtime/ported.h"
#include "../runtime/test_seams.h"
#include "globals.h"
#include "main_window.h"
#include "music_manager.h"
#include "crt.h"
#include "sound_manager.h"
#include "multi_placer.h"
#include "app.h"
#include "file_manager.h"
#include "verdata_ports.h"
#include "mouse_manager.h"
#include "intro_movie_ports.h"
#include "client_state.h"
#include "client_socket.h"
#include "text_entry.h"
#include "text_entry_ops_ports.h"
#include "client_helpers_31_ports.h"
#include "app_support_ports.h"
#include "entry_list.h"
#include "static_init_ports.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "comctl32.lib")

// --- globals -------------------------------------------------------------------------------
#define g_mainWindowHandle GLOBAL_AS(HWND, g_mainWindow)
#define g_playerObject GLOBAL_AS(void *, g_player)
#define g_appObject GLOBAL_PTR(void, g_app_storage)
#define g_fileMgr GLOBAL_PTR(void, g_fileManager_storage)
#define g_cursorMgr GLOBAL_PTR(void, g_mouse_storage)
#define g_unknownObj1 GLOBAL_PTR(void, g_clientState_storage)
#define g_unknownObj2 GLOBAL_PTR(void, g_unknownObj2_storage)
#define g_criticalSection2 GLOBAL_AS(CRITICAL_SECTION, g_presentLock_storage)

// strings read from the image (text not confirmed by pass2)

// --- original callees ----------------------------------------------------------------------

// 0x00480e80  size 0x4ec
// WinMain: single-instance check, colour-depth check, subsystem init, create window (0x00481370),
// game objects, menu check marks, message loop with fixed-step ticks, shutdown.
// The SEH frame (handler 0x0049babc, try levels 0..3 around the operator new / ctor pairs) is omitted.
// hPrevInstance / lpCmdLine / nCmdShow are reused as locals by the original compiler; they keep their names here.
int __stdcall WinMain_(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg = {0};
    void *obj;
    void *tmp;
    HACCEL hAccel;
    HCURSOR hOldCursor;
    DWORD lastTick;
    DWORD now;
    int bitsPixel;
    HDC hdc;
    HWND hwndDesktop;
    HWND *p;

    g_singleInstanceMapping = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 0x20, s_singleInstanceName);
    if (g_singleInstanceMapping != nullptr) {
        // Test seam: UO_DECOMP_MULTI lets a test run start beside another client.
        if (GetLastError() == ERROR_ALREADY_EXISTS && !::decomp::test_switch("UO_DECOMP_MULTI")) {
            CloseHandle(g_singleInstanceMapping);
            MessageBoxA(nullptr, s_alreadyRunningText, s_errorCaption, 0);
            ExitProcess(1);
        }
    }
    g_singleInstanceMapping = OpenFileMappingA(FILE_MAP_WRITE, FALSE, s_patchMappingName);
    if (g_singleInstanceMapping != nullptr) {
        CloseHandle(g_singleInstanceMapping);
        MessageBoxA(nullptr, s_patchRunningText, s_errorCaption, 0);
        ExitProcess(1);
    }
    hwndDesktop = GetDesktopWindow();
    hdc = GetWindowDC(hwndDesktop);
    bitsPixel = GetDeviceCaps(hdc, BITSPIXEL);
    hwndDesktop = GetDesktopWindow();
    ReleaseDC(hwndDesktop, hdc);
    if (bitsPixel < 0x10) {
        MessageBoxA(nullptr, s_colorModeText, s_colorModeCaption, 0);
        ExitProcess(1);
    }
    hOldCursor = LoadCursorA(nullptr, MAKEINTRESOURCEA(0x7f8a));
    hOldCursor = SetCursor(hOldCursor);
    atexit(AtExit_CloseHandle_004b0264);
    atexit(AtExit_KillPresentTimer);
    new CMultiPlacer;   // the constructor registers itself in g_multiPlacer; the pointer is unused
    InitCommonControls();
    if (hPrevInstance != nullptr)
        return 0;

    reinterpret_cast<CApp *>(g_appObject)->LoadObsceneAndIgnoreLists();
    reinterpret_cast<CFileManager *>(g_fileMgr)->Init();
    LoadVerdata();
    reinterpret_cast<CMouseManager *>(g_cursorMgr)->LoadCursorArt();
    reinterpret_cast<CApp *>(g_appObject)->AllocPlayerMobile();
    g_mainWindowObject.CreateMainWindow(hInstance, 0, (int)(intptr_t)lpCmdLine, nCmdShow);
    if (g_hideCursorFlag != 0) {
        g_hideCursorFlag = 0;
        ShowCursor(FALSE);
    }
    reinterpret_cast<CMouseManager *>(g_cursorMgr)->SetEnabled(1);
    g_soundManager.Init(g_mainWindowHandle);
    if (g_introMovieFlag != 0)
        PlayIntroMovie_Wve();
    hAccel = LoadAcceleratorsA(hInstance, MAKEINTRESOURCEA(0x8e));
    SetCursor(hOldCursor);
    reinterpret_cast<CClientState *>(g_unknownObj1)->CreateWorldGumps();

    // try level 1
    obj = new CNetInfo();
    // try level 2
    g_textEntry = new CTextEntry;
    PORT_THISCALL0(&FocusMainWindow, void, g_textEntry)();   // ECX may be NULL
    // try level 3
    g_pktSender = new CClientSocket();

    CheckMenuItem(GetMenu(g_mainWindowHandle), 0x9cbc, (g_profanityOn != 0) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(GetMenu(g_mainWindowHandle), 0x9c89, (g_ccSoundVal != 0) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(GetMenu(g_mainWindowHandle), 0x9c8a, (g_ccMusicVal != 0) ? MF_CHECKED : MF_UNCHECKED);
    PrintVersion();   // prints "UO Version %s" as a system message
    msg.message = 0x400;
    lastTick = GetTickCount();
    g_musicManager.PlayTrack(8, 1, 0);

    for (;;) {
        if (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE) != 0) {
            // drain loop
            for (;;) {
                if (msg.message == WM_QUIT)
                    goto shutdown;
                TranslateAcceleratorA(g_mainWindowHandle, hAccel, &msg);
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
                (&g_soundManager)->CSoundManager::Update();   // sound manager per-frame update
                if (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE) == 0)
                    break;
            }
        }
        if (msg.message == WM_QUIT)
            goto shutdown;
        reinterpret_cast<CClientSocket *>(g_pktSender)->Pump();
        (&g_soundManager)->CSoundManager::Update();
        now = GetTickCount();
        if (now - lastTick < g_frameInterval)
            continue;
        if (now - lastTick < g_frameInterval * 2) {
            PORT_THISCALL1(&App_GameTick, void, g_appObject, int)(0);
            lastTick = lastTick + g_frameInterval;
        } else {
            if (g_catchUpFlag != 0)
                PORT_THISCALL1(&App_GameTick, void, g_appObject, int)(1);
            PORT_THISCALL1(&App_GameTick, void, g_appObject, int)(0);
            lastTick = now;
        }
    }

shutdown:
    reinterpret_cast<CEntryList_00444090 *>(g_unknownObj2)->Release();
    EnterCriticalSection(&g_criticalSection2);
    if (g_timerId != 0)
        timeKillEvent(g_timerId);
    LeaveCriticalSection(&g_criticalSection2);
    p = (HWND *)&g_mainWindowObject;   // 0x00758368 .. 0x00758768
    do {
        if (*p != nullptr)
            SendMessageA(*p, WM_CLOSE, 0, 0);
        p = p + 1;
    } while ((uintptr_t)p < GEND(g_mainWindowObject_storage));
    if (g_menuHandle != nullptr)
        DestroyMenu(g_menuHandle);
    if (g_playerObject != nullptr)
        PORT_VSLOT1(g_playerObject, 0, void, int)(1);   // deleting destructor
    if (g_textEntry != nullptr)
        PORT_VSLOT1(g_textEntry, 0, void, int)(1);
    g_textEntry = nullptr;
    if (g_pktSender != nullptr)
        PORT_VSLOT1(g_pktSender, 0, void, int)(1);
    g_pktSender = nullptr;
    if (obj != nullptr)
        PORT_VSLOT1(obj, 0, void, int)(1);
    if (g_textLogFile != nullptr)
        fclose(g_textLogFile);   // ? 0x0048aad0 is the CRT fclose; pass2 guessed "free?"
    return (int)msg.wParam;
}
HOOK_FUNC(0x00480e80, WinMain_);
