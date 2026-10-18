// CEditGroupGump: the small dialog of the skills window for renaming or deleting a skill group
// ("EditGroup gump", vtable 0x004a0718, operator new(0xa0)). Opened by the skills window's double click
// (CSkillListGump::OnDoubleClick, 0x00474010). Layout from the assembly of 0x00475b70 .. 0x00476000.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"
#include "input_control_gump.h"

class CEditGroupGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    // 0x00475b70  RET 4: constructor; builds the framed background, the name edit control and three buttons.
    CEditGroupGump(int groupIndex);
    // 0x00475f30  vtable slot 0x00, RET 4.
    // 0x00475f60  destructor body.
    ~CEditGroupGump();
    void Dtor() { this->CEditGroupGump::~CEditGroupGump(); }
    // 0x00476000  vtable slot 0x08, RET 4.
    virtual void Draw(int *clip);
    // 0x004760d0  size 0x5  vtable slot 0x4c: OK; tail jump to CSkillGroupDlg::RenameGroupOK (0x0040f6b0) on this gump.
    virtual void OnOk();
    // 0x004760e0  size 0x5  vtable slot 0x50: Cancel; tail jump to 0x0040cf60, which deletes the gump (virtual slot 0, flag 1).
    virtual void OnCancel();

    int32_t unk_94;
    int32_t group;                 // +0x98  index of the group being edited
    CInputControlGump *edit;       // +0x9c  the name edit control
    virtual int HitTest(int x, int y);   // slot HitTest, copy of CDeleteCharGump::HitTestFrame
    virtual void MouseDown(int x, int y);   // slot MouseDown, copy of CSignGump::MouseDown
    virtual void MouseUp(int x, int y);   // slot MouseUp, copy of CMacroCmdGump::OnMouseUp
    virtual void RButtonUp(int x, int y);   // slot RButtonUp, copy of CCourseGump::CloseAndDelete
    virtual void MouseMove(int x, int y);   // slot MouseMove, copy of CSignGump::MouseMove
    virtual void OnChildChanged(int child);   // slot OnChildChanged, empty (as CLoginGump::Stub_00421390 (0x00421390))
};

static_assert(offsetof(CEditGroupGump, group) == 0x98, "CEditGroupGump::group");
static_assert(offsetof(CEditGroupGump, edit) == 0x9c, "CEditGroupGump::edit");
static_assert(sizeof(CEditGroupGump) == 0xa0, "CEditGroupGump size (operator new(0xa0))");
