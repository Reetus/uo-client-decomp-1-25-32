// Per-tick update of one flying missile record (0x00430c60) and the radians-to-angle thunk (0x00430c40).
#include "missile_update.h"

#include "../runtime/ported.h"
#include "crt.h"
#include "rotated_blit.h"
#include "art_record_ops_ports.h"
#include "map_world_ops_ports.h"
#include "dirty_rects_ports.h"
#include "globals.h"

#define g_artCache GLOBAL_PTR(void, g_artCache_storage)
#define g_frameDataAllocator GLOBAL_PTR(void, g_frameDataAllocator_storage)

// 0x00476130: tile art record of (id + 0x4000) in the art cache at 0x0068d2d8; record words at
// +4 / +6 are width / height. Stack: id, RET 4.
#define TileArt(o, a1) PORT_THISCALL1(&ArtRecord_Get, uint16_t *, (o), int)(a1)
// 0x0044f8b0: ECX = RECT {left, top, right, bottom}.
#define InvalidateRect_(o) PORT_THISCALL0(&DirtyRects_Invalidate, void, (o))()
// 0x00446310 on the allocator object at 0x0064cdac: release block (key, name), RET 8.
#define ReleaseBlock(o, a1, a2) PORT_THISCALL2(&Map_Free, void, (o), uint32_t, const char *)(a1, a2)

// 0x00430c40  size 0x1b
// fld arg; fmul float [0x0049e968] (128.0); fmul double [0x0049e970] (1/pi); __ftol.
// ? The x87 multiplications are done in double here.
int __stdcall RadiansToAngle256(float radians)
{
    return static_cast<int>(static_cast<double>(radians) * static_cast<double>(g_0049e968) * g_0049e970);
}
HOOK_FUNC(0x00430c40, RadiansToAngle256);

// 0x00430c60  size 0x181
// Art 0x2aa cycles its frame offset 0..7. After the tick counter reaches the lifetime the record
// leaves the table (released under "background for missile anim" and freed). Otherwise the
// position moves by (dx, dy); the old rect (sprite size + 0x3c on each side) is invalidated,
// clamped to the 640 x 480 screen, the sprite is blitted rotated at the new position and the
// scroll offset is added back to x and y (copied to lastX / lastY).
// ? Float arithmetic is done in double and rounded to float on store, like the x87 original;
// the integer positions are truncated from the unrounded x sum and from the rounded y sum.
void __thiscall CMissileAnim::Update()
{
    if (art == 0x2aa) {
        frame = static_cast<int8_t>(frame + 1);
        if (frame > 7)
            frame = 0;
    }
    uint16_t *sprite = TileArt(g_artCache, static_cast<int>(frame) + 0x4000 + art);   // 0x00476130
    int w = sprite[2];
    int h = sprite[3];
    tick = tick + 1;
    if (tick == lifetime) {
        for (int i = 0; i < 20; i++) {
            if (g_missileTable[i] == this) {
                ReleaseBlock(g_frameDataAllocator, g_missileTable[i]->key, "background for missile anim");   // 0x00446310
                operator delete(g_missileTable[i]);   // operator delete, 0x0048ad60
                g_missileTable[i] = nullptr;
                return;
            }
        }
        return;
    }

    double xSum = static_cast<double>(dx) + static_cast<double>(x);
    x = static_cast<float>(xSum);
    float ySum = static_cast<float>(static_cast<double>(dy) + static_cast<double>(y));
    y = ySum;
    int xi = static_cast<int>(xSum);   // __ftol, 0x0048c0d0
    int rect[4];
    rect[0] = xi - 0x3c;
    rect[2] = xi + w + 0x3c;
    int yi = static_cast<int>(ySum);   // __ftol, 0x0048c0d0
    rect[1] = yi - 0x3c;
    rect[3] = yi + h + 0x3c;
    InvalidateRect_(rect);   // 0x0044f8b0
    if (rect[3] > 0x1e0)
        rect[3] = 0x1e0;
    if (rect[2] > 0x280)
        rect[2] = 0x280;
    if (rect[1] < 0)
        rect[1] = 0;
    if (rect[0] < 0)
        rect[0] = 0;

    int destY = static_cast<int>(y) & 0xffff;   // __ftol, 0x0048c0d0
    int destX = static_cast<int>(x) & 0xffff;   // __ftol, 0x0048c0d0
    BlitRotatedRLESprite(reinterpret_cast<const uint8_t *>(sprite), rect, destX, destY, angle);   // 0x00430e50

    x = x + g_scrollX;
    y = y + g_scrollY;
    lastX = x;
    lastY = y;
}
HOOK_METHOD(0x00430c60, CMissileAnim, Update);
