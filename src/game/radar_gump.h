// CRadarGump: the radar (mini-map) gump. vtable 0x0049ff70, name "radar gump".
// Layout from decomp/pass2/0x00469eb0.cpp and 0x0046a4f0.cpp. Derived-class
// fields start at +0x94; the colour table (0x10000 u16, indexed by art tile id,
// statics at +0x4000) sits at +0xac.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CRadarGump : public CGump {
public:
    virtual int IsTag10c() { return 1; }
    // slot override (gump virtual-slot prep): own copy of the shared body
    virtual void RButtonUp(int x, int y);
    virtual void Tick();
    // 0x00469eb0  RET 0xc
    CRadarGump(int x_, int y_, int artId);
    // 0x0046a240
    ~CRadarGump();
    void Dtor() { this->CRadarGump::~CRadarGump(); }
    // 0x0046a400  RET 8
    virtual void MouseMove(int x, int y);
    // 0x0046a4f0
    void __thiscall Redraw();

    // Second pass, radar_gump_ops.cpp.
    // 0x0046a330 / 0x0046a390  RET 8 (mx, my): left button down / up.
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    // 0x0046ac90: rotate the map frame into the rotated frame (+0x200b4) by 45 degrees, then blink the player pixel.
    void __thiscall RotateFrame();
    // 0x0046adf0: every fifth call toggle the centre pixel of the rotated frame between white and its saved colour.
    void __thiscall BlinkPlayerPixel();
    // 0x0046aec0  RET 4: blit the rotated frame (or frame 1 when +0x60 is set) to the back buffer, clipped to `clip`.
    virtual void Draw(int *clip);
    // 0x0046af70  RET 8 (x, y): the pixel at (x, y) of the same bitmap is non-zero. (Not CGump::HitTest.)
    virtual int HitTest(int x, int y);
    // 0x0046afd0: Redraw when the last one is at least 500 ms old.
    void __thiscall RedrawIfDue();

    int32_t unk_94;             // +0x94  set to 1 once the mouse moved more than 4 pixels while pressed
    int32_t unk_98;
    int32_t unk_9c;             // +0x9c  mouse pressed / drag active
    int32_t unk_a0;
    int32_t unk_a4;
    uint32_t unk_a8;            // +0xa8  click state (cleared when a drag starts)
    uint16_t colorTable[0x10000]; // +0xac
    int16_t originX;            // +0x200ac  map-cell origin of the radar window
    int16_t originY;            // +0x200ae
    int16_t halfWidth;          // +0x200b0
    int16_t halfHeight;         // +0x200b2
    uint16_t *rotatedFrame;     // +0x200b4  (width * height u16 with 4-byte header)
    uint8_t *zBuffer;           // +0x200b8
    int16_t unk_200bc;          // +0x200bc  zero
    int16_t unk_200be;          // +0x200be  zero
    uint32_t lastRedrawTick;    // +0x200c0  GetTickCount() of the last Redraw
};

static_assert(offsetof(CRadarGump, unk_94) == 0x94, "CRadarGump::unk_94");
static_assert(offsetof(CRadarGump, unk_9c) == 0x9c, "CRadarGump::unk_9c");
static_assert(offsetof(CRadarGump, unk_a8) == 0xa8, "CRadarGump::unk_a8");
static_assert(offsetof(CRadarGump, colorTable) == 0xac, "CRadarGump::colorTable");
static_assert(offsetof(CRadarGump, originX) == 0x200ac, "CRadarGump::originX");
static_assert(offsetof(CRadarGump, originY) == 0x200ae, "CRadarGump::originY");
static_assert(offsetof(CRadarGump, halfWidth) == 0x200b0, "CRadarGump::halfWidth");
static_assert(offsetof(CRadarGump, halfHeight) == 0x200b2, "CRadarGump::halfHeight");
static_assert(offsetof(CRadarGump, rotatedFrame) == 0x200b4, "CRadarGump::rotatedFrame");
static_assert(offsetof(CRadarGump, zBuffer) == 0x200b8, "CRadarGump::zBuffer");
static_assert(offsetof(CRadarGump, unk_200bc) == 0x200bc, "CRadarGump::unk_200bc");
static_assert(offsetof(CRadarGump, lastRedrawTick) == 0x200c0, "CRadarGump::lastRedrawTick");
static_assert(sizeof(CRadarGump) == 0x200c4, "CRadarGump size (operator new(0x200c4))");
