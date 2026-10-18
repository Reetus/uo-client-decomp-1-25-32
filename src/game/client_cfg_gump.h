// CClientCfgGump: the client options gump ("client cfg gump"), four tabs selected by CGump::layer.
// Vtable 0x0049ccc0. Layout from decomp/pass2/0x00413c70.cpp. The object size is 0x170.
#pragma once

#include <cstddef>
#include <cstdint>

#include "button_gump.h"
#include "edit_list_gump.h"
#include "gump.h"
#include "input_control_gump.h"
#include "scroll_thumb_gump.h"
#include "toggle_control.h"
#include "globals.h"

class CClientCfgGump : public CGump {
public:
    virtual int IsTagc0() { return 1; }
    // Gump prep P5: own copy of a slot body the original shares between gumps.
    virtual int HitTest(int x, int y);
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual void RButtonUp(int x, int y);
    virtual void Slot64(int a);
    // Batch u1 (small_methods_u1.cpp).
    // 0x004101c0 / 0x00410200: thumb_124 one step down / up (only while visible), then OnScroll.
    void __thiscall Thumb124Dec();
    void __thiscall Thumb124Inc();
    // 0x00410240 / 0x00410280: the same for thumb_128.
    void __thiscall Thumb128Dec();
    void __thiscall Thumb128Inc();
    // 0x00410800: set the tab bytes of rectBtn_148 (0x63), image_120 (0x63) and list_110 (3).
    void __thiscall SetTabBytes();
    // 0x004128c0: tab 0x32 page: clear both password fields and redraw the background (art 0x965).
    void __thiscall ShowPasswordPage();
    // 0x00412980: switch to tab 3 and redraw the background (art 0x963).
    void __thiscall ShowTab3();
    // 0x004129c0: compare the two password fields; mismatch message or copy to 0x0069e630 and show tab 3.
    void __thiscall ConfirmPasswords();
    // 0x00413c70  size 0x1b65  constructor, RET 8, (x, y) (allocation 0x170).
    CClientCfgGump(int x, int y);
    // 0x00415810: destructor (closes the sub gumps that belong to this one, frees the frames, then the base).
    ~CClientCfgGump();
    // 0x004157e0: vtable slot 0, RET 4.
    void Dtor() { this->CClientCfgGump::~CClientCfgGump(); }
    // 0x00412a90: tab 3 password check button: on a match switch to tab 3 art 0x963.
    void __thiscall CheckPassword();
    // 0x00415920: show the key of the list entry at listCopyCur and its three modifier toggles, refill the list box.
    void __thiscall UpdateKeyBindingDisplay();
    // 0x00415ae0: RET 4: a child control changed.
    virtual void OnChildChanged(int child);
    // 0x00415d30: RET 0xc, (pos, range, thumb): volume sliders.
    virtual void OnScroll(int a, int b, int c);
    // 0x00415da0: RET 0xc, (pos, range, thumb): the sound volume slider was released.
    virtual void OnScrollEnd(int a, int b, int c);
    // 0x00415df0 (vtable slot 0x10) / 0x00415ea0 (slot 0x14) / 0x00415f00: mouse handlers, RET 8, (x, y).
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void MouseMove(int x, int y);
    // 0x00415f90: RET 4: the key field lost focus.
    virtual void OnFocus(int field);
    // 0x00415fc0: periodic check for the double click timeout.
    virtual void Tick();
    // 0x00416010: vtable slot 0x08, RET 4.
    virtual void Draw(int *clip);
    // 0x0040f830  OK: apply the options (CollectSelections), write them to uo.cfg, send the two 0x69 packets, delete this
    void __thiscall Apply();
    // 0x0040fae0  Defaults for the current tab (layer byte)
    void __thiscall SetDefaults();
    // 0x004100c0  switch to tab 1 and redraw the background (art 0x960)
    void __thiscall ShowTab1();
    // 0x00410160  switch to the tab in word_c6 and redraw the background (art word_c4)
    void __thiscall ShowTabC6();
    // 0x004102c0  append a blank key binding node (0xe4 bytes) to the list copy and make it current
    void __thiscall AddKeyBinding();
    // 0x004103a0  delete the current key binding node
    void __thiscall DeleteKeyBinding();
    // 0x00410470  select the previous key binding node
    void __thiscall PrevKeyBinding();
    // 0x004104d0  select the next key binding node
    void __thiscall NextKeyBinding();

    int32_t unk_94;
    int32_t unk_98;
    int32_t unk_9c;
    int32_t unk_a0;
    int32_t unk_a4;
    int32_t unk_a8;
    int32_t unk_ac;
    uint8_t pad_b0[0xb8 - 0xb0];
    CButtonGump *btn_b8;             // +0xb8  tab 1
    CButtonGump *btn_bc;             // +0xbc  tab 1
    uint16_t word_c0;                // +0xc0  from 0x0069e4d0
    uint16_t word_c2;                // +0xc2  from 0x0069e4d4
    uint16_t word_c4;                // +0xc4  0x963 / 0x964
    uint16_t word_c6;                // +0xc6  3 / 0x3c
    CToggleControl *toggle_c8;       // +0xc8  tab 1
    CToggleControl *toggle_cc;       // +0xcc  tab 1
    uint8_t pad_d0[0xd4 - 0xd0];
    CToggleControl *toggle_d4;       // +0xd4  tab 1
    CToggleControl *toggle_d8;       // +0xd8  tab 1
    CToggleControl *toggle_dc;       // +0xdc  tab 1
    CToggleControl *toggle_e0;       // +0xe0  tab 1
    CToggleControl *toggle_e4;       // +0xe4  tab 2
    CToggleControl *toggle_e8;       // +0xe8  tab 2
    CToggleControl *toggle_ec;       // +0xec  tab 2
    CInputControlGump *input_f0;     // +0xf0  tab 2
    CToggleControl *toggle_f4;       // +0xf4  tab 3
    CToggleControl *toggle_f8;       // +0xf8  tab 3
    uint8_t pad_fc[0x100 - 0xfc];
    CInputControlGump *input_100;    // +0x100 tab 0x3c, created only when 0x007211a0 != 0
    CInputControlGump *input_104;    // +0x104 tab 0x32
    CInputControlGump *input_108;    // +0x108 tab 0x32
    CEditListGump *list_10c;         // +0x10c tab 3
    CEditListGump *list_110;         // +0x110 tab 0x63
    uint8_t pad_114[0x120 - 0x114];
    CGump *image_120;                // +0x120 "Click to edit Profanity List" image, tab 3
    CScrollThumbGump *thumb_124;     // +0x124 tab 1
    CScrollThumbGump *thumb_128;     // +0x128 tab 1
    CEditListGump *list_12c;         // +0x12c tab 4
    CEditListGump *list_130;         // +0x130 tab 4
    CToggleControl *toggle_134;      // +0x134 tab 4
    CButtonGump *btn_138;            // +0x138 (the first five buttons without a tab all land here)
    CButtonGump *btn_13c;            // +0x13c
    CButtonGump *btn_140;            // +0x140
    CButtonGump *btn_144;            // +0x144
    CGump *rectBtn_148;              // +0x148 rectangle hit button (0x00413110), tab 3
    CButtonGump *btn_14c;            // +0x14c tab 3
    CButtonGump *btn_150;            // +0x150 tab 0x32
    CButtonGump *btn_154;            // +0x154 tab 0x32
    CButtonGump *btn_158;            // +0x158 tab 0x3c, created only when 0x007211a0 != 0
    uint8_t pad_15c[0x164 - 0x15c];
    void *listCopyHead;              // +0x164 deep copy of the list at 0x0069f85c (nodes of 0xe4 bytes)
    void *listCopyCur;               // +0x168 current node of that copy
    CGump *listBox_16c;              // +0x16c tab 2 list box (operator new(0xd0), 0x004184e0)
};

static_assert(offsetof(CClientCfgGump, unk_94) == 0x94, "CClientCfgGump::unk_94");
static_assert(offsetof(CClientCfgGump, unk_ac) == 0xac, "CClientCfgGump::unk_ac");
static_assert(offsetof(CClientCfgGump, btn_b8) == 0xb8, "CClientCfgGump::btn_b8");
static_assert(offsetof(CClientCfgGump, btn_bc) == 0xbc, "CClientCfgGump::btn_bc");
static_assert(offsetof(CClientCfgGump, word_c0) == 0xc0, "CClientCfgGump::word_c0");
static_assert(offsetof(CClientCfgGump, word_c2) == 0xc2, "CClientCfgGump::word_c2");
static_assert(offsetof(CClientCfgGump, word_c4) == 0xc4, "CClientCfgGump::word_c4");
static_assert(offsetof(CClientCfgGump, word_c6) == 0xc6, "CClientCfgGump::word_c6");
static_assert(offsetof(CClientCfgGump, toggle_c8) == 0xc8, "CClientCfgGump::toggle_c8");
static_assert(offsetof(CClientCfgGump, toggle_cc) == 0xcc, "CClientCfgGump::toggle_cc");
static_assert(offsetof(CClientCfgGump, toggle_d4) == 0xd4, "CClientCfgGump::toggle_d4");
static_assert(offsetof(CClientCfgGump, toggle_d8) == 0xd8, "CClientCfgGump::toggle_d8");
static_assert(offsetof(CClientCfgGump, toggle_dc) == 0xdc, "CClientCfgGump::toggle_dc");
static_assert(offsetof(CClientCfgGump, toggle_e0) == 0xe0, "CClientCfgGump::toggle_e0");
static_assert(offsetof(CClientCfgGump, toggle_e4) == 0xe4, "CClientCfgGump::toggle_e4");
static_assert(offsetof(CClientCfgGump, toggle_e8) == 0xe8, "CClientCfgGump::toggle_e8");
static_assert(offsetof(CClientCfgGump, toggle_ec) == 0xec, "CClientCfgGump::toggle_ec");
static_assert(offsetof(CClientCfgGump, input_f0) == 0xf0, "CClientCfgGump::input_f0");
static_assert(offsetof(CClientCfgGump, toggle_f4) == 0xf4, "CClientCfgGump::toggle_f4");
static_assert(offsetof(CClientCfgGump, toggle_f8) == 0xf8, "CClientCfgGump::toggle_f8");
static_assert(offsetof(CClientCfgGump, input_100) == 0x100, "CClientCfgGump::input_100");
static_assert(offsetof(CClientCfgGump, input_104) == 0x104, "CClientCfgGump::input_104");
static_assert(offsetof(CClientCfgGump, input_108) == 0x108, "CClientCfgGump::input_108");
static_assert(offsetof(CClientCfgGump, list_10c) == 0x10c, "CClientCfgGump::list_10c");
static_assert(offsetof(CClientCfgGump, list_110) == 0x110, "CClientCfgGump::list_110");
static_assert(offsetof(CClientCfgGump, image_120) == 0x120, "CClientCfgGump::image_120");
static_assert(offsetof(CClientCfgGump, thumb_124) == 0x124, "CClientCfgGump::thumb_124");
static_assert(offsetof(CClientCfgGump, thumb_128) == 0x128, "CClientCfgGump::thumb_128");
static_assert(offsetof(CClientCfgGump, list_12c) == 0x12c, "CClientCfgGump::list_12c");
static_assert(offsetof(CClientCfgGump, list_130) == 0x130, "CClientCfgGump::list_130");
static_assert(offsetof(CClientCfgGump, toggle_134) == 0x134, "CClientCfgGump::toggle_134");
static_assert(offsetof(CClientCfgGump, btn_138) == 0x138, "CClientCfgGump::btn_138");
static_assert(offsetof(CClientCfgGump, btn_13c) == 0x13c, "CClientCfgGump::btn_13c");
static_assert(offsetof(CClientCfgGump, btn_140) == 0x140, "CClientCfgGump::btn_140");
static_assert(offsetof(CClientCfgGump, btn_144) == 0x144, "CClientCfgGump::btn_144");
static_assert(offsetof(CClientCfgGump, rectBtn_148) == 0x148, "CClientCfgGump::rectBtn_148");
static_assert(offsetof(CClientCfgGump, btn_14c) == 0x14c, "CClientCfgGump::btn_14c");
static_assert(offsetof(CClientCfgGump, btn_150) == 0x150, "CClientCfgGump::btn_150");
static_assert(offsetof(CClientCfgGump, btn_154) == 0x154, "CClientCfgGump::btn_154");
static_assert(offsetof(CClientCfgGump, btn_158) == 0x158, "CClientCfgGump::btn_158");
static_assert(offsetof(CClientCfgGump, listCopyHead) == 0x164, "CClientCfgGump::listCopyHead");
static_assert(offsetof(CClientCfgGump, listCopyCur) == 0x168, "CClientCfgGump::listCopyCur");
static_assert(offsetof(CClientCfgGump, listBox_16c) == 0x16c, "CClientCfgGump::listBox_16c");
static_assert(sizeof(CClientCfgGump) == 0x170, "CClientCfgGump size (operator new(0x170))");

