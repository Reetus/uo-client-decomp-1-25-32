// The two long Draw bodies the original shares between many gump classes: 0x004222b0 (CChgPWGump::Draw, 0xce bytes: Login,
// AcctLogin, CityChoice, InitSkill, ChgPW) and 0x00419700 (CCharCreationGump::Draw, 0xcf bytes: CharCreation, CharStat,
// DeleteChar, PlayChar, YesNo, MainMenu, Tutorial, StringQuery). Each class gets its own member with the same body; the
// linker folds the copies (/OPT:ICF). See docs/matching.md "Gump virtual slots" (open question 6).
//
// Function order and the spelling of each body are source shape here: VC5 picks the register allocation of ox / oy by state
// it carries from function to function, so the same body compiles to 206 or 207 bytes depending on what came before it
// in the translation unit and on the declarations the unit sees. These functions therefore live in a file of their own with
// the smallest include set that compiles them (no vtables.h, obj.h, walk.h, ...), so that an unrelated header edit cannot
// move them. Do not reorder, add or remove anything in this file or its includes without re-checking the slot check
// (python scripts/match.py: "virtual tables of the compiler ... 0 problems").
#include "windows.h"
#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "globals.h"
#include "vcall.h"
#include "gump_base_ops_ports.h"
#include "char_creation_gump.h"
#include "char_flow_gumps.h"
#include "char_stat_gump.h"
#include "city_choice_gump.h"
#include "init_skill_gump.h"
#include "login_gumps.h"
#include "main_menu_gump.h"
#include "tutorial_gump.h"
#include "yes_no_gump.h"
#include "string_query_gump.h"

// Canonical, 0x004222b0
void __thiscall CChgPWGump::Draw(int *clip)
{
    int ox;
    int oy;
    int rect[4];
    int cr[4];

    if (visible == 0)
        return;
    ox = 0;
    oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);
    uint16_t *bmp = gfx::bitmapOf(this);
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = ox + bmp[0];
    rect[3] = oy + bmp[1];
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, const int *, int *)(clip, rect) == 0)
        return;
    GumpRectClip(clip, rect, cr);
    GFX_BlitFrame(gfx::artPlain(), bmp + 2, cr, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
    for (CGump *c = firstChild; c != nullptr; c = c->next)
        PORT_VSLOT1(c, kGumpSlot_Draw, void, const int *)(cr);
}

// Copy of the body at 0x00419700.
void __thiscall CYesNoGump::Draw(int *clip)
{
    if (visible == 0)
        return;
    int ox = 0;
    int oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);

    uint16_t *bmp = gfx::bitmapOf(this);
    int rect[4];
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = bmp[0] + ox;
    rect[3] = oy + bmp[1];

    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, void *, int *)(clip, rect) != 0) {
        int clipped[4];
        GumpRectClip(static_cast<const int *>(clip), rect, clipped);
        GFX_BlitFrame(gfx::artPlain(), bmp + 2, clipped, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
        for (CGump *c = firstChild; c != nullptr; c = c->next)
            PORT_VSLOT1(c, kGumpSlot_Draw, void, void *)(clipped);
    }
}

// Copy of the body at 0x00419700.
void __thiscall CMainMenuGump::Draw(int *clip)
{
    int ox;
    int oy;
    int rect[4];
    int cr[4];

    if (visible == 0)
        return;
    ox = 0;
    oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);
    uint16_t *bmp = gfx::bitmapOf(this);
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = ox + bmp[0];
    rect[3] = oy + bmp[1];
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, const int *, int *)(clip, rect) == 0)
        return;
    GumpRectClip(clip, rect, cr);
    GFX_BlitFrame(gfx::artPlain(), bmp + 2, cr, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
    for (CGump *c = firstChild; c != nullptr; c = c->next)
        PORT_VSLOT1(c, kGumpSlot_Draw, void, const int *)(cr);
}

// Copy of the body at 0x00419700.
void __thiscall CDeleteCharGump::Draw(int *clip)
{
    if (visible == 0)
        return;
    int ox = 0;
    int oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);

    uint16_t *bmp = gfx::bitmapOf(this);
    int rect[4];
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = bmp[0] + ox;
    rect[3] = oy + bmp[1];

    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, void *, int *)(clip, rect) != 0) {
        int clipped[4];
        GumpRectClip(static_cast<const int *>(clip), rect, clipped);
        GFX_BlitFrame(gfx::artPlain(), bmp + 2, clipped, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
        for (CGump *c = firstChild; c != nullptr; c = c->next)
            PORT_VSLOT1(c, kGumpSlot_Draw, void, void *)(clipped);
    }
}

// Copy of the body at 0x004222b0.
void __thiscall CCityChoiceGump::Draw(int *clip)
{
    if (visible == 0)
        return;
    int ox = 0;
    int oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);

    uint16_t *bmp = gfx::bitmapOf(this);
    int rect[4];
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = bmp[0] + ox;
    rect[3] = oy + bmp[1];

    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, void *, int *)(clip, rect) != 0) {
        int clipped[4];
        GumpRectClip(static_cast<const int *>(clip), rect, clipped);
        GFX_BlitFrame(gfx::artPlain(), bmp + 2, clipped, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
        for (CGump *c = firstChild; c != nullptr; c = c->next)
            PORT_VSLOT1(c, kGumpSlot_Draw, void, void *)(clipped);
    }
}

// Copy of the body at 0x004222b0.
void __thiscall CLoginGump::Draw(int *clip)
{
    int ox;
    int oy;
    int rect[4];
    int cr[4];

    if (visible == 0)
        return;
    ox = 0;
    oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);
    uint16_t *bmp = gfx::bitmapOf(this);
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = ox + bmp[0];
    rect[3] = oy + bmp[1];
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, const int *, int *)(clip, rect) == 0)
        return;
    GumpRectClip(clip, rect, cr);
    GFX_BlitFrame(gfx::artPlain(), bmp + 2, cr, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
    for (CGump *c = firstChild; c != nullptr; c = c->next)
        PORT_VSLOT1(c, kGumpSlot_Draw, void, const int *)(cr);
}

// Copy of the body at 0x00419700.
void __thiscall CStringQueryGump::Draw(int *clip)
{
    if (visible == 0)
        return;
    int ox = 0;
    int oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);

    uint16_t *bmp = gfx::bitmapOf(this);
    int rect[4];
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = bmp[0] + ox;
    rect[3] = oy + bmp[1];

    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, void *, int *)(clip, rect) != 0) {
        int clipped[4];
        GumpRectClip(static_cast<const int *>(clip), rect, clipped);
        GFX_BlitFrame(gfx::artPlain(), bmp + 2, clipped, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
        for (CGump *c = firstChild; c != nullptr; c = c->next)
            PORT_VSLOT1(c, kGumpSlot_Draw, void, void *)(clipped);
    }
}

// Canonical, 0x00419700
void __thiscall CCharCreationGump::Draw(int *clip)
{
    int ox;
    int oy;
    int rect[4];
    int cr[4];

    if (visible == 0)
        return;
    ox = 0;
    oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);
    uint16_t *bmp = gfx::bitmapOf(this);
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = ox + bmp[0];
    rect[3] = oy + bmp[1];
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, const int *, int *)(clip, rect) == 0)
        return;
    GumpRectClip(clip, rect, cr);
    GFX_BlitFrame(gfx::artPlain(), bmp + 2, cr, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
    for (CGump *c = firstChild; c != nullptr; c = c->next)
        PORT_VSLOT1(c, kGumpSlot_Draw, void, const int *)(cr);
}

// Copy of the body at 0x004222b0.
void __thiscall CAcctLoginGump::Draw(int *clip)
{
    if (visible == 0)
        return;
    int ox = 0;
    int oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);

    uint16_t *bmp = gfx::bitmapOf(this);
    int rect[4];
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = bmp[0] + ox;
    rect[3] = oy + bmp[1];

    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, void *, int *)(clip, rect) != 0) {
        int clipped[4];
        GumpRectClip(static_cast<const int *>(clip), rect, clipped);
        GFX_BlitFrame(gfx::artPlain(), bmp + 2, clipped, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
        for (CGump *c = firstChild; c != nullptr; c = c->next)
            PORT_VSLOT1(c, kGumpSlot_Draw, void, void *)(clipped);
    }
}

// Copy of the body at 0x004222b0.
void __thiscall CInitSkillGump::Draw(int *clip)
{
    int ox;
    int oy;
    int rect[4];
    int cr[4];

    if (visible == 0)
        return;
    ox = 0;
    oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);
    uint16_t *bmp = gfx::bitmapOf(this);
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = ox + bmp[0];
    rect[3] = oy + bmp[1];
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, const int *, int *)(clip, rect) == 0)
        return;
    GumpRectClip(clip, rect, cr);
    GFX_BlitFrame(gfx::artPlain(), bmp + 2, cr, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
    for (CGump *c = firstChild; c != nullptr; c = c->next)
        PORT_VSLOT1(c, kGumpSlot_Draw, void, const int *)(cr);
}

// Copy of the body at 0x00419700.
void __thiscall CTutorialGump::Draw(int *clip)
{
    int ox;
    int oy;
    int rect[4];
    int cr[4];

    if (visible == 0)
        return;
    ox = 0;
    oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);
    uint16_t *bmp = gfx::bitmapOf(this);
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = ox + bmp[0];
    rect[3] = oy + bmp[1];
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, const int *, int *)(clip, rect) == 0)
        return;
    GumpRectClip(clip, rect, cr);
    GFX_BlitFrame(gfx::artPlain(), bmp + 2, cr, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
    for (CGump *c = firstChild; c != nullptr; c = c->next)
        PORT_VSLOT1(c, kGumpSlot_Draw, void, const int *)(cr);
}

// Copy of the body at 0x00419700.
void __thiscall CPlayCharGump::Draw(int *clip)
{
    if (visible == 0)
        return;

    int ox = 0;
    int oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);

    uint16_t *bmp = gfx::bitmapOf(this);
    int rect[4];
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = bmp[0] + ox;
    rect[3] = oy + bmp[1];

    // 0x0043b540: thiscall on the gump manager object, (clip, &rect), RET 8
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, void *, int *)(clip, rect) != 0) {
        int clipped[4];
        GumpRectClip(static_cast<const int *>(clip), rect, clipped);   // RectClip
        GFX_BlitFrame(gfx::artPlain(), bmp + 2, clipped, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
        for (CGump *c = firstChild; c != nullptr; c = c->next)
            PORT_VSLOT1(c, kGumpSlot_Draw, void, void *)(clipped);
    }
}

// Copy of the body at 0x00419700.
void __thiscall CCharStatGump::Draw(int *clip)
{
    if (visible == 0)
        return;
    int ox = 0;
    int oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);

    uint16_t *bmp = gfx::bitmapOf(this);
    int rect[4];
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = bmp[0] + ox;
    rect[3] = oy + bmp[1];

    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, void *, int *)(clip, rect) != 0) {
        int clipped[4];
        GumpRectClip(static_cast<const int *>(clip), rect, clipped);
        GFX_BlitFrame(gfx::artPlain(), bmp + 2, clipped, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
        for (CGump *c = firstChild; c != nullptr; c = c->next)
            PORT_VSLOT1(c, kGumpSlot_Draw, void, void *)(clipped);
    }
}

HOOK_VMETHOD(0x00419700, CCharCreationGump, Draw);
HOOK_VMETHOD(0x004222b0, CChgPWGump, Draw);
