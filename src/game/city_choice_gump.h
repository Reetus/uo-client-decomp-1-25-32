// CCityChoiceGump: the character creation "choose your city" page. Vtable 0x0049e390, name "CityChoice gump".
// Layout from decomp/merged/chunk_07_00422a10.cpp (constructor 0x00423830, not ported here) and the
// button handler 0x00411f90 (decomp/pass2/0x00411f90.cpp). Global pointer to the live instance: 0x00648900.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"
#include "globals.h"

class CCityChoiceGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    virtual int IsTagf4() { return 1; }
    // Gump prep P5: slot target formerly a free function (vtable_stubs_u2.cpp).
    virtual void Slot60();
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual void Draw(int *clip);
    virtual int HitTest(int x, int y);
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void RButtonUp(int x, int y);
    virtual void Slot64(int a);
    // Batch u1 (small_methods_u1.cpp).
    // 0x00411470: button callback: show the character creation page art, hide this page and its decorations.
    void __thiscall BackToCreation();
    // 0x00411f90: handler of the button at (0x112, 0xd0). Hides the right-hand decoration and opens the
    // city list gump. ? name guessed from the "city picker" comment in pass2.
    void __thiscall OnCityListButton();
    // 0x004114c0: finish button: copy the character creation pages into the character globals, then show the
    // tutorial (when its first page exists) or send the create-character packet, and hide this page.
    void __thiscall Finish();
    // 0x00411ec0: handler of the server field button: hides deco1 and opens the server list gump.
    void __thiscall OnServerListButton();

    // Added by porting pass 2 (batch 08).
    // 0x00423830: constructor (operator new(0xa4) at the callers).
    CCityChoiceGump();
    // 0x00423e30: destructor (the base destructor follows).
    ~CCityChoiceGump();
    void Dtor() { this->CCityChoiceGump::~CCityChoiceGump(); }

    void *serverField;     // +0x94  server name field (CInputControlGump)
    void *cityField;       // +0x98  city name field (CInputControlGump)
    CGump *deco1;          // +0x9c  left decoration
    CGump *deco2;          // +0xa0  right decoration
};

static_assert(offsetof(CCityChoiceGump, serverField) == 0x94, "CCityChoiceGump::serverField");
static_assert(offsetof(CCityChoiceGump, cityField) == 0x98, "CCityChoiceGump::cityField");
static_assert(offsetof(CCityChoiceGump, deco1) == 0x9c, "CCityChoiceGump::deco1");
static_assert(offsetof(CCityChoiceGump, deco2) == 0xa0, "CCityChoiceGump::deco2");
static_assert(sizeof(CCityChoiceGump) == 0xa4, "CCityChoiceGump size (operator new(0xa4))");

