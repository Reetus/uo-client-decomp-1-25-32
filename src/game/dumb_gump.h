// CDumbGump: a gump that is just a frame0 bitmap plus children ("framedata for CDumbGump",
// "dumb frame0" in the constructors of several gumps). Only the draw handler is ported; the
// vtable slot 2 (+0x08) of CCharCreationGump and similar classes points at it.
// ? whether 0x0041dad0 is the slot of a distinct CDumbGump vtable or is only shared by those
//   classes is not settled; the name follows the allocation tags.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CDumbGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    // 0x0041dad0: draw handler (vtable slot +0x08), one stack argument (clip), RET 4.
    void __thiscall DrawBlitFrame(void *clip);

    // Second pass, dumb_gump_ops.cpp (vtable 0x0049ee58).
    // 0x0043d0a0: constructor, RET 0x20: (x, y, a3 (not read), width, height, tileArt, pixels (or 0), gumpArt (or -1)).
    CDumbGump(int px, int py, int a3, int w, int h, int tileArtId, const void *pixels, int gumpArt);
    // 0x0043d250: destructor body.
    ~CDumbGump();
    // 0x0043d2e0: draw handler (vtable slot +0x08), RET 4: tile art when +0x94 is set, else frame0.
    virtual void Draw(int *clip);

    int32_t useTileArt;     // +0x94  nonzero: draw tile art `tileArt` instead of frame0
    int32_t altBlit;        // +0x98  nonzero: frame0 is blitted with the alternate row blitter 0x004764f0
    int32_t tileArt;        // +0x9c
    // Every site allocates 0xb0 bytes; the weather particles (CWeatherParticle, weather.h) keep their life and velocity
    // at +0xa0..+0xac of the same block.
    uint8_t tail_a0[0x10];  // +0xa0
    virtual int HitTest(int x, int y);   // slot HitTest, copy of CMissileGump::HitTestNever
};

static_assert(offsetof(CDumbGump, useTileArt) == 0x94, "CDumbGump::useTileArt");
static_assert(offsetof(CDumbGump, altBlit) == 0x98, "CDumbGump::altBlit");
static_assert(offsetof(CDumbGump, tileArt) == 0x9c, "CDumbGump::tileArt");
static_assert(sizeof(CDumbGump) == 0xb0, "CDumbGump size (operator new(0xb0))");
