#include "char_creation_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "globals.h"
#include "crt.h"
#include "vcall.h"
#include "file_manager.h"
#include "gump_base_ops_ports.h"
#include "vtables.h"

// The SEH frame of the original (state counter, handler 0x4993d8) is omitted.


// 0x00418bd0  size 0x181
// Loads the 50 records of file pair 0xf / 0x10 (one flag byte, then the skill name), counts the ones that exist and
// sorts the skill indices by name (bubble sort on strcmp). A missing record leaves an empty name in the slot that the
// next record overwrites.
void __cdecl LoadSkillNameTable()
{
    int i;

    g_skillCount = 0;
    for (i = 0; i < 0x32; i++) {
        int len;
        int unused;
        uint8_t *entry = reinterpret_cast<CFileManager *>(GLOBAL_PTR(void, g_fileManager_storage))->GetRecord(0xf, 0x10, i, &len, &unused);
        if (entry != nullptr) {
            int n = len - 1;
            g_skillFlags[g_skillCount] = entry[0];
            strncpy(g_skillNames + g_skillCount * 0x50, reinterpret_cast<const char *>(entry + 1), static_cast<size_t>(n));
            g_skillNames[g_skillCount * 0x50 + n] = 0;
            g_skillCount++;
        } else {
            int slot = g_skillCount;
            g_skillFlags[slot] = 0;
            strcpy(g_skillNames + slot * 0x50, g_emptyString);
        }
    }

    int count = g_skillCount;
    memset(g_skillOrder, 0, 0x32);
    for (i = 0; i < count; i++)
        g_skillOrder[i] = static_cast<uint8_t>(i);

    int limit = count - 1;
    int swapped;
    do {
        swapped = 0;
        for (i = 0; i < limit; i++) {
            uint8_t b = g_skillOrder[i + 1];
            uint8_t a = g_skillOrder[i];
            if (strcmp(g_skillNames + a * 0x50, g_skillNames + b * 0x50) > 0) {
                g_skillOrder[i] = b;
                g_skillOrder[i + 1] = a;
                swapped = 1;
            }
        }
    } while (swapped != 0);
}
HOOK_FUNC(0x00418bd0, LoadSkillNameTable);

HOOK_VTABLE(0x0049d558, CCharCreationGump);
HOOK_SDD(0x00419640, CCharCreationGump);

// 0x00419670  size 0x89
// Destructor body: clears the global pointer to this gump, frees the frame bitmap and the frame table.
CCharCreationGump::~CCharCreationGump()
{
    g_charCreationGump = nullptr;
    if (frameData[1] != 0) {
        gfx::FreeTagged(gfx::bitmapOf(this), "CharCreation frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "CharCreation framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00419670, CCharCreationGump);

// 0x00419700  size 0xcf
// Draws frame 0 at the absolute origin, clipped to the part of the clip it intersects, then the children with the
// clipped rectangle. Nothing is drawn while the gump is hidden.
// CCharCreationGump::Draw (0x00419700) lives in gump_draw_copies.cpp with the other copies of its body (see the header comment there).

// 0x004198a0  size 0x1e3
// A scroll thumb moved to `pos` of `range`: updates the matching selector and recomputes the hue it controls, then
// repaints the preview if the selector changed. +0xcc / +0xd0 are the skin group and the shade inside it,
// +0xd4 / +0xdc the hair hue group and shade, +0xd8 / +0xe0 the beard hue group and shade.
void __thiscall CCharCreationGump::OnScroll(int pos, int range, int thumbArg)
{
    CGump * thumb = reinterpret_cast<CGump *>(thumbArg);
    if (thumb == thumb_ac) {
        int old = unk_cc;
        int n = (g_skinHueCount - 1) * pos / range;
        unk_cc = n;
        if (old == n)
            return;
        skinHueIndex = g_skinHueTable[n] + unk_d0 + 0x3e9;
        RedrawPreview();
        return;
    }
    if (thumb == thumb_b0) {
        int old = unk_d0;
        int span;
        if (unk_cc < g_skinHueCount - 1)
            span = g_skinHueTable[unk_cc + 1] - g_skinHueTable[unk_cc] - 1;
        else
            span = 0x37 - g_skinHueTable[unk_cc];
        int n = span * pos / range;
        unk_d0 = n;
        if (old == n)
            return;
        skinHueIndex = g_skinHueTable[unk_cc] + n + 0x3e9;
        RedrawPreview();
        return;
    }
    if (thumb == thumb_b4) {
        int old = unk_dc;
        int n = pos * 7 / range;
        unk_dc = n;
        if (n == old)
            return;
        hairHue = n + unk_d4 * 8 + 0x44d;
        RedrawPreview();
        return;
    }
    if (thumb == thumb_bc) {
        int old = unk_d4;
        int n = pos * 5 / range;
        unk_d4 = n;
        if (n == old)
            return;
        hairHue = unk_dc + n * 8 + 0x44d;
        RedrawPreview();
        return;
    }
    if (thumb == thumb_b8) {
        int old = unk_e0;
        int n = pos * 7 / range;
        unk_e0 = n;
        if (n == old)
            return;
        beardHue = n + unk_d8 * 8 + 0x44d;
        RedrawPreview();
        return;
    }
    if (thumb == thumb_c0) {
        int old = unk_d8;
        int n = pos * 5 / range;
        unk_d8 = n;
        if (n == old)
            return;
        beardHue = unk_e0 + n * 8 + 0x44d;
        RedrawPreview();
    }
}
HOOK_VMETHOD(0x004198a0, CCharCreationGump, OnScroll);

// 0x00419a90  size 0x1d3
// The gender box changed: reset the hair box to "Short" (value 0x203b); for the second gender also clear the beard box
// and hide the beard thumbs, otherwise show them when a beard is set. A changed hair / beard box shows or hides its hue
// thumbs. The preview is repainted in every case.
void __thiscall CCharCreationGump::OnChildChanged(int childArg)
{
    CGump * child = reinterpret_cast<CGump *>(childArg);
    if (genderBox == child) {
        int gender = genderBox->value;
        hairBox->value = 0x203b;
        if (gender == 0) {
            strcpy(hairBox->text, "Short");
            button_9c->flag_b0 = 0;
            if (beardBox->value != 0) {
                thumb_b8->visible = 1;
                thumb_c0->visible = 1;
            }
        } else {
            strcpy(hairBox->text, "Short");
            beardBox->value = 0;
            strcpy(beardBox->text, "NONE");
            button_9c->flag_b0 = 1;
            beardBox->Redraw();
            thumb_b8->visible = 0;
            thumb_c0->visible = 0;
        }
        hairBox->Redraw();
        RedrawPreview();
        PORT_VSLOT1(this, 0x44, void, CGump *)(hairBox);
        return;
    }
    if (hairBox == child) {
        int shown = 0;
        if (hairBox->value != 0)
            shown = 1;
        thumb_b4->visible = shown;
        thumb_bc->visible = shown;
        RedrawPreview();
        return;
    }
    if (beardBox == child) {
        int shown = 0;
        if (beardBox->value != 0)
            shown = 1;
        thumb_b8->visible = shown;
        thumb_c0->visible = shown;
        RedrawPreview();
    }
}
HOOK_VMETHOD(0x00419a90, CCharCreationGump, OnChildChanged);
