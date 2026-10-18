// CWeather: rain / snow particle gumps (packet handler, per-frame update, spawners).
#include "weather.h"

#include <cstdio>

#include "../runtime/ported.h"
#include "crt.h"
#include "app_support_ports.h"
#include "text_gump_ctor_ports.h"
#include "client_misc_ops_ports.h"
#include "dumb_gump.h"
#include "gump_art_ops_ports.h"
#include "batch02_helpers.h"
#include "art_row_blits_hue_ports.h"
#include "globals.h"

#define g_systemMessageObj GLOBAL_PTR(void, g_msgTarget_storage)

// random number in [0, n): rand() % n, n in ECX
#define Random(a1) (App_RandomMod(a1))
// system message (ECX = 0x006486b0): (0, 3, text)
#define AddSystemMessage(o, a1, a2, a3) PORT_THISCALL3(&CreateBarkGump, void, (o), int, int, const char *)(a1, a2, a3)
// returns 0 (stub)
#define Weather_ShowDebugInfo() (Misc_Return0())
// art-display gump ctor: (x, y, parent, w, h, 0, art name, -1)
#define ArtGump_Ctor(self, a1, a2, a3, a4, a5, a6, a7, a8) ((CWeatherParticle *)reinterpret_cast<CDumbGump *>(self)->Ctor(a1, a2, a3, a4, a5, a6, (const void *)(a7), a8))
// gump art size: ECX = art id, EDX = &width, stack = &height
#define GumpArt_GetSize(a1, a2, a3) (GumpArtSizeImpl(a1, a2, a3))
// draw gump art: ECX = row blit, EDX = art id, stack (x, y, dest, stride, destH)
#define GumpArt_Draw PORT_FUNC7(&DrawGumpArt, void, __fastcall, uintptr_t, int, int, int, int16_t *, int, int)

// Is `g` still in the top-level gump list (walk from the tail through +0x3c)?
static inline bool GumpIsLive(CGump *g)
{
    for (CGump *p = g_gumpListTail; p != nullptr; p = p->prev) {
        if (p == g)
            return true;
    }
    return false;
}

// Delete a particle through its deleting destructor (vtable slot 0, flags 1) if it is still in the list.
static inline void DeleteIfLive(CGump *g)
{
    if (GumpIsLive(g) && g != nullptr)
        PORT_VSLOT1(g, kGumpSlot_DeletingDtor, void, int)(1);
}

// Remove the first used slot: delete the gump, clear the slot, drop the live count.
static void RemoveFirstParticle(CWeather *w)
{
    int idx = 0;
    CGump **slot = &w->slots[0];
    do {
        if (*slot != nullptr)
            goto found;
        idx++;
        slot++;
    } while (idx < 0x46);
    idx = -1;
found:
    if (idx != -1) {
        DeleteIfLive(w->slots[idx]);
        w->slots[idx] = nullptr;
        g_weatherParticles = g_weatherParticles - 1;
    }
}

// 0x0047d610  size 0x2e3
// Weather packet: type 0xfe = season/temperature, 0 = clear, 1 = rain, 2 = snow, 4 = (only draws randoms);
// count is the new intensity.
void __thiscall CWeather::HandlePacket(int packetType, int count, int temp)
{
    char buf[0x80];
    int cur;
    int diff;
    int n;

    g_weatherFrame = 0;
    if (packetType == 0xfe) {
        temperature = temp;
        season = count;
        if (Weather_ShowDebugInfo() == 0)
            return;
        sprintf(buf, "Season: %d   Temperature: %d", season, temperature);
        AddSystemMessage(g_systemMessageObj, 0, 3, buf);
        return;
    }

    if (type == 0) {
        if (packetType == 1) {
            AddSystemMessage(g_systemMessageObj, 0, 3, "It begins to rain.");
            type = 1;
            intensity = (int8_t)count;
            if (count > 0) {
                n = count;
                do {
                    SpawnRain();
                    n--;
                } while (n != 0);
            }
            return;
        }
        if (packetType == 2) {
            AddSystemMessage(g_systemMessageObj, 0, 3, "It begins to snow.");
            type = 2;
            intensity = (int8_t)count;
            if (count > 0) {
                n = count;
                do {
                    SpawnSnow();
                    n--;
                } while (n != 0);
            }
            return;
        }
        return;
    }

    if (packetType == 4) {
        Random(0x1e0);
        Random(0x280);
        return;
    }

    if (packetType == 0) {
        type = 0;
        CGump **slot = &slots[0];
        n = 0x46;
        do {
            CGump *g = *slot;
            if (g != nullptr) {
                DeleteIfLive(g);
                *slot = nullptr;
            }
            slot++;
            n--;
        } while (n != 0);
        g_weatherParticles = 0;
    }

    cur = (int)intensity;
    if (count > cur) {
        diff = count - cur;
        if (packetType == 1) {
            if (diff > 0) {
                n = diff;
                do {
                    SpawnRain();
                    n--;
                } while (n != 0);
                intensity = (int8_t)count;
                return;
            }
        } else if (diff > 0) {
            // snow for every non-rain type, including type 0 after the clear above
            n = diff;
            do {
                SpawnSnow();
                n--;
            } while (n != 0);
            intensity = (int8_t)count;
            return;
        }
    } else if (count < cur) {
        diff = cur - count;
        if (packetType == 1) {
            if (diff > 0) {
                n = diff;
                do {
                    RemoveFirstParticle(this);
                    n--;
                } while (n != 0);
                intensity = (int8_t)count;
                return;
            }
        } else if (diff > 0) {
            n = diff;
            do {
                RemoveFirstParticle(this);
                n--;
            } while (n != 0);
        }
    }
    intensity = (int8_t)count;
}
HOOK_METHOD(0x0047d610, CWeather, HandlePacket);

// 0x0047d900  size 0x34f
// Per-frame update: keep the particle count at the intensity, decay the intensity after 0x168 frames,
// move each particle (wind gusts for snow) and respawn those whose life ran out.
void __thiscall CWeather::Update()
{
    int idx;
    CGump **slot;
    CGump **edi;
    int n;
    int tick;
    int dur;

    if (type == 0)
        return;

    if (g_weatherParticles < (int)intensity)
        SpawnSnow();   // ? called for rain as well; the original does not test the type here

    if (g_weatherParticles > (int)intensity) {
        idx = 0;
        slot = &slots[0];
        do {
            if (*slot != nullptr)
                goto found_free;
            idx++;
            slot++;
        } while (idx < 0x46);
        idx = -1;
    found_free:
        if (idx != -1) {
            DeleteIfLive(slots[idx]);
            slots[idx] = nullptr;
            g_weatherParticles = g_weatherParticles - 1;
        }
    }

    g_weatherFrame = g_weatherFrame + 1;
    if (g_weatherFrame > 0x168) {
        intensity = intensity - 2;
        if (intensity < 0)
            intensity = 0;
    }

    edi = &slots[0];
    n = 0x46;
    do {
        if (*edi != nullptr) {
            if (!GumpIsLive(*edi)) {
                *edi = nullptr;
                goto next_slot;
            }

            if (type == 2) {
                if (Random(0x61a8) == 10) {
                    windActive = 1;
                    windMax = Random(10);
                    windOffset = 0;
                    windStep = 2;
                    windTick = 0;
                    windDuration = Random(0x1f4) + 0x3a98;
                }
            }

            if (windActive != 0) {
                tick = windTick + 1;
                windTick = tick;
                if (tick % 100 == 0) {
                    windOffset = windOffset + windStep;
                    if (windOffset > windMax)
                        windOffset = windMax;
                    if (windOffset < 0)
                        windOffset = 0;
                }
                dur = windDuration;
                if (tick > (dur >> 1)) {
                    if (windStep > 0)
                        windStep = -windStep;
                }
                if (tick > dur) {
                    windActive = 0;
                    windTick = 0;
                    windDuration = 0;
                }
            } else {
                windStep = 0;
                windOffset = 0;
            }

            CWeatherParticle *g = static_cast<CWeatherParticle *>(*edi);
            g->life = g->life - 1;
            g = static_cast<CWeatherParticle *>(*edi);
            g->x = g->x - g->velX;
            g = static_cast<CWeatherParticle *>(*edi);
            g->y = g->y + g->velY;
            g = static_cast<CWeatherParticle *>(*edi);
            g->x = g->x - windOffset;

            if (type == 2) {
                if (Random(10) == 2) {
                    g = static_cast<CWeatherParticle *>(*edi);
                    g->velX = g->velX + (Random(2) - 1);
                }
            }

            g = static_cast<CWeatherParticle *>(*edi);
            if (g->life < 1) {
                g->velX = Random(4) + 2;
                g = static_cast<CWeatherParticle *>(*edi);
                g->velY = Random(4) + 2;
                g = static_cast<CWeatherParticle *>(*edi);
                g->x = Random(0x280);
                g = static_cast<CWeatherParticle *>(*edi);
                g->y = Random(0x1e0);
                if (type == 1) {
                    g = static_cast<CWeatherParticle *>(*edi);
                    g->life = Random(10) + 10;
                }
                if (type == 2) {
                    g = static_cast<CWeatherParticle *>(*edi);
                    g->life = Random(0x28) + 0x28;
                    g = static_cast<CWeatherParticle *>(*edi);
                    int16_t *bmp = *(int16_t **)(g->frameData + 1);   // frameData[1] = bitmap of slot 0 (width, height, pixels)
                    // ECX = row blit 0x004761e0, EDX = art id 0x1770
                    GumpArt_Draw(PORT_ADDR(&RowBlitCopyAll), 0x1770, 0, 0, bmp + 2, (uint16_t)bmp[0], (uint16_t)bmp[1]);
                }
            }
        }
    next_slot:
        edi++;
        n--;
    } while (n != 0);
}
HOOK_METHOD(0x0047d900, CWeather, Update);

// 0x0047dca0  size 0xf5
// SEH frame (handler 0x0049b9fb) around new/ctor omitted. Spawn one rain particle in the first free slot.
void __thiscall CWeather::SpawnRain()
{
    int idx;
    CGump **slot;
    int x;
    int y;
    void *mem;
    CWeatherParticle *gump;

    if (intensity > 0x46)
        return;

    idx = 0;
    slot = &slots[0];
    do {
        if (*slot == nullptr)
            goto found;
        idx++;
        slot++;
    } while (idx < 0x46);
    idx = -1;
found:
    if (idx == -1)
        return;

    x = Random(0x280);
    y = Random(0x1e0);
    gump = (CWeatherParticle *)new CDumbGump(x, y, 0, 3, 3, 0, (const void *)(g_weatherRainArt), -1);
    slots[idx] = gump;
    gump->velX = 0x10;
    static_cast<CWeatherParticle *>(slots[idx])->velY = 0x10;
    static_cast<CWeatherParticle *>(slots[idx])->life = Random(10) + 10;
    CGump_AddChild(slots[idx], nullptr, 1);
    g_weatherParticles = g_weatherParticles + 1;
}
HOOK_METHOD(0x0047dca0, CWeather, SpawnRain);

// 0x0047dda0  size 0x129
// SEH frame (handler 0x0049ba1b) around new/ctor omitted. Spawn one snow particle (only if art 0x1771 exists).
void __thiscall CWeather::SpawnSnow()
{
    int idx;
    CGump **slot;
    int x;
    int y;
    int artW = 0;
    int artH = 0;
    void *mem;
    CWeatherParticle *gump;

    if (intensity > 0x46)
        return;

    idx = 0;
    slot = &slots[0];
    do {
        if (*slot == nullptr)
            goto found;
        idx++;
        slot++;
    } while (idx < 0x46);
    idx = -1;
found:
    if (idx == -1)
        return;

    x = Random(0x280);
    y = Random(0x1e0);
    GumpArt_GetSize(0x1771, &artW, &artH);
    if (artW == 0)
        return;
    gump = (CWeatherParticle *)new CDumbGump(x, y, 0, 3, 3, 0, (const void *)(g_weatherSnowArt), -1);
    slots[idx] = gump;
    static_cast<CWeatherParticle *>(slots[idx])->velX = Random(4) + 2;
    static_cast<CWeatherParticle *>(slots[idx])->velY = Random(4) + 2;
    static_cast<CWeatherParticle *>(slots[idx])->life = Random(0x28) + 0x28;
    CGump_AddChild(slots[idx], nullptr, 1);
    g_weatherParticles = g_weatherParticles + 1;
}
HOOK_METHOD(0x0047dda0, CWeather, SpawnSnow);
