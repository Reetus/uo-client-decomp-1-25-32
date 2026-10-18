#include "char_stat_gump.h"

#include "../runtime/ported.h"
#include "batch01_helpers.h"

// 0x0041ac00  size 0x680
// The dragged stat becomes 10 + pos*35/range; the other two are rebalanced so the total stays 0x41, each clamped
// to [10, 0x2d], and their thumbs re-synced. The three mirror boxes are refreshed only when one of our thumbs matched.
// PORT_VSLOT(box, 0x3c, void, int)(box, 0xe023) is the virtual call [vtbl+0x3c](box, 0xe023) the original makes
// before each redraw. ? the meaning of that slot is not settled.
void __thiscall CCharStatGump::OnScroll(int pos, int range, int scrollbarArg)
{
    CGump * scrollbar = reinterpret_cast<CGump *>(scrollbarArg);
    int  s0, s1, s2;                                  // current values of +0xa0, +0xa4, +0xa8 (as held in registers)
    unsigned int sum;
    int under, half, rest, room;               // EDI = rest, EAX = half, EDX = room

    if (scrollbar == thumb_ac) {
        // ---- first stat dragged: +0xa0 ----
        edit_a0->value = (pos * 7 * 5) / range;
        edit_a0->value = edit_a0->value + 10;
        PORT_VSLOT1(edit_a0, 0x3c, void, int)(0xe023);
        edit_a0->Redraw();

        s1 = edit_a4->value;            // ECX
        s2 = edit_a8->value;            // EBX
        s0 = edit_a0->value;            // EAX
        sum = s2 + s1 + s0;
        if (sum > 0x41) {
            rest = static_cast<int>(sum - 0x41);                        // EDI
            half = rest / 2;                          // EAX (signed, toward zero)
            rest = rest - half;
            room = s1 - 10;                           // EDX
            if (half > room) {
                rest = rest + (half - s1) + 10;
                half = room;
            } else {
                room = s2 - 10;
                if (rest > room) {
                    half = half + (rest - s2) + 10;
                    rest = room;
                }
            }
            edit_a4->value = s1 - half;
            PORT_VSLOT1(edit_a4, 0x3c, void, int)(0xe023);
            edit_a4->Redraw();
            gfx::ScrollThumbSet(thumb_b0, edit_a4->value - 10, 0x23);

            edit_a8->value = edit_a8->value - rest;     // EBX = [..] - EDI
            PORT_VSLOT1(edit_a8, 0x3c, void, int)(0xe023);
            edit_a8->Redraw();
            gfx::ScrollThumbSet(thumb_b4, edit_a8->value - 10, 0x23);
        } else if (sum < 0x41) {
            under = 0x41 - s0 - s2 - s1;              // EDI
            half = under / 2;
            rest = under - half;
            room = 0x2d - s1;
            if (half > room) {
                rest = rest + (half + s1) - 0x2d;
                half = room;
            } else {
                room = 0x2d - s2;
                if (rest > room) {
                    rest = rest + s2;
                    half = half + rest - 0x2d;
                    rest = room;
                }
            }
            edit_a4->value = s1 + half;
            PORT_VSLOT1(edit_a4, 0x3c, void, int)(0xe023);
            edit_a4->Redraw();
            gfx::ScrollThumbSet(thumb_b0, edit_a4->value - 10, 0x23);

            edit_a8->value = edit_a8->value + rest;
            PORT_VSLOT1(edit_a8, 0x3c, void, int)(0xe023);
            edit_a8->Redraw();
            gfx::ScrollThumbSet(thumb_b4, edit_a8->value - 10, 0x23);
        }
        // sum == 0x41: nothing to rebalance
    }
    else if (scrollbar == thumb_b0) {
        // ---- second stat dragged: +0xa4 ----
        edit_a4->value = (pos * 7 * 5) / range;
        edit_a4->value = edit_a4->value + 10;
        PORT_VSLOT1(edit_a4, 0x3c, void, int)(0xe023);
        edit_a4->Redraw();

        s0 = edit_a0->value;            // ECX
        s1 = edit_a4->value;            // EAX
        s2 = edit_a8->value;            // EBX
        sum = s2 + s1 + s0;
        if (sum > 0x41) {
            rest = static_cast<int>(sum - 0x41);
            half = rest / 2;
            rest = rest - half;
            room = s0 - 10;
            if (half > room) {
                rest = rest + (half - s0) + 10;
                half = room;
            } else {
                room = s2 - 10;
                if (rest > room) {
                    half = half + (rest - s2) + 10;
                    rest = room;
                }
            }
            edit_a0->value = s0 - half;
            PORT_VSLOT1(edit_a0, 0x3c, void, int)(0xe023);
            edit_a0->Redraw();
            gfx::ScrollThumbSet(thumb_ac, edit_a0->value - 10, 0x23);

            edit_a8->value = edit_a8->value - rest;
            PORT_VSLOT1(edit_a8, 0x3c, void, int)(0xe023);
            edit_a8->Redraw();
            gfx::ScrollThumbSet(thumb_b4, edit_a8->value - 10, 0x23);
        } else if (sum < 0x41) {
            under = 0x41 - s0 - s1 - s2;
            half = under / 2;
            rest = under - half;
            room = 0x2d - s0;
            if (half > room) {
                rest = rest + (half + s0) - 0x2d;
                half = room;
            } else {
                room = 0x2d - s2;
                if (rest > room) {
                    rest = rest + s2;
                    half = half + rest - 0x2d;
                    rest = room;
                }
            }
            edit_a0->value = s0 + half;
            PORT_VSLOT1(edit_a0, 0x3c, void, int)(0xe023);
            edit_a0->Redraw();
            gfx::ScrollThumbSet(thumb_ac, edit_a0->value - 10, 0x23);

            edit_a8->value = edit_a8->value + rest;
            PORT_VSLOT1(edit_a8, 0x3c, void, int)(0xe023);
            edit_a8->Redraw();
            gfx::ScrollThumbSet(thumb_b4, edit_a8->value - 10, 0x23);
        }
    }
    else if (scrollbar == thumb_b4) {
        // ---- third stat dragged: +0xa8 ----
        edit_a8->value = (pos * 7 * 5) / range;
        edit_a8->value = edit_a8->value + 10;
        PORT_VSLOT1(edit_a8, 0x3c, void, int)(0xe023);
        edit_a8->Redraw();

        s0 = edit_a0->value;            // ECX
        s1 = edit_a4->value;            // EBX
        s2 = edit_a8->value;            // EAX
        sum = s2 + s1 + s0;
        if (sum > 0x41) {
            rest = static_cast<int>(sum - 0x41);
            half = rest / 2;
            rest = rest - half;
            room = s0 - 10;
            if (half > room) {
                rest = rest + (half - s0) + 10;
                half = room;
            } else {
                room = s1 - 10;
                if (rest > room) {
                    half = half + (rest - s1) + 10;
                    rest = room;
                }
            }
            edit_a0->value = s0 - half;
            PORT_VSLOT1(edit_a0, 0x3c, void, int)(0xe023);
            edit_a0->Redraw();
            gfx::ScrollThumbSet(thumb_ac, edit_a0->value - 10, 0x23);

            edit_a4->value = edit_a4->value - rest;
            PORT_VSLOT1(edit_a4, 0x3c, void, int)(0xe023);
            edit_a4->Redraw();
            gfx::ScrollThumbSet(thumb_b0, edit_a4->value - 10, 0x23);
        } else if (sum < 0x41) {
            under = 0x41 - s0 - s2 - s1;
            half = under / 2;
            rest = under - half;
            room = 0x2d - s0;
            if (half > room) {
                rest = rest + (half + s0) - 0x2d;
                half = room;
            } else {
                room = 0x2d - s1;
                if (rest > room) {
                    rest = rest + s1;
                    half = half + rest - 0x2d;
                    rest = room;
                }
            }
            edit_a0->value = s0 + half;
            PORT_VSLOT1(edit_a0, 0x3c, void, int)(0xe023);
            edit_a0->Redraw();
            gfx::ScrollThumbSet(thumb_ac, edit_a0->value - 10, 0x23);

            edit_a4->value = edit_a4->value + rest;
            PORT_VSLOT1(edit_a4, 0x3c, void, int)(0xe023);
            edit_a4->Redraw();
            gfx::ScrollThumbSet(thumb_b0, edit_a4->value - 10, 0x23);
        }
    }
    else {
        return;                                       // not one of our thumbs
    }

    // refresh the read-only mirror boxes (0x41b210)
    mirror_b8->value = edit_a0->value;
    mirror_b8->Redraw();
    mirror_bc->value = edit_a8->value;
    mirror_bc->Redraw();
    mirror_c0->value = edit_a4->value;
    mirror_c0->Redraw();

}
HOOK_VMETHOD(0x0041ac00, CCharStatGump, OnScroll);
