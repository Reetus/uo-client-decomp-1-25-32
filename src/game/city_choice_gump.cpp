#include "city_choice_gump.h"

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "crt.h"
#include "scroll_choice_gump.h"
#include "globals.h"

// The SEH frame of the original (try level around operator new) is omitted.

#define g_cityNames GLOBAL_AS(int32_t *, g_locNames)       // pointer to an array of city entries (0 = unused)
#define g_cityListGump GLOBAL_AS(void *, g_scrollChoiceGump)       // the live city list gump

// 0x0041c930: list gump constructor (operator new(0x22cc)), RET 0xc, (owner, anchor field, a3). Not ported.
// 0x0041d7b0: add a list entry, RET 0xc, (index, entry, 0). Not ported.
// 0x0041d830: finish the list, plain RET. Not ported.

// 0x00411f90  size 0xbb
// City picker button: hides deco2, builds the 0x22cc-byte list gump over cityField, fills it and shows it.
void __thiscall CCityChoiceGump::OnCityListButton()
{
    deco2->visible = 0;
    CGump *g = new CScrollChoiceGump((CGump *)(this), (CInputControlGump *)(cityField), 0x4cc);   // 0x0041c930
    for (int i = 0; i < g_locCount; i++) {
        int s = g_cityNames[i];
        if (s != 0)
            reinterpret_cast<CScrollChoiceGump *>(g)->AddEntry(i, (const char *)(s), (uint16_t)(0));
    }
    reinterpret_cast<CScrollChoiceGump *>(g)->Redraw();   // plain thiscall, no stack arguments
    gfx::AddChild(g, nullptr);
    g_cityListGump = g;
}
HOOK_METHOD(0x00411f90, CCityChoiceGump, OnCityListButton);
