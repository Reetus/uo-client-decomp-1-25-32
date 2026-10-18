// CEffectSequence::Step. gumps.cxx, 0x0043e0a0.
#include "effect_sequence.h"

#include <cstring>

#include "../runtime/ported.h"
#include "gump.h"
#include "missile_gump.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "sound_ops_ports.h"
#include "mobile_anim_queue.h"
#include "globals.h"

// Head of the list of live objects, linked through +0xbc. A record whose
// target is not on this list is skipped.
#define g_liveObjects GLOBAL_AS(uint8_t *, g_mobileListHead)
// The sound manager object (0x0068db30) and its play-at-position method.
#define g_soundManagerObj GLOBAL_PTR(void, g_soundManager_storage)

// Packed record readers: the record fields sit at unaligned offsets. RD(int16_t, d, off) reads a T; the type
// is given through a null T * because Visual C++ 5.0 accepts no explicit function-template argument.
template <class T>
static T rd_(const uint8_t *d, unsigned off, T *)
{
    T v;
    memcpy(&v, d + off, sizeof(T));
    return v;
}
#define RD(T, d, off) rd_((d), (off), static_cast<T *>(0))

static Pos rd_pos(const uint8_t *d, unsigned off)
{
    Pos p;
    p.x = RD(int16_t, d, off);
    p.y = RD(int16_t, d, off + 2);
    p.z = RD(int16_t, d, off + 4);
    p.pad = 0;
    return p;
}

static bool is_alive(const void *target)
{
    for (uint8_t *seq = g_liveObjects; seq != nullptr; seq = RAWF(uint8_t *, seq, 0xbc)) {
        if (seq == target)
            return true;
    }
    return false;
}

// 0x0043e0a0  size 0x65f
// SEH frame handler 0x0049a565 omitted. The three copies of the "advance"
// block (0x0043e448, 0x0043e5a1, 0x0043e634) are one helper here.
// The "advance" block the original repeats inline at 0x0043e448, 0x0043e5a1 and 0x0043e634 (it does not call
// CEffectSequence::Advance): next node, else restart from the head while the repeat count lasts, else delete self.
static void AdvanceInline(CEffectSequence *seq)
{
    SeqNode *nxt = seq->current->next;
    if (nxt != nullptr) {
        seq->current = nxt;
        seq->Step(nxt);
        return;
    }
    seq->repeat = (uint8_t)(seq->repeat - 1);
    if (seq->repeat != 0) {
        seq->current = seq->head;
        seq->Step(seq->head);
        return;
    }
    PORT_VSLOT1(seq, 0x00, void, int)(1);   // deleting dtor, vtable slot 0
}

void __thiscall CEffectSequence::Step(SeqNode *node)
{
    int type = node->type;
    const uint8_t *d = node->data;
    CMissileGump *obj = nullptr;

    if ((unsigned int)type > 8)
        return;

    switch (type) {
    case 0: {
        obj = new CMissileGump(rd_pos(d, 0), RD(uint16_t, d, 6), (int8_t)d[8], d[9], (int8_t)d[10], (int8_t)d[11]);
        break;
    }

    case 1: {
        CObj *target = RD(CObj *, d, 0);
        if (!is_alive(target)) {
            AdvanceInline(this);
            return;
        }
        obj = new CMissileGump(target, RD(uint16_t, d, 4), d[6], d[7], d[8], d[9]);
        break;
    }

    case 2: {
        CObj *target = RD(CObj *, d, 0);
        if (!is_alive(target)) {
            AdvanceInline(this);
            return;
        }
        obj = new CMissileGump(target);
        break;
    }

    case 3: {
        obj = new CMissileGump(rd_pos(d, 2), rd_pos(d, 8), RD(uint16_t, d, 0), d[14], d[15], d[16]);
        break;
    }

    case 4: {
        CObj *target = RD(CObj *, d, 8);
        if (!is_alive(target)) {
            AdvanceInline(this);
            return;
        }
        obj = new CMissileGump(rd_pos(d, 2), target, RD(uint16_t, d, 0), d[12], d[13], d[14]);
        break;
    }

    case 5: {
        CObj *target = RD(CObj *, d, 2);
        if (!is_alive(target)) {
            AdvanceInline(this);
            return;
        }
        obj = new CMissileGump(target, rd_pos(d, 6), RD(uint16_t, d, 0), d[12], d[13], d[14]);
        break;
    }

    case 6: {
        CObj *target = RD(CObj *, d, 2);    // ESI
        CObj *target2 = RD(CObj *, d, 6);   // EBX
        if (!is_alive(target)) {
            AdvanceInline(this);
            return;
        }
        if (!is_alive(target2)) {
            AdvanceInline(this);
            return;
        }
        obj = new CMissileGump(target, target2, RD(uint16_t, d, 0), d[10], d[11], d[12]);
        break;
    }

    case 7: {   // animate a mobile; neither links a gump nor advances
        uint8_t *m = RD(uint8_t *, d, 0);
        int16_t s4 = RD(int16_t, d, 4);
        int16_t s6 = RD(int16_t, d, 6);
        int16_t s8 = RD(int16_t, d, 8);
        if (!is_alive(m)) {
            AdvanceInline(this);
            return;
        }
        decomp::vcall_void(m, 0x14);
        RAWF(int32_t, m, 0x1a4) = 1;
        RAWF(int16_t, m, 0x1ae) = 0;
        RAWF(int16_t, m, 0x1a8) = 0;
        RAWF(int16_t, m, 0x1ac) = s4;
        RAWF(int16_t, m, 0x1aa) = s6;
        RAWF(int16_t, m, 0x1b0) = s8;
        RAWF(CEffectSequence *, m, 0x1c4) = this;
        decomp::thiscall_at_void(PORT_ADDR(&CMobileAnimQueue::RegisterActiveAnim), m);
        return;
    }

    case 8: {   // play a sound at a world position, then advance
        Pos pos = rd_pos(d, 2);
        PORT_THISCALL6(&PlaySound3D, void, g_soundManagerObj, int, Pos *, float, int, int, int)(RD(uint16_t, d, 0), &pos, 1.0f, 0, 0, 0);
        AdvanceInline(this);
        return;
    }
    }

    // link the new gump and remember which sequence owns it
    CGump_AddChild(obj, nullptr, 1);
    obj->sequence = this;
}
HOOK_METHOD(0x0043e0a0, CEffectSequence, Step);
