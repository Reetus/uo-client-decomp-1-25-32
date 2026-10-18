#include "init_skill_gump.h"

#include "../runtime/ported.h"
#include "batch01_helpers.h"

// 0x0041c3a0  size 0x558
// Three separate arms (the rebalance order differs per arm):
//   thumb_b4 moved: absorb into skill_ac (thumb_b8) first, then skill_b0 (thumb_bc)
//   thumb_b8 moved: absorb into skill_a8 (thumb_b4) first, then skill_b0 (thumb_bc)
//   thumb_bc moved: absorb into skill_a8 (thumb_b4) first, then skill_ac (thumb_b8)
// value = pos*50/range (signed idiv); the sum test against 0x64 is unsigned; halves are signed /2.
// PORT_VSLOT(box, 0x3c, void, int)(box, 0xe023) is the virtual call [vtbl+0x3c](box, 0xe023) made before each redraw.
// ? the meaning of that slot is not settled.
void __thiscall CInitSkillGump::OnScroll(int pos, int range, int scrollbarArg)
{
    CGump * scrollbar = reinterpret_cast<CGump *>(scrollbarArg);
    int s0, s1, s2;                                   // current K0, K1, K2 values
    unsigned int sum;
    int rest, half, room;                             // EDI, EAX, EDX

    if (scrollbar == thumb_b4) {
        // ---- K0 dragged ----
        skill_a8->value = (pos * 50) / range;
        PORT_VSLOT1(skill_a8, 0x3c, void, int)(0xe023);
        skill_a8->Redraw();

        s2 = skill_b0->value;            // EBX
        s1 = skill_ac->value;            // ECX
        s0 = skill_a8->value;            // EAX
        sum = s0 + s1 + s2;
        if (sum > 0x64) {
            rest = static_cast<int>(sum - 0x64);
            half = rest / 2;
            rest = rest - half;
            if (half > s1) {
                rest = rest + (half - s1);
                half = s1;
            } else if (rest > s2) {
                half = half + (rest - s2);
                rest = s2;
            }
            skill_ac->value = s1 - half;
            PORT_VSLOT1(skill_ac, 0x3c, void, int)(0xe023);
            skill_ac->Redraw();
            gfx::ScrollThumbSet(thumb_b8, skill_ac->value, 0x32);

            skill_b0->value = skill_b0->value - rest;
            PORT_VSLOT1(skill_b0, 0x3c, void, int)(0xe023);
            skill_b0->Redraw();
            gfx::ScrollThumbSet(thumb_bc, skill_b0->value, 0x32);
        } else if (sum < 0x64) {
            rest = 0x64 - s2 - s1 - s0;
            half = rest / 2;
            rest = rest - half;
            room = 0x32 - s1;
            if (half > room) {
                rest = rest + (half + s1) - 0x32;
                half = room;
            } else {
                room = 0x32 - s2;
                if (rest > room) {
                    rest = rest + s2;
                    half = half + rest - 0x32;
                    rest = room;
                }
            }
            skill_ac->value = half + s1;
            PORT_VSLOT1(skill_ac, 0x3c, void, int)(0xe023);
            skill_ac->Redraw();
            gfx::ScrollThumbSet(thumb_b8, skill_ac->value, 0x32);

            skill_b0->value = skill_b0->value + rest;
            PORT_VSLOT1(skill_b0, 0x3c, void, int)(0xe023);
            skill_b0->Redraw();
            gfx::ScrollThumbSet(thumb_bc, skill_b0->value, 0x32);
        }
    }
    else if (scrollbar == thumb_b8) {
        // ---- K1 dragged ----
        skill_ac->value = (pos * 50) / range;
        PORT_VSLOT1(skill_ac, 0x3c, void, int)(0xe023);
        skill_ac->Redraw();

        s2 = skill_b0->value;            // EBX
        s1 = skill_ac->value;            // EAX
        s0 = skill_a8->value;            // ECX
        sum = s0 + s1 + s2;
        if (sum > 0x64) {
            rest = static_cast<int>(sum - 0x64);
            half = rest / 2;
            rest = rest - half;
            if (half > s0) {
                rest = rest + (half - s0);
                half = s0;
            } else if (rest > s2) {
                half = half + (rest - s2);
                rest = s2;
            }
            skill_a8->value = s0 - half;
            PORT_VSLOT1(skill_a8, 0x3c, void, int)(0xe023);
            skill_a8->Redraw();
            gfx::ScrollThumbSet(thumb_b4, skill_a8->value, 0x32);

            skill_b0->value = skill_b0->value - rest;
            PORT_VSLOT1(skill_b0, 0x3c, void, int)(0xe023);
            skill_b0->Redraw();
            gfx::ScrollThumbSet(thumb_bc, skill_b0->value, 0x32);
        } else if (sum < 0x64) {
            rest = 0x64 - s2 - s1 - s0;
            half = rest / 2;
            rest = rest - half;
            room = 0x32 - s0;
            if (half > room) {
                rest = rest + (half + s0) - 0x32;
                half = room;
            } else {
                room = 0x32 - s2;
                if (rest > room) {
                    rest = rest + s2;
                    half = half + rest - 0x32;
                    rest = room;
                }
            }
            skill_a8->value = half + s0;
            PORT_VSLOT1(skill_a8, 0x3c, void, int)(0xe023);
            skill_a8->Redraw();
            gfx::ScrollThumbSet(thumb_b4, skill_a8->value, 0x32);

            skill_b0->value = skill_b0->value + rest;
            PORT_VSLOT1(skill_b0, 0x3c, void, int)(0xe023);
            skill_b0->Redraw();
            gfx::ScrollThumbSet(thumb_bc, skill_b0->value, 0x32);
        }
    }
    else if (scrollbar == thumb_bc) {
        // ---- K2 dragged ----
        skill_b0->value = (pos * 50) / range;
        PORT_VSLOT1(skill_b0, 0x3c, void, int)(0xe023);
        skill_b0->Redraw();

        s1 = skill_ac->value;            // EBX
        s2 = skill_b0->value;            // EAX
        s0 = skill_a8->value;            // ECX
        sum = s0 + s1 + s2;
        if (sum > 0x64) {
            rest = static_cast<int>(sum - 0x64);
            half = rest / 2;
            rest = rest - half;
            if (half > s0) {
                rest = rest + (half - s0);
                half = s0;
            } else if (rest > s1) {
                half = half + (rest - s1);
                rest = s1;
            }
            skill_a8->value = s0 - half;
            PORT_VSLOT1(skill_a8, 0x3c, void, int)(0xe023);
            skill_a8->Redraw();
            gfx::ScrollThumbSet(thumb_b4, skill_a8->value, 0x32);

            skill_ac->value = skill_ac->value - rest;
            PORT_VSLOT1(skill_ac, 0x3c, void, int)(0xe023);
            skill_ac->Redraw();
            gfx::ScrollThumbSet(thumb_b8, skill_ac->value, 0x32);
        } else if (sum < 0x64) {
            rest = 0x64 - s2 - s1 - s0;
            half = rest / 2;
            rest = rest - half;
            room = 0x32 - s0;
            if (half > room) {
                rest = rest + (half + s0) - 0x32;
                half = room;
            } else {
                room = 0x32 - s1;
                if (rest > room) {
                    rest = rest + s1;
                    half = half + rest - 0x32;
                    rest = room;
                }
            }
            skill_a8->value = half + s0;
            PORT_VSLOT1(skill_a8, 0x3c, void, int)(0xe023);
            skill_a8->Redraw();
            gfx::ScrollThumbSet(thumb_b4, skill_a8->value, 0x32);

            skill_ac->value = skill_ac->value + rest;
            PORT_VSLOT1(skill_ac, 0x3c, void, int)(0xe023);
            skill_ac->Redraw();
            gfx::ScrollThumbSet(thumb_b8, skill_ac->value, 0x32);
        }
    }
    // else: not one of our thumbs, fall through to the epilogue

}
HOOK_VMETHOD(0x0041c3a0, CInitSkillGump, OnScroll);
