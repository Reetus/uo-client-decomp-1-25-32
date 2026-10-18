// CMissileAnim: one flying missile record of the table at 0x004b7250 (operator new, freed when
// its lifetime ends). Layout from the assembly of 0x00430c60.
#pragma once

#include <cstddef>
#include <cstdint>

class CMissileAnim {
public:
    // 0x00430c60  size 0x181  advance one tick: pick the frame, move by (dx, dy), invalidate and
    // redraw the rotated sprite, convert the position back to world space. When the lifetime is
    // over the record is released and freed instead.
    void __thiscall Update();

    float x;                // +0x00
    float y;                // +0x04
    float dx;               // +0x08
    float dy;               // +0x0c
    uint8_t pad_10[8];
    float lastX;            // +0x18  copy of x after the scroll offset was added
    float lastY;            // +0x1c
    uint8_t angle;          // +0x20  0..255
    uint8_t pad_21;
    uint16_t art;           // +0x22  art id (0x2aa cycles its frame)
    uint8_t pad_24[8];
    int32_t tick;           // +0x2c
    int32_t lifetime;       // +0x30
    int8_t frame;           // +0x34  frame offset added to the art id
    uint8_t pad_35[3];
    uint32_t key;           // +0x38  key released with the record
};

static_assert(offsetof(CMissileAnim, dx) == 0x08, "CMissileAnim::dx");
static_assert(offsetof(CMissileAnim, lastX) == 0x18, "CMissileAnim::lastX");
static_assert(offsetof(CMissileAnim, angle) == 0x20, "CMissileAnim::angle");
static_assert(offsetof(CMissileAnim, art) == 0x22, "CMissileAnim::art");
static_assert(offsetof(CMissileAnim, tick) == 0x2c, "CMissileAnim::tick");
static_assert(offsetof(CMissileAnim, lifetime) == 0x30, "CMissileAnim::lifetime");
static_assert(offsetof(CMissileAnim, frame) == 0x34, "CMissileAnim::frame");
static_assert(offsetof(CMissileAnim, key) == 0x38, "CMissileAnim::key");

// 0x00430c40  size 0x1b  radians to the 0..255 angle scale: (int)(f * 128 / pi).
int __stdcall RadiansToAngle256(float radians);
