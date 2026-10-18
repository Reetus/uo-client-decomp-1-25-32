#pragma once

#include "obj.h"

// A mobile with the idle-animation method (placeholder derived class, no extra fields).
class CMobileIdle : public CMobile {
public:
    // 0x00455f40  size 0x131  plain RET
    void __thiscall RandomIdleAnim();
};
static_assert(sizeof(CMobileIdle) == sizeof(CMobile), "CMobileIdle adds no fields");

