// CEditGroupGump (0x00475b70 .. 0x00476000). The SEH frames of the constructor and destructor are omitted; every
// "operator new returned NULL" branch of the original is kept (the object pointer becomes 0).
#include "edit_group_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "button_gump.h"
#include "globals.h"
#include "gump_frame.h"
#include "crt.h"
#include "char_flow_gumps.h"
#include "small_methods_u1_ports.h"
#include "vtables.h"
#include "input_control_gump.h"
#include "class_0049c690.h"

#define g_focusedControl GLOBAL_AS(CGump *, g_focusedInput)

int __stdcall GumpRectIntersects(const int *a, const int *b);
void __fastcall GumpRectClip(const int *in, const int *clip, int *out);

// 0x00475b70  size 0x3b3
// The frame is the 9-slice of art 0x13ec; "Group Name:" at (0x14, 0x14), the name edit control at (0x14, 0x1c)
// holding the current name, captions "Rename" / "Delete" / "Cancel" and a button over each
// (callbacks 0x0040f6b0, 0x0040f740, 0x0040cf60).
CEditGroupGump::CEditGroupGump(int groupIndex)
{
    int rc[4];

    name = const_cast<char *>("EditGroup gump");
    dirty = 0;
    unk_50 = 0x13a6;
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    width = 0xc8;
    height = 0x78;
    x = 0xdc;
    y = 0xb4;
    frameIndex = 0;
    frameCount = 1;

    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CDumbGump"));   // sic: tag copied from the dumb gump
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    gfx::setBitmap(this, gfx::AllocTagged(height * width * 2 + 4, "dumb frame0"));   // sic
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(height * width * 2));
    PaintGumpFrame(0x13ec, gfx::bitmapOf(this), 0);

    group = groupIndex;
    rc[0] = 0;
    rc[1] = 0;
    rc[2] = width;
    rc[3] = height;
    gfx::TextDraw(0x14, 0x14, 1, 0x386, "Group Name:", gfx::bitmapOf(this) + 2, width, rc);

    edit = new CInputControlGump(0x14, 0x1c, this, 0x145a, 0x145d, 0, 0xc, 1, 0x386, 1, 0, 0xa0);
    gfx::AddChild(edit, this);
    g_focusedControl = edit;
    strcpy(edit->text, g_groupNames + group * 0x11);
    PORT_VSLOT1(edit, 0x3c, void, int)(0xe023);
    edit->Redraw();

    gfx::TextDraw(0x10, height - 0x32, 1, 0x386, "Rename", gfx::bitmapOf(this) + 2, width, rc);
    gfx::TextDraw(0x54, height - 0x32, 1, 0x386, "Delete", gfx::bitmapOf(this) + 2, width, rc);
    gfx::TextDraw(0x8c, height - 0x32, 1, 0x386, "Cancel", gfx::bitmapOf(this) + 2, width, rc);

    const int self = static_cast<int>(reinterpret_cast<uintptr_t>(this));
    CButtonGump *b1 = new CButtonGump(0x14, height - 0x28, self, 0, 0, PORT_ADDR(&CSkillGroupDlg::RenameGroupOK), 0x13ba, 0x13bb, 0);
    gfx::AddChild(b1, this);
    CButtonGump *b2 = new CButtonGump(0x54, height - 0x28, self, 0, 0, PORT_ADDR(&CSkillGroupDlg::DeleteGroup), 0x13bc, 0x13bd, 0);
    gfx::AddChild(b2, this);
    CButtonGump *b3 = new CButtonGump(0x90, height - 0x28, self, 0, 0, PORT_ADDR(&DeleteGumpCallback), 0x13bc, 0x13bd, 0);
    gfx::AddChild(b3, this);
}
HOOK_CTOR(0x00475b70, CEditGroupGump, (int));

HOOK_VTABLE(0x004a0718, CEditGroupGump);
HOOK_SDD(0x00475f30, CEditGroupGump);

// 0x00475f60  size 0x99
// Destructor body: delete the edit control, free frame0 and the frame data, base destructor.
CEditGroupGump::~CEditGroupGump()
{
    if (edit != nullptr) {
        PORT_VSLOT1(edit, 0x00, void, int)(1);
        edit = nullptr;
    }
    if (frameData[1] != 0) {
        gfx::FreeTagged(gfx::bitmapOf(this), "EditGroup frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "EditGroup framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00475f60, CEditGroupGump);

// 0x00476000  size 0xcc
// Draw (vtable slot 0x08): when the clip overlaps frame0, blit it clipped, then draw the children with the clipped rect.
void __thiscall CEditGroupGump::Draw(int *clip)
{
    int px = 0;
    int py = 0;
    int rect[4];
    int out[4];

    LocalToScreen(&px, &py);
    uint16_t *bmp = gfx::bitmapOf(this);
    rect[0] = px;
    rect[1] = py;
    rect[2] = px + bmp[0];
    rect[3] = py + bmp[1];
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, const int *, const int *)(clip, rect) != 0) {
        GumpRectClip(clip, rect, out);
        GFX_BlitFrame(gfx::artPlain(), gfx::bitmapOf(this) + 2, out, bmp[0], bmp[1], px, py, g_backBuffer, 0x280, 0x1e0);
        for (CGump *c = firstChild; c != nullptr; c = c->next)
            PORT_VSLOT1(c, 0x08, void, int *)(out);
    }
}
HOOK_VMETHOD(0x00476000, CEditGroupGump, Draw);
