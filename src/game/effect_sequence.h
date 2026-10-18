// CEffectSequence: a queue of effect records (missile gumps, sounds, mobile
// animations) executed one node at a time; each effect that finishes calls
// back to run the next node. Referenced from CMissileGump::sequence (+0x104).
// See decomp/pass2/0x0043e0a0.cpp. Object size not known.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"

// Queue node: type selects the record layout in `data` (packed, unaligned).
struct SeqNode {
    int32_t type;      // +0x00  0..8
    uint8_t *data;     // +0x04
    SeqNode *next;     // +0x08
};
static_assert(sizeof(SeqNode) == 0xc, "SeqNode size");

class CEffectSequence {
public:
    // 0x0043e0a0: execute one node, then advance to the next / loop / self-delete.
    void __thiscall Step(SeqNode *node);
    // 0x0043dfa0: constructor; vtable 0x0049f1b4, head / current / repeat zeroed.
    CEffectSequence();
    // 0x0043dfc0: scalar deleting destructor (vtable slot 0, the compiler's). Frees every node and its data.
    virtual ~CEffectSequence();
    // 0x0043e020: RET 0xc. Append a node holding a private copy of `size` bytes of `data`.
    void __thiscall Append(int type, const void *data, unsigned size);
    // 0x0043e730: next node, or restart from the head while the repeat count lasts, else delete self.
    void __thiscall Advance();

    SeqNode *head;      // +0x04
    SeqNode *current;   // +0x08
    uint8_t repeat;     // +0x0c  remaining repeat count
};

static_assert(offsetof(CEffectSequence, head) == 0x04, "CEffectSequence::head");
static_assert(offsetof(CEffectSequence, current) == 0x08, "CEffectSequence::current");
static_assert(offsetof(CEffectSequence, repeat) == 0x0c, "CEffectSequence::repeat");
