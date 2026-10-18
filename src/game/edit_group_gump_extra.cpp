// Third porting pass, batch u3: OK / Cancel slots of CEditGroupGump (vtable 0x004a0718).
#include "edit_group_gump.h"

#include "../runtime/ported.h"
#include "char_flow_gumps.h"
#include "small_methods_u1_ports.h"
#include "class_0049c690.h"

// 0x004760d0  size 0x5
// Vtable slot 0x4c: OK. Tail jump to CSkillGroupDlg::RenameGroupOK (0x0040f6b0) with this gump in ECX.
void __thiscall CEditGroupGump::OnOk()
{
    reinterpret_cast<CSkillGroupDlg *>(this)->RenameGroupOK();
}
HOOK_VMETHOD(0x004760d0, CEditGroupGump, OnOk);

// 0x004760e0  size 0x5
// Vtable slot 0x50: Cancel. Tail jump to 0x0040cf60, which deletes the object in ECX through its virtual slot 0
// with flag 1 (null-safe).
void __thiscall CEditGroupGump::OnCancel()
{
    DeleteGumpCallback(this);
}
HOOK_VMETHOD(0x004760e0, CEditGroupGump, OnCancel);
