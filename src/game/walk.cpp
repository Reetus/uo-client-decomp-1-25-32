#include "walk.h"

#include <cstring>

#include "../runtime/ported.h"
#include "pos3.h"
#include "globals.h"
#include "obj.h"
#include "vcall.h"
#include "world_obj_ops.h"
#include "app_support_ports.h"
#include "sound_ops_ports.h"
#include "walk_queue_ports.h"
#include "dirty_rects_ports.h"
#include "gump_manager.h"
#include "world_renderer.h"
#include "weather.h"

// Fall / z-step state shared with the camera code (Ghidra addresses).
#define g_mobileSeed GLOBAL_AS(int, g_gameTick)
#define g_scrollView GLOBAL_PTR(void, g_weatherObj_storage)
#define g_dirtyOwner GLOBAL_PTR(void, g_worldPicker_storage)

// 0x00449490  size 0x446
// Per-tick walk animation step. Advances the frame counter of the current queue entry; when the step
// completes (frame count reached or the facing changes) the player is moved one tile (or only turned),
// the entry is removed if acknowledged or the queue advances, and the screen scroll delta is applied.
// The SEH frame is omitted.
void __thiscall CWalkManager::ProcessStep()
{
    CMobile *player = static_cast<CMobile *>(g_player);
    if (player == nullptr)
        return;
    if (g_suppressSpeech != 0)
        return;

    if (count != 0 && Entry(cur)->drops != 0) {
        g_dropping = 1;
        g_dropZ = player->z;
        g_dropTargetZ = Entry(cur)->z;
    } else {
        g_dropping = 0;
    }

    if (player->inWorld != 0) {
        bool callVt10 = false;
        if (count == 0) {
            if (player->flag148 == 0) {
                decomp::vcall_void(player, 0x14);
                static_cast<CMobile *>(g_player)->flag148 = 1;
                g_walkTick = 3;
                callVt10 = true;
            }
        } else {
            if (player->flag148 != 0) {
                decomp::vcall_void(player, 0x14);
                static_cast<CMobile *>(g_player)->flag148 = 0;
                callVt10 = true;
            }
        }
        if (callVt10) {
            decomp::vcall_void(g_player, 0x10);
            player = static_cast<CMobile *>(g_player);
        }
    }

    Pos3 oldPos = *reinterpret_cast<Pos3 *>(&player->x);

    if (cur >= count)
        return;
    if (requireAck != 0 && Entry(cur)->acked == 0)
        return;

    PORT_FUNC1(&CMobile::EndAnim, void, __fastcall, CMobile *)(player);   // ECX = player
    PORT_FUNC1(&CWorldObj::InvalidateEquipment, void, __fastcall, CObj *)(g_player);
    decomp::vcall_void(g_player, 0x14);
    static_cast<CMobile *>(g_player)->unk_14c = g_mobileSeed;

    int prevScrollY = g_scrollY;
    int curIdx = cur;
    int prevScrollX = g_scrollX;
    total = total + 1;
    frame = frame + 1;
    unsigned int dir = Entry(curIdx)->dir;
    player = static_cast<CMobile *>(g_player);

    int frames;
    if (player->equip[25] != nullptr) {   // +0x128: mounted
        if (dirState != 0) {
            frames = g_stepFrameTable[player->graphic * 2 + 1];
            frames = frames / 2;
        } else {
            frames = g_stepFrameTable[player->graphic * 2];
            frames = frames / 2;
        }
        wq_stepFrames = frames;
    } else if (g_dropping != 0) {
        int baseFrames = g_stepFrameTable[player->graphic * 2];
        int drop = (g_dropZ - g_dropTargetZ) / 11;
        wq_stepFrames = baseFrames + drop * 2;
        player = static_cast<CMobile *>(g_player);
        frames = wq_stepFrames;
    } else if (dirState != 0) {
        wq_stepFrames = frames = g_stepFrameTable[player->graphic * 2 + 1];
    } else {
        wq_stepFrames = frames = g_stepFrameTable[player->graphic * 2];
    }

    if (frame == frames || dir != Entry(cur)->nextDir) {
        if (g_dropping != 0) {
            if (decomp::vcall(PORT_RET(int), player, 0x8c) == 0 && g_dropZ - g_dropTargetZ >= 0x16) {
                g_player->CreateBarkKeepTwo("ouch!", g_colorSay, g_colorSayFont, nullptr);
                int r = App_RandomMod(5);   // random number in [0, 5)
                unsigned int sel = (g_player->graphic & 1) + r * 2;
                PlaySoundEffectIfEnabled(g_stepSounds[sel], 0, 0);
            }
            g_dropping = 0;
        }
        frame = 0;
        dirState = Entry(cur)->dirState;
        g_player->facing = static_cast<uint8_t>(dir);
        if (dir == Entry(cur)->nextDir) {
            CObj *pl = g_player;
            pl->x = static_cast<int16_t>(pl->x + *reinterpret_cast<int16_t *>(GLOBAL_PTR(uint8_t, g_dirDx) + dir * 4));
            pl = g_player;
            pl->y = static_cast<int16_t>(pl->y + *reinterpret_cast<int16_t *>(GLOBAL_PTR(uint8_t, g_dirDy) + dir * 4));
            g_player->z = static_cast<int16_t>(Entry(cur)->z);
        } else {
            g_player->facing = static_cast<uint8_t>(dir);
        }
        int idx = cur;
        if (Entry(idx)->acked != 0) {
            // acked: remove the entry, shifting the tail down by 5 dwords per entry
            int j = idx + 1;
            if (j < count) {
                WalkEntry *dst = Entry(j - 1);
                do {
                    memcpy(dst, dst + 1, sizeof(WalkEntry));
                    j++;
                    dst++;
                } while (j < count);
            }
            count = count - 1;
        } else {
            cur = idx + 1;
        }
    }
    Walk_ScrollOffset(static_cast<int>(dir), frame);
    decomp::vcall_void(g_player, 0x88);
    int dyS = prevScrollY - g_scrollY;
    int dxS = prevScrollX - g_scrollX;
    decomp::thiscall_at_void(PORT_ADDR(&CWeather::OffsetParticles), g_scrollView, dxS, dyS);
    decomp::thiscall_at_void(PORT_ADDR(&CGumpManager::ShiftMissileGumps), g_gumpManager, dxS, dyS);
    if (dxS != 0 || dyS != 0) {
        int adx = dxS < 0 ? -dxS : dxS;
        int ady = dyS < 0 ? -dyS : dyS;
        if (adx < 0x280 && ady < 0x140)
            ScrollBackBuffer(dxS, dyS);
        else
            PORT_FUNC0(&InvalidateAll, void, __cdecl)();
        PORT_FUNC1(&CGumpManager::RepositionBarks, void, __fastcall, void *)(g_gumpManager);
    }
    decomp::vcall_void(g_player, 0x10);
    player = static_cast<CMobile *>(g_player);
    if (oldPos.x != player->x || oldPos.y != player->y || oldPos.z != player->z)
        PORT_FUNC1(&CWorldRenderer::UpdateRoofHideLimit, void, __fastcall, void *)(g_dirtyOwner);
}
HOOK_METHOD(0x00449490, CWalkManager, ProcessStep);
