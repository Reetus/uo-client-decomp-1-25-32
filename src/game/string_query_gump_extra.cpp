// Third porting pass, batch u3: OK / Cancel slots of CStringQueryGump (vtable 0x004a04e8).
#include "string_query_gump.h"

#include "../runtime/ported.h"
#include "char_creation_pages.h"

// 0x00470380  size 0x5
// Vtable slot 0x4c: OK. Tail jump to CGumpHandlers04::TextEntryOk (0x00412e90) with this gump in ECX.
void __thiscall CStringQueryGump::OnOk()
{
    reinterpret_cast<CGumpHandlers04 *>(this)->TextEntryOk();
}
HOOK_VMETHOD(0x00470380, CStringQueryGump, OnOk);

// 0x00470390  size 0xf
// Vtable slot 0x50: Cancel. When bit 0 of flags (+0x9f, "OK and Cancel") is set, tail jump to
// CGumpHandlers04::TextEntryCancel (0x00412f20); otherwise nothing. Followed by padding.
void __thiscall CStringQueryGump::OnCancel()
{
    if ((flags & 1) != 0)
        reinterpret_cast<CGumpHandlers04 *>(this)->TextEntryCancel();
}
HOOK_VMETHOD(0x00470390, CStringQueryGump, OnCancel);
