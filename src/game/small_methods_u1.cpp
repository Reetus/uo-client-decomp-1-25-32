// Batch u1 (third pass): small procedures that nothing calls directly. Vtable-only methods, button callbacks
// (called with the parent gump in ECX) and jump thunks, between 0x00406640 and 0x00416710.
// Callbacks that never read ECX are free __fastcall functions taking it as `self`.
#include <cstring>

#include "../runtime/ported.h"
#include "bb_gump.h"
#include "bill_gump.h"
#include "button_gump.h"
#include "char_creation_gump.h"
#include "char_stat_gump.h"
#include "city_choice_gump.h"
#include "class_0049c690.h"
#include "client_cfg_gump.h"
#include "fighting_gump.h"
#include "globals.h"
#include "init_skill_gump.h"
#include "crt.h"
#include "paperdoll_gump.h"
#include "raw_field.h"
#include "scroll_thumb_gump.h"
#include "spell_book_gump.h"
#include "toggle_control.h"
#include "vcall.h"
#include "batch02_helpers.h"
#include "sound_manager.h"
#include "app_session_ops_ports.h"
#include "gump_manager.h"
#include "hue_gump_ops_ports.h"
#include "client_misc_ops_ports.h"
#include "window_helpers_31_ports.h"
#include "rotated_blit.h"
#include "macro_reload_callback_ports.h"
#include "object_manager.h"
#include "input_control_gump.h"
#include "art_row_blits_ports.h"
#include "small_methods_u1_ports.h"
#include "global_views.h"


// Gump art drawer, 0x00433ed0: ECX = row blitter object (0x004764c0), EDX = art id, stack (x, y, dest, stride, height).
static inline void U1_DrawGumpArt(int artId, const CGump *g)
{
    uint8_t *bmp = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(g->frameData[1])));
    PORT_FUNC7(&DrawGumpArt, void, __fastcall, void *, int, int, int, void *, int, int)(
        PTR_AT(void, PORT_ADDR(&RowBlitCopyNonZero)), artId, 0, 0, bmp + 4, g->width, g->height);
}

// 0x00406640  size 0x5
// Vtable slots 0x44 / 0x48 of the class at 0x0049c690 (and 15 more vtables): the 16-bit value at +0x24. No stack
// argument (plain RET); the upper half of EAX is whatever the caller left there.
uint32_t __thiscall CWorldObj::GetTopZ()
{
    return RAWF(uint16_t, this, 0x24);
}
HOOK_VMETHOD(0x00406640, CWorldObj, GetTopZ);

// Slot 0x48 of the same tables: the identical body under its own name (folded with GetTopZ by the linker).
uint32_t __thiscall CWorldObj::GetBaseZ()
{
    return RAWF(uint16_t, this, 0x24);
}

// 0x004067a0  size 0x8
// Vtable slot 0x3c: tail call of slot 0x3c of the owner (+0x48), same (empty) argument list.
int __thiscall CObj0049c690::IsHidden()
{
    return decomp::vcall(PORT_RET(int), owner, 0x3c);
}
HOOK_VMETHOD(0x004067a0, CObj0049c690, IsHidden);

// 0x0040b240  size 0x48
// Spell book, vtable slot 0x118: turn to the next spread (the page word is the left page).
void __thiscall CSpellBookGump::NextPage()
{
    uint16_t cur = page;
    if (static_cast<int>(cur) + 1 < static_cast<int>(pageCount)) {
        page = static_cast<uint16_t>(cur + 2);
        decomp::vcall_void(this, 0x114);   // redraw
        // CSoundManager::PlaySound 0x004800f0 (soundId 0x55) on the sound manager at 0x0068db30
        reinterpret_cast<CSoundManager *>(GLOBAL_PTR(void, g_soundManager_storage))->PlaySound(0x55, 0, 0, 0, 0, 0);
    }
}
HOOK_VMETHOD(0x0040b240, CSpellBookGump, NextPage);

// 0x0040b290  size 0x36
// Spell book, vtable slot 0x11c: turn to the previous spread.
void __thiscall CSpellBookGump::PrevPage()
{
    uint16_t cur = page;
    if (cur > 1) {
        page = static_cast<uint16_t>(cur - 2);
        decomp::vcall_void(this, 0x114);   // redraw
        reinterpret_cast<CSoundManager *>(GLOBAL_PTR(void, g_soundManager_storage))->PlaySound(0x55, 0, 0, 0, 0, 0);
    }
}
HOOK_VMETHOD(0x0040b290, CSpellBookGump, PrevPage);

// 0x0040c7c0  size 0x3
// Vtable slot shared by 2511 slots: returns 0, no stack argument.
int __fastcall U1_Return0(void *)
{
    return 0;
}
HOOK_FUNC(0x0040c7c0, U1_Return0);

// 0x0040c7d0  size 0x6
// Vtable slot shared by 171 slots: returns 1, no stack argument.
int __fastcall U1_Return1(void *)
{
    return 1;
}
HOOK_FUNC(0x0040c7d0, U1_Return1);

// 0x0040cf20  size 0x3
// __vec_ctor element constructor that does nothing: returns the element (ECX).
void *__fastcall U1_VecCtorNop(void *self)
{
    return self;
}
HOOK_FUNC(0x0040cf20, U1_VecCtorNop);

// 0x0040cf60  size 0xb
// Button callback (and confirm dialog "no"): delete the gump in ECX through its scalar deleting destructor.
void __thiscall CObj0049c690::DeleteSelf()
{
    if (this != nullptr)
        decomp::vdelete(this);
}
HOOK_VMETHOD(0x0040cf60, CObj0049c690, DeleteSelf);

void __fastcall DeleteGumpCallback(void *self)
{
    if (self != nullptr)
        decomp::vdelete(self);
}

// 0x0040cf70  size 0x15
// Paperdoll war button: when the owner answers slot 0x28 non-zero, toggle the war mode (SetWarMode 0x0044dda0, ECX = -1).
void __thiscall CPaperdollGump::OnWarButton()
{
    if (decomp::vcall(PORT_RET(int), owner, 0x28) != 0)
        SetWarMode(-1);
}
HOOK_METHOD(0x0040cf70, CPaperdollGump, OnWarButton);

// 0x0040d4d0  size 0x13
// Paperdoll button: open the gump of type 7 (CGumpManager::CreateGump 0x0043bd20 on the manager at 0x006488c0).
void *__fastcall U1_OpenGumpType7(void *)
{
    return reinterpret_cast<CGumpManager *>(g_gumpManager)->CreateGump(nullptr, 7, nullptr, 0);
}
HOOK_FUNC(0x0040d4d0, U1_OpenGumpType7);

// 0x0040d680  size 0x16
// Paperdoll button: open the gump of type 0x1388.
void *__fastcall U1_OpenGumpType1388(void *)
{
    return reinterpret_cast<CGumpManager *>(g_gumpManager)->CreateGump(nullptr, 0x1388, nullptr, 0);
}
HOOK_FUNC(0x0040d680, U1_OpenGumpType1388);

// 0x0040d710  size 0x5
// Paperdoll help button: jump thunk to HelpRequest_Send (0x004428d0, cdecl, no arguments).
void __fastcall U1_HelpButton(void *)
{
    HelpRequest_Send();
}
HOOK_FUNC(0x0040d710, U1_HelpButton);

// 0x0040e370  size 0x1f
// Bill gump button: start the signature animation unless it is running.
void __thiscall CBillGump::StartSigning()
{
    if (signing == 0) {
        signing = 1;
        sigWidth = 0;
    }
}
HOOK_METHOD(0x0040e370, CBillGump, StartSigning);

// 0x0040ea70  size 0x34
// Character skill page, back button: show the stat page again (and its art child), hide this page and its art child.
void __thiscall CInitSkillGump::ShowStatPage()
{
    if (g_pCharStatGump != nullptr) {
        g_pCharStatGump->visible = 1;
        g_pCharStatGump->art_c8->visible = 1;
        visible = 0;
        art_98->visible = 0;
    }
}
HOOK_METHOD(0x0040ea70, CInitSkillGump, ShowStatPage);

// 0x0040f820  size 0xa
// Callback and vtable slot: run slot 0x1c with (0, 0) on the gump in ECX.
int __fastcall U1_CallSlot1c(void *self)
{
    return decomp::vcall(PORT_RET(int), self, 0x1c, 0, 0);
}
HOOK_FUNC(0x0040f820, U1_CallSlot1c);

// 0x004101c0  size 0x38
// Options gump, volume slider thumb_124 one step left: report the new position through slot 0x30.
void __thiscall CClientCfgGump::Thumb124Dec()
{
    CScrollThumbGump *t = thumb_124;
    if (t->visible != 0) {
        int32_t cur = t->x;
        if (cur > t->minX) {
            t->x = cur - 1;
            CScrollThumbGump *t2 = thumb_124;
            decomp::vcall_void(this, 0x30, t2->x, t2->maxX, t2);
        }
    }
}
HOOK_METHOD(0x004101c0, CClientCfgGump, Thumb124Dec);

// 0x00410200  size 0x38
// thumb_124 one step right.
void __thiscall CClientCfgGump::Thumb124Inc()
{
    CScrollThumbGump *t = thumb_124;
    if (t->visible != 0) {
        int32_t cur = t->x;
        if (cur < t->maxX) {
            t->x = cur + 1;
            CScrollThumbGump *t2 = thumb_124;
            decomp::vcall_void(this, 0x30, t2->x, t2->maxX, t2);
        }
    }
}
HOOK_METHOD(0x00410200, CClientCfgGump, Thumb124Inc);

// 0x00410240  size 0x38
// thumb_128 one step left.
void __thiscall CClientCfgGump::Thumb128Dec()
{
    CScrollThumbGump *t = thumb_128;
    if (t->visible != 0) {
        int32_t cur = t->x;
        if (cur > t->minX) {
            t->x = cur - 1;
            CScrollThumbGump *t2 = thumb_128;
            decomp::vcall_void(this, 0x30, t2->x, t2->maxX, t2);
        }
    }
}
HOOK_METHOD(0x00410240, CClientCfgGump, Thumb128Dec);

// 0x00410280  size 0x38
// thumb_128 one step right.
void __thiscall CClientCfgGump::Thumb128Inc()
{
    CScrollThumbGump *t = thumb_128;
    if (t->visible != 0) {
        int32_t cur = t->x;
        if (cur < t->maxX) {
            t->x = cur + 1;
            CScrollThumbGump *t2 = thumb_128;
            decomp::vcall_void(this, 0x30, t2->x, t2->maxX, t2);
        }
    }
}
HOOK_METHOD(0x00410280, CClientCfgGump, Thumb128Inc);

// 0x00410800  size 0x1f
// Options gump: set the tab byte (+0x54) of rectBtn_148 and image_120 to 0x63 and of list_110 to 3.
void __thiscall CClientCfgGump::SetTabBytes()
{
    rectBtn_148->layer = 0x63;
    image_120->layer = 0x63;
    list_110->layer = 3;
}
HOOK_METHOD(0x00410800, CClientCfgGump, SetTabBytes);

// 0x00410870  size 0xd
// Bulletin board "new post" button: OnPostNew(0, "") with the board in ECX.
void __thiscall CBBGump::OnNewPostButton()
{
    this->CBBGump::OnPostNew(0, const_cast<char *>(g_emptyString));
}
HOOK_METHOD(0x00410870, CBBGump, OnNewPostButton);

// 0x00410db0  size 0xb
// Quit button: exit(0) through the original CRT.
void __fastcall U1_ExitButton(void *)
{
    exit(0);
}
HOOK_FUNC(0x00410db0, U1_ExitButton);

// 0x00411090  size 0x32
// Main menu "documentation" button: leave full screen, open www.owo.com, or show "Documentation not found!".
void __fastcall U1_DocumentationButton(void *)
{
    Misc_ToggleIfFullscreen();   // Misc_ToggleIfFullscreen
    if (LaunchWebBrowser(g_004a6790) == 0) {   // LaunchWebBrowser("www.owo.com")
        if (g_launchDirError != 0)
            PORT_THISCALL4(&CreateGump2ac, void *, g_gumpManager, int, int, int, int)((int)(g_004a6774), 0, 0, 0);   // CreateGump2ac
    }
}
HOOK_FUNC(0x00411090, U1_DocumentationButton);

// 0x004110d0  size 0xf
// Main menu "registration" button: leave full screen, then tail call LaunchWebBrowser("ultima-registration.com").
int __fastcall U1_RegistrationButton(void *)
{
    Misc_ToggleIfFullscreen();   // Misc_ToggleIfFullscreen
    return LaunchWebBrowser(g_004a679c);
}
HOOK_FUNC(0x004110d0, U1_RegistrationButton);

// 0x00411470  size 0x44
// City choice gump back button: show the creation page art children, hide this page and its two decorations.
void __thiscall CCityChoiceGump::BackToCreation()
{
    g_charCreationGump->visible = 1;
    g_charCreationGump->art_c4->visible = 1;
    g_charCreationGump->art_c8->visible = 1;
    deco1->visible = 0;
    deco2->visible = 0;
    visible = 0;
}
HOOK_METHOD(0x00411470, CCityChoiceGump, BackToCreation);

// 0x00411bf0  size 0x28
// Character creation: toggle the gender value in the hidden box (+0xe8) and run slot 0x44 for it.
void __thiscall CCharCreationGump::ToggleGender()
{
    CInputControlGump *box = genderBox;
    RAWF(int32_t, box, 0xe8) = (RAWF(int32_t, box, 0xe8) == 0) ? 1 : 0;
    decomp::vcall_void(this, 0x44, genderBox);
}
HOOK_METHOD(0x00411bf0, CCharCreationGump, ToggleGender);

// 0x00412050  size 0x77
// War mode sliders: remember the toggle frame and both slider positions as percentages in 0x0069f868 / 0x0069f86c /
// 0x0069f870, then SendWarModePacket (0x00431480: ECX, EDX, one stack argument).
void __thiscall CFightingGump::SendWarSliders()
{
    int32_t frame = static_cast<CGump *>(toggleB)->frameIndex;
    g_fightMode = frame;
    CScrollThumbGump *a = static_cast<CScrollThumbGump *>(sliderA);
    int32_t pa = static_cast<int32_t>(static_cast<uint32_t>(a->x - a->minX) * 100u) / (a->maxX - a->minX);
    g_fightValueA = pa;
    CScrollThumbGump *b = static_cast<CScrollThumbGump *>(sliderB);
    int32_t pb = static_cast<int32_t>(static_cast<uint32_t>(b->x - b->minX) * 100u) / (b->maxX - b->minX);
    g_fightValueB = pb;
    SendWarModePacket(frame, pa, pb);
}
HOOK_METHOD(0x00412050, CFightingGump, SendWarSliders);

// 0x004120d0  size 0x1a
// War mode toggle A pressed: set its frame to 1, then slot 0x44 (OnToggle) for it.
void __thiscall CFightingGump::PressToggleA()
{
    static_cast<CGump *>(toggleA)->frameIndex = 1;
    decomp::vcall_void(this, 0x44, toggleA);
}
HOOK_METHOD(0x004120d0, CFightingGump, PressToggleA);

// 0x00412180  size 0x1f
void __thiscall CFightingGump::SliderADec()
{
    CScrollThumbGump *t = static_cast<CScrollThumbGump *>(sliderA);
    if (t->visible != 0 && t->x > t->minX)
        t->x = t->x - 1;
}
HOOK_METHOD(0x00412180, CFightingGump, SliderADec);

// 0x004121a0  size 0x1f
void __thiscall CFightingGump::SliderAInc()
{
    CScrollThumbGump *t = static_cast<CScrollThumbGump *>(sliderA);
    if (t->visible != 0 && t->x < t->maxX)
        t->x = t->x + 1;
}
HOOK_METHOD(0x004121a0, CFightingGump, SliderAInc);

// 0x004121c0  size 0x1f
void __thiscall CFightingGump::SliderBDec()
{
    CScrollThumbGump *t = static_cast<CScrollThumbGump *>(sliderB);
    if (t->visible != 0 && t->x > t->minX)
        t->x = t->x - 1;
}
HOOK_METHOD(0x004121c0, CFightingGump, SliderBDec);

// 0x004121e0  size 0x1f
void __thiscall CFightingGump::SliderBInc()
{
    CScrollThumbGump *t = static_cast<CScrollThumbGump *>(sliderB);
    if (t->visible != 0 && t->x < t->maxX)
        t->x = t->x + 1;
}
HOOK_METHOD(0x004121e0, CFightingGump, SliderBInc);

// 0x00412200  size 0x5
// Callback stored by packet 0x55: jump thunk to 0x0045c510 (load macros, finish login; cdecl, no arguments).
void __fastcall U1_LoginCompleteCallback(void *)
{
    Callback_ReloadMacros_0045c510();
}
HOOK_FUNC(0x00412200, U1_LoginCompleteCallback);

// 0x00412730  size 0xf
// Tutorial gump finish: delete the gump (ECX) if any, then tail call SendCreateCharacter (0x0044e430, cdecl).
void __fastcall U1_TutorialFinish(void *self)
{
    if (self != nullptr)
        decomp::vdelete(self);
    SendCreateCharacter();
}
HOOK_FUNC(0x00412730, U1_TutorialFinish);

// 0x00412890  size 0x2d
// "This will lower notoriety !" confirm: run the target cursor callback (0x006f4784) on the object with the serial at
// 0x005285f8, then delete the dialog (ECX).
void __fastcall U1_LowerNotorietyYes(void *self)
{
    void *obj = reinterpret_cast<CObjectManager *>(GLOBAL_PTR(void, g_objMgr_storage))->FindBySerial(GLOBAL_AS(uint32_t, g_dlgOwner));   // FindBySerial
    if (obj != nullptr)
        reinterpret_cast<void(__fastcall *)(void *)>(g_targetCallback)(obj);
    if (self != nullptr)
        decomp::vdelete(self);
}
HOOK_FUNC(0x00412890, U1_LowerNotorietyYes);

// 0x004128c0  size 0xb6
// Options gump, tab 0x32: clear both password fields (text at +0xec of the input controls), redraw each, and redraw
// the background with art 0x965. The inlined strlen/strcpy of the empty string (0x004b6b40) is strcpy.
void __thiscall CClientCfgGump::ShowPasswordPage()
{
    layer = 0x32;
    word_c6 = 0x32;
    word_c4 = 0x965;
    U1_DrawGumpArt(0x965, this);   // 0x00433ed0
    strcpy(reinterpret_cast<char *>(input_104) + 0xec, g_emptyString);
    input_104->CInputControlGump::Redraw();   // CInputControlGump::Redraw
    strcpy(reinterpret_cast<char *>(input_108) + 0xec, g_emptyString);
    input_108->CInputControlGump::Redraw();
}
HOOK_METHOD(0x004128c0, CClientCfgGump, ShowPasswordPage);

// 0x00412980  size 0x3c
// Options gump: switch to tab 3 and redraw the background (art 0x963).
void __thiscall CClientCfgGump::ShowTab3()
{
    layer = 3;
    word_c6 = 3;
    word_c4 = 0x963;
    U1_DrawGumpArt(0x963, this);   // 0x00433ed0
}
HOOK_METHOD(0x00412980, CClientCfgGump, ShowTab3);

// 0x004129c0  size 0xc4
// Options gump password page OK: when the two password fields differ show "Passwords did not match!", else copy
// the password to 0x0069e630 and switch to tab 3. The inlined strcmp / strcpy are the host calls.
void __thiscall CClientCfgGump::ConfirmPasswords()
{
    if (strcmp(input_104->text, input_108->text) != 0) {
        PORT_THISCALL4(&CreateGump2ac, void *, g_gumpManager, int, int, int, int)((int)(g_004a69d8), 0, 0, 0);   // CreateGump2ac
        return;
    }
    strcpy(g_0069e630, input_104->text);
    layer = 3;
    word_c6 = 3;
    word_c4 = 0x963;
    U1_DrawGumpArt(0x963, this);   // 0x00433ed0
}
HOOK_METHOD(0x004129c0, CClientCfgGump, ConfirmPasswords);

// 0x004134a0  size 0xb
// Button vtable slot 0x24: clear +0xbc.
void __thiscall CButtonGump::MouseLeave()
{
    unk_bc = 0;
}
HOOK_VMETHOD(0x004134a0, CButtonGump, MouseLeave);

// 0x004134b0  size 0x46
// Button vtable slot 0x2c: auto-repeat of a held button of type 2. Counts +0x98 down; at zero it hands +0x9c to the
// parent (+0x08) when +0xac is set and calls the callback with the parent in ECX.
void __thiscall CButtonGump::Tick()
{
    if (unk_c8 == 0)
        return;
    if (param_94 != 2)
        return;
    int32_t count = unk_98;
    if (count > 0) {
        unk_98 = count - 1;
        return;
    }
    if (unk_ac != 0)
        parent->unk_08 = unk_9c;
    PORT_THISCALL0((uint32_t)(callback), void, parent)();
}
HOOK_VMETHOD(0x004134b0, CButtonGump, Tick);

// 0x00413960  size 0xb
// Scroll thumb vtable slot 0x24: clear the hover flag (+0x98).
void __thiscall CScrollThumbGump::MouseLeave()
{
    hot = 0;
}
HOOK_VMETHOD(0x00413960, CScrollThumbGump, MouseLeave);

// 0x00416710  size 0x24
// Toggle control vtable slot 0x24: leave the hover state and redraw when there is a hover art.
void __thiscall CToggleControl::MouseLeave()
{
    if (hovering != 0) {
        int32_t art = artHover;
        hovering = 0;
        if (art != 0)
            this->CToggleControl::Redraw();   // CToggleControl::Redraw
    }
}
HOOK_VMETHOD(0x00416710, CToggleControl, MouseLeave);

// 0x00419c70  size 0x5
// Character creation vtable slot 0x4c: jump thunk to ShowCityChoice (0x0040e4f0).
void __thiscall CCharCreationGump::OnOk()
{
    this->CCharCreationGump::ShowCityChoice();
}
HOOK_VMETHOD(0x00419c70, CCharCreationGump, OnOk);

// 0x00419c80  size 0x5
// Character creation vtable slot 0x50: jump thunk to ShowInitSkill (0x0040e5c0).
void __thiscall CCharCreationGump::OnCancel()
{
    this->CCharCreationGump::ShowInitSkill();
}
HOOK_VMETHOD(0x00419c80, CCharCreationGump, OnCancel);

// 0x00419c90  size 0x18
// Character creation vtable slot 0x60: show the two art children.
void __thiscall CCharCreationGump::Slot60()
{
    art_c4->visible = 1;
    art_c8->visible = 1;
}
HOOK_VMETHOD(0x00419c90, CCharCreationGump, Slot60);

// 0x0041b5a0  size 0x5
// Character stat page vtable slot 0x4c: jump thunk to NextFromName (0x0040e660).
void __thiscall CCharStatGump::OnOk()
{
    this->CCharStatGump::NextFromName();
}
HOOK_VMETHOD(0x0041b5a0, CCharStatGump, OnOk);

// 0x0041b5b0  size 0x5
// Character stat page vtable slot 0x50: jump thunk to CancelCreation (0x0040e7d0).
void __thiscall CCharStatGump::OnCancel()
{
    this->CCharStatGump::CancelCreation();
}
HOOK_VMETHOD(0x0041b5b0, CCharStatGump, OnCancel);

// 0x0041b5c0  size 0xe
// Character stat page vtable slot 0x60: show the art child.
void __thiscall CCharStatGump::Slot60()
{
    art_c8->visible = 1;
}
HOOK_VMETHOD(0x0041b5c0, CCharStatGump, Slot60);
