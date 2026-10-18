// World object hierarchy: CWorldObj (0x34, world_obj.h) -> CObj (0x78) -> CDynamicObj (0xa8) -> CContainerObj
// -> CMobile (0x20c). Layouts from decomp/pass2 (0x004514e0, 0x00451c70,
// 0x00451f70, 0x00452580, 0x004526e0, 0x004539e0, 0x00453ef0, 0x00454210) and
// the allocation sites (operator new 0x78 pool slab, 0x20c player mobile).
//
// None of the classes declares virtual functions: objects keep their original
// vtable pointer (vtables: CObj 0x0049f7e0, CDynamicObj 0x0049f860,
// CContainerObj 0x0049f8e0, CMobile 0x0049fa60) and virtual calls go through
// decomp::vcall until the slot tables are recovered.
//
// Names follow the pass2 comments (CObj::ToWorld, CObj::ToContainer,
// CContainerObj, CMobile); CDynamicObj is the Linux-symbol spelling for the
// 0xa8-byte class. Treat them as working names.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"
#include "gump.h"
#include "terrain_tile.h"
#include "world_obj.h"
#include "globals.h"

class CDynamicObj;
class CContainerObj;
class CMobile;

// ---------------------------------------------------------------------------
// CObj: the 0x78-byte pooled object (tile / static / part of a multi).
// ---------------------------------------------------------------------------
class CObj : public CWorldObj {
public:
    // CWorldObj (world_obj.h) provides +0x00 .. +0x33: vtable (0x0049f7e0 for CObj), the bucket link
    // `next` (+0x14, the 64x64 tile bucket table at 0x0069fff8), `inWorld` (+0x1c), x/y/z (+0x20..+0x24),
    // key28 (+0x28) and sumXY (+0x2c).
    uint8_t pad_34[4];
    uint16_t graphic;       // +0x38
    uint8_t unk_3a;
    uint8_t pad_3b;
    uint16_t unk_3c;
    uint16_t hue;           // +0x3e
    CObj *link40;           // +0x40  world list (head DAT_0068ce5c, tail DAT_0068ce58); also next in a container
    CObj *link44;           // +0x44  see ToWorld / ToContainer: which of link40/link44 is "next" differs per list
    CObj *link48;           // +0x48  light list (head DAT_0068ce60, tail DAT_0068ce64)
    CObj *link4c;           // +0x4c
    CObj *link50;           // +0x50  list of objects with tiledata flag 0x1000000 (head DAT_0068ce78)
    CObj *link54;           // +0x54
    CObj *link58;           // +0x58  list of objects with tiledata flag 0x20000 (head DAT_0068ce68, tail DAT_0068ce6c)
    CObj *link5c;           // +0x5c
    int32_t unk_60;         // +0x60
    int32_t unk_64;         // +0x64
    void *barkList;         // +0x68  first bark gump (next at gump+0xac, prev at gump+0xb0); also a child list in the dtor
    uint8_t unlinked;       // +0x6c  0 while the object is linked into the world / a container
    uint8_t onLightList;    // +0x6d  1 once linked into the light list
    uint8_t facing;         // +0x6e  facing (mobiles) / light index (lights)
    uint8_t pad_6f;
    int32_t unk_70;         // +0x70
    uint8_t flags74;        // +0x74  bit 0: registered in list DAT_0068ce70/74
    uint8_t pad_75[3];

    // 0x00451be0  size 0x6d  constructor (inline in the original: the 0xa8-byte class has it inlined). obj_inline.h.
    CObj();
    // 0x00451c70  size 0x94  destructor of the 0x78-byte pooled object (inline too). obj_inline.h. Slot 0 is the vector
    // deleting destructor 0x00451640.
    virtual ~CObj();
    // The old names, for callers that take an address or still construct in place (obj_ctor_pool_ops.cpp).
    void __thiscall Dtor() { this->~CObj(); }
    // 0x00452580  size 0x160  link into the world lists and set x/y/z. xy = x | y << 16.
    void __thiscall ToWorld(uint32_t xy, uint16_t z);
    // 0x00453320  size 0xae  create a bark gump, keeping at most two existing ones.
    void *__thiscall CreateBarkKeepTwo(const char *text, uint16_t barkHue, uint16_t barkFont, CGump *param5);
    // 0x004533d0  size 0x87  create a bark gump after deleting all existing ones.
    void *__thiscall CreateBarkReplace(const char *text, uint16_t barkHue, uint16_t barkFont, CGump *param5);

    // Constructs in place with the real constructor; returns this.
    CObj *__thiscall Ctor();
    // 0x00451d10  size 0x69  give the object back to the pool: unlink, delete its bark gumps.
    virtual void __thiscall DeleteSelf();   // slot 0x04
    // 0x00452220  size 0x2c  push front onto the list DAT_0068ce70/74 (next at +0x60, prev at +0x64).
    void __thiscall LinkList70();
    // 0x00452250  size 0x3c  unlink from that list.
    void __thiscall UnlinkList70();
    // 0x00452af0  size 0x246  take the object out of the world, its container or its owner's equipment.
    virtual void __thiscall ToVoid();   // slot 0x50
    // 0x00452d40  size 0xc8  equip this object on `mobile` at `layer`. RET 8.
    void __thiscall ToEquipped(CMobile *mobile, int layer);
    // 0x00453190  size 0x2d  display name of this object (static buffer of BuildItemName).
    char *__thiscall ItemName();
    // 0x004531c0  size 0x92  art record of this object (stack-size dependent art id); null above 0x4000.
    // 0x004532c0  size 0x39  tiledata flag 0x800 set, not a "has outline" object and quantity above 1.
    // Virtual slots of the CObj vtable that all the subclasses share (third pass, batch u3). Names from behaviour.
    // 0x00448840  size 0x29  vtable slot 0x44: z + the height of the object (virtual slot 0x6c, halved when flags slot 0x40 has 0x400).
    // The original leaves the high half of EAX as the flags / height left it; only the low 16 bits are the sum.
    virtual uint32_t __thiscall GetTopZ();   // slot 0x44
    // 0x00451c50  size 0x7  vtable slot 0x64: bit 0 of flags74 (registered in the list 0x0068ce70/74).
    // 0x00451c60  size 0x5  vtable slot 0x68: set bit 0 of flags74.
    // 0x00453260  size 0x1e  vtable slot 0x6c: height, the byte at +0x12 of this graphic's tiledata record.
    // 0x00453280  size 0x18  vtable slot 0x40: first dword (flags) of this graphic's tiledata record.
    virtual uint32_t __thiscall TileFlags();
    // 0x004532a0  size 0x1f  vtable slot 0x70: the word at +0x10 of this graphic's tiledata record.
    // 0x00453300  size 0x1d  vtable slot 0x7c: graphic is 0x28e or 0xeed.
    // The overrides of the CObj family (docs/matching.md "World-object virtual slots"); the bodies are in world_obj_ops.cpp.
    virtual void __thiscall Draw(int *clip);                // slot 0x08, 0x0042c730
    virtual void __thiscall ComputeScreenRect();            // slot 0x0c, 0x0042c690
    virtual void __thiscall AddToWorld();                   // slot 0x10, 0x0042b4f0 (links into the bucket)
    virtual int __thiscall Slot18() { return 1; }
    virtual int __thiscall Slot2c() { return 0; }
    virtual int __thiscall IsHidden();                      // slot 0x3c, 0x0042c130
    // Slots 0x54 .. 0x7c are new in CObj (declaration order = slot order). 0x00451c50 / 0x00451c60: bit 0 of flags74 (the
    // list 0x0068ce70/74); 0x00453260: height, the byte at +0x12 of the tiledata record; 0x004532a0: the word at +0x10 of the
    // record; 0x004531c0: art record (stack-size dependent art id), null above 0x4000; 0x004532c0: tiledata flag 0x800 set,
    // not a "has outline" object and quantity above 1; 0x00453300: graphic is 0x28e or 0xeed.
    virtual int __thiscall Slot54() { return 0; }
    virtual int __thiscall IsEquipped() { return 0; }
    virtual int __thiscall IsMovable() { return 0; }
    virtual int __thiscall Slot60() { return 0; }
    virtual int __thiscall IsOnList70();
    virtual void __thiscall SetOnList70Flag();
    virtual int __thiscall Height();
    virtual int __thiscall TileWord10();
    virtual void *__thiscall ArtRecord();
    virtual int __thiscall IsStackedDisplay();
    virtual int __thiscall IsGraphic28eOrEed();
};

static_assert(sizeof(CObj) == 0x78, "CObj size");
static_assert(offsetof(CObj, graphic) == 0x38, "CObj::graphic");
static_assert(offsetof(CObj, unk_3a) == 0x3a, "CObj::unk_3a");
static_assert(offsetof(CObj, unk_3c) == 0x3c, "CObj::unk_3c");
static_assert(offsetof(CObj, hue) == 0x3e, "CObj::hue");
static_assert(offsetof(CObj, link40) == 0x40, "CObj::link40");
static_assert(offsetof(CObj, link44) == 0x44, "CObj::link44");
static_assert(offsetof(CObj, link48) == 0x48, "CObj::link48");
static_assert(offsetof(CObj, link4c) == 0x4c, "CObj::link4c");
static_assert(offsetof(CObj, link50) == 0x50, "CObj::link50");
static_assert(offsetof(CObj, link54) == 0x54, "CObj::link54");
static_assert(offsetof(CObj, link58) == 0x58, "CObj::link58");
static_assert(offsetof(CObj, link5c) == 0x5c, "CObj::link5c");
static_assert(offsetof(CObj, unk_60) == 0x60, "CObj::unk_60");
static_assert(offsetof(CObj, unk_64) == 0x64, "CObj::unk_64");
static_assert(offsetof(CObj, barkList) == 0x68, "CObj::barkList");
static_assert(offsetof(CObj, unlinked) == 0x6c, "CObj::unlinked");
static_assert(offsetof(CObj, onLightList) == 0x6d, "CObj::onLightList");
static_assert(offsetof(CObj, facing) == 0x6e, "CObj::facing");
static_assert(offsetof(CObj, unk_70) == 0x70, "CObj::unk_70");
static_assert(offsetof(CObj, flags74) == 0x74, "CObj::flags74");

// Part list of a multi object (CDynamicObj::partList): see part_list.h.
struct PartList;

// ---------------------------------------------------------------------------
// CDynamicObj: the 0xa8-byte class (items). Constructor 0x00451d80.
// ---------------------------------------------------------------------------
class CDynamicObj : public CObj {
public:
    int32_t serial;         // +0x78
    CObj *container;        // +0x7c  owner object (item in a container / carried)
    CDynamicObj *listNext;  // +0x80  global list head DAT_0068ce7c, tail DAT_0068ce80
    CDynamicObj *listPrev;  // +0x84
    CGump *gump88;          // +0x88  gump whose owner (+0x34) is this
    CGump *gump8c;          // +0x8c
    uint8_t pad_90[4];
    PartList *partList;     // +0x94  payload freed by the destructor (BuildPartList result)
    uint8_t flags98;        // +0x98  bit 7 read by the footstep code
    uint8_t pad_99[3];
    int32_t light;          // +0x9c  light level added to DAT_00721030 for the player
    void *buf_a0;           // +0xa0  freed by the destructor
    int32_t unk_a4;         // +0xa4

    // 0x00451d80  size 0xe2  constructor: CObj base constructor (inlined), vtable 0x0049f860, links into the global list.
    CDynamicObj();
    // 0x00451f70  size 0x1f7  destructor of the 0xa8-byte class (CObj's inlined).
    virtual ~CDynamicObj();   // slot 0 is the vector deleting destructor 0x00451800
    void __thiscall Dtor() { this->~CDynamicObj(); }
    // 0x004526e0  size 0x1b0  put this item into a container.
    void __thiscall ToContainer(CContainerObj *cont, uint32_t xy, uint16_t z);
    // 0x00452890  size 0xea  sorted insert by x into a container.
    void __thiscall ToContainerSortX(CContainerObj *cont, uint32_t xy, uint16_t z);
    // 0x00452e10  size 0x10e  within 3 tiles and passes the line-of-sight test.
    int __thiscall IsReachable();   // returns 0 / 1 in EAX (callers test EAX)
    // 0x00452f20  size 0x12c  within `range` tiles of the view origin.
    int __thiscall IsInRange(int range);   // returns 0 / 1 in EAX (callers test EAX)

    // Constructs in place with the real constructor; returns this.
    CDynamicObj *__thiscall Ctor();
    // 0x00452980  size 0xa3  like ToContainerSortX but sorted by y. RET 0xc.
    void __thiscall ToContainerSortY(CContainerObj *cont, uint32_t xy, uint16_t z);
    // 0x00452a30  size 0xba  like ToContainerSortX but sorted by quantity (+0x3c). RET 0xc.
    void __thiscall ToContainerSortQty(CContainerObj *cont, uint32_t xy, uint16_t z);
    // 0x00453460  size 0x39  the container is a mobile and one of its 30 equipment slots holds this object.
    virtual int __thiscall IsEquipped();
    // 0x004534a0  size 0x3b  1 when 0x00478cd0 is nonzero, flag +0x98 & 0x20 is set or the tiledata byte +4 is <= 0x5a.
    virtual int __thiscall IsMovable();
    // 0x004534e0  size 0x3c0  take the item out of the world / its container / its owner and redraw what showed it.
    virtual void __thiscall ToVoid();   // slot 0x50
    // 0x004538a0  size 0x56  set the light level (+0x9c); for the player in the world adjusts DAT_00721030. RET 4.
    void __thiscall SetLight(int newLight);
    // 0x00451e70  size 0x100  vtable slot 0x04 of the 0x0049f860 class: give the item back to the dynamic pool
    // (unlink from the item list, delete its bark gumps, push onto the free list headed at 0x00721060).
    virtual void __thiscall DeleteSelf();   // slot 0x04
    virtual int __thiscall Slot2c() { return 1; }
};

static_assert(sizeof(CDynamicObj) == 0xa8, "CDynamicObj size");
static_assert(offsetof(CDynamicObj, serial) == 0x78, "CDynamicObj::serial");
static_assert(offsetof(CDynamicObj, container) == 0x7c, "CDynamicObj::container");
static_assert(offsetof(CDynamicObj, listNext) == 0x80, "CDynamicObj::listNext");
static_assert(offsetof(CDynamicObj, listPrev) == 0x84, "CDynamicObj::listPrev");
static_assert(offsetof(CDynamicObj, gump88) == 0x88, "CDynamicObj::gump88");
static_assert(offsetof(CDynamicObj, gump8c) == 0x8c, "CDynamicObj::gump8c");
static_assert(offsetof(CDynamicObj, partList) == 0x94, "CDynamicObj::partList");
static_assert(offsetof(CDynamicObj, flags98) == 0x98, "CDynamicObj::flags98");
static_assert(offsetof(CDynamicObj, light) == 0x9c, "CDynamicObj::light");
static_assert(offsetof(CDynamicObj, buf_a0) == 0xa0, "CDynamicObj::buf_a0");

// ---------------------------------------------------------------------------
// The class with vtable 0x0049f960, derived from CDynamicObj (dtor 0x00453bc0).
// ---------------------------------------------------------------------------
class CObjVt49f960 : public CDynamicObj {
public:
    void *list_a8;          // +0xa8  singly linked list, next at node+8
    int32_t unk_ac;         // +0xac
    CGump *obj_b0;          // +0xb0  deleted through virtual slot 0 (flags 1)

    // 0x00453bc0  size 0x9b  destructor
    ~CObjVt49f960();
    // 0x00453af0  size 0x21  scalar deleting destructor, RET 4.
    virtual void __thiscall DeleteSelf();   // slot 0x04, the root body
    virtual int __thiscall Slot60() { return 1; }
    // 0x00453b20  size 0x9f  constructor (serial, graphic, byte at +0x3a, flag); same shape as CContainerObj's. RET 0x10.
    CObjVt49f960(int serial, int graphic, uint8_t b3a, int flag);
    // Constructs in place (old name, for packet_handlers_*.cpp until they use `new`).
    CObjVt49f960 *__thiscall Ctor(int serial, int graphic, uint8_t b3a, int flag);
    // 0x00453c60  size 0x175  edit the list of (x, y) points at +0xa8 (count at +0xac):
    // cmd 1 append, 2 insert at index `idx` (0 = at the front), 3 set point `idx`, 4 remove point `idx`, 5 clear.
    // RET 0x10.
    void __thiscall ListOp(int cmd, int idx, int x, int y);
};
static_assert(sizeof(CObjVt49f960) == 0xb4, "CObjVt49f960 size (operator new 0xb4)");
static_assert(offsetof(CObjVt49f960, list_a8) == 0xa8, "CObjVt49f960::list_a8");
static_assert(offsetof(CObjVt49f960, obj_b0) == 0xb0, "CObjVt49f960::obj_b0");

// ---------------------------------------------------------------------------
// CContainerObj: vtable 0x0049f8e0, derived from CDynamicObj (dtor 0x004539e0).
// Allocated with operator new 0xbc; CMobile (0x1dc) derives from it.
// ---------------------------------------------------------------------------
class CContainerObj : public CDynamicObj {
public:
    CObj *firstChild;       // +0xa8  child list head (next at child+0x40, prev at +0x44)
    CGump *contGump;        // +0xac  the container gump showing this object
    CObj *list_b0;          // +0xb0  second list, drained through virtual slot +4
    uint8_t pad_b4[4];
    int32_t unk_b8;         // +0xb8  cleared by the constructors

    // Default constructor, inline in the original (expanded in the constructors of CMobile and the corpse class,
    // see obj_inline.h).
    CContainerObj();
    // 0x00453930  size 0xa5  constructor (serial, graphic, byte at +0x3a, flag); flag 0 takes it off the global list. RET 0x10.
    CContainerObj(int serial, int graphic, uint8_t b3a, int flag);
    // Constructs in place (old name, for packet_handlers_*.cpp until they use `new`).
    CContainerObj *__thiscall Ctor(int serial, int graphic, uint8_t b3a, int flag);
    // 0x004539e0  size 0xe3  destructor
    ~CContainerObj();
    // 0x00453900  size 0x21  scalar deleting destructor, RET 4.
    virtual void __thiscall DeleteSelf();   // slot 0x04, the root body
    virtual int __thiscall Slot30() { return 1; }
    // 0x00453ad0  size 0x1e  destroy every child of the +0xa8 list through virtual slot 4.
    void __thiscall DestroyChildren();
};
static_assert(offsetof(CContainerObj, firstChild) == 0xa8, "CContainerObj::firstChild");
static_assert(offsetof(CContainerObj, contGump) == 0xac, "CContainerObj::contGump");
static_assert(offsetof(CContainerObj, list_b0) == 0xb0, "CContainerObj::list_b0");
static_assert(offsetof(CContainerObj, unk_b8) == 0xb8, "CContainerObj::unk_b8");
static_assert(sizeof(CContainerObj) == 0xbc, "CContainerObj size (operator new 0xbc)");

// The class with vtable 0x0049f9e0, derived from CContainerObj (dtor 0x00453e90).
class CObjVt49f9e0 : public CContainerObj {
public:
    virtual void __thiscall ComputeScreenRect();   // 0x0042d0e0 (was Mobile_CalcRectSitting)
    virtual void __thiscall Draw(int *clip);   // 0x0042d3f0 (was Mobile_DrawClipSitting)
    virtual void __thiscall AddToWorld();   // 0x0042b620: depth = z + 1, then slice objects up the screen height
    virtual void __thiscall RemoveFromWorld();   // 0x0042aee0: first destroys the child list at +0xb0, bucket skewed by +0xb8
    virtual uint32_t __thiscall TileFlags() { return 0; }
    virtual int __thiscall Slot54() { return 1; }
    int32_t tail_bc[0x1a];  // +0xbc..+0x123  zeroed by the constructor

    // 0x00453e90  size 0x55  destructor
    ~CObjVt49f9e0();
    // 0x00453e00  size 0x21  scalar deleting destructor, RET 4.
    // 0x00453e30  size 0x59  constructor (serial, graphic); object size 0x124, +0xbc..+0x123 zeroed. RET 8.
    CObjVt49f9e0(int serial, int graphic);
    // Constructs in place (old name, for packet_handlers_*.cpp until they use `new`).
    CObjVt49f9e0 *__thiscall Ctor(int serial, int graphic);
    virtual int __thiscall Height();   // slot 0x6c: 0x00453df0, always 6
};
static_assert(sizeof(CObjVt49f9e0) == 0x124, "CObjVt49f9e0 size (operator new 0x124)");

// ---------------------------------------------------------------------------
// CMobile: vtable 0x0049fa60, 0x1dc bytes (operator new 0x1dc). The player object (0x00721190) is 0x20c
// bytes and derives from it (constructor 0x00454c30); +0x1dc..+0x20b belong to CPlayer.
// ---------------------------------------------------------------------------
struct MobileStep {         // 10-byte element of CMobile::steps (vector-constructed by 0x0040cf20, destroyed by 0x0040caf0)
    MobileStep() {}
    ~MobileStep() {}
    int16_t x;              // +0
    int16_t y;              // +2
    uint8_t pad_04[6];
};
static_assert(sizeof(MobileStep) == 10, "MobileStep size");

class CMobile : public CContainerObj {
public:
    virtual void __thiscall ComputeScreenRect();   // 0x0042d6d0 (was Mobile_CalcRect)
    virtual void __thiscall Draw(int *clip);   // 0x0042dc30 (was Mobile_DrawClip)
    virtual void __thiscall AddToWorld();   // 0x0042b810
    virtual void __thiscall RemoveFromWorld();   // 0x0042afb0: facing-dependent bucket when +0x13c is set
    virtual int __thiscall Slot24() { return 1; }
    virtual int __thiscall IsHidden();   // 0x0042c3b0: 1 when the cell is covered by a roof
    virtual uint32_t __thiscall TileFlags() { return 0; }
    virtual int __thiscall IsMovable() { return 0; }
    virtual int __thiscall Height();   // slot 0x6c: 0x00454050, always 0xf
    virtual int __thiscall TileWord10();   // slot 0x70: 0x00454700, always 1
    virtual void *__thiscall ArtRecord();   // slot 0x74: 0x00454710
    // Slots 0x80 .. 0x94 are new in CMobile.
    virtual int __thiscall HitTest(int px, int py);   // 0x00471a20 (was Mobile_HitTestCached)
    virtual int __thiscall AnimWordOrMinus1();   // 0x004545a0: the word at +0x1a8 when +0x1a4 is set, else -1
    virtual void __thiscall UpdateViewZ();   // 0x0044a170 non-player: view z for its walk phase
    virtual int __thiscall IsGhostBody();   // 0x00454ce0: graphic is 0x192 or 0x193
    virtual void __thiscall FootstepSound();   // 0x00454a10
    virtual void __thiscall SetUnk1d8(int v);   // 0x004544f0: set +0x1d8 and invalidate this object
    CMobile *listNext;      // +0xbc  global mobile list (head DAT_0068ce84)
    CMobile *listPrev;      // +0xc0
    CObj *equip[30];        // +0xc4  equipment by layer; [1] = weapon hand (0xc8), [25] = mount (0x128)
    int32_t unk_13c;        // +0x13c  walk frame counter of non-player mobiles
    int32_t moveTick;       // +0x140
    int32_t moving;         // +0x144
    int32_t flag148;        // +0x148  player: walking flag; footstep code: "mnt" argument
    int32_t unk_14c;        // +0x14c
    int32_t unk_150;        // +0x150
    MobileStep steps[4];    // +0x154  recent positions; entry n (1-based) is at 0x14a + n * 10
    int16_t fromX;          // +0x17c
    int16_t fromY;          // +0x17e
    int16_t fromZ;          // +0x180
    uint8_t pad_182[2];
    int32_t stepCount;      // +0x184
    int32_t unk_188;        // +0x188  (x pixel offset, read by LightPass::Apply)
    int32_t unk_18c;        // +0x18c  (y pixel offset)
    int32_t viewZ;          // +0x190
    int32_t unk_194;        // +0x194
    int32_t zMoving;        // +0x198
    int32_t zDelta;         // +0x19c
    uint8_t pad_1a0[4];
    int32_t unk_1a4;        // +0x1a4
    uint16_t unk_1a8;       // +0x1a8
    uint16_t animFrame;     // +0x1aa
    uint16_t unk_1ac;       // +0x1ac
    uint8_t pad_1ae[10];
    uint16_t unk_1b8;       // +0x1b8
    uint8_t pad_1ba[2];
    int32_t unk_1bc;        // +0x1bc
    CGump *gumpList1c0;     // +0x1c0  list of gumps, next at gump+0x100
    int32_t unk_1c4;        // +0x1c4
    int32_t unk_1c8;        // +0x1c8
    int32_t unk_1cc;        // +0x1cc
    int32_t unk_1d0;        // +0x1d0  -1 when none
    CGump *gump1d4;         // +0x1d4
    int32_t unk_1d8;        // +0x1d8

    // 0x00453ef0  size 0x15f  default constructor.
    CMobile();
    // 0x00454090  size 0x17a  constructor (serial, graphic, byte).
    CMobile(int serial, int graphic, uint8_t b3a);
    // 0x00454210  size 0x1f1  destructor.
    ~CMobile();
    // 0x00454410  size 0xb0  status update from the network: marks the serial, queues a pending update when
    // `queue` is nonzero and starts the body-dependent action (2 / 3, 8, 8 / 0xc, 0x15 / 0x16). RET 8.
    void __thiscall StartStatusAction(int queue, int alt);
    // 0x004544c0  size 0x2b  +0x1d8 divided by 24, truncated toward zero.
    int __thiscall Div24();
    // 0x00454540  size 0x5e  frame step table lookup (0x004bab38) for this body, walking / moving / idle.
    int __thiscall StepFrames();
    // 0x004545c0  size 0x11b  start an animation action (queued on the effect sequence while 0x007211b0 is set). RET 0x18.
    void __thiscall StartAction(int p1, int p2, int p3, int p4, int p5, int p6);
    // 0x004546e0  size 0x1c  end the current animation (+0x1a4) through 0x00430970; 0 when none.
    int __thiscall EndAnim();
};

static_assert(offsetof(CMobile, listNext) == 0xbc, "CMobile::listNext");
static_assert(offsetof(CMobile, listPrev) == 0xc0, "CMobile::listPrev");
static_assert(offsetof(CMobile, equip) == 0xc4, "CMobile::equip");
static_assert(offsetof(CMobile, unk_13c) == 0x13c, "CMobile::unk_13c");
static_assert(offsetof(CMobile, moveTick) == 0x140, "CMobile::moveTick");
static_assert(offsetof(CMobile, moving) == 0x144, "CMobile::moving");
static_assert(offsetof(CMobile, flag148) == 0x148, "CMobile::flag148");
static_assert(offsetof(CMobile, unk_14c) == 0x14c, "CMobile::unk_14c");
static_assert(offsetof(CMobile, unk_150) == 0x150, "CMobile::unk_150");
static_assert(offsetof(CMobile, steps) == 0x154, "CMobile::steps");
static_assert(offsetof(CMobile, fromX) == 0x17c, "CMobile::fromX");
static_assert(offsetof(CMobile, fromY) == 0x17e, "CMobile::fromY");
static_assert(offsetof(CMobile, fromZ) == 0x180, "CMobile::fromZ");
static_assert(offsetof(CMobile, stepCount) == 0x184, "CMobile::stepCount");
static_assert(offsetof(CMobile, unk_188) == 0x188, "CMobile::unk_188");
static_assert(offsetof(CMobile, unk_18c) == 0x18c, "CMobile::unk_18c");
static_assert(offsetof(CMobile, viewZ) == 0x190, "CMobile::viewZ");
static_assert(offsetof(CMobile, unk_194) == 0x194, "CMobile::unk_194");
static_assert(offsetof(CMobile, zMoving) == 0x198, "CMobile::zMoving");
static_assert(offsetof(CMobile, zDelta) == 0x19c, "CMobile::zDelta");
static_assert(offsetof(CMobile, unk_1a4) == 0x1a4, "CMobile::unk_1a4");
static_assert(offsetof(CMobile, unk_1a8) == 0x1a8, "CMobile::unk_1a8");
static_assert(offsetof(CMobile, animFrame) == 0x1aa, "CMobile::animFrame");
static_assert(offsetof(CMobile, unk_1ac) == 0x1ac, "CMobile::unk_1ac");
static_assert(offsetof(CMobile, unk_1b8) == 0x1b8, "CMobile::unk_1b8");
static_assert(offsetof(CMobile, unk_1bc) == 0x1bc, "CMobile::unk_1bc");
static_assert(offsetof(CMobile, gumpList1c0) == 0x1c0, "CMobile::gumpList1c0");
static_assert(offsetof(CMobile, unk_1c4) == 0x1c4, "CMobile::unk_1c4");
static_assert(offsetof(CMobile, unk_1c8) == 0x1c8, "CMobile::unk_1c8");
static_assert(offsetof(CMobile, unk_1cc) == 0x1cc, "CMobile::unk_1cc");
static_assert(offsetof(CMobile, unk_1d0) == 0x1d0, "CMobile::unk_1d0");
static_assert(offsetof(CMobile, gump1d4) == 0x1d4, "CMobile::gump1d4");
static_assert(offsetof(CMobile, unk_1d8) == 0x1d8, "CMobile::unk_1d8");
static_assert(sizeof(CMobile) == 0x1dc, "CMobile size (operator new 0x1dc)");

// The player object (vtable 0x0049faf8): CMobile plus 0x30 bytes, 0x20c in all (operator new 0x20c).
class CPlayer : public CMobile {
public:
    virtual void __thiscall ComputeScreenRect();   // 0x0042e430 (was Player_CalcRect)
    virtual void __thiscall Draw(int *clip);   // 0x0042e8b0 (was Player_DrawClip)
    virtual void __thiscall AddToWorld();   // 0x0042ba80
    virtual void __thiscall RemoveFromWorld();   // 0x0042b0c0: keyed on the global 0x00648858, facings 1..5
    virtual int __thiscall Slot28() { return 1; }
    virtual int __thiscall IsHidden();   // 0x0042c680, the root body
    virtual int __thiscall HitTest(int px, int py);   // 0x004725d0 (was Mobile_HitTestPreviewCached)
    virtual void __thiscall UpdateViewZ();   // 0x00449cb0 player: camera scroll and view z for the current walk phase
    virtual void __thiscall SetUnk1d8(int v);   // 0x00454510: set +0x1d8 and invalidate every mobile of the global list
    virtual void __thiscall DeleteSelf();   // slot 0x04, the root body
    uint8_t pad_1dc[0x1c];
    int32_t unk_1f8;        // +0x1f8  following flag (map gump)
    int32_t unk_1fc;        // +0x1fc  followed serial
    uint8_t pad_200[0xc];

    // 0x00454c30  size 0x81  constructor (serial, graphic, byte at +0x3a). RET 0xc.
    CPlayer(int serial, int graphic, uint8_t b3a);
    // 0x00454cc0  size 0x15  destructor: vtable, clears DAT_00721190, then the mobile destructor.
    ~CPlayer();
};
static_assert(offsetof(CPlayer, unk_1f8) == 0x1f8, "CPlayer::unk_1f8");
static_assert(offsetof(CPlayer, unk_1fc) == 0x1fc, "CPlayer::unk_1fc");
static_assert(sizeof(CPlayer) == 0x20c, "CPlayer size (operator new 0x20c)");

// The pooled land tile (vtable 0x0049f788, 0x74 bytes, pool counters 0x00721074..0x00721080).
class CTerrainTileOps : public CTerrainTile {
public:
    virtual void __thiscall ComputeScreenRect();   // 0x0042f640 (was TerrainTile_CalcRect)
    virtual void __thiscall Draw(int *clip);   // 0x0042f6e0 (was TerrainTile_DrawTriangles)
    virtual int __thiscall IsHidden();   // 0x0042c0f0 (was IsHiddenByCutaway)
    // 0x00451b80  size 0x12  constructor.
    CTerrainTileOps();
    // 0x00451ba0  size 0xb  destructor; slot 0 is the vector deleting destructor 0x00451ac0.
    virtual ~CTerrainTileOps();
    // 0x00451bb0  size 0x2f  give the tile back to the pool.
    virtual void __thiscall DeleteSelf();   // slot 0x04
};
static_assert(sizeof(CTerrainTileOps) == 0x74, "CTerrainTileOps size (operator new 0x3a04 = cookie + 0x80 * 0x74)");

// The pooled world object with vtable 0x0049f730 (pool counters 0x00721060.. / 0x00721068 / 0x00721070).
class CObjVt49f730 : public CWorldObjLow {
public:
    virtual void __thiscall Draw(int *clip);   // 0x0042ef20 (was TerrainTile_DrawSurface)
    virtual int __thiscall IsHidden();   // 0x0042c0c0 (was IsHiddenByCutaway)
    virtual void __thiscall ComputeScreenRect();   // 0x0042eee0 (was RectFromCell_0042eee0)
    virtual int __thiscall Slot1c() { return 1; }
    // 0x00451b20  size 0x12  constructor.
    CObjVt49f730();
    // 0x00451b40  size 0xb  destructor; slot 0 is the vector deleting destructor 0x00451960.
    virtual ~CObjVt49f730();
    // 0x00451b50  size 0x2f  give the object back to its pool (free list head 0x00721070, next at +0x14).
    virtual void __thiscall DeleteSelf();   // slot 0x04

    uint8_t pad_34[8];   // +0x34 .. 0x3c: the pooled object is 0x3c bytes (operator new 0x1e04 = cookie + 0x80 * 0x3c)
};
static_assert(sizeof(CObjVt49f730) == 0x3c, "CObjVt49f730 size");

// Free functions of the object code.
// 0x004519c0  size 0xf9  pool "new" of a land tile: ECX = tile id, EDX = coords, one stack arg (corner heights). RET 4.
CTerrainTile *__fastcall CTerrainTile_PoolNew(int tileId, TileCoords *coords, signed char *corners);
// 0x00452170  size 0x28  sanitize a hue: keeps bit 15, the low 15 bits only when below 0xbb8 (else 1).
uint32_t __fastcall CObj_SanitizeHue(uint32_t hue);
// 0x004521a0  size 0x74  set the status byte (+0x98) of an item or mobile; hides/refreshes the owner's gumps.
void __fastcall CDynamicObj_SetStatusFlags(CDynamicObj *obj, uint32_t flags);
// 0x00452290  size 0x56  pending object update whose field 1 equals `key`: the item with that update's serial, or null.
CDynamicObj *__fastcall PendingUpdate_FindObj(uint32_t key);
// 0x004522f0  size 0x36  1 when a pending update has field 0 == a or field 1 == b.
int __fastcall PendingUpdate_Has(uint32_t a, uint32_t b);
// 0x00452330  size 0x155  remove the pending updates that match (a, b), apply them to their items; RET 4.
void __fastcall PendingUpdate_Apply(uint32_t a, uint32_t b, int c);
// 0x00452490  size 0xed  add a pending update unless one with field 0 == a exists. RET 8.
void __fastcall PendingUpdate_Add(uint32_t a, uint32_t b, uint32_t c, uint32_t d);
// 0x004548e0  size 0x130  animation id and frame of a mobile body (ECX = graphic; EDX unused; 8 stack args and two
// output pointers). RET 0x28. Stack words in order: a8 (tick), ac (fixed frame, -1 = none), a10 (unused), a14, a18,
// a1c, a20, a24, then the two outputs.
int __fastcall GetMobileAnimation(uint32_t graphic, uint32_t edxUnused, int a8, int ac, int a10, int a14, int a18,
                                  int a1c, int a20, int a24, int *animOut, int *frameOut);

// Free functions of the object code.
// 0x004514e0  size 0x155  pool "new" + init of a 0x78-byte object; ECX = graphic id.
CObj *__fastcall CObj_PoolNew(int graphic);
// 0x00453050  size 0x131  item display name into the static buffer at 0x00721088.
char *__fastcall BuildItemName(int graphic, int flag);

// Globals touched by the object code.
#define g_worldListTail GLOBAL_AS(CObj *, g_objMgr_storage)   // oldest object of the world list
