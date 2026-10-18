// CMobile and CPlayer operations: status-driven actions, animation start, step tables, the mobile animation
// selector, player constructor / destructor and the scalar deleting destructors.
// 0x00454060 .. 0x00454c30 (porting pass 2, batch 20). The SEH frames of the original are omitted.
#include "obj.h"
#include "player.h"

#include "../runtime/ported.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "world_obj_ops.h"
#include "effect_sequence.h"
#include "mobile_anim_queue.h"
#include "vtables.h"


#define World_Invalidate(self) (reinterpret_cast<CWorldObj *>(self)->Invalidate())                    // ECX = object (CWorldObj::Invalidate)
#define EffectSeq_Append(self, a1, a2, a3) (reinterpret_cast<CEffectSequence *>(self)->Append(a1, a2, a3))   // ECX = sequence, type, data, size
#define MobileAnim_Register(self) (reinterpret_cast<CMobileAnimQueue *>(self)->RegisterActiveAnim())                 // CMobileAnimQueue::RegisterActiveAnim
#define MobileAnim_Finish(self) (reinterpret_cast<CMobileAnimQueue *>(self)->FinishAnim())                    // CMobileAnimQueue::FinishAnim

// 0x00454410  size 0xb0
// Marks the serial (bit 31), queues {queue, serial, facing, alt} as a pending update when `queue` is nonzero
// (0x00452490), then starts the action chosen by the body id (graphic): below 0x96 action 2 / 3, below 0xc8
// action 8 with 0xf frames, below 0x190 action 8 / 0xc, otherwise 0x15 / 0x16 (the first of the pair when
// `alt` is nonzero). RET 8.
void __thiscall CMobile::StartStatusAction(int queue, int alt)
{
    serial = static_cast<int32_t>(static_cast<uint32_t>(serial) | 0x80000000u);
    if (queue != 0)
        PendingUpdate_Add(static_cast<uint32_t>(queue), static_cast<uint32_t>(serial), facing, static_cast<uint32_t>(alt));   // 0x00452490
    uint16_t body = graphic;
    if (body < 0x96) {
        StartAction(alt != 0 ? 2 : 3, 5, 0, 1, 0, 0);   // 0x004545c0
        return;
    }
    if (body < 0xc8) {
        StartAction(8, 0xf, 0, 1, 0, 0);   // 0x004545c0
        return;
    }
    if (body < 0x190) {
        StartAction(alt != 0 ? 8 : 0xc, 5, 0, 1, 0, 0);   // 0x004545c0
        return;
    }
    StartAction(alt != 0 ? 0x15 : 0x16, 5, 0, 1, 0, 0);   // 0x004545c0
}
HOOK_METHOD(0x00454410, CMobile, StartStatusAction);

// 0x004544c0  size 0x2b
// +0x1d8 divided by 24, truncated toward zero (the original multiplies by the reciprocal and fixes the sign).
int __thiscall CMobile::Div24()
{
    return unk_1d8 / 24;
}
HOOK_METHOD(0x004544c0, CMobile, Div24);

// 0x004544f0  size 0x15
void __thiscall CMobile::SetUnk1d8(int v)
{
    unk_1d8 = v;
    World_Invalidate(this);   // 0x0042bcf0
}
HOOK_VMETHOD(0x004544f0, CMobile, SetUnk1d8);

// 0x00454510  size 0x2d
void __thiscall CPlayer::SetUnk1d8(int v)
{
    unk_1d8 = v;
    for (CMobile *m = g_mobileListHead; m != nullptr; m = m->listNext)
        World_Invalidate(m);   // 0x0042bcf0
}
HOOK_VMETHOD(0x00454510, CPlayer, SetUnk1d8);

// 0x00454540  size 0x5e
// Frame step of the body from the table at 0x004bab38 (two signed bytes per graphic: standing / walking, then
// moving). While climbing (+0x198) the walk byte plus twice the quotient of +0x19c by 11 (the multiply by
// 0x2e8ba2e9 is a division by 11).
int __thiscall CMobile::StepFrames()
{
    if (zMoving != 0)
        return g_stepFrameTable[graphic * 2] + (zDelta / 11) * 2;
    if (moving != 0)
        return g_stepFrameTable[graphic * 2 + 1];
    return g_stepFrameTable[graphic * 2];
}
HOOK_METHOD(0x00454540, CMobile, StepFrames);

// 0x004545c0  size 0x11b
// While the effect sequence is recording (DAT_007211b0) the six words, behind the mobile pointer, are appended to
// it as a type 7 node of 0x10 bytes. Otherwise the action is started on the mobile itself: +0x14c gets the game
// tick, the mobile leaves the world and re-enters after +0x1a4 = 1, the action (+0x1ac), frame count (+0x1aa),
// repeat flags and sequence state are set and the mobile joins the active animations (0x00430920).
void __thiscall CMobile::StartAction(int p1, int p2, int p3, int p4, int p5, int p6)
{
    if (g_sequencing != 0) {
        uint8_t *node = static_cast<uint8_t *>(::operator new(0x10));
        *reinterpret_cast<CMobile **>(node) = this;
        uint16_t *w = reinterpret_cast<uint16_t *>(node + 4);
        *w++ = static_cast<uint16_t>(p1);
        *w++ = static_cast<uint16_t>(p2);
        *w++ = static_cast<uint16_t>(p3);
        *w++ = static_cast<uint16_t>(p4);
        *w++ = static_cast<uint16_t>(p5);
        *w = static_cast<uint16_t>(p6);
        EffectSeq_Append(GLOBAL_AS(void *, g_effectSequence), 7, node, 0x10);   // 0x0043e020
        operator delete(node);
        return;
    }
    unk_14c = static_cast<int32_t>(g_gameTick);
    decomp::vcall_void(this, 0x14);
    unk_1ac = static_cast<uint16_t>(p1);
    unk_1a4 = 1;
    if (static_cast<uint16_t>(p5) != 0) {
        unk_1a8 = static_cast<uint16_t>(p2 - 1);
        unk_1b8 = static_cast<uint16_t>(p4 - 1);
    } else {
        unk_1a8 = 0;
        unk_1b8 = 0;
    }
    RAWF(uint16_t, this, 0x1b6) = static_cast<uint16_t>(p4);
    RAWF(uint16_t, this, 0x1b2) = static_cast<uint16_t>(p5);
    RAWF(uint16_t, this, 0x1b0) = static_cast<uint16_t>(p3);
    RAWF(uint16_t, this, 0x1b4) = static_cast<uint16_t>(p6);
    RAWF(uint16_t, this, 0x1ae) = 0;
    animFrame = static_cast<uint16_t>(p2);
    MobileAnim_Register(this);   // 0x00430920
    decomp::vcall_void(this, 0x10);
}
HOOK_METHOD(0x004545c0, CMobile, StartAction);

// 0x004546e0  size 0x1c
// ECX = mobile. When an animation is active (+0x1a4) clear it and finish it through 0x00430970 (tail jump,
// its result is returned); otherwise 0.
int __thiscall CMobile::EndAnim()
{
    if (unk_1a4 != 0) {
        unk_1a4 = 0;
        return MobileAnim_Finish(this);   // 0x00430970
    }
    return 0;
}
HOOK_METHOD(0x004546e0, CMobile, EndAnim);

// 0x004548e0  size 0x130
// ECX = body id (low word), EDX unused, ten stack words (RET 0x28). Picks the animation id and frame:
// bodies below 0xc8 use 0x13 for a handful of ids when `a18` is set, else 0 (ten frames); bodies below 0x190
// use 1 / 0 (five frames); larger bodies use 7 (one frame), 0x17 / 0x18 or 4 / 6 / 0 / 2 (ten frames) by a18,
// a1c, a20 and a24. With a fixed frame `ac` of -1 the output frame is a8 modulo (frames - 1) (0 for one frame) and
// the animation is the selected one; otherwise the animation is a14 and the frame is `ac`. Returns EAX as the
// original leaves it (the quotient, or the address of the frame output).
int __fastcall GetMobileAnimation(uint32_t graphic, uint32_t edxUnused, int a8, int ac, int a10, int a14, int a18,
                                  int a1c, int a20, int a24, int *animOut, int *frameOut)
{
    (void)edxUnused;
    (void)a10;
    uint16_t body = static_cast<uint16_t>(graphic);
    int anim;
    int frames;

    if (body < 0xc8) {
        if (a18 != 0) {
            if (body == 4 || body == 5 || body == 6 || body == 9 || body == 0xa || body == 0xc || body == 0x1e ||
                body == 0x27 || body == 0x3b || body == 0x3c || body == 0x3d)
                anim = 0x13;
            else
                anim = 0;
        } else {
            anim = 0;
        }
        frames = 0xa;
    } else if (body < 0x190) {
        if (a18 != 0) {
            anim = 1;
            frames = 5;
        } else {
            anim = 0;
            frames = 5;
        }
    } else {
        if (a20 != 0 && a1c == 0) {
            anim = 7;
            frames = 1;
        } else if (a20 != 0 || a1c != 0) {
            if (a18 == 0) {
                anim = 0x17;
                frames = 5;
            } else {
                anim = 0x18;
                frames = 5;
            }
        } else {
            if (a18 != 0)
                anim = (a24 != 1) ? 4 : 6;
            else
                anim = (a24 != 1) ? 0 : 2;
            frames = 0xa;
        }
    }

    if (ac == -1) {
        *animOut = anim;
        if (frames == 1) {
            *frameOut = 0;
            return static_cast<int>(reinterpret_cast<uintptr_t>(frameOut));
        }
        *frameOut = a8 % (frames - 1);
        return a8 / (frames - 1);
    }
    *animOut = a14;
    *frameOut = ac;
    return static_cast<int>(reinterpret_cast<uintptr_t>(frameOut));
}
HOOK_FUNC(0x004548e0, GetMobileAnimation);

// 0x00454c30  size 0x81
// Player constructor: the default mobile constructor (0x00453ef0), then +0x1dc..+0x208 and +0x150 cleared,
// the serial, vtable 0x0049faf8, graphic and the byte at +0x3a. RET 0xc.
CPlayer::CPlayer(int ser, int gfx, uint8_t b3a)
{
    RAWF(int32_t, this, 0x1dc) = 0;
    RAWF(int32_t, this, 0x1e0) = 0;
    RAWF(int32_t, this, 0x1e4) = 0;
    RAWF(int32_t, this, 0x1e8) = 0;
    RAWF(int32_t, this, 0x1ec) = 0;
    RAWF(int32_t, this, 0x1f0) = 0;
    RAWF(int32_t, this, 0x1f4) = 0;
    RAWF(int32_t, this, 0x1f8) = 0;
    RAWF(int32_t, this, 0x1fc) = 0;
    RAWF(int32_t, this, 0x200) = 0;
    RAWF(int32_t, this, 0x204) = 0;
    RAWF(int32_t, this, 0x208) = 0;
    unk_150 = 0;
    serial = ser;
    graphic = static_cast<uint16_t>(gfx & 0x3fff);
    unk_3a = b3a;
}
HOOK_CTOR(0x00454c30, CPlayer, (int, int, uint8_t));

// 0x00454cc0  size 0x15
// Vtable, then the global player pointer is cleared and the mobile destructor runs (tail jump).
CPlayer::~CPlayer()
{
    g_player = nullptr;
}
HOOK_DTOR(0x00454cc0, CPlayer);
HOOK_VTABLE(0x0049faf8, CPlayer);
HOOK_SDD(0x00454c00, CPlayer);
