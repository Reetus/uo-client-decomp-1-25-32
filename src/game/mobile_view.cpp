// Camera / view-z computation of mobiles for the current walk phase.
// decomp/pass2/0x00449cb0.cpp (player) and 0x0044a170.cpp (other mobiles).
#include "../runtime/ported.h"
#include "globals.h"
#include "map_world.h"
#include "obj.h"
#include "vcall.h"
#include "walk.h"
#include "dirty_rects_ports.h"
#include "walk_manager_ops_ports.h"


// Slope z for a surface with height h above base: facing group {0,1,7} rises (3/4 h at phase 1, h at
// phase 2), group {3,4,5} falls (h/4 at phase 1, base at phase 2), {2,6} and everything else is the
// mid height. Written out from the four jump tables 0x44a0e4 / 0x44a104 / 0x44a124 / 0x44a144
// (and 0x44a56c.. for the non-player code), which all have the same shape.
static int SlopeZ(int h, int base, unsigned idx, int phase)
{
    int val = (h >> 1) + base;   // default / "half" case
    switch (idx) {
    case 0:
    case 1:
    case 7:
        if (phase == 1)
            val = (h - (h >> 2)) + base;
        else if (phase == 2)
            val = h + base;
        break;
    case 3:
    case 4:
    case 5:
        if (phase == 1)
            val = (h >> 2) + base;
        else if (phase == 2)
            val = base;
        break;
    default:
        break;
    }
    return val;
}

// z of the map cell corners interpolated for (phase, facing): 0x00446030 gives the four corner z values,
// 0x0044fe30 the axis flag, 0x00448600 the height.
static int MapCellZ(int x, int y, int phase, unsigned facing)
{
    uint8_t *corners = reinterpret_cast<CMap *>(g_mapObject)->GetNeighborZ(x, y);
    facing &= 7;
    int axis = RectIsTall((const int8_t *)(corners));
    return Walk_GetZVal((const int8_t *)(corners), axis, phase, static_cast<int>(facing));
}

// Interpolated z while the step changes z (frame >= 2): from g_dropZ towards g_dropTargetZ.
static int DropZ(int frame)
{
    if (frame < 2)
        return g_dropZ;
    return ((g_dropTargetZ - g_dropZ) * (frame - 2)) / (wq_stepFrames - 2) + g_dropZ;
}

// 0x00449cb0  size 0x431
// Player mobile: compute the camera scroll (DAT_004ba2a8 / DAT_004ba2ac) and the view z (this+0x190) for
// the current walk phase. During the first 3 frames the player stands on its own tile; afterwards on the
// tile it is walking to. The surface under it is the first object of the tile bucket that passes
// virtual +0x18 with z == the walk z.
void __thiscall CPlayer::UpdateViewZ()
{
    int phase;
    int zz;
    int yy;
    int xx;
    unsigned facingV;
    CObj *obj;

    g_scrollX = g_viewOffsetX + ((g_scrollBase + x) - y) * 22 - 0x13f;

    int frame = g_walk.frame;   // DAT_00648858
    if (frame < 3) {
        yy = y;
        zz = z;
        xx = x;
        facingV = facing;
        phase = frame;
    } else {
        unsigned d = g_walk.Entry(g_walk.cur)->dir;
        zz = g_walk.Entry(g_walk.cur)->z;
        xx = g_dirDx[d] + x;
        yy = g_dirDy[d] + y;
        phase = 4 - frame;
        facingV = static_cast<unsigned>(g_dirMap[facing]);
    }

    obj = g_tileBuckets[(yy & 0x3f) * 0x40 + (xx & 0x3f)];
    while (obj != nullptr) {
        if (decomp::vcall(PORT_RET(int), obj, 0x18) != 0 && static_cast<int16_t>(decomp::vcall(PORT_RET(int), obj, 0x44)) == zz)
            break;
        obj = static_cast<CObj *>(obj->next);
    }

    if (obj == nullptr) {
        if (g_dropping == 0)
            viewZ = zz;
        else
            viewZ = DropZ(frame);
    } else if (decomp::vcall(PORT_RET(int), obj, 0x1c) != 0) {
        int r;
        if (g_dropping != 0)
            r = DropZ(frame);
        else
            r = MapCellZ(xx, yy, phase, facingV);
        viewZ = r;
    } else if (decomp::vcall(PORT_RET(uint32_t), obj, 0x40) & 0x400) {
        uint32_t tf = decomp::vcall(PORT_RET(uint32_t), obj, 0x40) & 0xc0000000;
        if (tf == 0) {
            int h = decomp::vcall(PORT_RET(int), obj, 0x6c);
            int base = obj->z;
            int val = (h >> 1) + base;
            if (facingV <= 7)
                val = SlopeZ(h, base, facingV, phase);
            viewZ = val;
        } else if (tf == 0x40000000) {
            int h = decomp::vcall(PORT_RET(int), obj, 0x6c);
            int base = obj->z;
            viewZ = SlopeZ(h, base, (facingV - 2) & 7, phase);
        } else if (tf == 0xc0000000) {
            int h = decomp::vcall(PORT_RET(int), obj, 0x6c);
            int base = obj->z;
            viewZ = SlopeZ(h, base, (facingV - 4) & 7, phase);
        } else if (tf == 0x80000000) {
            int h = decomp::vcall(PORT_RET(int), obj, 0x6c);
            int base = obj->z;
            viewZ = SlopeZ(h, base, (facingV + 2) & 7, phase);
        }
        // any other tf value: viewZ left unchanged
    } else {
        int r;
        if (g_dropping != 0)
            r = DropZ(frame);
        else
            r = static_cast<int16_t>(decomp::vcall(PORT_RET(int), obj, 0x44));
        viewZ = r;
    }

    g_scrollY = ((x + y) * 11) * 2 - viewZ * 4 + g_viewOffsetY - 0xd9;
}
HOOK_VMETHOD(0x00449cb0, CPlayer, UpdateViewZ);

// 0x0044a170  size 0x3f9
// Non-player mobile: compute the view z (this+0x190) for its walk phase. The lerp is
// (z - target) * (remain - 2) / (total - 2) + target with z = this+0x24 and target = this[0x19c] + z.
// 0x00454540 (ECX = this, no stack arguments) gives the total frame count. An unknown slope-flag
// combination returns without writing.
void __thiscall CMobile::UpdateViewZ()
{
    int total = PORT_FUNC1(&CMobile::StepFrames, int, __fastcall, CMobile *)(this);
    int cur = unk_13c;
    int remain = total - cur;
    int xx;
    int yy;
    int zz;
    int phase;
    unsigned facingV;
    CObj *obj;

    if (cur == 0) {
        xx = x;
        yy = y;
        facingV = facing;
        zz = z;
        phase = 0;
    } else if (cur > (total >> 1)) {
        xx = fromX;
        yy = fromY;
        facingV = facing;
        zz = fromZ;
        phase = remain;
    } else {
        xx = x;
        phase = cur;
        yy = y;
        zz = z;
        facingV = static_cast<unsigned>(g_dirMap[facing]);
    }

    obj = g_tileBuckets[(yy & 0x3f) * 0x40 + (xx & 0x3f)];
    while (obj != nullptr) {
        if (decomp::vcall(PORT_RET(int), obj, 0x18) != 0 && static_cast<int16_t>(decomp::vcall(PORT_RET(int), obj, 0x44)) == zz)
            break;
        obj = static_cast<CObj *>(obj->next);
    }

    int target;
    int zcur;

    if (obj == nullptr) {
        if (zMoving == 0) {
            viewZ = zz;
            return;
        }
        zcur = z;
        target = zDelta + zcur;
        goto lerp;
    }

    if (decomp::vcall(PORT_RET(int), obj, 0x1c) != 0) {
        if (zMoving != 0) {
            zcur = z;
            target = zDelta + zcur;
            goto lerp;
        }
        viewZ = MapCellZ(xx, yy, phase, facingV);
        return;
    }

    if (decomp::vcall(PORT_RET(uint32_t), obj, 0x40) & 0x400) {
        uint32_t tf = decomp::vcall(PORT_RET(uint32_t), obj, 0x40) & 0xc0000000;
        if (tf == 0) {
            int h = decomp::vcall(PORT_RET(int), obj, 0x6c);
            int base = obj->z;
            int val = (h >> 1) + base;
            if (facingV <= 7)   // table 0x44a56c has 8 entries; larger values take the default
                val = SlopeZ(h, base, facingV, phase);
            viewZ = val;
            return;
        }
        if (tf == 0x40000000) {
            int h = decomp::vcall(PORT_RET(int), obj, 0x6c);
            int base = obj->z;
            viewZ = SlopeZ(h, base, (facingV - 2) & 7, phase);
            return;
        }
        if (tf == 0xc0000000) {
            int h = decomp::vcall(PORT_RET(int), obj, 0x6c);
            int base = obj->z;
            viewZ = SlopeZ(h, base, (facingV - 4) & 7, phase);
            return;
        }
        if (tf == 0x80000000) {
            int h = decomp::vcall(PORT_RET(int), obj, 0x6c);
            int base = obj->z;
            viewZ = SlopeZ(h, base, (facingV + 2) & 7, phase);
            return;
        }
        return;
    }

    // plain (non-slope) surface
    if (zMoving != 0) {
        zcur = z;
        target = zDelta + zcur;
        goto lerp;
    }
    viewZ = static_cast<int16_t>(decomp::vcall(PORT_RET(int), obj, 0x44));
    return;

lerp:
    if (remain < 2)
        viewZ = target;
    else
        viewZ = ((zcur - target) * (remain - 2)) / (total - 2) + target;
}
HOOK_VMETHOD(0x0044a170, CMobile, UpdateViewZ);
