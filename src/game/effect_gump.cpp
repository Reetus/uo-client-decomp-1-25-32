// Graphical effect creation: build a missile / stationary effect gump of kind 0..3, or queue the
// request while effects are held back. The SEH frame of the original is omitted.
#include <cstdint>
#include <cstring>

#include "../runtime/ported.h"
#include "batch02_helpers.h"
#include "globals.h"
#include "crt.h"
#include "missile_gump.h"

// World position passed by value (dword x|y<<16, word z, padded to 8 bytes).
struct PosArg {
    int32_t a;
    int16_t b;
    int16_t pad;
};

#define g_effectQueue GLOBAL_AS(int *, g_effectSequence)           // queue object, head node pointer at +4

// Missile / effect gumps (object size 0x108). PosArg and Pos have the same 8-byte layout.
#define AsPos(p) (*reinterpret_cast<Pos *>(&(p)))

// Unaligned field stores into a packed queue record: PUT(uint16_t, rec, off, value) stores the value as a T. A
// macro because Visual C++ 5.0 accepts no explicit function-template argument; the type of the stored value is
// deduced from the cast.
template <class T>
static inline void put_(uint8_t *rec, size_t off, T v)
{
    memcpy(rec + off, &v, sizeof(T));
}
#define PUT(T, rec, off, v) put_((rec), (off), static_cast<T>(v))

// Append {type, copy of rec (size bytes), next} at the tail of the queue and free rec.
static void enqueue(int type, uint8_t *rec, size_t size)
{
    int *queue = g_effectQueue;
    int *node = static_cast<int *>(::operator new(0xc));              // {type, data, next}
    node[0] = type;
    uint8_t *data = static_cast<uint8_t *>(::operator new(size));
    node[1] = ptr_i32(data);
    memcpy(data, rec, size);
    node[2] = 0;
    int *cur = static_cast<int *>(i32_ptr(queue[1]));
    if (cur == nullptr) {
        queue[1] = ptr_i32(node);
        operator delete(rec);
        return;
    }
    while (cur[2] != 0)
        cur = static_cast<int *>(i32_ptr(cur[2]));
    cur[2] = ptr_i32(node);
    operator delete(rec);
}

// 0x00437190  size 0x8d1
// Create an effect gump of kind 0..3 (0 missile, 1 effect at object, 2 stationary, 3 fixed at
// object), or queue it while g_effectsHeld is set. 14 dword arguments (RET 0x38); the low byte of
// the first is the kind.
void __stdcall CreateEffectGump(unsigned int kind, int p2, int p3, unsigned short p4, int p5,
                                       unsigned short p6, int p7, unsigned short p8, unsigned char p9,
                                       unsigned char p10, unsigned char p11, unsigned char p12,
                                       unsigned char p13, unsigned char p14)
{
    CGump *obj;
    switch (kind & 0xff) {
    case 0:
        if (p2 == 0 && p3 == 0) {
            if (g_sequencing != 0) {
                uint8_t *rec = static_cast<uint8_t *>(::operator new(0x11));
                PUT(uint16_t, rec, 0, p4);
                PUT(int32_t, rec, 2, p5);
                PUT(uint16_t, rec, 6, p6);
                PUT(int32_t, rec, 8, p7);
                PUT(uint16_t, rec, 0xc, p8);
                rec[0xe] = p9;
                rec[0xf] = p13;
                rec[0x10] = p14;
                enqueue(3, rec, 0x11);
                return;
            }
            PosArg s = {p5, static_cast<int16_t>(p6), 0};
            PosArg e = {p7, static_cast<int16_t>(p8), 0};
            obj = new CMissileGump(AsPos(s), AsPos(e), p4, p9, p13, p14);
        } else if (p2 != 0 && p3 == 0) {
            if (g_sequencing != 0) {
                uint8_t *rec = static_cast<uint8_t *>(::operator new(0xf));
                PUT(uint16_t, rec, 0, p4);
                PUT(int32_t, rec, 2, p2);
                PUT(int32_t, rec, 6, p7);
                PUT(uint16_t, rec, 0xa, p8);
                rec[0xc] = p9;
                rec[0xd] = p13;
                rec[0xe] = p14;
                enqueue(5, rec, 0xf);
                return;
            }
            PosArg e = {p7, static_cast<int16_t>(p8), 0};
            obj = new CMissileGump(reinterpret_cast<CObj *>(p2), AsPos(e), p4, p9, p13, p14);
        } else if (p2 == 0 /* && p3 != 0 */) {
            if (g_sequencing != 0) {
                uint8_t *rec = static_cast<uint8_t *>(::operator new(0xf));
                PUT(uint16_t, rec, 0, p4);
                PUT(int32_t, rec, 2, p5);
                PUT(uint16_t, rec, 6, p6);
                PUT(int32_t, rec, 8, p3);
                rec[0xc] = p9;
                rec[0xd] = p13;
                rec[0xe] = p14;
                enqueue(4, rec, 0xf);
                return;
            }
            PosArg s = {p5, static_cast<int16_t>(p6), 0};
            obj = new CMissileGump(AsPos(s), reinterpret_cast<CObj *>(p3), p4, p9, p13, p14);
        } else {                                       // p2 != 0 && p3 != 0
            if (g_sequencing != 0) {
                uint8_t *rec = static_cast<uint8_t *>(::operator new(0xd));
                PUT(uint16_t, rec, 0, p4);
                PUT(int32_t, rec, 2, p2);
                PUT(int32_t, rec, 6, p3);
                rec[0xa] = p9;
                rec[0xb] = p13;
                rec[0xc] = p14;
                enqueue(6, rec, 0xd);
                return;
            }
            obj = new CMissileGump(reinterpret_cast<CObj *>(p2), reinterpret_cast<CObj *>(p3), p4, p9, p13, p14);
        }
        CGump_AddChild(obj, nullptr, 1);
        return;

    case 1:
        if (g_sequencing != 0) {
            uint8_t *rec = static_cast<uint8_t *>(::operator new(0x4));
            PUT(int32_t, rec, 0, p2);
            enqueue(2, rec, 0x4);
            return;
        }
        if (p2 == 0)
            return;
        obj = new CMissileGump(reinterpret_cast<CObj *>(p2));
        CGump_AddChild(obj, nullptr, 1);
        return;

    case 2:
        if (g_sequencing != 0) {
            uint8_t *rec = static_cast<uint8_t *>(::operator new(0xc));
            PUT(int32_t, rec, 0, p5);
            PUT(uint16_t, rec, 4, p6);
            PUT(uint16_t, rec, 6, p4);
            rec[8] = p9;
            rec[9] = p10;
            rec[10] = p11;
            rec[11] = p12;
            enqueue(0, rec, 0xc);
            return;
        }
        {
            PosArg s = {p5, static_cast<int16_t>(p6), 0};
            obj = new CMissileGump(AsPos(s), p4, p9, p10, p11, p12);
        }
        CGump_AddChild(obj, nullptr, 1);
        return;

    case 3:
        if (g_sequencing != 0) {
            uint8_t *rec = static_cast<uint8_t *>(::operator new(0xa));
            PUT(int32_t, rec, 0, p2);
            PUT(uint16_t, rec, 4, p4);
            rec[6] = p9;
            rec[7] = p10;
            rec[8] = p11;
            rec[9] = p12;
            enqueue(1, rec, 0xa);
            return;
        }
        if (p2 == 0)
            return;
        obj = new CMissileGump(reinterpret_cast<CObj *>(p2), p4, p9, p10, p11, p12);
        CGump_AddChild(obj, nullptr, 1);
        return;

    default:
        return;
    }
}
HOOK_FUNC(0x00437190, CreateEffectGump);
