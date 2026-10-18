// CWeather: rain / snow particle gumps. Layout from decomp/pass2/0x0047d610.cpp
// and 0x0047d900.cpp. The particle gump is a 0xb0 byte CGump subclass built by
// 0x0043d0a0 (an art-display gump); only its +0xa0..+0xa8 fields are used here.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

// Weather particle: art-display gump (operator new(0xb0), ctor 0x0043d0a0).
class CWeatherParticle : public CGump {
public:
    uint8_t pad_094[0xc];
    int life;       // +0xa0  frames until respawn
    int velX;       // +0xa4  x velocity (subtracted)
    int velY;       // +0xa8  y velocity (added)
    int unk_ac;     // +0xac
};

static_assert(offsetof(CWeatherParticle, life) == 0xa0, "CWeatherParticle::life");
static_assert(offsetof(CWeatherParticle, velX) == 0xa4, "CWeatherParticle::velX");
static_assert(offsetof(CWeatherParticle, velY) == 0xa8, "CWeatherParticle::velY");
static_assert(sizeof(CWeatherParticle) == 0xb0, "CWeatherParticle size");

class CWeather {
public:
    // 0x0047d610: weather packet (type, intensity, temperature). type 0xfe = season/temperature report.
    void __thiscall HandlePacket(int type, int count, int temperature);
    // 0x0047d900: per-frame update; moves the particles.
    void __thiscall Update();
    // 0x0047dca0: spawn one rain particle.
    void __thiscall SpawnRain();
    // 0x0047dda0: spawn one snow particle.
    void __thiscall SpawnSnow();
    // 0x0047d590: zero the slots, no weather, temperature 0x48, season 0, clear the globals. Returns this.
    CWeather *__thiscall Ctor();
    // 0x0047d5d0: delete the live particles but leave the slots (dead slots are cleared).
    void __thiscall DestroyParticles();
    // 0x0047dc50: type = 0, delete all particles, clear the slots and the particle count.
    void __thiscall Stop();
    // 0x0047ded0: shift every particle by (dx, dy).
    void __thiscall OffsetParticles(int dx, int dy);

    CGump *slots[0x46];     // +0x000  particle gumps (null = free)
    int8_t type;            // +0x118  0 none, 1 rain, 2 snow
    int8_t intensity;       // +0x119  signed
    uint8_t pad_11a[2];
    int temperature;        // +0x11c
    int season;             // +0x120
    uint8_t pad_124[4];
    int windActive;         // +0x128
    int windTick;           // +0x12c
    int windDuration;       // +0x130
    int windOffset;         // +0x134
    int windStep;           // +0x138
    int windMax;            // +0x13c
};

static_assert(offsetof(CWeather, type) == 0x118, "CWeather::type");
static_assert(offsetof(CWeather, intensity) == 0x119, "CWeather::intensity");
static_assert(offsetof(CWeather, temperature) == 0x11c, "CWeather::temperature");
static_assert(offsetof(CWeather, season) == 0x120, "CWeather::season");
static_assert(offsetof(CWeather, windActive) == 0x128, "CWeather::windActive");
static_assert(offsetof(CWeather, windTick) == 0x12c, "CWeather::windTick");
static_assert(offsetof(CWeather, windDuration) == 0x130, "CWeather::windDuration");
static_assert(offsetof(CWeather, windOffset) == 0x134, "CWeather::windOffset");
static_assert(offsetof(CWeather, windStep) == 0x138, "CWeather::windStep");
static_assert(offsetof(CWeather, windMax) == 0x13c, "CWeather::windMax");
