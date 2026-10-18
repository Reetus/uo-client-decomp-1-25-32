// Active mobile animations: registration, stepping and finishing (0x00430900 .. 0x00430be0).
#include "mobile_anim_queue.h"

#include <cstring>

#include "../runtime/ported.h"
#include "missile_update.h"
#include "raw_field.h"
#include "vcall.h"
#include "effect_sequence.h"
#include "obj.h"
#include "globals.h"


// 0x0043e730: release the attached effect object (ECX = object).
#define ReleaseEffect(self) (reinterpret_cast<CEffectSequence *>(self)->Advance())
// 0x00452330: ECX = 0, EDX = serial, stack = 0, RET 4.
#define ObjectAnimDone(a1, a2, a3) (PendingUpdate_Apply((uint32_t)(a1), (uint32_t)(a2), a3))

// 0x00430900  size 0x11
// Zero the 20 slots.
void __cdecl MobileAnim_ClearActive()
{
    memset(g_activeAnim, 0, 20 * sizeof(CMobileAnimQueue *));
}
HOOK_FUNC(0x00430900, MobileAnim_ClearActive);

// 0x00430920  size 0x45
void __thiscall CMobileAnimQueue::RegisterActiveAnim()
{
    if (unk_1d0 == -1) {
        int slot = 0;
        for (CMobileAnimQueue **p = &g_activeAnim[0]; reinterpret_cast<intptr_t>(p) < reinterpret_cast<intptr_t>(&g_activeAnim[20]); p++) {
            if (*p == nullptr) {
                g_activeAnim[slot] = this;
                unk_1d0 = slot;
                g_animDirtyFlag = 0;
                return;
            }
            slot++;
        }
    }
    g_animDirtyFlag = 0;
}
HOOK_METHOD(0x00430920, CMobileAnimQueue, RegisterActiveAnim);

// 0x00430970  size 0xf1
// +0x1ac is the current animation action. The "final" actions per body range are
// bodies < 0x96: 2, 3; < 0xc8: 8; < 0x190: 8, 0xc; others: 0x15, 0x16.
int __thiscall CMobileAnimQueue::FinishAnim()
{
    int isFinal = 0;

    if (graphic < 0x96) {
        if (unk_1ac == 2 || unk_1ac == 3)
            isFinal = 1;
    } else if (graphic < 0xc8) {
        if (unk_1ac == 8)
            isFinal = 1;
    } else if (graphic < 0x190) {
        if (unk_1ac == 8 || unk_1ac == 0xc)
            isFinal = 1;
    } else {
        if (unk_1ac == 0x15 || unk_1ac == 0x16)
            isFinal = 1;
    }

    int slot = unk_1d0;
    if (slot != -1) {
        if (inWorld != 0) {
            decomp::vcall_void(this, 0x14);
            unk_1a4 = 0;
            decomp::vcall_void(this, 0x10);
        } else {
            unk_1a4 = 0;
        }
        if (unk_1c4 != 0) {
            ReleaseEffect(reinterpret_cast<void *>(static_cast<uintptr_t>(static_cast<uint32_t>(unk_1c4))));
            unk_1c4 = 0;
        }
        g_activeAnim[slot] = nullptr;
        unk_1d0 = -1;
    }
    if (isFinal != 0) {
        ObjectAnimDone(0, serial, 0);
        return 1;
    }
    g_animDirtyFlag = 1;
    return 0;
}
HOOK_METHOD(0x00430970, CMobileAnimQueue, FinishAnim);

// 0x00430a70  size 0x16c
// Words: +0x1a8 frame, +0x1aa frame count, +0x1ae tick counter, +0x1b2 backwards flag,
// +0x1b4 repeat (ping-pong) flag, +0x1b6 sequence count, +0x1b8 sequence index.
// Slot 0x14 / 0x10 around each change are the unlink / relink of the object in the world.
void __thiscall CMobileAnimQueue::AnimStep()
{
    uint16_t &frame = RAWF(uint16_t, this, 0x1a8);
    uint16_t &frameCount = RAWF(uint16_t, this, 0x1aa);
    uint16_t &tick = RAWF(uint16_t, this, 0x1ae);
    uint16_t &backwards = RAWF(uint16_t, this, 0x1b2);
    uint16_t &repeat = RAWF(uint16_t, this, 0x1b4);
    uint16_t &seqCount = RAWF(uint16_t, this, 0x1b6);
    uint16_t &seq = RAWF(uint16_t, this, 0x1b8);

    if (backwards != 0) {
        if (frame != 0 && frame <= frameCount) {
            decomp::vcall_void(this, 0x14);
            frame--;
            tick = 0;
            decomp::vcall_void(this, 0x10);
            return;
        }
        if (seq != 0) {
            decomp::vcall_void(this, 0x14);
            uint16_t cnt = frameCount;
            seq--;
            frame = static_cast<uint16_t>(cnt - 1);
            tick = 0;
            decomp::vcall_void(this, 0x10);
            return;
        }
        if (repeat != 0) {
            backwards = 0;
            repeat = 0;
            decomp::vcall_void(this, 0x14);
            frame++;
            tick = 0;
            decomp::vcall_void(this, 0x10);
            return;
        }
        FinishAnim();   // 0x00430970
        return;
    }

    if (static_cast<int>(frame) < static_cast<int>(frameCount) - 1) {
        decomp::vcall_void(this, 0x14);
        frame++;
        tick = 0;
        decomp::vcall_void(this, 0x10);
        return;
    }
    if (static_cast<int>(seq) != static_cast<int>(seqCount) - 1) {
        decomp::vcall_void(this, 0x14);
        seq++;
        frame = 0;
        tick = 0;
        decomp::vcall_void(this, 0x10);
        return;
    }
    if (repeat != 0) {
        backwards = 1;
        repeat = 0;
        decomp::vcall_void(this, 0x14);
        frame--;
        tick = 0;
        decomp::vcall_void(this, 0x10);
        return;
    }
    FinishAnim();   // 0x00430970
}
HOOK_METHOD(0x00430a70, CMobileAnimQueue, AnimStep);

// 0x00430be0  size 0x5d
// ? Calling convention not annotated (no arguments, plain ret).
void __cdecl MobileAnim_TickAll()
{
    for (int i = 0; i < 20; i++) {
        CMobileAnimQueue *m = g_activeAnim[i];
        if (m != nullptr) {
            g_animDirtyFlag = 0;
            uint16_t before = RAWF(uint16_t, m, 0x1ae);
            RAWF(uint16_t, m, 0x1ae) = static_cast<uint16_t>(before + 1);
            if (before > RAWF(uint16_t, m, 0x1b0))
                m->AnimStep();   // 0x00430a70
        }
    }
    for (int i = 0; i < 20; i++) {
        if (g_missileTable[i] != nullptr)
            g_missileTable[i]->Update();   // 0x00430c60
    }
}
HOOK_FUNC(0x00430be0, MobileAnim_TickAll);
