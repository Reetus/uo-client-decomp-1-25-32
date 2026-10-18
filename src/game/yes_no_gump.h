// CYesNoGump: modal question with a Yes and a No button ("YesNo gump", vtable 0x0049e160).
// operator new(0xa4) at the callers (map_gump.cpp, app_startup.cpp). Only the fields the constructor writes are named.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CYesNoGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    // Gump prep P1 step 2: own copy of a body the original shares between gumps (gump_p1_overrides.cpp).
    virtual void Draw(int *clip);
    virtual int HitTest(int x, int y);
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void RButtonUp(int x, int y);
    virtual void MouseMove(int x, int y);
    virtual void OnOk();
    virtual void OnCancel();
    // 0x00422e50: RET 0x10 (text, yesCallback, noCallback, extra). The callbacks are code addresses of the
    // click handlers given to the two buttons; `extra` is stored at +0x98.
    CYesNoGump(const char *text, void *yesCallback, void *noCallback, int extra);

    // Added by porting pass 2 (batch 08).
    // 0x004230e0: destructor.
    ~CYesNoGump();
    void Dtor() { this->CYesNoGump::~CYesNoGump(); }

    int32_t unk_94;     // +0x94  set to 0
    int32_t extra;      // +0x98  fourth constructor argument
    uint8_t pad_9c[8];  // +0x9c  not touched by the constructor  // ?
};

static_assert(offsetof(CYesNoGump, extra) == 0x98, "CYesNoGump::extra");
static_assert(sizeof(CYesNoGump) == 0xa4, "CYesNoGump size (operator new(0xa4))");
