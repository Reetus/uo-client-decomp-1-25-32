// CMissileGump and CMissileHelper. gumps.cxx, 0x00437cf0..0x0043a000.
//
// The SEH frames of the original constructors (unwind states around the
// operator new calls) are omitted. __ftol calls are plain C casts.
#include "missile_gump.h"

#include <cmath>
#include <cstring>

#include "../runtime/ported.h"
#include "effect_sequence.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "missile_update.h"
#include "app_support_ports.h"
#include "world_obj_ops.h"
#include "world_obj.h"
#include "gump.h"
#include "globals.h"

// Map origin / scroll state used by the world to screen transform.
// Tiledata animation block, 0x48 bytes per art id starting at 0x005286b0:
// +0x00..0x3f frame offsets (signed), +0x41 frame count, +0x42 delay,
// +0x43 initial delay.
#define g_tileAnim GLOBAL_PTR(uint8_t, g_animData_storage)
// Art header table, 0x28 bytes per id; +5 is a frame count.

// Original helpers not ported yet.
static inline void MissileHelper_Release(CMissileHelper *h) { h->CWorldObj::RemoveFromWorld(); }    // run when helper+0x1c != 0
static inline void MissileHelper_Register(CMissileHelper *h) { h->CWorldObj::AddToWorld(); }   // after the position fields are set
#define AngleByte(a1) (RadiansToAngle256(a1))    // heading (radians) -> 0..255
#define RandomBelow(a1) (App_RandomMod(a1))   // ECX = n, result in AX

static unsigned char heading(const CMissileGump *g)
{
    return (unsigned char)(AngleByte((float)atan2(g->velY, g->velX)) + 0x80);
}

// Frame setup shared by the flying constructors and 0x00439120.
static inline void setup_art(CMissileGump *g, uint16_t art, uint8_t noRotate, bool useNoRotate)
{
    unsigned a = (unsigned)art * 0x48;
    if (g_tileAnim[a + 0x41] != 0) {
        g->tiledataAnim = 1;
        g->frameDelay = g_tileAnim[a + 0x43];
        g->firstFrame = art;
        g->lastFrame = 0;
        g->curFrame = (uint16_t)((int16_t)(int8_t)g_tileAnim[a] + art);
    } else {
        g->tiledataAnim = 0;
        g->firstFrame = art;
        g->curFrame = art;
        if (useNoRotate && noRotate != 0)
            g->lastFrame = art;
        else
            g->lastFrame = (uint16_t)(g_tileData[(unsigned)art * 0x28 + 5] + art - 1);
    }
}

// Fields every constructor initialises the same way after the art block.
static inline void clear_links(CMissileGump *g)
{
    g->name = const_cast<char *>("missile gump");   // 0x004a8364
    g->dirty = 0;
    g->unk_50 = 0;
    g->unk_60 = 0;
    g->firstChild = nullptr;
    g->parent = nullptr;
    g->lastChild = nullptr;
    g->next = nullptr;
    g->prev = nullptr;
}

static inline void flying_flags(CMissileGump *g, uint8_t noRotate, uint8_t flag)
{
    g->spawnImpact = (flag != 0);
    g->randomFrame = 0;
    g->unk_f4 = 0;
    g->pingPong = 0;
    g->backward = 0;
    g->attached = 0;
    g->spin = 0;
    g->rotateToHeading = (noRotate == 0);
    g->nextEffect = nullptr;
}

// The world-position helper. The stores through it after the call are not
// null-checked in the original either.
inline CMissileHelper::CMissileHelper(CMissileGump *ownerGump, int16_t hx, int16_t hy, int16_t hz)
{
    x = hx;
    y = hy;
    z = hz;
    wx = (double)hx;
    owner = ownerGump;
    wy = (double)hy;
    wz = (double)hz;
    MissileHelper_Register(this);
}

// Normalise the screen velocity, scale by speed, and rescale the helper's
// world deltas by velX / (destScreenX - x) (0x00438530, 0x00438cf0).
static inline void scale_to_target(CMissileGump *g)
{
    double vx = g->velX;
    double vy = g->velY;
    double len = sqrt(vy * vy + vx * vx);
    g->velX = vx / len;
    g->velY = vy / len;
    double spd = (double)g->stepSpeed;
    g->velX = g->velX * spd;
    g->velY = g->velY * spd;

    CMissileHelper *hh = g->helper;
    hh->dx = hh->dx * g->velX / (double)(g->destScreenX - g->x);
    hh = g->helper;
    hh->dy = hh->dy * g->velX / (double)(g->destScreenX - g->x);
    hh = g->helper;
    hh->dz = hh->dz * g->velX / (double)(g->destScreenX - g->x);
}

// Update the helper's integer position from its sub-tile position.
static inline void sync_helper(CMissileHelper *h)
{
    h->x = (int16_t)(int)h->wx;
    h->y = (int16_t)(int)h->wy;
    h->z = (int16_t)(int)h->wz;
}

// 0x00437cf0  size 0x6e
// Destructor body of the world-position helper (vtable 0x0049ebd0).
// SEH frame handler 0x0049a0e8 omitted.
CMissileHelper::~CMissileHelper()
{
    if (inWorld != 0)
        MissileHelper_Release(this);
    CMissileGump *o = owner;
    if (o != nullptr) {
        if (o->helper == this)
            o->helper = nullptr;
    }
}
HOOK_DTOR(0x00437cf0, CMissileHelper);
HOOK_VTABLE(0x0049ebd0, CMissileHelper);
HOOK_SDD(0x00437cc0, CMissileHelper);

// 0x00438150  size 0x3a7
// Missile from world (x,y,z) to world (x,y,z). SEH frame handler 0x0049a141 omitted.
CMissileGump::CMissileGump(Pos start, Pos end, uint16_t art, uint8_t speed, uint8_t noRotate, uint8_t flag)
{
    int sx = start.x, sy = start.y, sz = start.z;
    int ex = end.x, ey = end.y, ez = end.z;

    sequence = nullptr;
    x = (g_scrollBase + sx - sy) * 22 - g_scrollX;
    y = (sy + sx) * 22 - sz * 4 - g_scrollY + 0x2b;
    destScreenX = (g_scrollBase + ex - ey) * 22 - g_scrollX;
    destScreenY = (ey + ex) * 22 - ez * 4 - g_scrollY + 0x2b;

    int dx = destScreenX - x;
    int dy = destScreenY - y;
    int steps = (int)sqrt((double)(dx * dx + dy * dy)) / (speed * 2 + 10);
    double dsteps = (double)steps;
    velX = (double)dx / dsteps;
    dy = destScreenY - y;
    velY = (double)dy / dsteps;
    posX = (double)x;
    posY = (double)y;

    if (noRotate != 0)
        angle = 0;
    else
        angle = heading(this);
    lifetime = (uint8_t)((char)steps + 1);
    stepSpeed = (uint8_t)((speed + 3) << 1);
    delayCounter = 0;
    frameDelay = 1;
    destX = end.x;
    destY = end.y;
    target = nullptr;
    destZ = end.z;

    setup_art(this, art, noRotate, true);
    flying_flags(this, noRotate, flag);
    clear_links(this);

    CMissileHelper *h = new CMissileHelper(this, start.x, start.y, start.z);
    helper = h;   // may be null, not checked below
    h->dx = (double)(ex - sx) / dsteps;
    helper->dy = (double)(ey - sy) / dsteps;
    helper->dz = (double)(ez - sz) / dsteps;
}
HOOK_CTOR(0x00438150, CMissileGump, (Pos, Pos, uint16_t, uint8_t, uint8_t, uint8_t));

// 0x00438530  size 0x420
// Missile from world (x,y,z) to a target object's rect centre. SEH frame handler 0x0049a189 omitted.
CMissileGump::CMissileGump(Pos start, CObj *tgt, uint16_t art, uint8_t speed, uint8_t noRotate, uint8_t flag)
{
    int sx = start.x, sy = start.y, sz = start.z;

    sequence = nullptr;
    target = tgt;
    destX = RAWF(int16_t, tgt, 0x20);
    destY = RAWF(int16_t, tgt, 0x22);
    destZ = RAWF(int16_t, tgt, 0x24);

    x = (g_scrollBase - sy + sx) * 22 - g_scrollX;
    y = (sy + sx) * 22 - sz * 4 - g_scrollY + 0x2b;
    posX = (double)x;
    posY = (double)y;

    destScreenX = ((RAWF(int, tgt, 0xc) + RAWF(int, tgt, 0x4)) >> 1) - g_scrollX;
    destScreenY = ((RAWF(int, tgt, 0x10) + RAWF(int, tgt, 0x8)) >> 1) - g_scrollY;

    velX = (double)(destScreenX - x);
    velY = (double)(destScreenY - y);
    if (noRotate != 0)
        angle = 0;
    else
        angle = heading(this);

    stepSpeed = (uint8_t)((speed + 3) << 1);
    lifetime = 0;
    delayCounter = 0;
    frameDelay = 1;

    setup_art(this, art, noRotate, true);
    flying_flags(this, noRotate, flag);
    clear_links(this);

    CMissileHelper *h = new CMissileHelper(this, start.x, start.y, start.z);
    helper = h;   // may be null, not checked below
    h->dx = (double)(destX - sx);
    helper->dy = (double)(destY - sy);
    helper->dz = (double)(destZ - sz);

    scale_to_target(this);
}
HOOK_CTOR(0x00438530, CMissileGump, (Pos, CObj *, uint16_t, uint8_t, uint8_t, uint8_t));

// 0x00438950  size 0x39f
// Missile from a source object's rect centre to world (x,y,z). SEH frame handler 0x0049a1e1 omitted.
CMissileGump::CMissileGump(CObj *src, Pos end, uint16_t art, uint8_t speed, uint8_t noRotate, uint8_t flag)
{
    int ex = end.x, ey = end.y, ez = end.z;

    sequence = nullptr;

    int16_t srcX = RAWF(int16_t, src, 0x20);   // copy of the source world position
    int16_t srcY = RAWF(int16_t, src, 0x22);
    int16_t srcZ = RAWF(int16_t, src, 0x24);

    x = ((RAWF(int, src, 0xc) + RAWF(int, src, 0x4)) >> 1) - g_scrollX;
    y = ((RAWF(int, src, 0x8) + RAWF(int, src, 0x10)) >> 1) - g_scrollY;
    destScreenX = (g_scrollBase + ex - ey) * 22 - g_scrollX;
    destScreenY = (ey + ex) * 22 - ez * 4 - g_scrollY + 0x2b;

    int dx = destScreenX - x;
    int dy = destScreenY - y;
    int steps = (int)sqrt((double)(dx * dx + dy * dy)) / (speed * 2 + 10);
    double dsteps = (double)steps;
    velX = (double)dx / dsteps;
    dy = destScreenY - y;
    velY = (double)dy / dsteps;
    posX = (double)x;
    posY = (double)y;

    if (noRotate != 0)
        angle = 0;
    else
        angle = heading(this);
    delayCounter = 0;
    lifetime = (uint8_t)((char)steps + 1);
    stepSpeed = (uint8_t)((speed + 3) << 1);
    frameDelay = 1;
    target = nullptr;
    destX = end.x;
    destY = end.y;
    destZ = end.z;

    setup_art(this, art, noRotate, true);
    flying_flags(this, noRotate, flag);
    clear_links(this);

    CMissileHelper *h = new CMissileHelper(this, srcX, srcY, srcZ);
    helper = h;   // may be null, not checked below
    h->dx = (double)(ex - srcX) / dsteps;
    helper->dy = (double)(ey - srcY) / dsteps;
    helper->dz = (double)(ez - srcZ) / dsteps;
}
HOOK_CTOR(0x00438950, CMissileGump, (CObj *, Pos, uint16_t, uint8_t, uint8_t, uint8_t));

// 0x00438cf0  size 0x429
// Missile from a source object's rect centre to a target object's rect centre.
// SEH frame handler 0x0049a221 omitted.
CMissileGump::CMissileGump(CObj *src, CObj *dst, uint16_t art, uint8_t speed, uint8_t noRotate, uint8_t flag)
{
    sequence = nullptr;

    x = ((RAWF(int, src, 0x4) + RAWF(int, src, 0xc)) >> 1) - g_scrollX;
    y = ((RAWF(int, src, 0x8) + RAWF(int, src, 0x10)) >> 1) - g_scrollY;
    target = dst;
    destX = RAWF(int16_t, dst, 0x20);
    destY = RAWF(int16_t, dst, 0x22);
    destZ = RAWF(int16_t, dst, 0x24);
    destScreenX = ((RAWF(int, dst, 0x4) + RAWF(int, dst, 0xc)) >> 1) - g_scrollX;
    destScreenY = ((RAWF(int, dst, 0x8) + RAWF(int, dst, 0x10)) >> 1) - g_scrollY;

    velX = (double)(destScreenX - x);
    velY = (double)(destScreenY - y);
    posX = (double)x;
    posY = (double)y;
    if (noRotate != 0)
        angle = 0;
    else
        angle = heading(this);

    lifetime = 0;
    delayCounter = 0;
    stepSpeed = (uint8_t)((speed + 3) << 1);
    frameDelay = 1;

    setup_art(this, art, noRotate, true);
    flying_flags(this, noRotate, flag);
    clear_links(this);

    int16_t srcX = RAWF(int16_t, src, 0x20);
    int16_t srcY = RAWF(int16_t, src, 0x22);
    int16_t srcZ = RAWF(int16_t, src, 0x24);
    CMissileHelper *h = new CMissileHelper(this, srcX, srcY, srcZ);
    helper = h;   // may be null, not checked below
    h->dx = (double)(destX - srcX);
    helper->dy = (double)(destY - srcY);
    helper->dz = (double)(destZ - srcZ);

    scale_to_target(this);
}
HOOK_CTOR(0x00438cf0, CMissileGump, (CObj *, CObj *, uint16_t, uint8_t, uint8_t, uint8_t));

// 0x00439120  size 0x23c
// Stationary effect at world xyz. SEH frame handler 0x0049a269 omitted.
// p4 selects the frame delay (0x0b - p4), p5 the lifetime.
CMissileGump::CMissileGump(Pos pos, uint16_t art, int8_t p4, uint8_t p5, int8_t flagA, int8_t flagB)
{
    int px = pos.x;
    int py = pos.y;
    sequence = nullptr;
    int sx = (g_scrollBase + px) - py;
    x = sx * 0x16 - g_scrollX;
    int pz = pos.z;
    int sy = py + px;
    lifetime = p5;
    y = (sy * 0x16 - pz * 4) - g_scrollY + 0x2b;
    angle = 0;
    frameDelay = (uint8_t)(0x0b - p4);
    delayCounter = 0;
    stepSpeed = 0;

    setup_art(this, art, 0, false);
    randomFrame = 0;
    backward = (flagA != 0);
    unk_f4 = 0;
    spawnImpact = 0;
    spin = 0;
    pingPong = (flagB != 0);
    attached = 1;
    destX = 0;
    destY = 0;
    destZ = 0;
    target = nullptr;
    nextEffect = nullptr;
    clear_links(this);

    CMissileHelper *h = new CMissileHelper(this, pos.x, pos.y, pos.z);
    helper = h;   // null when the allocation failed
}
HOOK_CTOR(0x00439120, CMissileGump, (Pos, uint16_t, int8_t, uint8_t, int8_t, int8_t));

// 0x004394f0  size 0x1ec
// Fixed effect at an object's position (art 0x4e20). SEH frame handler 0x0049a2b1 omitted.
CMissileGump::CMissileGump(CObj *obj)
{
    sequence = nullptr;
    angle = 0;
    lifetime = 6;
    frameDelay = 1;
    delayCounter = 0;
    stepSpeed = 0;
    firstFrame = 0x4e20;
    lastFrame = 0x4e29;
    curFrame = (uint16_t)(RandomBelow(10) + firstFrame);
    randomFrame = 1;
    rotateToHeading = 0;
    pingPong = 0;
    backward = 0;
    unk_f4 = 0;
    spawnImpact = 0;
    spin = 0;
    tiledataAnim = 0;
    attached = 1;
    target = nullptr;
    destX = RAWF(int16_t, obj, 0x20);
    destY = RAWF(int16_t, obj, 0x22);
    destZ = RAWF(int16_t, obj, 0x24);
    nextEffect = nullptr;
    int wx = destX;
    int wy = destY;
    x = ((g_scrollBase + wx) - wy) * 0x16 - g_scrollX;
    name = const_cast<char *>("missile gump");
    dirty = 0;
    unk_50 = 0;
    unk_60 = 0;
    firstChild = nullptr;
    int wz = destZ;
    parent = nullptr;
    lastChild = nullptr;
    y = (wx + wy) * 0x16 - wz * 4 - g_scrollY;   // no +0x2b here
    next = nullptr;
    prev = nullptr;

    CMissileHelper *h = new CMissileHelper(this, destX, destY, destZ);
    helper = h;   // null when the allocation failed
}
HOOK_CTOR(0x004394f0, CMissileGump, (CObj *));

// 0x004396e0  size 0x10e
// Destructor body. SEH frame handler 0x0049a2d6 omitted.
CMissileGump::~CMissileGump()
{
    CMissileHelper *h = helper;
    if (h != nullptr) {
        PORT_VSLOT1(h, 0x00, void, int)(1);   // deleting dtor
        helper = nullptr;
    }
    if (attached != 0) {
        CObj *ownerObj = target;
        if (ownerObj != nullptr) {
            // unlink from the owner's effect list at owner+0x1c0
            CMissileGump *p = RAWF(CMissileGump *, ownerObj, 0x1c0);
            if (p == this) {
                RAWF(CMissileGump *, ownerObj, 0x1c0) = p->nextEffect;
            } else {
                CMissileGump *n = p->nextEffect;
                while (n != this) {
                    p = n;
                    n = p->nextEffect;
                }
                CMissileGump *victim = p->nextEffect;
                p->nextEffect = victim->nextEffect;
            }
            CObj *o = target;
            decomp::vcall_void(o, 0x14);
            o = target;
            decomp::vcall_void(o, 0x10);
        }
    }
    CEffectSequence *fd = sequence;
    if (fd != nullptr) {
        SeqNode *nxt = fd->current->next;
        if (nxt != nullptr) {
            fd->current = nxt;
            fd->Step(nxt);
        } else {
            uint8_t cnt = (uint8_t)(fd->repeat - 1);
            fd->repeat = cnt;
            if (cnt != 0) {
                SeqNode *first = fd->head;
                fd->current = first;
                fd->Step(first);
            } else {
                PORT_VSLOT1(fd, 0x00, void, int)(1);   // deleting dtor
            }
        }
    }
}
HOOK_DTOR(0x004396e0, CMissileGump);

// 0x004397f0  size 0x68c
// Per-frame update (virtual). SEH frame handler 0x0049a2fe omitted. The
// x87 stack sequences of the original are plain double arithmetic here.
void __thiscall CMissileGump::Update()
{
    CMissileHelper *h0 = helper;
    uint16_t oldFrame = curFrame;
    CObj *tgt;

    if (h0 == nullptr) {
        if (target == nullptr)
            goto destroy;
        if (attached != 0)
            goto animate;
        goto destroy;
    }
    if (attached != 0)
        goto animate;

    tgt = target;
    if (tgt == nullptr) {
        // straight flight: advance the sub-pixel position by the velocity
        double nx = velX + posX;
        double ny = velY + posY;
        posX = nx;
        posY = ny;
        x = (int)nx;
        y = (int)ny;
        lifetime = (uint8_t)(lifetime - 1);
        if (lifetime != 0)
            goto animate;
        if (spawnImpact == 0)
            goto destroy;
        {
            Pos p = {destX, destY, destZ, 0};
            CMissileGump *g = new CMissileGump(p, 0x36b1, 10, 0xd, 0, 0);
            CGump_AddChild(g, nullptr, 1);   // also when the allocation failed
        }
        goto destroy;
    }

    // homing on an object: re-aim if the object moved
    if (destX != RAWF(int16_t, tgt, 0x20) || destY != RAWF(int16_t, tgt, 0x22) || destZ != RAWF(int16_t, tgt, 0x24)) {
        destX = RAWF(int16_t, tgt, 0x20);
        destY = RAWF(int16_t, tgt, 0x22);
        destZ = RAWF(int16_t, tgt, 0x24);
        int cx = (RAWF(int, tgt, 4) + RAWF(int, tgt, 0xc)) >> 1;
        destScreenX = cx;
        destScreenX = cx - g_scrollX;
        int cy = (RAWF(int, tgt, 0x10) + RAWF(int, tgt, 8)) >> 1;
        destScreenY = cy;
        destScreenY = cy - g_scrollY;

        CMissileHelper *hh = helper;
        hh->dx = (double)(int)destX - hh->wx;
        hh = helper;
        hh->dy = (double)(int)destY - hh->wy;
        hh = helper;
        hh->dz = (double)(int)destZ - hh->wz;

        velX = (double)destScreenX - posX;
        velY = (double)destScreenY - posY;
        double vx = velX;
        double vy = velY;
        double len = sqrt(vx * vx + vy * vy);
        velX = vx / len;
        velY = vy / len;

        double spd = (double)(int)stepSpeed;
        double sx = spd * velX;
        double sy = spd * velY;
        double dist = (double)destScreenX - posX;
        velX = sx;
        velY = sy;

        hh = helper;
        hh->dx = (sx * hh->dx) / dist;
        hh = helper;
        hh->dy = (hh->dy * velX) / ((double)destScreenX - posX);
        hh = helper;
        hh->dz = (hh->dz * velX) / ((double)destScreenX - posX);

        if (rotateToHeading != 0)
            angle = heading(this);
    }

    {
        int ddx = destScreenX - x;
        if (ddx < 0)
            ddx = -ddx;
        if (ddx > 0x18)
            goto advance;
        int ddy = destScreenY - y;
        if (ddy < 0)
            ddy = -ddy;
        if (ddy > 0x18)
            goto advance;
    }

    // arrived
    if (spawnImpact == 0)
        goto destroy;
    {
        Pos p = {destX, destY, (int16_t)(destZ + 1), 0};
        CMissileGump *g = new CMissileGump(p, 0x36cb, 10, 9, 0, 0);
        CGump_AddChild(g, nullptr, 1);
    }
    goto destroy;

advance:
    {
        double nx = velX + posX;
        double ny = velY + posY;
        posX = nx;
        posY = ny;
        x = (int)nx;
        y = (int)ny;
    }

animate:
    {
        uint8_t tick = (uint8_t)(delayCounter + 1);
        delayCounter = tick;
        if (tick >= frameDelay) {
            delayCounter = 0;
            if (tiledataAnim != 0) {
                // tiledata animation
                uint16_t art = firstFrame;
                unsigned idx = (unsigned)art * 0x48;
                uint8_t delay = g_tileAnim[idx + 0x42];
                lastFrame = (uint16_t)(lastFrame + 1);
                frameDelay = delay;
                if (lastFrame >= (uint16_t)g_tileAnim[idx + 0x41])
                    lastFrame = 0;
                curFrame = (uint16_t)((int16_t)(int8_t)g_tileAnim[idx + lastFrame] + art);
            } else if (randomFrame != 0) {
                unsigned span = (unsigned)lastFrame - (unsigned)firstFrame + 1;
                curFrame = (uint16_t)((int16_t)RandomBelow((int)span) + (int16_t)firstFrame);
            } else if (backward != 0) {
                curFrame = (uint16_t)(curFrame - 1);
                if (curFrame < firstFrame) {
                    if (pingPong != 0) {
                        backward = 0;
                        curFrame = (uint16_t)(firstFrame + 1);
                    } else {
                        curFrame = lastFrame;
                    }
                }
            } else {
                curFrame = (uint16_t)(curFrame + 1);
                if (curFrame > lastFrame) {
                    if (pingPong != 0) {
                        backward = 1;
                        curFrame = (uint16_t)(lastFrame - 1);
                    } else {
                        curFrame = firstFrame;
                    }
                }
            }
        }
    }
    if (spin != 0)
        angle = (uint8_t)(angle + 0x14);

    if (attached == 0) {
        // flying with a world-space helper: step its position by its velocity
        CMissileHelper *h = helper;
        h->wx = h->dx + h->wx;
        h = helper;
        h->wy = h->dy + h->wy;
        h = helper;
        h->wz = h->dz + h->wz;
        h = helper;
        decomp::vcall_void(h, 0x14);
        sync_helper(h);
        decomp::vcall_void(h, 0x10);
        return;
    }

    lifetime = (uint8_t)(lifetime - 1);
    if (lifetime == 0)
        goto destroy;
    if (oldFrame == curFrame)
        return;
    {
        CMissileHelper *h = helper;
        if (h != nullptr) {
            decomp::vcall_void(h, 0x14);
            sync_helper(h);
            decomp::vcall_void(h, 0x10);
            return;
        }
        CObj *o = target;
        decomp::vcall_void(o, 0x14);
        o = target;
        decomp::vcall_void(o, 0x10);
        return;
    }

destroy:
    PORT_VSLOT1(this, 0x00, void, int)(1);   // deleting dtor, slot 0 ("this != 0" test dropped)
}
HOOK_VMETHOD(0x004397f0, CMissileGump, Update);
