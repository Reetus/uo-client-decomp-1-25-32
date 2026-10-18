// CMainMenuGump: the title screen with its six buttons ("MainMenu gump", vtable 0x0049e278).
// operator new(0x9c) at the callers (char_flow_gumps.cpp). (Porting pass 2, batch 08.)
// +0x94 is a full-screen fade overlay (CDumbGump) that is removed when the fade ends, +0x98 the
// number of fade steps left (0x14 at the start, 0 when there is no fade).
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CMainMenuGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual void Draw(int *clip);
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void RButtonUp(int x, int y);
    virtual void MouseOver(int x, int y);
    // 0x00423180: constructor.
    CMainMenuGump();
    // 0x00423650: destructor.
    ~CMainMenuGump();
    void Dtor() { this->CMainMenuGump::~CMainMenuGump(); }
    // 0x004236f0: RET 8 (px, py). Non-zero when visible, no fade is running and the point is inside frame0.
    virtual int HitTest(int x, int y);
    // 0x00423740: plain RET. One fade step per call; removes the overlay when the fade is over.
    virtual void Update();

    CGump *overlay;        // +0x94
    int32_t fadeSteps;     // +0x98
};

static_assert(offsetof(CMainMenuGump, overlay) == 0x94, "CMainMenuGump::overlay");
static_assert(offsetof(CMainMenuGump, fadeSteps) == 0x98, "CMainMenuGump::fadeSteps");
static_assert(sizeof(CMainMenuGump) == 0x9c, "CMainMenuGump size (operator new(0x9c))");
