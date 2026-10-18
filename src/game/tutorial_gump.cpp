#include "tutorial_gump.h"

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "batch01_helpers.h"
#include "city_choice_gump.h"
#include "tga_load_ports.h"
#include "app_session_ops_ports.h"
#include "globals.h"

// Table of page file names at 0x004a6eb0: "tutorial\1gen.tga" .. "tutorial\6gen.tga", then "1", "2", "3"
// (single characters select a skill page), then an empty string as terminator.
// Object whose children at +0x9c / +0xa0 / +0xa4 hold the three chosen skill indexes (value at +0xe8).
// ? class not identified in this batch.

// 0x0043ebe0: load a .tga into a frame bitmap. ECX = path, EDX = bitmap; non-zero on success; plain RET.
#define Orig_LoadTga(a1, a2) (LoadTGA16(a1, (uint16_t *)(a2)))
// 0x0044e430: no arguments, plain RET; called after the gump deleted itself. ? purpose not read from pass2.
#define Orig_AfterTutorial() (SendCreateCharacter())

// Value (+0xe8) of the child of g_skillSelectObj chosen by the single-character table entry, plus one.
static int skillPageNumber(char c)
{
    unsigned off;
    if (c == '1')
        off = 0x9c;
    else if (c == '2')
        off = 0xa0;
    else
        off = 0xa4;
    uint8_t *child = *reinterpret_cast<uint8_t **>(g_pInitSkillGump + off);
    return *reinterpret_cast<int *>(child + 0xe8) + 1;
}

// 0x00412490  size 0x14c
// Advances pageIndex first and reads the NEW index from the table. Recurses when a skill page fails to load.
void __thiscall CTutorialGump::NextPage()
{
    char skillFile[0x28];
    char **table = g_tutorialTable;
    char *cur;

    pageIndex = pageIndex + 1;
    cur = table[pageIndex];
    if (strlen(cur) != 0) {
        for (;;) {
            cur = table[pageIndex];
            if (strlen(cur) == 1) {
                // single-character entry: '1' / '2' / other selects which skill slot
                int n = skillPageNumber(*table[pageIndex]);
                sprintf(skillFile, "tutorial\\skill%02d.tga", n);
                void *bmp = gfx::bitmapOf(this);
                char *path = g_app.ResolveDataPath2(skillFile);
                if (Orig_LoadTga(path, bmp) == 0)
                    NextPage();   // load failed: try the next page (recursive)
                return;
            }
            void *bmp = gfx::bitmapOf(this);
            char *path = g_app.ResolveDataPath2(cur);
            if (Orig_LoadTga(path, bmp) != 0)
                return;   // image loaded
            pageIndex = pageIndex + 1;
            if (strlen(table[pageIndex]) == 0)
                break;
        }
    }
    // the original tests `this != NULL` here; it is always true for a method call
    PORT_VSLOT1(this, kGumpSlot_DeletingDtor, void, int)(1);   // vtable[0], scalar deleting dtor
    Orig_AfterTutorial();
}
HOOK_METHOD(0x00412490, CTutorialGump, NextPage);

// 0x004125e0  size 0x14f
// Steps back with a pre-decrement of pageIndex while it is > 0. Out of pages: delete the gump and make the
// city choice gump and its two decorations visible again.
void __thiscall CTutorialGump::PrevPage()
{
    char skillFile[0x28];
    char **table = g_tutorialTable;

    while (pageIndex > 0) {
        int idx = pageIndex - 1;
        pageIndex = idx;
        char *cur = table[idx];
        if (strlen(cur) == 1) {
            int n = skillPageNumber(*table[idx]);
            sprintf(skillFile, "tutorial\\skill%02d.tga", n);
            void *bmp = gfx::bitmapOf(this);
            char *path = g_app.ResolveDataPath2(skillFile);
            if (Orig_LoadTga(path, bmp) == 0)
                PrevPage();   // load failed: step back again (recursive)
            return;
        }
        void *bmp = gfx::bitmapOf(this);
        char *path = g_app.ResolveDataPath2(cur);
        if (Orig_LoadTga(path, bmp) != 0)
            return;   // image loaded
    }
    // the original tests `this != NULL` here; it is always true for a method call
    PORT_VSLOT1(this, kGumpSlot_DeletingDtor, void, int)(1);   // vtable[0], scalar deleting dtor
    g_cityChoiceGump->visible = 1;
    g_cityChoiceGump->deco1->visible = 1;
    g_cityChoiceGump->deco2->visible = 1;
}
HOOK_METHOD(0x004125e0, CTutorialGump, PrevPage);
