// CObjectManager: the one object (at 0x0068ce58) that owns the world object
// lists, the tiledata buffers, the held-item ("in hand") state and the queue
// of pending overhead labels. Also the class with vtable 0x0049fb90.
//
// Layout partial. Offsets come from decomp/pass2/0x00454f10 (constructor),
// 0x00454fc0 (destructor), 0x00456140, 0x00456b80, 0x00456da0, 0x00457080 and
// 0x00457230. The held-item block (+0x38..+0x58) is the same memory the
// inbound 0x27 handler reads at 0x0068ce90..0x0068ceb0 (0x0068ce58 + off).
#pragma once

#include <cstddef>
#include <cstdint>
#include <new>

#include "../runtime/ported.h"
#include "obj.h"
#include "cstr.h"
#include "pos3.h"
#include "globals.h"

// 8-byte coordinate struct passed by value to CObj::ToWorld / ToContainer:
// the x | y << 16 dword and the z word, padded to a dword.
struct ObjCoords {
    uint16_t x;
    uint16_t y;
    uint16_t z;
    uint16_t pad;
};
static_assert(sizeof(ObjCoords) == 8, "ObjCoords size");

// Pending overhead-label record (0x24 bytes, operator new): see 0x00457080.
struct LabelRecord {
    uint32_t serial;        // +0x00  serial of the object the label belongs to
    uint8_t unk_04;         // +0x04  zeroed
    uint8_t pad_05[3];
    CStr text;              // +0x08  string object (ctor 0x004703a0, dtor 0x004703c0); first dword = char *
    uint16_t hue;           // +0x18
    uint16_t font;          // +0x1a
    uint32_t unk_1c;        // +0x1c  copied from child+0x94
    uint32_t unk_20;        // +0x20  copied from child+0x98

    LabelRecord(uint32_t serial_, const char *s, uint16_t hue_, uint16_t font_, uint32_t c94, uint32_t c98)
    {
        serial = serial_;
        text.Assign(s);
        hue = hue_;
        font = font_;
        unk_04 = 0;
        unk_1c = c94;
        unk_20 = c98;
    }

    // 0x00457420  size 0x8  destroys the text string object at +0x08 (tail call to 0x004703c0).
    void __thiscall DtorText();
};
static_assert(sizeof(LabelRecord) == 0x24, "LabelRecord size");
static_assert(offsetof(LabelRecord, text) == 0x08, "LabelRecord::text");
static_assert(offsetof(LabelRecord, hue) == 0x18, "LabelRecord::hue");
static_assert(offsetof(LabelRecord, font) == 0x1a, "LabelRecord::font");
static_assert(offsetof(LabelRecord, unk_1c) == 0x1c, "LabelRecord::unk_1c");
static_assert(offsetof(LabelRecord, unk_20) == 0x20, "LabelRecord::unk_20");

// Doubly linked list node of the pending label list: {next, prev, record}.
struct LabelNode {
    LabelNode *next;        // +0x00
    LabelNode *prev;        // +0x04
    LabelRecord *record;    // +0x08

    // 0x00457430  size 0x25  destroy and delete the record, clear the pointer.
    void __thiscall FreeRecord();
};
static_assert(sizeof(LabelNode) == 0xc, "LabelNode size");

// The pending label list: doubly linked, newest at the tail. Its destructor deletes every node and record.
struct CLabelList {
    LabelNode *head;        // +0x00
    LabelNode *tail;        // +0x04
    int32_t count;          // +0x08

    CLabelList()
    {
        head = nullptr;
        tail = nullptr;
        count = 0;
    }
    ~CLabelList()
    {
        LabelNode *node = head;
        while (node != nullptr) {
            LabelNode *prevNode = node->prev;
            LabelNode *nextNode = node->next;
            if (prevNode != nullptr)
                prevNode->next = nextNode;
            else
                head = nextNode;
            if (nextNode != nullptr)
                nextNode->prev = prevNode;
            else
                tail = prevNode;
            LabelRecord *payload = node->record;
            node->record = nullptr;
            node->FreeRecord();
            operator delete(node);
            node = nextNode;
            count = count - 1;
            if (payload != nullptr) {
                payload->text.~CStr();
                operator delete(payload);
            }
        }
    }
};
static_assert(sizeof(CLabelList) == 0xc, "CLabelList size");

class CObjectManager {
public:
    // 0x00454f10  size 0xa4  constructor: clears the lists, allocates the two tiledata buffers.
    CObjectManager();
    // 0x00454fc0  size 0xc9  destructor: frees the tiledata buffers, then the pending label list (member).
    ~CObjectManager();
    CObjectManager *Ctor() { this->CObjectManager::CObjectManager(); return this; }
    void Dtor() { this->~CObjectManager(); }
    // 0x00456140  size 0x646  per-tick object update: flag refresh, mobile walk step, pixel offsets.
    void __thiscall UpdateMovement();
    // 0x00456b80  size 0x1ef  begin picking up `obj` (qty 0 = all): fill the held-item state,
    // send the pickup packet, create the drag gump.
    void __thiscall BeginPickUp(CDynamicObj *obj, uint16_t qty);
    // 0x00456da0  size 0x259  create a mobile / item / corpse and place it in a container or mobile.
    void __thiscall CreateObject(uint32_t serial, uint32_t graphic, uint32_t param, uint16_t qty,
                                 uint16_t x, uint16_t y, int parentSerial, uint16_t hue);
    // 0x00457080  size 0x1ae  queue a label record for each child of `gump`.
    void __thiscall QueueLabels(CGump *gump);
    // 0x00457230  size 0xe2  flush the pending label records that belong to `obj`.
    void __thiscall FlushLabels(CDynamicObj *obj);
    // 0x00454d00  size 0x98  ground z at (x, y): the map cell's mean height (when DAT_0069e0c0 is 0) raised by
    // the top of every static there. 0 when nothing reaches above -0x7f. RET 8.
    int __thiscall GroundZ(int x, int y);
    // 0x00455090  size 0x8c  delete every object but the player (and the player's equipment).
    void __thiscall DeleteAllObjects();
    // 0x00455140  size 0x32c  patch item tiledata group `group` (0x200 based) from `data` into memory and tiledata.mul.
    // RET 8.
    void __thiscall SetItemTileData(int group, const uint8_t *data);
    // 0x00455470  size 0x14c  patch terrain tiledata group `group` from `data` into memory and tiledata.mul. RET 8.
    void __thiscall SetTerrTileData(int group, const uint8_t *data);
    // 0x004555c0  size 0x194  read tiledata.mul into the two buffers.
    void __thiscall LoadTileData();
    // 0x00455760  size 0x7da  create or update the item / mobile `serial` from a world packet. RET 0x28.
    void __thiscall AddOrUpdateObject(uint32_t serial, uint32_t graphic, int8_t offset, uint16_t qty, int x, int y,
                                      int z, uint8_t facing, uint32_t hue, uint32_t flags);
    // 0x00456d70  size 0x22  RET 4: item with this serial in the list at +0x24 (next at obj+0x80), or null.
    CDynamicObj *__thiscall FindBySerial(uint32_t serial);
    // 0x004567b0  size 0x8b  RET 8: first object of the world list at +0x04 (next at obj+0x44) on tile
    // (x, y) whose z <= pos.z and whose z + virtual slot 0x6c (height) is above pos.z.
    CObj *__thiscall FindAtCoords(Pos3 pos);
    // 0x00457320  size 0x84  age every pending label record; drop those whose counter passes 4.
    void __thiscall AgeLabels();

    CObj *listTail;         // +0x00  (world list tail, g_worldListTail)
    CObj *listHead;         // +0x04  (world list head, g_worldListHead)
    int32_t unk_08;
    int32_t unk_0c;
    int32_t unk_10;
    int32_t unk_14;
    CObj *list18;           // +0x18  first object of the list walked first by UpdateMovement (next at obj+0x60)
    int32_t unk_1c;
    int32_t unk_20;
    CDynamicObj *list24;    // +0x24  first object of the list walked second (next at obj+0x80)
    int32_t unk_28;
    int32_t unk_2c;
    CObj *firstFb90;        // +0x30  head of the list of class-0x0049fb90 objects (next at obj+0x78)
    CGump *dragGump;        // +0x34  drag gump of the item in hand
    // Held item ("in hand") state, filled by BeginPickUp, read by the 0x27 handler.
    Pos3 heldPos;           // +0x38  item x, y, z (6 bytes; the empty destructor 0x0040caf0)
    uint16_t heldGraphic;   // +0x3e
    uint8_t heldFlag40;     // +0x40  item+0x98
    uint8_t heldOffset;     // +0x41  item+0x3a
    uint8_t pad_42[2];
    uint32_t heldSerial;    // +0x44
    uint16_t heldHue;       // +0x48
    uint16_t heldQty;       // +0x4a
    uint32_t heldTileFlags; // +0x4c  first dword of the tiledata record
    CObj *heldContainer;    // +0x50
    CObj *heldMobile;       // +0x54
    uint16_t heldLayer;     // +0x58
    uint8_t pad_5a[2];
    int32_t unk_5c;
    uint8_t *tileItems;     // +0x60  item tiledata, 0x4000 * 0x28 (g_tileData)
    uint8_t *tileTerrain;   // +0x64  terrain tiledata, 0x70000 bytes
    int32_t unk_68;         // +0x68  0x4000
    int32_t unk_6c;         // +0x6c  0x4000
    int32_t heldState;      // +0x70  1 while an item is in hand
    CLabelList labels;      // +0x74  pending overhead labels
};

static_assert(offsetof(CObjectManager, listTail) == 0x00, "CObjectManager::listTail");
static_assert(offsetof(CObjectManager, listHead) == 0x04, "CObjectManager::listHead");
static_assert(offsetof(CObjectManager, unk_08) == 0x08, "CObjectManager::unk_08");
static_assert(offsetof(CObjectManager, unk_14) == 0x14, "CObjectManager::unk_14");
static_assert(offsetof(CObjectManager, list18) == 0x18, "CObjectManager::list18");
static_assert(offsetof(CObjectManager, unk_20) == 0x20, "CObjectManager::unk_20");
static_assert(offsetof(CObjectManager, list24) == 0x24, "CObjectManager::list24");
static_assert(offsetof(CObjectManager, unk_2c) == 0x2c, "CObjectManager::unk_2c");
static_assert(offsetof(CObjectManager, firstFb90) == 0x30, "CObjectManager::firstFb90");
static_assert(offsetof(CObjectManager, dragGump) == 0x34, "CObjectManager::dragGump");
static_assert(offsetof(CObjectManager, heldPos) == 0x38, "CObjectManager::heldPos");
static_assert(offsetof(CObjectManager, heldGraphic) == 0x3e, "CObjectManager::heldGraphic");
static_assert(offsetof(CObjectManager, heldFlag40) == 0x40, "CObjectManager::heldFlag40");
static_assert(offsetof(CObjectManager, heldOffset) == 0x41, "CObjectManager::heldOffset");
static_assert(offsetof(CObjectManager, heldSerial) == 0x44, "CObjectManager::heldSerial");
static_assert(offsetof(CObjectManager, heldHue) == 0x48, "CObjectManager::heldHue");
static_assert(offsetof(CObjectManager, heldQty) == 0x4a, "CObjectManager::heldQty");
static_assert(offsetof(CObjectManager, heldTileFlags) == 0x4c, "CObjectManager::heldTileFlags");
static_assert(offsetof(CObjectManager, heldContainer) == 0x50, "CObjectManager::heldContainer");
static_assert(offsetof(CObjectManager, heldMobile) == 0x54, "CObjectManager::heldMobile");
static_assert(offsetof(CObjectManager, heldLayer) == 0x58, "CObjectManager::heldLayer");
static_assert(offsetof(CObjectManager, unk_5c) == 0x5c, "CObjectManager::unk_5c");
static_assert(offsetof(CObjectManager, tileItems) == 0x60, "CObjectManager::tileItems");
static_assert(offsetof(CObjectManager, tileTerrain) == 0x64, "CObjectManager::tileTerrain");
static_assert(offsetof(CObjectManager, unk_68) == 0x68, "CObjectManager::unk_68");
static_assert(offsetof(CObjectManager, unk_6c) == 0x6c, "CObjectManager::unk_6c");
static_assert(offsetof(CObjectManager, heldState) == 0x70, "CObjectManager::heldState");
static_assert(offsetof(CObjectManager, labels) == 0x74, "CObjectManager::labels");
// sizeof(CObjectManager) is not known (static object, no operator new site).

#define g_objMgr GLOBAL_AS(CObjectManager, g_objMgr_storage)

// The class with vtable 0x0049fb90, derived from CObj (0x78 bytes): linked into a
// list headed at CObjectManager::firstFb90 (next at +0x78, prev at +0x7c).
// Size is not known: the allocation site is not in this batch.
class CObjVt49fb90 : public CObj {
public:
    virtual void __thiscall Draw(int *clip);   // 0x0042caa0 (was Item_DrawWithFlag8)
    virtual void __thiscall DeleteSelf();   // slot 0x04, the root body
    // 0x00454da0  size 0xa7  constructor (graphic, x, y); RET 0xc.
    CObjVt49fb90(uint32_t graphicId, uint16_t tileX, uint16_t tileY);
    // 0x00454e90  size 0x7f  destructor.
    ~CObjVt49fb90();

    CObjVt49fb90 *next;     // +0x78
    CObjVt49fb90 *prev;     // +0x7c
};
static_assert(offsetof(CObjVt49fb90, next) == 0x78, "CObjVt49fb90::next");
static_assert(sizeof(CObjVt49fb90) == 0x80, "CObjVt49fb90 size (operator new 0x80)");
static_assert(offsetof(CObjVt49fb90, prev) == 0x7c, "CObjVt49fb90::prev");

// 0x00455120  size 0x1b  picks the world object under screen point (x, y) through 0x0042fd50 (ECX = 0x0069e0c0, flag 1). RET 8.
int __stdcall ObjectManager_GetObjectAtScreen(int x, int y);

// 0x00457030  size 0x20  tiledata flag 0x100000 of `graphic`.
uint32_t __stdcall ObjectManager_TileFlagTest_0x100000(uint32_t graphic);
// 0x00457010  size 0x20  tiledata flag 0x200000 of `graphic` (low 16 bits); the ECX the callers pass is unused.
uint32_t __stdcall ObjectManager_TileFlagTest_0x200000(uint32_t graphic);
