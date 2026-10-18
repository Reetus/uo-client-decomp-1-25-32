// Third porting pass, batch u3: the callback at 0x0045c510 (reached through the jump thunk at 0x00412200).
#include <cstdint>

#include "../runtime/ported.h"
#include "raw_field.h"
#include "app.h"
#include "text_entry.h"
#include "gump_manager.h"
#include "globals.h"

// 0x0045c510  size 0x3c
// Callback taking no arguments (a button callback; ECX is overwritten before it is read): reload the key names and
// macros of the app object at 0x0069e0e8 (0x0044d780), set +0x68 of the object whose pointer is at 0x0069dda0 and the
// flag at 0x005285c8, refresh the input line (0x00443590 on the object at 0x0069e4ec), then on the gump manager at
// 0x006488c0 delete the transient gumps (0x0043c670) and reload the desktop (0x0043c6c0, a tail call in the original;
// its result is left in EAX).
int __cdecl Callback_ReloadMacros_0045c510()
{
    reinterpret_cast<CApp *>(GLOBAL_PTR(void, g_app_storage))->LoadKeynamesAndMacros();   // CApp::LoadKeynamesAndMacros
    RAWF(int32_t, GLOBAL_AS(void *, g_screenGump), 0x68) = 1;
    void *inputLine = g_textEntry;
    g_inputActive = 1;
    reinterpret_cast<CTextEntry *>(inputLine)->RefreshLine();                      // CTextEntry::RefreshLine
    reinterpret_cast<CGumpManager *>(GLOBAL_PTR(void, g_gumpMgr_storage))->DeleteTransientGumps();   // CGumpManager::DeleteTransientGumps
    return PORT_THISCALL0(&CGumpManager::LoadDesktop, int, GLOBAL_PTR(void, g_gumpMgr_storage))();   // CGumpManager::LoadDesktop
}
HOOK_FUNC(0x0045c510, Callback_ReloadMacros_0045c510);
