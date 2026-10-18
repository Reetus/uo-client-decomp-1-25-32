// Gump list-row control with +/- handlers. NOTE: restored by batch 06 after an accidental overwrite;
// layout re-derived from decomp/pass2/0x0040e280.cpp (offsets +0xa4, +0xac, +0xb0 checked against the assembly).
// The batch-06 skills window (0x004742xx..0x004755xx) lives in skill_list_gump.h as CSkillListGump.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CSkillGump : public CGump {
public:
    // 0x0040e280  decrease handler (negated steps -1/-2/-5/-10)
    void __thiscall OnDecrease();
    // 0x0040e190  increase handler (steps 1/2/5/10)
    void __thiscall OnIncrease();

    uint8_t pad_94[0xa4 - 0x94];
    int32_t lockFlag;       // +0xa4
    uint8_t pad_a8[0xac - 0xa8];
    int32_t disabled;       // +0xac
    void *parentList;       // +0xb0  owning list (its +0xcc is the node list head)
};

static_assert(offsetof(CSkillGump, lockFlag) == 0xa4, "CSkillGump::lockFlag");
static_assert(offsetof(CSkillGump, disabled) == 0xac, "CSkillGump::disabled");
static_assert(offsetof(CSkillGump, parentList) == 0xb0, "CSkillGump::parentList");
