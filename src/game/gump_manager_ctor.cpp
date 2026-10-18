// CGumpManager constructor / destructors and 0x00436f70 (gump lookup by target object).
// gumps.cxx, 0x00436f70..0x00437186. The ctor and dtors have no SEH frame in the original.
#include <windows.h>

#include "../runtime/ported.h"
#include "gump_lookup.h"
#include "gump_manager.h"
#include "crt.h"
#include "raw_field.h"
#include "skill_groups_ports.h"
#include "display_ops_ports.h"
#include "vtables.h"
#include "globals.h"

// 640x480 16-bit back buffer (0x96000 bytes) and the memory DC, both owned by the gump manager.
#define GMC_backBuffer GLOBAL_AS(void *, g_ringScratchB)
#define GMC_backDC GLOBAL_AS(uintptr_t, g_memDC)
// 0x00472940: fills the skill group tables. No arguments.
#define GMC_InitSkillGroups() (InitSkillGroups())
// 0x0047e230: deletes the display's back buffer bitmap (0x0064cd34). No arguments.
#define GMC_DestroyBackBuffer() (DestroyBackBuffer())

// 0x00436f70  size 0x87
CGump *__thiscall CGump_00436f70::FindOpenGumpForTarget()
{
    void *target = reinterpret_cast<void *>(static_cast<uintptr_t>(static_cast<uint32_t>(unk_7c)));
    CGump *g;

    if (target != nullptr && PORT_VSLOT0(target, 0x24, int)() != 0) {
        g = g_gumpListTail;
        while (g != nullptr) {
            if (PORT_VSLOT0(g, 0x78, int)() != 0) {
                if (static_cast<int32_t>(reinterpret_cast<uintptr_t>(g->owner)) == unk_7c)
                    return g;
            }
            g = g->prev;
        }
        return nullptr;
    }
    target = reinterpret_cast<void *>(static_cast<uintptr_t>(static_cast<uint32_t>(unk_7c)));
    if (target == nullptr)
        return nullptr;
    if (PORT_VSLOT0(target, 0x30, int)() == 0)
        return nullptr;
    g = g_gumpListTail;
    while (g != nullptr) {
        if (PORT_VSLOT0(g, 0x90, int)() != 0) {
            if (static_cast<int32_t>(reinterpret_cast<uintptr_t>(g->owner)) == unk_7c)
                return g;
        }
        g = g->prev;
    }
    return nullptr;
}
HOOK_METHOD(0x00436f70, CGump_00436f70, FindOpenGumpForTarget);

// 0x00437000  size 0xdd
CGumpManager::CGumpManager()
{
    GMC_backBuffer = ::operator new(0x96000);                            // operator new 0x0048ad70
    cascadeY = 0x10;
    cascadeX = 0x10;
    unk_5c = 1;
    RAWF(int32_t, this, 0x60) = 0;
    desktopLoaded = 0;
    RAWF(int32_t, this, 0x68) = 1;
    RAWF(int32_t, this, 0x0c) = 0;
    configGump = nullptr;
    radarGump = nullptr;
    listHead = nullptr;
    listTail = nullptr;
    RAWF(int32_t, this, 0x4474) = 0;
    RAWF(int32_t, this, 0x4470) = 0;
    RAWF(int32_t, this, 0x14) = 0;
    RAWF(int32_t, this, 0x10) = 0;
    RAWF(int32_t, this, 0x5ec) = 0;
    RAWF(int32_t, this, 0x28) = 0;
    RAWF(int32_t, this, 0x34) = 0;
    RAWF(int32_t, this, 0x44) = 0;
    RAWF(int32_t, this, 0x48) = 0;
    RAWF(int32_t, this, 0x4c) = 0;
    RAWF(int32_t, this, 0x50) = 0;
    RAWF(int32_t, this, 0x40) = 0;
    RAWF(int32_t, this, 0x54) = 0;
    RAWF(int32_t, this, 0x58) = 0;
    GMC_InitSkillGroups();                                            // 0x00472940
    RAWF(int32_t, this, 0x44bc) = 0;
    RAWF(uint8_t, this, 0x449e) = 0;
    RAWF(uint16_t, this, 0x44a0) = 0;
    RAWF(uint16_t, this, 0x44a2) = 0;
    RAWF(uint16_t, this, 0x44a4) = 0;
    RAWF(uint16_t, this, 0x44aa) = 0;
    RAWF(uint16_t, this, 0x44ac) = 0;
    RAWF(uint16_t, this, 0x44ae) = 0;
    RAWF(uint16_t, this, 0x44b0) = 0;
    RAWF(int32_t, this, 0x44b4) = 0;
    RAWF(uint16_t, this, 0x44b8) = 0;
    RAWF(uint16_t, this, 0x44ba) = 0;
}
HOOK_CTOR(0x00437000, CGumpManager, ());

// 0x004370e0  size 0x5d  (the scalar deleting destructor, ??_G; the destructor body is expanded in it)
// Deletes every top-level gump (each deleting destructor unlinks itself from the list), the memory
// DC, the display back buffer and the 0x96000-byte buffer.
inline CGumpManager::~CGumpManager()
{
    while (listTail != nullptr)
        PORT_VSLOT1(listTail, kGumpSlot_DeletingDtor, void *, int)(1);
    if (GMC_backDC != 0)
        DeleteDC(reinterpret_cast<HDC>(GMC_backDC));
    GMC_DestroyBackBuffer();                                          // 0x0047e230
    if (GMC_backBuffer != nullptr)
        operator delete(GMC_backBuffer);                                    // operator delete 0x0048ad60
}
HOOK_VTABLE(0x0049ebc8, CGumpManager);
HOOK_SDD(0x004370e0, CGumpManager);   // slot 0

// 0x00437140  size 0x46
// The destructor as the atexit thunk 0x0044ac40 jumps to it (ECX = g_gumpMgr): a plain function that expands the inline
// destructor.
void __fastcall GumpManager_Dtor_00437140(void *self, void *)
{
    reinterpret_cast<CGumpManager *>(self)->CGumpManager::~CGumpManager();
}
HOOK_FUNC(0x00437140, GumpManager_Dtor_00437140);
