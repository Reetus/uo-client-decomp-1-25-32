#include "edit_host_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch06_helpers.h"
#include "globals.h"
#include "input_control_gump.h"
#include "crt.h"

// 0x0041e0f0 on the control, no args.
#define EditControl_Init(self) (reinterpret_cast<CInputControlGump *>(self)->Redraw())

// 0x004694d0  size 0x102  (gump with edit-control member at +0x11c) create the edit control once
// The SEH frame around the `new` is omitted.
void __thiscall CEditHostGump::CreateEditControl()
{
    if (editControl != nullptr)
        return;

    // ? argument 4 is (short)(unk_24 - 0x20); the upper half of the pushed ECX is left over in the original
    uint8_t *ctl = reinterpret_cast<uint8_t *>(new CInputControlGump(0xd, 0xc, this, static_cast<uint16_t>(unk_24 - 0x20), static_cast<uint16_t>(0x10), 0, 0x1d, 1, 0xe, 1, 0, 0));
    editControl = ctl;

    strcpy(reinterpret_cast<char *>(ctl + 0xec), text);   // ? no null check in the original

    *reinterpret_cast<int32_t *>(editControl + 0xbc) = 1;
    EditControl_Init(editControl);
    CGump_AddChild(editControl, this, 1);

    strcpy(text, g_emptyString);
}
HOOK_METHOD(0x004694d0, CEditHostGump, CreateEditControl);
