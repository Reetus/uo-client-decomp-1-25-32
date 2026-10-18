// Third porting pass, batch u2: compiler static initialisers (entries of the CRT init table at 0x004a6000)
// and an atexit handler. All are __cdecl with no arguments.
#include <cstdint>

#include "../runtime/ported.h"
#include "crt.h"
#include "raw_field.h"
#include "palette_manager_ports.h"
#include "entry_list.h"
#include "static_init_u2_ports.h"
#include "globals.h"

// 0x00436870: PackRGB16 on the palette manager at 0x0068dd98 (three stack args, RET 0xc; result in AX).
#define U2_PackRGB16(r, g, b) PORT_THISCALL3(&PackRGB16, uint32_t, GLOBAL_PTR(void, g_clientState_storage), int, int, int)(r, g, b)

// 0x00431d80  size 0x20
// Static initialiser: the 16-bit pixel value of white (PackRGB16 0x00436870), stored at 0x004bab20.
void __cdecl StaticInit_ColorWhite_00431d80()
{
    g_004bab20 = static_cast<uint16_t>(U2_PackRGB16(0xff, 0xff, 0xff));
}
HOOK_FUNC(0x00431d80, StaticInit_ColorWhite_00431d80);

// 0x00431da0  size 0x1d
// Static initialiser: PackRGB16(0, 0xaa, 0xaa), stored at 0x004bab1c.
void __cdecl StaticInit_ColorCyan_00431da0()
{
    g_004bab1c = static_cast<uint16_t>(U2_PackRGB16(0, 0xaa, 0xaa));
}
HOOK_FUNC(0x00431da0, StaticInit_ColorCyan_00431da0);

// 0x00431dc0  size 0x1d
// Static initialiser: PackRGB16(0xaa, 0, 0xaa), stored at 0x004bab14.
void __cdecl StaticInit_ColorMagenta_00431dc0()
{
    g_004bab14 = static_cast<uint16_t>(U2_PackRGB16(0xaa, 0, 0xaa));
}
HOOK_FUNC(0x00431dc0, StaticInit_ColorMagenta_00431dc0);

// 0x00431de0  size 0x20
// Static initialiser: PackRGB16(0x9b, 0x9b, 0x9b), stored at 0x004bab18.
void __cdecl StaticInit_ColorGray_00431de0()
{
    g_004bab18 = static_cast<uint16_t>(U2_PackRGB16(0x9b, 0x9b, 0x9b));
}
HOOK_FUNC(0x00431de0, StaticInit_ColorGray_00431de0);

// 0x00444030  size 0x5a
// atexit handler of the object at 0x005285e0: when it is initialised, release and free every entry of the
// list at 0x005285f4 (linked through +0x30; Release is 0x004440e0), free the buffer at 0x005285ec and
// clear the flag.
void __cdecl AtExit_Obj005285e0_00444030()
{
    if (GLOBAL_AS(uint8_t, g_unknownObj2_storage) == 0)
        return;
    void *entry = g_005285f4;
    if (entry != nullptr) {
        void *next;
        do {
            next = RAWF(void *, entry, 0x30);
            if (entry != nullptr) {
                reinterpret_cast<CEntry_004440e0 *>(entry)->Release();
                operator delete(entry);
            }
            entry = next;
            g_005285f4 = entry;
        } while (next != nullptr);
    }
    operator delete(g_005285ec);
    g_005285ec = nullptr;
    GLOBAL_AS(uint8_t, g_unknownObj2_storage) = 0;
}
HOOK_FUNC(0x00444030, AtExit_Obj005285e0_00444030);

// 0x00443fe0  size 0x48
// Static initialiser of the object at 0x005285e0: flag 0, name pointer -> "English" (0x004a8d34), two
// zeroed fields, an inline "Eng" buffer at +0x10 and a zeroed list head at +0x14; registers the atexit
// handler 0x00444030.
void __cdecl StaticInit_Obj005285e0_00443fe0()
{
    GLOBAL_AS(uint8_t, g_unknownObj2_storage) = 0;
    g_005285e4 = GADDR(str_004a8d34);
    g_005285e8 = 0;
    GLOBAL_AS(uint32_t, g_005285ec) = 0;
    GLOBAL_AS(uint32_t, g_005285f4) = 0;
    g_005285f0 = 0x45;
    g_005285f1 = 0x6e;
    g_005285f2 = 0x67;
    g_005285f3 = 0;
    atexit(AtExit_Obj005285e0_00444030);   // AtExit_Obj005285e0_00444030
}
HOOK_FUNC(0x00443fe0, StaticInit_Obj005285e0_00443fe0);
