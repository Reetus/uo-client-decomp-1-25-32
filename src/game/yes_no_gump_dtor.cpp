// CYesNoGump destructor and scalar deleting destructor. (Porting pass 2, batch 08.)
#include "yes_no_gump.h"

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "crt.h"
#include "vtables.h"
#include "globals.h"


// 0x004230e0  size 0x92
// Clears the global when it still points at this gump, frees frame0 and the frame data, runs the base destructor.
CYesNoGump::~CYesNoGump()
{
    if (g_scrollChoiceGump == this)
        g_scrollChoiceGump = nullptr;
    if (gfx::bitmapOf(this) != nullptr) {
        gfx::FreeTagged(gfx::bitmapOf(this), "YesNo frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "YesNo framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x004230e0, CYesNoGump);

HOOK_VTABLE(0x0049e160, CYesNoGump);
HOOK_SDD(0x004230b0, CYesNoGump);
