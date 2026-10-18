// CWeather: constructor, particle teardown and offsetting (the packet handler and spawners are in weather.cpp).
#include "weather.h"

#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"


// Is `g` still in the top-level gump list (walk from the tail through +0x3c)?
static bool WoGumpIsLive(CGump *g)
{
    for (CGump *p = g_gumpListTail; p != nullptr; p = p->prev) {
        if (p == g)
            return true;
    }
    return false;
}

// 0x0047d590  size 0x38
CWeather *__thiscall CWeather::Ctor()
{
    memset(slots, 0, sizeof(slots));
    type = 0;
    intensity = 0;
    temperature = 0x48;
    season = 0;
    g_weatherFrame = 0;
    g_weatherParticles = 0;
    return this;
}
HOOK_METHOD(0x0047d590, CWeather, Ctor);

// 0x0047d5d0  size 0x3e
// For each used slot: if the gump is still in the list, delete it (the slot keeps the stale pointer);
// otherwise just clear the slot.
void __thiscall CWeather::DestroyParticles()
{
    for (int i = 0; i < 0x46; i++) {
        CGump *g = slots[i];
        if (g == nullptr)
            continue;
        if (!WoGumpIsLive(g)) {
            slots[i] = nullptr;
            continue;
        }
        PORT_VSLOT1(g, 0x00, void, int)(1);
    }
}
HOOK_METHOD(0x0047d5d0, CWeather, DestroyParticles);

// 0x0047dc50  size 0x4f
void __thiscall CWeather::Stop()
{
    type = 0;
    for (int i = 0; i < 0x46; i++) {
        CGump *g = slots[i];
        if (g == nullptr)
            continue;
        if (WoGumpIsLive(g))
            PORT_VSLOT1(g, 0x00, void, int)(1);
        slots[i] = nullptr;
    }
    g_weatherParticles = 0;
}
HOOK_METHOD(0x0047dc50, CWeather, Stop);

// 0x0047ded0  size 0x31
void __thiscall CWeather::OffsetParticles(int dx, int dy)
{
    for (int i = 0; i < 0x46; i++) {
        if (slots[i] != nullptr) {
            slots[i]->x += dx;
            slots[i]->y += dy;
        }
    }
}
HOOK_METHOD(0x0047ded0, CWeather, OffsetParticles);
