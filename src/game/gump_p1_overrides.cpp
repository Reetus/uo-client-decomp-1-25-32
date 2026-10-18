// Gump prep P1 step 2: per-class overrides for the slot bodies the original shares between several gumps (one function in
// many tables). Each class gets its own member with an identical body; the linker folds them (/OPT:ICF). See docs/matching.md
// "Gump virtual slots". Referenced by the port tables only.
#include <cstdlib>
#include <windows.h>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "globals.h"
#include "crt.h"
#include "vcall.h"
#include "gump_base_ops_ports.h"
#include "mouse_manager.h"
#include "vtables.h"
#include "raw_field.h"
#define g_mouseManager GLOBAL_PTR(void, g_mouse_storage)
#include "char_creation_gump.h"
#include "char_flow_gumps.h"
#include "char_stat_gump.h"
#include "city_choice_gump.h"
#include "client_cfg_gump.h"
#include "edit_list_gump.h"
#include "init_skill_gump.h"
#include "input_control_gump.h"
#include "list_box_control_gump.h"
#include "login_gumps.h"
#include "macro_gump.h"
#include "main_menu_gump.h"
#include "scroll_choice_gump.h"
#include "toggle_control.h"
#include "tutorial_gump.h"
#include "yes_no_gump.h"
#include "string_query_gump.h"
#include "window_helpers_31_ports.h"















void __thiscall CMainMenuGump::MouseDown(int, int)
{
    CGump_BringToFront(this, 0);
}

void __thiscall CMainMenuGump::MouseUp(int, int)
{
    g_dragGump = nullptr;
}

void __thiscall CMainMenuGump::RButtonUp(int, int)
{
    g_dragGump = nullptr;
}

void __thiscall CMainMenuGump::MouseOver(int, int)
{
    reinterpret_cast<CMouseManager *>(g_mouseManager)->SetMouseCursor(0x11);
}


int __thiscall CAcctLoginGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

void __thiscall CAcctLoginGump::MouseDown(int, int)
{
    CGump_BringToFront(this, 0);
}

void __thiscall CAcctLoginGump::MouseUp(int, int)
{
    g_dragGump = nullptr;
}

void __thiscall CAcctLoginGump::RButtonUp(int, int)
{
    g_dragGump = nullptr;
}


void __thiscall CAcctLoginGump::OnChildChanged(int)
{
}

void __thiscall CAcctLoginGump::Slot64(int)
{
}

int __thiscall CLoginGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

void __thiscall CLoginGump::MouseUp(int, int)
{
    g_dragGump = nullptr;
}

void __thiscall CLoginGump::RButtonUp(int, int)
{
    g_dragGump = nullptr;
}

void __thiscall CLoginGump::OnChildChanged(int)
{
}

void __thiscall CLoginGump::OnTab()
{
}

void __thiscall CLoginGump::Slot64(int)
{
}

int __thiscall CChgPWGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

void __thiscall CChgPWGump::MouseDown(int mx, int my)
{
    BringToFront(0);
    RAWF(int32_t, this, 0x94) = 1;
    unk_6c = mx;
    unk_70 = my;
}

void __thiscall CChgPWGump::MouseUp(int, int)
{
    RAWF(int32_t, this, 0x94) = 0;
    g_dragGump = nullptr;
}

void __thiscall CChgPWGump::RButtonUp(int, int)
{
    g_dragGump = nullptr;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}

void __thiscall CChgPWGump::MouseMove(int mx, int my)
{
    if (RAWF(int32_t, this, 0x94) != 0)
        Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);
}

void __thiscall CChgPWGump::OnCancel()
{
    PORT_VSLOT2(this, 0x1c, void, int, int)(0, 0);
}

void __thiscall CDeleteCharGump::MouseDown(int mx, int my)
{
    BringToFront(0);
    RAWF(int32_t, this, 0x94) = 1;
    unk_6c = mx;
    unk_70 = my;
}


void __thiscall CDeleteCharGump::MouseUp(int, int)
{
    RAWF(int32_t, this, 0x94) = 0;
    g_dragGump = nullptr;
}

void __thiscall CDeleteCharGump::RButtonUp(int, int)
{
    g_dragGump = nullptr;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}

void __thiscall CDeleteCharGump::MouseMove(int mx, int my)
{
    if (RAWF(int32_t, this, 0x94) != 0)
        Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);
}

void __thiscall CDeleteCharGump::OnCancel()
{
    PORT_VSLOT2(this, 0x1c, void, int, int)(0, 0);
}

int __thiscall CPlayCharGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

void __thiscall CPlayCharGump::MouseDown(int mx, int my)
{
    BringToFront(0);
    RAWF(int32_t, this, 0x94) = 1;
    unk_6c = mx;
    unk_70 = my;
}

void __thiscall CPlayCharGump::MouseUp(int, int)
{
    RAWF(int32_t, this, 0x94) = 0;
    g_dragGump = nullptr;
}


void __thiscall CPlayCharGump::RButtonUp(int, int)
{
    g_dragGump = nullptr;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}

void __thiscall CPlayCharGump::MouseMove(int mx, int my)
{
    if (RAWF(int32_t, this, 0x94) != 0)
        Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);
}

void __thiscall CPlayCharGump::OnCancel()
{
    PORT_VSLOT2(this, 0x1c, void, int, int)(0, 0);
}

int __thiscall CYesNoGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

void __thiscall CYesNoGump::MouseDown(int mx, int my)
{
    BringToFront(0);
    RAWF(int32_t, this, 0x94) = 1;
    unk_6c = mx;
    unk_70 = my;
}

void __thiscall CYesNoGump::MouseUp(int, int)
{
    RAWF(int32_t, this, 0x94) = 0;
    g_dragGump = nullptr;
}


void __thiscall CYesNoGump::RButtonUp(int, int)
{
    g_dragGump = nullptr;
}

void __thiscall CYesNoGump::MouseMove(int mx, int my)
{
    if (RAWF(int32_t, this, 0x94) != 0)
        Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);
}

void __thiscall CYesNoGump::OnOk()
{
}

void __thiscall CYesNoGump::OnCancel()
{
}

int __thiscall CCityChoiceGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

void __thiscall CCityChoiceGump::MouseDown(int, int)
{
    CGump_BringToFront(this, 0);
}

void __thiscall CCityChoiceGump::MouseUp(int, int)
{
    g_dragGump = nullptr;
}

void __thiscall CCityChoiceGump::RButtonUp(int, int)
{
    g_dragGump = nullptr;
}

void __thiscall CCityChoiceGump::Slot64(int)
{
}

int __thiscall CTutorialGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

void __thiscall CTutorialGump::MouseDown(int mx, int my)
{
    BringToFront(0);
    RAWF(int32_t, this, 0x94) = 1;
    unk_6c = mx;
    unk_70 = my;
}

void __thiscall CTutorialGump::MouseUp(int, int)
{
    RAWF(int32_t, this, 0x94) = 0;
    g_dragGump = nullptr;
}

int __thiscall CCharCreationGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

void __thiscall CCharCreationGump::MouseUp(int, int)
{
    RAWF(int32_t, this, 0x94) = 0;
    g_dragGump = nullptr;
}

void __thiscall CCharCreationGump::OnTab()
{
}

void __thiscall CCharCreationGump::Slot64(int)
{
}

int __thiscall CCharStatGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

void __thiscall CCharStatGump::MouseUp(int, int)
{
    RAWF(int32_t, this, 0x94) = 0;
    g_dragGump = nullptr;
}

void __thiscall CCharStatGump::RButtonUp(int, int)
{
    g_dragGump = nullptr;
}

void __thiscall CCharStatGump::Slot64(int)
{
}

int __thiscall CInitSkillGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

void __thiscall CInitSkillGump::MouseUp(int, int)
{
    RAWF(int32_t, this, 0x94) = 0;
    g_dragGump = nullptr;
}

void __thiscall CInitSkillGump::RButtonUp(int, int)
{
    g_dragGump = nullptr;
}

void __thiscall CInitSkillGump::OnChildChanged(int)
{
}

int __thiscall CScrollChoiceGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

int __thiscall CInputControlGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

int __thiscall CListBoxControlGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

int __thiscall CToggleControl::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}



void __thiscall CToggleControl::RButtonUp(int px, int py)
{
    g_dragGump = nullptr;
    if (parent != nullptr)
        PORT_VSLOT2(parent, 0x1c, void, int, int)(px, py);
}

void __thiscall CClientCfgGump::RButtonUp(int, int)
{
    g_dragGump = nullptr;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}

void __thiscall CClientCfgGump::Slot64(int)
{
}

int __thiscall CEditListGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

void __thiscall CEditListGump::RButtonUp(int, int)
{
    CGump *par = parent;
    if (par != nullptr)
        PORT_VSLOT2(par, 0x1c, void, int, int)(0, 0);
}


int __thiscall CMacroCmdGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

int __thiscall CMacroListGump::HitTest(int px, int py)
{
    uint16_t *bmp = gfx::bitmapOf(this);
    int w = bmp[0];
    int h = bmp[1];
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return 1;
}

void __thiscall CMacroListGump::RButtonUp(int, int)
{
    CGump *par = parent;
    if (par != nullptr)
        PORT_VSLOT2(par, 0x1c, void, int, int)(0, 0);
}

void __thiscall CMacroListGump::OnTab()
{
}

// ---- Gump virtual slots, prep piece P5.
// Slot override: empty, as Stub_00401260 (0x00401260); the linker folds the copies.
void __thiscall CCharCreationGump::Slot40(int)
{
}


// Slot override: same body as CDragReleaseGump::ClearDragCapture (0x0041c2f0); the linker folds the copies.
void __thiscall CCharCreationGump::RButtonUp(int mx, int my)
{
    (void)mx;
    (void)my;
    g_dragGump = nullptr;
}



// Slot override: same body as CStringPickerGump::MouseUp (0x0046fdc0); the linker folds the copies.
void __thiscall CScrollChoiceGump::MouseUp(int a, int b)
{
    (void)a;
    (void)b;
    pendingClick = 0;
    if (unk_94 != 0) {
        unk_94 = 0;
        waitingDouble = 0;
        g_dragGump = nullptr;
        return;
    }
    if (waitingDouble == 2)
        waitingDouble = 0;
    g_dragGump = nullptr;
}

// Slot override: same body as CGump::MouseOver (0x0040c990), the original's slot 0x28 target; the linker folds the copies.
void __thiscall CScrollChoiceGump::MouseMove(int, int)
{
    PORT_THISCALL1(&SetCursorForRegion, int, reinterpret_cast<void *>(GADDR(g_app_storage)), int)(0);
}

// Slot override: same body as CCourseGump::HitTest (0x00429130); the linker folds the copies.
int __thiscall CClientCfgGump::HitTest(int px, int py)
{
    uint16_t *bmp;
    if (unk_60 == 0)
        bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
    else
        bmp = static_cast<uint16_t *>(i32_ptr(frameData[3]));
    int w = bmp[0];
    int h = bmp[1];
    bmp += 2;
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return bmp[w * py + px] != 0;
}

// Slot override: same body as CEditListGump::MouseUp (0x00417610); the linker folds the copies.
// Slot override: same body as CEditListGump::MouseMove (0x00417660); the linker folds the copies.
// Slot override: same body as CEditListGump::Tick (0x004176f0); the linker folds the copies.

