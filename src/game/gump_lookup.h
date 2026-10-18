// A gump that points at another object through +0x7c and can find the open top-level gump of that
// object (0x00436f70). Placeholder class: the identity of the gump is not known.
#pragma once

#include "gump.h"

class CGump_00436f70 : public CGump {
public:
    // 0x00436f70: last top-level gump (walking from the tail) that answers the slot test and whose
    // owner (+0x34) is the object at this+0x7c; 0 when there is none.
    CGump *__thiscall FindOpenGumpForTarget();
};
