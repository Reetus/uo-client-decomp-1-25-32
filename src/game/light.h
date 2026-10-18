// Lighting: the light tables (lightidx.mul / light.mul) and the pass that
// darkens the back buffer per 64x48 tile. decomp/pass2/0x0044f2e0.cpp,
// 0x0044fe60.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"

// 0x0044f2e0  size 0xd1  load the light tables: lightidx.mul (DAT_0072101c file object, DAT_0071c764 view)
// and light.mul (DAT_0071c760, DAT_00721018). Thiscall whose ECX is preserved and returned.
void *__fastcall LoadLightTables(void *self);

// LightPass: `this` is the rectangle to light, in back-buffer pixels.
struct LightPass {
    int32_t left;           // +0x00
    int32_t top;            // +0x04
    int32_t right;          // +0x08
    int32_t bottom;         // +0x0c

    // 0x0044fe60  size 0x586  darken the rectangle by the ambient level minus the visible lights.
    void __thiscall Apply();
};

static_assert(sizeof(LightPass) == 0x10, "LightPass size");
static_assert(offsetof(LightPass, top) == 0x04, "LightPass::top");
static_assert(offsetof(LightPass, right) == 0x08, "LightPass::right");
static_assert(offsetof(LightPass, bottom) == 0x0c, "LightPass::bottom");
