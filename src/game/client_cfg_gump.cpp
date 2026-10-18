#include "client_cfg_gump.h"

#include <cstdlib>
#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "globals.h"
#include "crt.h"
#include "char_creation_events_ports.h"
#include "button_gump.h"
#include "toggle_control.h"
#include "edit_list_gump.h"
#include "macro_gump.h"
#include "font_manager.h"
#include "dumb_gump.h"
#include "char_ui_gumps.h"
#include "small_methods_u1_ports.h"
#include "vtables.h"
#include "input_control_gump.h"

// The SEH frame of the original (state counter at [ebp-4], handler 0x4990cd) is omitted. Every
// "p = operator new(size); obj = p ? ctor(p, ...) : 0" of the original is kept inside the make* helpers
// below; the code after a helper dereferences the result without a NULL check, as the original does.

// Globals read while the gump is built.

// Original constructors / helpers not ported yet (argument lists from the RET sizes and pass2).
// 0x00418bd0: plain RET, does not read ECX; called right after the base constructor. ? purpose not settled.
#define Orig_Fn418bd0() (LoadSkillNameTable())
// 0x00413110: rectangle hit button, RET 0x24, (x1, y1, x2, y2, parent, mode, 0, handler, 0).
// 0x00416120: toggle control, RET 0x2c, (x, y, parent, artOn, artOff, 1, 0x386, text, value, a10, a11).
#define Orig_NewToggle(self, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) (reinterpret_cast<CToggleControl *>(self)->Ctor(a1, a2, (int)(a3), a4, a5, a6, a7, a8, a9, a10, a11))
// 0x00416d40: add a row to an edit list, RET 8, (name, value).
#define Orig_EditListAdd(self, a1, a2) (reinterpret_cast<CEditListGump *>(self)->AddItem(a1, a2))
// 0x00418830: add a row to the list box, RET 0x14, (a, b, c, 4, 0xe).
#define Orig_ListBoxAdd(self, a1, a2, a3, a4, a5) (reinterpret_cast<CMacroListGump *>(self)->AddRow(a1, a2, (const char *)(a3), a4, a5))
// 0x00415920: plain RET, ECX = the config gump. ? purpose not settled.
#define Orig_Fn415920(self) (reinterpret_cast<CClientCfgGump *>(self)->UpdateKeyBindingDisplay())
// 0x004328e0: text size / layout query on the text object, RET 0x1c, (font, text, wrapWidth, &w, &h, &base, 2).
#define Orig_TextLayout(self, a1, a2, a3, a4, a5, a6, a7) (reinterpret_cast<CFontManager *>(self)->MeasureTextWrapped(a1, (char *)(a2), a3, a4, a5, a6, a7))
// 0x0043d0a0: art display / image gump, RET 0x20, (x, y, parent, w, h, 0, pixels, -1).
#define Orig_NewImageGump(self, a1, a2, a3, a4, a5, a6, a7, a8) ((CGump *)reinterpret_cast<CDumbGump *>(self)->Ctor(a1, a2, (int)(a3), a4, a5, a6, (const void *)(a7), a8))

#define makeButton(x, y, parent, p4, callback, artUp, artDown)     new CButtonGump(x, y, static_cast<int>(reinterpret_cast<uintptr_t>(parent)), p4, 0, callback, artUp, artDown, 0)
#define makeToggle(x, y, parent, value, a10, a11)     new CToggleControl(x, y, (int)(parent), 0x867, 0x869, 1, 0x386, g_emptyString, value, a10, a11)
#define makeThumb(x, y, x2, parent) new CScrollThumbGump(x, y, 0, x2, (int)(parent), 0x845)
#define makeRectButton(x1, y1, x2, y2, parent, mode, handler) new CButtonGump(x1, y1, x2, y2, parent, mode, 0, handler, 0)
#define makeInput(x, y, parent, art, maxLen) new CInputControlGump(x, y, parent, art, art, 0, maxLen, 1, 0x386, 1, 0, 0)

// 0x00413c70  size 0x1b65
// Client options gump: frame0 (art 0x960) and frame1 (art 0x1389) bitmaps, then the widgets of tabs 1..4 and
// the tab-less buttons. The widget's tab is the byte at CGump::layer.
CClientCfgGump::CClientCfgGump(int x_, int y_)
{
    int textW, textH, textBase;
    int i, j, k, off;

    Orig_Fn418bd0();
    g_clientCfgGump = this;
    name = const_cast<char *>("client cfg gump");
    dirty = 0;
    owner = nullptr;
    listCopyCur = nullptr;
    listCopyHead = nullptr;
    unk_a0 = 0;
    unk_9c = 0;
    unk_98 = 0;
    unk_94 = 0;
    unk_ac = 0;
    unk_a8 = 0;
    unk_a4 = 0;
    x = x_;
    y = y_;
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    unk_50 = 0x960;   // art id of frame 0
    layer = 1;        // current tab
    gfx::ArtSize(unk_50, &width, &height);   // art 0x960 size -> +0x1c / +0x20
    gfx::ArtSize(0x1389, &unk_24, &unk_28);  // art 0x1389 size -> +0x24 / +0x28
    frameIndex = 0;
    frameCount = 2;
    frameData = static_cast<int32_t *>(gfx::AllocTagged(0x10, "framedata for CClientCfgGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    frameData[1] = static_cast<int32_t>(reinterpret_cast<uintptr_t>(gfx::AllocTagged(height * width * 2 + 4, "cfg frame0")));
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(height * width * 2));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);
    frameData[3] = static_cast<int32_t>(reinterpret_cast<uintptr_t>(gfx::AllocTagged(unk_24 * unk_28 * 2 + 4, "cfg frame1")));
    uint16_t *frame1 = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[3])));
    memset(reinterpret_cast<uint8_t *>(frame1) + 4, 0, static_cast<size_t>(unk_24 * unk_28 * 2));
    frame1[0] = static_cast<uint16_t>(unk_24);
    frame1[1] = static_cast<uint16_t>(unk_28);
    gfx::DrawArt(gfx::artPlain(), 0x960, 0, 0, gfx::bitmapOf(this) + 2, width, height);
    gfx::DrawArt(gfx::artPlain(), 0x1389, 0, 0, frame1 + 2, frame1[0], frame1[1]);
    gfx::TextMeasure(1, g_refText, &textW, &textH, &textBase);   // results unused
    word_c0 = g_colorSay;

    // ---- tab 1 ----
    btn_b8 = makeButton(0xcc, 0x9a, this, 0, PORT_ADDR(&CCharCreateGump::ToggleSpeechSample), 0x9a8, 0x9a9);
    btn_b8->artHover = 0x9aa;
    btn_b8->layer = 1;
    gfx::AddChild(btn_b8, this);
    word_c2 = g_hue1;

    btn_bc = makeButton(0xe0, 0xaf, this, 0, PORT_ADDR(&CCharCreateGump::ToggleEmoteSample), 0x9a8, 0x9a9);
    btn_bc->artHover = 0x9aa;
    btn_bc->layer = 1;
    gfx::AddChild(btn_bc, this);

    // toggle controls: (x, y, parent, value, 0, 0)
    toggle_c8 = makeToggle(0x59, 0x78, this, g_ccMusicVal, 0, 0);
    toggle_c8->artHover = 0x868;
    toggle_c8->artHoverChecked = 0x86a;
    toggle_c8->layer = 1;
    gfx::AddChild(toggle_c8, this);

    toggle_cc = makeToggle(0x46, 0x5e, this, g_ccSoundVal, 0, 0);
    toggle_cc->artHover = 0x868;
    toggle_cc->artHoverChecked = 0x86a;
    toggle_cc->layer = 1;
    gfx::AddChild(toggle_cc, this);

    toggle_d4 = makeToggle(0x46, 0xc6, this, g_ccVal62c, 0, 0);
    toggle_d4->artHover = 0x868;
    toggle_d4->artHoverChecked = 0x86a;
    toggle_d4->layer = 1;
    gfx::AddChild(toggle_d4, this);

    toggle_d8 = makeToggle(0x46, 0x99, this, g_ccVal2cc, 0, 0);
    toggle_d8->artHover = 0x868;
    toggle_d8->artHoverChecked = 0x86a;
    toggle_d8->layer = 1;
    gfx::AddChild(toggle_d8, this);

    toggle_dc = makeToggle(0x59, 0xaf, this, g_ccVal220, 0, 0);
    toggle_dc->artHover = 0x868;
    toggle_dc->artHoverChecked = 0x86a;
    toggle_dc->layer = 1;
    gfx::AddChild(toggle_dc, this);

    toggle_e0 = makeToggle(0xcc, 0xc2, this, g_warnOnAttack, 0, 0);
    toggle_e0->artHover = 0x868;
    toggle_e0->artHoverChecked = 0x86a;
    toggle_e0->layer = 1;
    gfx::AddChild(toggle_e0, this);

    // scroll thumbs
    thumb_124 = makeThumb(0xeb, 0x7d, 0x148, this);
    thumb_124->artAlt = 0x846;
    thumb_124->layer = 1;
    if (toggle_c8->checked == 0)
        thumb_124->visible = 0;
    gfx::ScrollThumbSet(thumb_124, g_byte_68db4c, 0xff);
    gfx::AddChild(thumb_124, this);

    thumb_128 = makeThumb(0xd7, 0x65, 0x134, this);
    thumb_128->artAlt = 0x846;
    thumb_128->layer = 1;
    if (toggle_cc->checked == 0)
        thumb_128->visible = 0;
    gfx::ScrollThumbSet(thumb_128, g_byte_69faac, 0xff);
    gfx::AddChild(thumb_128, this);

    // four hit-rect buttons
    CGump *w;
    w = makeRectButton(0xca, 0x66, 0xd7, 0x76, this, 2, PORT_ADDR(&CClientCfgGump::Thumb128Dec));
    w->layer = 1;
    gfx::AddChild(w, this);

    w = makeRectButton(0x134, 0x66, 0x141, 0x76, this, 2, PORT_ADDR(&CClientCfgGump::Thumb128Inc));
    w->layer = 1;
    gfx::AddChild(w, this);

    w = makeRectButton(0xdd, 0x7d, 0xea, 0x8d, this, 2, PORT_ADDR(&CClientCfgGump::Thumb124Dec));
    w->layer = 1;
    gfx::AddChild(w, this);

    w = makeRectButton(0x148, 0x7d, 0x155, 0x8d, this, 2, PORT_ADDR(&CClientCfgGump::Thumb124Inc));
    w->layer = 1;
    gfx::AddChild(w, this);

    // ---- deep copy of the global singly linked list at g_listHead (g_listCount nodes) ----
    // node = 0xe4 bytes: [0x14] = entry count, entries of 0x14 bytes from node+0x18 (entry+8 = char *), next at node+0xe0
    {
        uint8_t *src = g_macroListHead;
        i = 0;
        uint8_t *prevCopy = nullptr;
        uint8_t *copy = nullptr;
        if (g_ccListCount > 0) {
            do {
                if (i == 0) {
                    copy = static_cast<uint8_t *>(::operator new(0xe4));
                    listCopyHead = copy;
                } else {
                    copy = static_cast<uint8_t *>(::operator new(0xe4));
                    *reinterpret_cast<uint8_t **>(prevCopy + 0xe0) = copy;
                }
                memcpy(copy, src, 0x39 * 4);
                *reinterpret_cast<int *>(copy + 0xe0) = 0;
                memcpy(copy + 0x18, src + 0x18, static_cast<size_t>(*reinterpret_cast<int *>(src + 0x14)) * 20);
                j = 0;
                if (*reinterpret_cast<int *>(src + 0x14) > 0) {
                    uint8_t *ep = src + 0x20;
                    ptrdiff_t delta = copy - src;
                    do {
                        if (*reinterpret_cast<char **>(ep) != nullptr) {
                            size_t len = strlen(*reinterpret_cast<char **>(ep)) + 1;
                            void *d = ::operator new(len);
                            *reinterpret_cast<void **>(ep + delta) = d;
                            memcpy(d, *reinterpret_cast<char **>(ep), len);
                        }
                        j++;
                        ep += 0x14;
                    } while (j < *reinterpret_cast<int *>(src + 0x14));
                }
                src = *reinterpret_cast<uint8_t **>(src + 0xe0);
                prevCopy = copy;
                i++;
            } while (i < g_ccListCount);
        }
    }
    listCopyCur = listCopyHead;

    // ---- tab 2 ----
    listBox_16c = new CMacroListGump(0x30, 0x9a, 0x13a, 0x46, 0, 10);
    listBox_16c->layer = 2;
    gfx::AddChild(listBox_16c, this);
    {
        uint8_t *cur = static_cast<uint8_t *>(listCopyCur);
        if (cur != nullptr) {
            if (*reinterpret_cast<int *>(cur + 0x14) > 0) {
                off = 0;
                for (k = 0; k < *reinterpret_cast<int *>(static_cast<uint8_t *>(listCopyCur) + 0x14); k++) {
                    cur = static_cast<uint8_t *>(listCopyCur);
                    Orig_ListBoxAdd(listBox_16c, *reinterpret_cast<int *>(cur + off + 0x18),
                                    *reinterpret_cast<int *>(cur + off + 0x1c),
                                    *reinterpret_cast<int *>(cur + off + 0x20), 4, 0xe);
                    off += 0x14;
                }
            }
        }
        cur = static_cast<uint8_t *>(listCopyCur);
        if (cur == nullptr || *reinterpret_cast<int *>(cur + 0x14) < 10)
            Orig_ListBoxAdd(listBox_16c, 0, 0, 0, 4, 0xe);
    }

    {
        CButtonGump *b;
        b = makeButton(0x50, 0x59, this, 2, PORT_ADDR(&CClientCfgGump::AddKeyBinding), 0x99c, 0x99d);
        b->artHover = 0x99e;
        b->layer = 2;
        gfx::AddChild(b, this);

        b = makeButton(0x85, 0x59, this, 2, PORT_ADDR(&CClientCfgGump::DeleteKeyBinding), 0x99f, 0x9a0);
        b->artHover = 0x9a1;
        b->layer = 2;
        gfx::AddChild(b, this);

        b = makeButton(0xc9, 0x59, this, 2, PORT_ADDR(&CClientCfgGump::PrevKeyBinding), 0x9a2, 0x9a3);
        b->artHover = 0x9a4;
        b->layer = 2;
        gfx::AddChild(b, this);

        b = makeButton(0x11d, 0x59, this, 2, PORT_ADDR(&CClientCfgGump::NextKeyBinding), 0x9a5, 0x9a6);
        b->artHover = 0x9a7;
        b->layer = 2;
        gfx::AddChild(b, this);
    }

    // toggle controls with 11 args: (x, y, this, 0x867, 0x869, 1, 0x386, "", 0, 0x867, 0x869)
    toggle_e4 = makeToggle(0x126, 0x78, this, 0, 0x867, 0x869);
    toggle_e4->artHover = 0x868;
    toggle_e4->artHoverChecked = 0x86a;
    toggle_e4->layer = 2;
    gfx::AddChild(toggle_e4, this);

    toggle_e8 = makeToggle(0xf1, 0x78, this, 0, 0x867, 0x869);
    toggle_e8->artHover = 0x868;
    toggle_e8->artHoverChecked = 0x86a;
    toggle_e8->layer = 2;
    gfx::AddChild(toggle_e8, this);

    toggle_ec = makeToggle(0xb0, 0x78, this, 0, 0x867, 0x869);
    toggle_ec->artHover = 0x868;
    toggle_ec->artHoverChecked = 0x86a;
    toggle_ec->layer = 2;
    gfx::AddChild(toggle_ec, this);

    // text entry
    input_f0 = makeInput(0x3d, 0x7a, this, 0x98b, 0x10);
    input_f0->layer = 2;
    input_f0->marginX = 7;
    input_f0->marginY = 5;
    gfx::AddChild(input_f0, this);

    Orig_Fn415920(this);
    word_c6 = 3;
    word_c4 = 0x963;
    if (g_ccVal1a0 != 0) {
        input_100 = makeInput(0x80, 0x8d, this, 0x988, 0x10);
        input_100->marginX = 8;
        input_100->marginY = 5;
        input_100->layer = 0x3c;
        input_100->password = 1;
        gfx::AddChild(input_100, this);

        btn_158 = makeButton(0xbe, 0xad, this, 0, PORT_ADDR(&CClientCfgGump::CheckPassword), 0x992, 0x993);
        btn_158->artHover = 0x994;
        btn_158->layer = 0x3c;
        gfx::AddChild(btn_158, this);

        word_c4 = 0x964;
        word_c6 = 0x3c;
    }

    // ---- tab 3 ----
    list_10c = new CEditListGump(0xd1, 0x6f, 0, 0, 0x981, 0x14);
    list_10c->count_ca = 4;
    list_10c->child_94->x += -6;
    list_10c->child_98->x += -6;
    list_10c->layer = 3;
    gfx::AddChild(list_10c, this);
    for (i = 0; i < g_ccSel650; i++)
        Orig_EditListAdd(list_10c, g_ccNames658 + i * 0x1e, g_ccVals8b0[i]);
    Orig_EditListAdd(list_10c, g_newRowText, 0);

    toggle_f4 = makeToggle(0x2e, 0x50, this, g_profanityOn, 0, 0);
    toggle_f4->artHover = 0x868;
    toggle_f4->artHoverChecked = 0x86a;
    toggle_f4->layer = 3;
    gfx::AddChild(toggle_f4, this);

    toggle_f8 = makeToggle(0x2e, 0xd4, this, g_ccVal1a0, 0, 0);
    toggle_f8->artHover = 0x868;
    toggle_f8->artHoverChecked = 0x86a;
    toggle_f8->layer = 3;
    gfx::AddChild(toggle_f8, this);

    list_110 = new CEditListGump(0x28, 0x6f, 0, 0, 0x97f, 0x50, 0x1e);
    list_110->layer = 0x63;
    gfx::AddChild(list_110, this);
    for (i = 0; i < g_ccSel654; i++)
        Orig_EditListAdd(list_110, g_ccNames900 + i * 0x1e, 0);
    Orig_EditListAdd(list_110, g_emptyString, 0);

    rectBtn_148 = makeRectButton(0x28, 0x6f, 0xd2, 0xd3, this, 0, PORT_ADDR(&CClientCfgGump::SetTabBytes));
    rectBtn_148->layer = 3;
    gfx::AddChild(rectBtn_148, this);

    btn_14c = makeButton(0xda, 0xd4, this, 0, PORT_ADDR(&CClientCfgGump::ShowPasswordPage), 0x9a8, 0x9a9);
    btn_14c->artHover = 0x9aa;
    btn_14c->layer = 3;
    gfx::AddChild(btn_14c, this);

    input_104 = makeInput(0x83, 0x73, this, 0x988, 0x10);
    input_104->marginX = 8;
    input_104->marginY = 5;
    input_104->layer = 0x32;
    input_104->password = 1;
    gfx::AddChild(input_104, this);

    input_108 = makeInput(0x82, 0xa3, this, 0x988, 0x10);
    input_108->marginX = 8;
    input_108->marginY = 5;
    input_108->layer = 0x32;
    input_108->password = 1;
    gfx::AddChild(input_108, this);

    btn_154 = makeButton(0x98, 0xc5, this, 0, PORT_ADDR(&CClientCfgGump::ShowTab3), 0x997, 0x996);
    btn_154->artHover = 0x995;
    btn_154->layer = 0x32;
    gfx::AddChild(btn_154, this);

    btn_150 = makeButton(0xdf, 0xc4, this, 0, PORT_ADDR(&CClientCfgGump::ConfirmPasswords), 0x992, 0x993);
    btn_150->artHover = 0x994;
    btn_150->layer = 0x32;
    gfx::AddChild(btn_150, this);

    // "Click to edit Profanity List" rendered into a temporary 16-bit buffer and wrapped in a static image gump
    {
        int tw, th, tbase;
        Orig_TextLayout(gfx::textObj(), 1, g_profanityText, 0x98, &tw, &th, &tbase, 2);
        if ((((abs(tw) & 1) ^ (tw >> 31)) - (tw >> 31)) == 1)   // odd positive width: round up
            tw++;
        void *buf = gfx::AllocTagged(th * tw * 2, "jiz ctrl");
        memset(buf, 0, static_cast<size_t>(th * tw * 2));
        int rect[4];
        rect[0] = 0;
        rect[1] = 0;
        rect[2] = tw;
        rect[3] = th;
        gfx::TextDrawWrapped(0, 0, 1, 0x386, g_profanityText, 0x98, buf, tw, rect, 2, 0);
    void *mem;
                image_120 = new CDumbGump(0x32, 0x82, (int)(this), tw, th, 0, (const void *)(buf), -1);
        image_120->layer = 3;
        gfx::AddChild(image_120, this);
        gfx::FreeTagged(buf, "jiz tmpbuf");
    }

    // ---- tab 4 ----
    list_12c = new CEditListGump(0x28, 0x6f, 0, 0, 0x97f, 0x14);
    list_12c->count_ca = 0xc;
    list_12c->layer = 4;
    gfx::AddChild(list_12c, this);
    i = 0;
    if (g_ccSel260 > 0) {
        char *nm = g_ccNames26c;
        int *pv = g_ccVals7bc;
        do {
            Orig_EditListAdd(list_12c, nm, *(pv - 0xa0 / 4));   // (&DAT_0069f71c)[i]
            if (*pv != 0) {   // (&DAT_0069f7bc)[i]
                (*list_12c->rows_bc)->hue = 0x170;
                (*list_12c->rows_bc)->Redraw();
            }
            if (*(pv + 0x50 / 4) == 0) {   // (&DAT_0069f80c)[i]
                (*list_12c->rows_bc)->hue = 0x21;
                (*list_12c->rows_bc)->Redraw();
            }
            nm += 0x1e;
            i++;
            pv += 1;
        } while (i < g_ccSel260);
    }
    Orig_EditListAdd(list_12c, g_newRowText, 0);

    list_130 = new CEditListGump(0xd1, 0x6f, 0, 0, 0x981, 0x14);
    list_130->count_ca = 4;
    list_130->child_94->x += -6;
    list_130->child_98->x += -6;
    list_130->layer = 4;
    gfx::AddChild(list_130, this);
    for (i = 0; i < g_ccSel264; i++)
        Orig_EditListAdd(list_130, g_ccNames4c4 + i * 0x1e, g_ccVals76c[i]);
    Orig_EditListAdd(list_130, g_newRowText, 0);

    toggle_134 = makeToggle(0x11e, 0xd0, this, g_ccVal268, 0, 0);
    toggle_134->artHover = 0x868;
    toggle_134->artHoverChecked = 0x86a;
    toggle_134->layer = 4;
    gfx::AddChild(toggle_134, this);

    // local RECT {0, 0, width, height} is stored in the original but never read again

    // buttons without a tab byte (tab 0 = every tab); the first five are all stored into btn_138, only the last survives
    btn_138 = makeButton(0x3a, 0x0e, this, 0, PORT_ADDR(&CClientCfgGump::ShowTab1), 0x966, 0x967);
    btn_138->artHover = 0x968;
    gfx::AddChild(btn_138, this);

    btn_138 = makeButton(0x85, 0x20, this, 0, PORT_ADDR(&CGump_00410110::SetStateTwo), 0x969, 0x96a);
    btn_138->artHover = 0x96b;
    gfx::AddChild(btn_138, this);

    btn_138 = makeButton(0xdc, 0x20, this, 0, PORT_ADDR(&CClientCfgGump::ShowTabC6), 0x96c, 0x96d);
    btn_138->artHover = 0x96e;
    gfx::AddChild(btn_138, this);

    btn_138 = makeButton(0x126, 0x0e, this, 0, PORT_ADDR(&U1_CallSlot1c), 0x972, 0x973);
    btn_138->artHover = 0x974;
    gfx::AddChild(btn_138, this);

    btn_138 = makeButton(0x3a, 0xf4, this, 0, PORT_ADDR(&U1_CallSlot1c), 0x972, 0x973);
    btn_138->artHover = 0x974;
    gfx::AddChild(btn_138, this);

    btn_13c = makeButton(0x127, 0xf5, this, 0, PORT_ADDR(&CClientCfgGump::Apply), 0x97b, 0x97c);
    btn_13c->artHover = 0x97d;
    gfx::AddChild(btn_13c, this);

    btn_144 = makeButton(0x85, 0xed, this, 0, PORT_ADDR(&CCharCreateGump::CollectSelections), 0x975, 0x976);
    btn_144->artHover = 0x977;
    gfx::AddChild(btn_144, this);

    btn_140 = makeButton(0xdc, 0xed, this, 0, PORT_ADDR(&CClientCfgGump::SetDefaults), 0x978, 0x979);
    btn_140->artHover = 0x97a;
    gfx::AddChild(btn_140, this);

    btn_138->visible = 1;
    btn_13c->visible = 1;
    btn_140->visible = 1;
    btn_144->visible = 1;

    switch (layer) {   // current tab (always 1 at this point in practice)
    case 1:
        break;
    case 3:
        if (toggle_f4->layer != 3) {
            btn_13c->visible = 0;
            btn_140->visible = 0;
            btn_144->visible = 0;
        }
        break;
    case 4:
        btn_144->visible = 0;
        break;
    default:
        break;
    }
}
HOOK_CTOR(0x00413c70, CClientCfgGump, (int, int));
