// Small client helpers around 0x0047d430..0x0047d590: version message, a buffer-pointer release, and a
// table-size initialiser whose owning class is not identified.
#include <cstdint>
#include <cstdio>

#include "../runtime/ported.h"
#include "crt.h"
#include "class_0049c690.h"
#include "client_helpers_31.h"
#include "text_gump_ctor_ports.h"
#include "globals.h"

// ? the real classes of these objects are not identified; the names are placeholders
#define chp_Nop(self) (reinterpret_cast<CWorldObj *>(self)->CWorldObj::ToVoid())
#define chp_AddSystemMessage(o, a1, a2, a3) PORT_THISCALL3(&CreateBarkGump, void, (o), int, int, const char *)(a1, a2, a3)
#define chp_systemMessageObj GLOBAL_PTR(void, g_msgTarget_storage)

// 0x0047d430  size 0x1a
void __thiscall CObj0047d430::FreeBuffer()
{
    if (buffer != nullptr) {
        operator delete(buffer);
        buffer = nullptr;
    }
}
HOOK_METHOD(0x0047d430, CObj0047d430, FreeBuffer);

// 0x0047d450  size 0x36
// "UO Version %s." as a system message (mode 0, type 3).
void __cdecl PrintVersion()
{
    char buf[0x80];

    sprintf(buf, "UO Version %s.", chp_versionString);
    chp_AddSystemMessage(chp_systemMessageObj, 0, 3, buf);
}
HOOK_FUNC(0x0047d450, PrintVersion);

// 0x0047d490  size 0xc1
CObj0047d490 *__thiscall CObj0047d490::InitSizes()
{
    g_00758220 = 0;
    g_0075822c = 0;
    g_00758254 = 0x49248;
    AUO_unk75821c = 0x49248;
    g_00758238 = 0x10000;
    g_00758218 = 0x400;
    g_00758248 = 0x800;
    g_00758214 = 0x80;
    g_00758228 = 0x2000;
    g_00758234 = 0x32;
    g_00758250 = 0x1000;
    g_0075824c = 0x5cefa;
    g_tileGroupBase = 0x59248;
    g_loadProgressArg = 0x59648;
    g_00758210 = 0x59e48;
    g_regionIdBase = 0x59ec8;
    g_00758224 = 0x5bec8;
    g_00758244 = 0x5befa;
    g_0075825c = 0x5cefa;
    g_00758258 = 0x5cefa;
    chp_Nop(this);
    return this;
}
HOOK_METHOD(0x0047d490, CObj0047d490, InitSizes);

// 0x0047d560  size 0x11
void __thiscall CObj0047d490::NopTwice()
{
    chp_Nop(this);
    chp_Nop(this);
}
HOOK_METHOD(0x0047d560, CObj0047d490, NopTwice);

// 0x0047d580  size 0xf
// The original tail-jumps to the no-op; EAX on return is the flag value (callers ignore it).
void __thiscall CObj0047d490::NopIfVersioned()
{
    if (g_loginByAddr != 0)
        chp_Nop(this);
}
HOOK_METHOD(0x0047d580, CObj0047d490, NopIfVersioned);
