// Intro movie playback: the .wve player (through UV_PlayerStart, 0x00483f40) and the .avi fallback (MCIWnd).
#include <cstdint>
#include <cstring>

#include <windows.h>

#include "../runtime/ported.h"
#include "app.h"
#include "com_call.h"
#include "ddraw_display.h"
#include "globals.h"
#include "mouse_manager.h"
#include "music_manager.h"
#include "crt.h"
#include "sound_manager.h"
#include <vfw.h>
#include <ddraw.h>
#include "uv_player.h"
#include "main_window_ports.h"

#define im_mainWnd GLOBAL_AS(HWND, g_mainWindow)
#define im_ddraw GLOBAL_AS(DDrawDisplay, g_ddraw_storage)
// The player's parameter block (0x007582b8 ...)
// DirectDrawCreate thunk
// MCIWndCreateA thunk (cdecl)
// UV_PlayerStart: (path, hwnd, parameter block)
#define im_PlayerStart(a1, a2, a3) (UvpPlayerStart(a1, a2, (UvpPlayer *)(a3)))

// 0x00482970  size 0x270
// Play music\intro.avi in an MCI window, disabling music and sound while it runs. The movie ends on a key or
// button press, a restore command, or when the position reaches the length.
// ? when neither copy of the file opens, the function returns after disabling music and shutting the sound
// ? manager down, without restoring them (as the original does)
static void __cdecl PlayIntroMovie_Avi()
{
    WINDOWPLACEMENT wp;
    int savedMusic;
    int soundWasOn;
    const char *path;
    FILE *f;
    HWND mw;
    RECT rc;
    MSG msg;
    LRESULT length;

    memset(&msg, 0, sizeof(msg));
    wp.length = 0x2c;
    GetWindowPlacement(im_mainWnd, &wp);
    if (wp.showCmd == 0 || wp.showCmd == 6)
        return;

    savedMusic = g_musicManager.enabled;
    g_musicManager.SetEnabled(0);   // 0x0047f410
    if (g_ccMusicVal != 0) {
        soundWasOn = (g_soundManager.IsInitialized() != 0) ? 1 : 0;   // 0x0047f9d0
    } else {
        soundWasOn = 0;
    }
    if (soundWasOn != 0)
        g_soundManager.Shutdown();   // 0x0047f960

    f = fopen(g_app.ResolveDataPath(const_cast<char *>("music\\intro.avi")), g_fopenReadMode);
    if (f != nullptr) {
        fclose(f);
        path = g_app.ResolveDataPath(const_cast<char *>("music\\intro.avi"));
    } else {
        f = fopen(g_app.ResolveDataPath2(const_cast<char *>("music\\intro.avi")), g_fopenReadMode);
        if (f == nullptr)
            return;
        fclose(f);
        path = g_app.ResolveDataPath2(const_cast<char *>("music\\intro.avi"));
    }
    mw = MCIWndCreateA(im_mainWnd, im_hInstance, 0xa, path);   // MCIWndCreateA via 0x0049868e
    GetClientRect(im_mainWnd, &rc);
    SetWindowPos(mw, nullptr, 0, 0, 0x280, 0x1e0, 4);
    SendMessageA(mw, 0x48f, 0, reinterpret_cast<LPARAM>(&rc));
    im_playing = 1;
    g_mouse.SetEnabled(0);   // 0x00447660
    SendMessageA(mw, 0x470, 0, 0x3e8);
    SendMessageA(mw, 0x806, 0, 0);
    length = SendMessageA(mw, 0x468, 0, 0);
    for (;;) {
        SetFocus(mw);
        PeekMessageA(&msg, nullptr, 0, 0, 1);
        if (static_cast<int>(SendMessageA(mw, 0x466, 0, 0)) >= static_cast<int>(length))
            break;
        if (msg.message == 0x100 || msg.message == 0x201)
            break;
        if (msg.message == 0x112 && msg.wParam == 0xf120)
            break;
    }
    g_mouse.SetEnabled(1);
    im_playing = 0;
    SendMessageA(mw, 0x10, 0, 0);
    if (g_paletteOn != 0) {
        HDC dc = GetDC(im_mainWnd);
        SelectPalette(dc, g_palette, 0);
        RealizePalette(dc);
        ReleaseDC(im_mainWnd, dc);
    }
    g_musicManager.SetEnabled(savedMusic);
    if (soundWasOn != 0)
        g_soundManager.Init(im_mainWnd);   // 0x0047f770
}
HOOK_FUNC(0x00482970, PlayIntroMovie_Avi);

// 0x00482640  size 0x327
// Play music\intro.wve through the original player (0x00483f40). Windowed mode creates its own DirectDraw
// object, primary surface and clipper; full screen reuses the display object's. Falls back to the .avi when
// the .wve is in neither data path. The player block at 0x007582b8 gets flags 0x84006c (full screen) or
// 0x840060 | 0x83 (windowed).
void __cdecl PlayIntroMovie_Wve()
{
    WINDOWPLACEMENT wp;
    char pathBuf[0x104];
    const char *path;
    FILE *f;
    void *clipper = nullptr;
    int savedMusic;
    RECT rc;
    POINT pt;
    uint32_t desc[27];

    wp.length = 0x2c;
    GetWindowPlacement(im_mainWnd, &wp);
    if (wp.showCmd == 0 || wp.showCmd == 6)
        return;

    f = fopen(g_app.ResolveDataPath(const_cast<char *>("music\\intro.wve")), g_fopenReadMode);
    if (f != nullptr) {
        fclose(f);
        path = g_app.ResolveDataPath(const_cast<char *>("music\\intro.wve"));
    } else {
        f = fopen(g_app.ResolveDataPath2(const_cast<char *>("music\\intro.wve")), g_fopenReadMode);
        if (f == nullptr) {
            PlayIntroMovie_Avi();   // 0x00482970
            return;
        }
        fclose(f);
        path = g_app.ResolveDataPath2(const_cast<char *>("music\\intro.wve"));
    }
    strcpy(pathBuf, path);

    im_dsPrimary = g_soundManager.primaryBuffer;
    im_dsound = g_soundManager.pDirectSound;
    im_frameCallback = PORT_ADDR(&IntroMovieFrame);   // IntroMovieFrame
    im_flags = 0x840060;
    if (g_bFullscreen != 0) {
        im_pDD = im_ddraw.dd;
        g_introSurface = im_ddraw.primary;
        im_flags = 0x84006c;
    } else {
        if (DirectDrawCreate(nullptr, reinterpret_cast<IDirectDraw **>(&im_pDD), nullptr) != 0)   // 0x00498694
            return;
        if (com_call(PORT_RET(long), im_pDD, 0x50, static_cast<HWND>(nullptr), static_cast<DWORD>(8)) != 0)   // SetCooperativeLevel
            return;
        memset(desc, 0, sizeof(desc));
        desc[0] = 0x6c;
        desc[1] = 1;
        desc[26] = 0x200;   // DDSCAPS_PRIMARYSURFACE
        if (com_call(PORT_RET(long), im_pDD, 0x18, desc, &g_introSurface, static_cast<void *>(nullptr)) != 0) {   // CreateSurface
            MessageBoxA(im_mainWnd, "Failed to create primary surface.", "Note!", 0);
            return;
        }
        if (com_call(PORT_RET(long), im_pDD, 0x10, static_cast<DWORD>(0), &clipper, static_cast<void *>(nullptr)) != 0)   // CreateClipper
            return;
        if (com_call(PORT_RET(long), clipper, 0x20, static_cast<DWORD>(0), im_mainWnd) != 0)   // IDirectDrawClipper::SetHWnd
            return;
        if (com_call(PORT_RET(long), g_introSurface, 0x70, clipper) != 0)   // SetClipper
            return;
        pt.x = 0;
        pt.y = 0;
        ClientToScreen(im_mainWnd, &pt);
        GetClientRect(im_mainWnd, &rc);
        im_destX = (rc.right - 0x280) / 2 + pt.x;
        im_destY = (rc.bottom - 0x1e0) / 2 + pt.y;
        im_destCenterX = rc.right / 2;
        im_flags = im_flags | 0x83;
    }
    im_playing = 1;
    g_mouse.SetEnabled(0);   // 0x00447660
    savedMusic = g_musicManager.enabled;
    g_introFrameCounter = 0;
    g_musicManager.SetEnabled(0);   // 0x0047f410
    im_PlayerStart(pathBuf, im_mainWnd, &im_flags);   // 0x00483f40
    g_mouse.SetEnabled(1);
    im_playing = 0;
    g_musicManager.SetEnabled(savedMusic);
    DeleteObject(im_movieBitmap);
    if (g_bFullscreen == 0) {
        if (clipper != nullptr)
            com_call(PORT_RET(long), clipper, 0x08);
        com_call(PORT_RET(long), g_introSurface, 0x08);
        com_call(PORT_RET(long), im_pDD, 0x08);
    }
}
HOOK_FUNC(0x00482640, PlayIntroMovie_Wve);
