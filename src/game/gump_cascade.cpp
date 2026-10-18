#include "book_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"
#include "crt.h"
#include "gump_cascade_ports.h"

// Cascade slots: positions on a 16 / 0x70 ... grid that minimized gumps (spell book, spell icons) take so
// they do not stack on top of each other. Two parallel int arrays at the pointers below, `count` used of
// `capacity`.

// 0x0040d1b0  size 0x145
// Find the first free (x, y) cell walking y = 0x10, 0x50 ... < 0x190 inside x = 0x10, 0x70 ... < 0x210, store it
// in *outX / *outY and record it in the slot arrays, growing them by 8 entries when full.
// Nothing is written to the outputs when every cell is taken.
void __fastcall GumpCascade_Alloc(int32_t *outX, int32_t *outY)
{
    int32_t count = g_cascadeSlotCount;
    int32_t candX = 0x10;
    int32_t candY;
    int i;
    for (;;) {
        candY = 0x10;
        for (;;) {
            i = 0;
            if (count > 0) {
                int32_t *ys = g_cascadeSlotY;
                int32_t *xs = g_cascadeSlotX;
                while (i < count) {
                    if (xs[i] == candX && ys[i] == candY)
                        break;
                    i++;
                }
            }
            if (i == count)
                goto found;
            candY += 0x40;
            if (candY >= 0x190)
                break;
        }
        candX += 0x60;
        if (candX >= 0x210)
            return;
    }
found:
    *outX = candX;
    *outY = candY;
    count = g_cascadeSlotCount;
    if (count == g_cascadeCapacity) {
        g_cascadeCapacity = g_cascadeCapacity + 8;
        int32_t *newX = static_cast<int32_t *>(::operator new(g_cascadeCapacity * 4));   // operator new 0x0048ad70
        int32_t *newY = static_cast<int32_t *>(::operator new(g_cascadeCapacity * 4));
        count = g_cascadeSlotCount;
        if (count != 0) {
            memcpy(newX, g_cascadeSlotX, count * 4);
            memcpy(newY, g_cascadeSlotY, g_cascadeSlotCount * 4);
            operator delete(g_cascadeSlotX);   // operator delete 0x0048ad60
            operator delete(g_cascadeSlotY);
            count = g_cascadeSlotCount;
        }
        g_cascadeSlotX = newX;
        g_cascadeSlotY = newY;
    }
    g_cascadeSlotX[count] = candX;
    g_cascadeSlotY[g_cascadeSlotCount] = candY;
    g_cascadeSlotCount = g_cascadeSlotCount + 1;
}
HOOK_FUNC(0x0040d1b0, GumpCascade_Alloc);

// 0x0040d300  size 0x81
// Remove the slot (x, y) from the arrays (shifting the rest down) if it is present.
void __fastcall GumpCascade_Free(int32_t x, int32_t y)
{
    int32_t *xs = g_cascadeSlotX;
    int32_t count = g_cascadeSlotCount;
    int i = 0;
    if (count > 0) {
        int32_t *ys = g_cascadeSlotY;
        while (i < count) {
            if (xs[i] == x && ys[i] == y)
                break;
            i++;
        }
    }
    if (i < count) {
        int j = i + 1;
        if (j < count) {
            do {
                g_cascadeSlotX[j - 1] = g_cascadeSlotX[j];
                g_cascadeSlotY[j - 1] = g_cascadeSlotY[j];
                count = g_cascadeSlotCount;
                j++;
            } while (j < count);
        }
        count--;
        g_cascadeSlotCount = count;
    }
}
HOOK_FUNC(0x0040d300, GumpCascade_Free);

// 0x0040d390  size 0x73
// Iconify: take a cascade slot if the gump has none yet (and cascading is on), remember the current
// position in +0x7c / +0x80, switch to the small frame (unk_60) and move to the slot position (+0x84 / +0x88).
void __thiscall CBookGump::Iconify()
{
    if (unk_8c == 0 && unk_90 == 0 && g_004a75a0 != 0) {
        GumpCascade_Alloc(&unk_8c, &unk_90);   // AllocGumpCascadeSlot
        unk_84 = unk_8c;
        unk_88 = unk_90;
    }
    unk_7c = x;
    unk_60 = 1;
    unk_80 = y;
    x = unk_84;
    y = unk_88;
}
HOOK_METHOD(0x0040d390, CBookGump, Iconify);

// 0x0040d410  size 0xb5
// Back from the small frame: remember the small-frame position in +0x84 / +0x88, restore the saved
// position, and release the cascade slot (0x0040d300 is inlined here).
void __thiscall CBookGump::Uniconify()
{
    int32_t curX = x;
    int32_t curY = y;
    unk_84 = curX;
    x = unk_7c;
    unk_88 = curY;
    unk_60 = 0;
    y = unk_80;
    int32_t keyX = unk_8c;
    if (keyX == 0)
        return;
    int32_t count = g_cascadeSlotCount;
    int32_t keyY = unk_90;
    int32_t *xs = g_cascadeSlotX;
    int i = 0;
    if (count > 0) {
        int32_t *ys = g_cascadeSlotY;
        while (i < count) {
            if (xs[i] == keyX && ys[i] == keyY)
                break;
            i++;
        }
    }
    if (i < count) {
        int j = i + 1;
        if (j < count) {
            do {
                g_cascadeSlotX[j - 1] = g_cascadeSlotX[j];
                g_cascadeSlotY[j - 1] = g_cascadeSlotY[j];
                count = g_cascadeSlotCount;
                j++;
            } while (j < count);
        }
        count--;
        g_cascadeSlotCount = count;
    }
}
HOOK_METHOD(0x0040d410, CBookGump, Uniconify);
