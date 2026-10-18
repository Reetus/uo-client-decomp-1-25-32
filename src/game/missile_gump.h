// CMissileGump: a projectile / spell-effect sprite drawn on the world view
// ("missile gump", vtable 0x0049ec28, 0x108 bytes), and CMissileHelper, the
// 0x70-byte world-position object (vtable 0x0049ebd0) that tracks the effect
// in world coordinates. See decomp/pass2/0x00438150.cpp and friends.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"
#include "vtables.h"
#include "world_obj.h"

class CObj;
class CEffectSequence;
class CMissileGump;

// World position passed by value: three shorts, 8 bytes on the stack.
struct Pos {
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t pad;
};
static_assert(sizeof(Pos) == 8, "Pos size");

// Vtable 0x0049ebd0, derived from CWorldObj (base ctor 0x0042ad20, base dtor 0x0042ad70). The screen rect and the
// in-world flag are the base's rectX0.. / inWorld.
class CMissileHelper : public CWorldObj {
public:
    virtual uint32_t __thiscall TileFlags();   // slot 0x40: 0x00438140 (was Const4_00438140), returns 4
    virtual int __thiscall Slot34() { return 1; }
    // The constructor is expanded inside every CMissileGump constructor in the original (defined in missile_gump.cpp).
    // Sets the position, registers the object in the world (0x0042b3e0 through the AddToWorld slot).
    CMissileHelper(CMissileGump *owner, int16_t hx, int16_t hy, int16_t hz);
    // 0x00437cf0: destructor (slot 0 is the scalar deleting destructor 0x00437cc0).
    virtual ~CMissileHelper();
    // 0x00437d60: screen rect of the owner's current frame (art or tile), rotated by its angle and scrolled.
    virtual void __thiscall ComputeScreenRect();
    // 0x00437f60: draw the effect's current frame at the helper's screen rect (vtable slot +0x08).
    virtual void __thiscall Draw(int *clip);

    uint8_t pad_34[4];
    CMissileGump *owner;      // +0x38
    uint8_t pad_3c[4];
    double wx;                // +0x40  world position (sub-tile)
    double wy;                // +0x48
    double wz;                // +0x50
    double dx;                // +0x58  per-step world delta
    double dy;                // +0x60
    double dz;                // +0x68
};

static_assert(offsetof(CMissileHelper, owner) == 0x38, "CMissileHelper::owner");
static_assert(offsetof(CMissileHelper, wx) == 0x40, "CMissileHelper::wx");
static_assert(offsetof(CMissileHelper, dx) == 0x58, "CMissileHelper::dx");
static_assert(offsetof(CMissileHelper, dz) == 0x68, "CMissileHelper::dz");
static_assert(sizeof(CMissileHelper) == 0x70, "CMissileHelper size");

class CMissileGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    virtual int IsTagf0() { return 1; }
    // slot override (gump virtual-slot prep): own copy of the shared body
    virtual void PreDraw(int *clip);
    virtual void Draw(int *clip);
    // Constructors (object size 0x108). They are told apart by the argument types: a world position (Pos) or an
    // object (CObj *) for each end, the stationary and attached effects by their trailing parameters.
    // 0x00438150  size 0x3a7  from world (x,y,z) to world (x,y,z). RET 0x1c.
    CMissileGump(Pos start, Pos end, uint16_t art, uint8_t speed, uint8_t noRotate, uint8_t flag);
    // 0x00438530  size 0x420  from world (x,y,z) to a target object's rect centre.
    CMissileGump(Pos start, CObj *target, uint16_t art, uint8_t speed, uint8_t noRotate, uint8_t flag);
    // 0x00438950  from a source object's rect centre to world (x,y,z).
    CMissileGump(CObj *src, Pos end, uint16_t art, uint8_t speed, uint8_t noRotate, uint8_t flag);
    // 0x00438cf0  source object's rect centre to target object's rect centre.
    CMissileGump(CObj *src, CObj *dst, uint16_t art, uint8_t speed, uint8_t noRotate, uint8_t flag);
    // 0x00439120  size 0x23c  stationary effect at world xyz.
    CMissileGump(Pos pos, uint16_t art, int8_t p4, uint8_t p5, int8_t flagA, int8_t flagB);
    // 0x004394f0  size 0x1ec  fixed effect at an object's position (art 0x4e20).
    explicit CMissileGump(CObj *obj);
    // 0x004396e0: destructor (deletes the helper, unlinks from the owner's effect list, advances the sequence, then the base).
    ~CMissileGump();
    void Dtor() { this->CMissileGump::~CMissileGump(); }
    // 0x00439e80: vtable slot 0x0c, RET 8: hit test that never hits (returns 0).
    virtual int HitTest(int x, int y);
    // 0x00439360  size 0x187  effect attached to an object (appended to the object's effect list at +0x1c0).
    CMissileGump(CObj *obj, uint16_t art, uint8_t speed, uint8_t lifetime, uint8_t flagA, uint8_t flagB);
    // 0x004397f0: per-frame update (virtual).
    virtual void Update();

    int16_t destX;            // +0x94  destination world position
    int16_t destY;            // +0x96
    int16_t destZ;            // +0x98
    uint8_t pad_9a[2];
    CObj *target;             // +0x9c  object the effect is attached to / homing on
    CMissileHelper *helper;   // +0xa0
    int32_t destScreenX;      // +0xa4
    int32_t destScreenY;      // +0xa8
    uint8_t pad_ac[4];
    double velX;              // +0xb0
    double velY;              // +0xb8
    double posX;              // +0xc0  sub-pixel screen position
    double posY;              // +0xc8
    uint8_t angle;            // +0xd0
    uint8_t lifetime;         // +0xd1
    uint8_t frameDelay;       // +0xd2
    uint8_t delayCounter;     // +0xd3
    uint8_t stepSpeed;        // +0xd4
    uint8_t pad_d5;
    uint16_t firstFrame;      // +0xd6
    uint16_t lastFrame;       // +0xd8  last frame, or tiledata animation index
    uint16_t curFrame;        // +0xda
    int32_t spin;             // +0xdc
    int32_t spawnImpact;      // +0xe0
    int32_t rotateToHeading;  // +0xe4
    int32_t attached;         // +0xe8  1 attached to target, 0 flying
    int32_t backward;         // +0xec
    int32_t pingPong;         // +0xf0
    int32_t unk_f4;           // +0xf4
    int32_t tiledataAnim;     // +0xf8
    int32_t randomFrame;      // +0xfc
    CMissileGump *nextEffect; // +0x100  link in the owner's effect list (owner+0x1c0)
    CEffectSequence *sequence; // +0x104
};

static_assert(offsetof(CMissileGump, destX) == 0x94, "CMissileGump::destX");
static_assert(offsetof(CMissileGump, target) == 0x9c, "CMissileGump::target");
static_assert(offsetof(CMissileGump, helper) == 0xa0, "CMissileGump::helper");
static_assert(offsetof(CMissileGump, destScreenX) == 0xa4, "CMissileGump::destScreenX");
static_assert(offsetof(CMissileGump, velX) == 0xb0, "CMissileGump::velX");
static_assert(offsetof(CMissileGump, posX) == 0xc0, "CMissileGump::posX");
static_assert(offsetof(CMissileGump, posY) == 0xc8, "CMissileGump::posY");
static_assert(offsetof(CMissileGump, angle) == 0xd0, "CMissileGump::angle");
static_assert(offsetof(CMissileGump, stepSpeed) == 0xd4, "CMissileGump::stepSpeed");
static_assert(offsetof(CMissileGump, firstFrame) == 0xd6, "CMissileGump::firstFrame");
static_assert(offsetof(CMissileGump, curFrame) == 0xda, "CMissileGump::curFrame");
static_assert(offsetof(CMissileGump, spin) == 0xdc, "CMissileGump::spin");
static_assert(offsetof(CMissileGump, attached) == 0xe8, "CMissileGump::attached");
static_assert(offsetof(CMissileGump, tiledataAnim) == 0xf8, "CMissileGump::tiledataAnim");
static_assert(offsetof(CMissileGump, randomFrame) == 0xfc, "CMissileGump::randomFrame");
static_assert(offsetof(CMissileGump, nextEffect) == 0x100, "CMissileGump::nextEffect");
static_assert(offsetof(CMissileGump, sequence) == 0x104, "CMissileGump::sequence");
static_assert(sizeof(CMissileGump) == 0x108, "CMissileGump size (operator new(0x108), all seven constructors)");

// Vtable of the missile gump (0x0049ec28) and of the helper (0x0049ebd0).
#define kMissileGumpVtable reinterpret_cast<uintptr_t>(vtbl_CMissileGump)
