// Screen-level flow helpers: world gump creation and the per-frame death fade.
// The SEH frames of the originals (try level around the operator new calls) are omitted.
#include "client_state.h"

#include <cstdint>

#include <windows.h>

#include "../runtime/ported.h"
#include "batch02_helpers.h"
#include "globals.h"
#include "crt.h"
#include "map_gump.h"
#include "gump_manager.h"
#include "main_menu_gump.h"
#include "dirty_rects_ports.h"
#include "bark_gump.h"
#include "death_menu_gump.h"
#include "app_session_ops_ports.h"

#define g_activeGump GLOBAL_AS(void *, g_scrollChoiceGump)        // current gump slot (also used by CChgPWGump)
#define g_gump9c GLOBAL_AS(void *, g_pMainMenu)
#define g_appObject GLOBAL_PTR(void, g_app_storage)

// gump manager call with no arguments
#define GumpMgr_Refresh(self) (reinterpret_cast<CGumpManager *>(self)->CreateMemDC())
// 0x00436d20 callees
#define Fade_Finish() (FlushDirtyRects())
#define Fade_End() (InvalidateAll())
// gump manager: invalidate / redraw a rect (rect, flag)
#define GumpMgr_Redraw(self, a1, a2) (reinterpret_cast<CGumpManager *>(self)->DrawGumps((int *)(a1), a2))
#define App_Present(o) PORT_THISCALL0(&RedrawInputLine, void, (o))()

// 0x00436c70  size 0xac
void __thiscall CClientState::CreateWorldGumps()
{
    CGump *obj = new CMapGump();
    worldMapGump = obj;
    obj->visible = 0;                // no NULL check in the asm
    GumpMgr_Refresh(g_gumpManager);

    CGump *obj2 = new CMainMenuGump();
    g_gump9c = obj2;              // ? stored in DAT_006488f0
    CGump_AddChild(obj2, nullptr, 1);
}
HOOK_METHOD(0x00436c70, CClientState, CreateWorldGumps);

// 0x00436d20  size 0x1c3
// Per-frame death fade step: darken the back buffer one level per frame, show the
// "You are dead!" bark at step 0x10, switch gumps at step 0x20.
void __cdecl DeathFadeStep()
{
    RECT rect;

    if (g_pktSender == nullptr)
        return;

    if (g_suppressSpeech == 0) {
        Fade_Finish();
    } else {
        uint16_t *px = g_backBuffer;
        int n = 0x4b000;
        if (g_is565 != 0) {              // 565
            do {
                unsigned int v = *px;
                unsigned int r = v >> 11;
                unsigned int g = (v >> 6) & 0x1f;
                unsigned int b = v & 0x1f;
                if (r != 0)
                    r--;
                if (g != 0)
                    g--;
                if (b != 0)
                    b--;
                px++;
                *(px - 1) = static_cast<uint16_t>((((r << 6) | g) << 5) | b);
                n--;
            } while (n != 0);
        } else {                         // 555
            do {
                unsigned int v = *px;
                unsigned int r = (v >> 10) & 0x1f;
                unsigned int g = (v >> 5) & 0x1f;
                unsigned int b = v & 0x1f;
                if (r != 0)
                    r--;
                if (g != 0)
                    g--;
                if (b != 0)
                    b--;
                px++;
                *(px - 1) = static_cast<uint16_t>((((r << 5) | g) << 5) | b);
                n--;
            } while (n != 0);
        }

        g_suppressSpeech = g_suppressSpeech + 1;
        if (g_suppressSpeech == 0x10) {
            CGump *bark = new CBarkGump(nullptr, "You are dead!", 0, 3, nullptr);   // 0x004012a0; dereferenced below without a check
            bark->x = 0x140 - bark->width / 2;
            bark->y = 0xf0 - bark->height / 2;
        }
        if (g_suppressSpeech >= 0x20) {
            g_suppressSpeech = 0;
            Fade_End();
            CGump *obj = new CDeathMenuGump;
            g_activeGump = obj;
            CGump_AddChild(obj, nullptr, 1);
        }
    }

    rect.left = 0;
    rect.top = 0;
    rect.right = 0x280;
    rect.bottom = 0x1e0;
    GumpMgr_Redraw(g_gumpManager, &rect, 1);
    g_dirtyCount = 0;
    App_Present(g_appObject);
}
HOOK_FUNC(0x00436d20, DeathFadeStep);
