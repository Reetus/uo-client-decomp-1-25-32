// Second-pass ports in the object manager range 0x00455f40 .. 0x00457460:
//   0x00455f40 CMobile idle animation roll            0x00456080 face the combat target
//   0x004567b0 CObjectManager::FindAtCoords           0x00456840 graphic test (0xef3..0xef9, 0x1f2d..0x1f6c)
//   0x00456890 use (double click) an object            0x00456a10 single click: overhead name
//   0x00456d70 CObjectManager::FindBySerial            0x00457000 constant 1
//   0x00457010 tiledata flag 0x200000                  0x00457050 graphic test (0x28e, 0xeed)
//   0x00457320 CObjectManager::AgeLabels               0x00457420 / 0x00457430 label record release
//   0x00457460 profanity filter
// The SEH frames of 0x004567b0 / 0x00456890 are omitted (they guard nothing that can throw).
#include "object_manager.h"

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"
#include "pos3.h"
#include "gump_manager.h"
#include "crt.h"
#include "packet_cursor.h"
#include "raw_field.h"
#include "vcall.h"
#include "walk.h"
#include "obj.h"
#include "mobile_idle.h"
#include "client_misc_ops_ports.h"
#include "text_search_ports.h"
#include "packet_builders_batch24_ports.h"
#include "app_session_ops_ports.h"
#include "cstr.h"

#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)

// ---------------------------------------------------------------------------
// Original code not ported yet.
// ---------------------------------------------------------------------------
// CMobile::StartAction (0x004545c0), RET 0x18: (action, frames, repeat, one, zero, flag).
static void Mobile_StartAction(CMobile *m, int a1, int a2, int a3, int a4, int a5, int a6)
{
    m->CMobile::StartAction(a1, a2, a3, a4, a5, a6);
}
static int Ignored_0x00478cd0() { return Misc_Return0(); }   // ? xor eax,eax; ret
static int StrFindNoCase_0x004067b0(const char *needle, const char *haystack)
{
    return StrFindNoCase(needle, haystack);
}

// ---------------------------------------------------------------------------
// 0x00455f40  size 0x131
// Roll for a random idle animation of an unmounted mobile that has been still for 15 s while
// at most 20 mobiles are around; the animation depends on the body type range.
// ---------------------------------------------------------------------------
void CMobileIdle::RandomIdleAnim()
{
    if (equip[25] != nullptr)                                  // +0x128: mount
        return;
    if (g_mobileCount > 0x14)
        return;
    if (g_gameTick - static_cast<uint32_t>(unk_14c) <= 0x3a98u)
        return;
    int32_t roll = rand();
    int32_t mod = g_mobileCount * 4 + 0x78;
    int32_t k = roll % mod;
    if (k >= 2)
        return;

    uint32_t body = graphic;
    if (body == 0x15) {
        Mobile_StartAction(this, k == 0 ? 0x12 : 0x11, 5, 0, 1, 0, 1);
        return;
    }
    if (body == 5) {
        Mobile_StartAction(this, 0x11, 5, 0, 1, 0, 0);
        return;
    }
    if (body < 0x96) {
        Mobile_StartAction(this, k == 0 ? 0x12 : 0x11, 5, 0, 1, 0, 0);
        return;
    }
    if (body < 200) {
        if (k == 0)
            Mobile_StartAction(this, 4, 0x14, 0, 1, 0, 0);
        else
            Mobile_StartAction(this, 3, 0xf, 0, 1, 0, 0);
        return;
    }
    if (body < 0x190) {
        if (k == 0)
            Mobile_StartAction(this, 0xa, 3, 0, 1, 0, 0);
        else
            Mobile_StartAction(this, 9, 5, 0, 1, 0, 0);
        return;
    }
    if (k == 0)
        Mobile_StartAction(this, 6, 5, 0, 1, 0, 0);
    else
        Mobile_StartAction(this, 5, 5, 0, 1, 0, 1);
}
HOOK_METHOD(0x00455f40, CMobileIdle, RandomIdleAnim);

// ---------------------------------------------------------------------------
// 0x00456080  size 0xc0
// If more than 3 ticks passed since the last request, turn the player toward the combat
// target (found in the item list at 0x0068ce7c by serial) when it is a mobile.
// ---------------------------------------------------------------------------
void __cdecl Combat_FaceTarget()
{
    if (GLOBAL_AS(uint32_t, g_combatTimeA) - g_combatTimeB <= 3u)
        return;

    CDynamicObj *target = g_dynListHead;
    CDynamicObj *found = nullptr;
    if (target != nullptr) {
        int32_t serial = g_combatTargetSerial;
        for (; target != nullptr; target = target->listNext) {
            if (target->serial == serial) {
                found = target;
                break;
            }
        }
    }
    if (found == nullptr)
        return;
    if (decomp::vcall(PORT_RET(int), found, 0x24) == 0)                  // slot 0x24: is mobile
        return;

    CObj *player = g_player;
    int dx = found->x - player->x;
    int dy = found->y - player->y;
    dx = dx < 0 ? -1 : (dx > 0 ? 1 : 0);
    dy = dy < 0 ? -1 : (dy > 0 ? 1 : 0);
    int dir = 0;
    do {
        if (g_dirDx[dir] == dx && g_dirDy[dir] == dy)
            break;
        dir++;
    } while (dir < 7);
    if (dir != player->facing)
        g_walk.RequestWalk(dir + 1, -1);
}
HOOK_FUNC(0x00456080, Combat_FaceTarget);

// ---------------------------------------------------------------------------
// 0x004567b0  size 0x8b   (RET 8; ECX = manager)
// First object of the world list on tile (x, y) whose vertical extent contains z.
// ---------------------------------------------------------------------------
CObj *CObjectManager::FindAtCoords(Pos3 pos)
{
    for (CObj *o = listHead; o != nullptr; o = o->link44) {
        if (static_cast<uint16_t>(pos.x) != static_cast<uint16_t>(o->x))
            continue;
        if (static_cast<uint16_t>(pos.y) != static_cast<uint16_t>(o->y))
            continue;
        if (pos.z < o->z)
            continue;
        int height = decomp::vcall(PORT_RET(int), o, 0x6c);
        if (pos.z < height + o->z)
            return o;
    }
    return nullptr;
}
HOOK_METHOD(0x004567b0, CObjectManager, FindAtCoords);

// ---------------------------------------------------------------------------
// 0x00456840  size 0x44   (ECX = graphic word)
// Graphics 0xef3..0xef9 and 0x1f2d..0x1f6c.
// ---------------------------------------------------------------------------
uint32_t __fastcall Graphic_IsInRanges_0x00456840(uint32_t graphic)
{
    uint16_t g = static_cast<uint16_t>(graphic);
    if (g >= 0xef3 && g <= 0xef9)
        return 1;
    if (g >= 0x1f2d && g <= 0x1f6c)
        return 1;
    return 0;
}
HOOK_FUNC(0x00456840, Graphic_IsInRanges_0x00456840);

// ---------------------------------------------------------------------------
// 0x00456890  size 0x167   (RET 4)
// Use an object (double click). Objects that answer slot 0x2c: remember the serial; slot 0x60
// objects focus their gump when one is open, otherwise (and for every other graphic except the
// target-cursor ones) send the double-click packet. Graphics 0xef3..0xef9 and 0x383 start the
// target cursor with type 0, 1 or 2 instead.
// ---------------------------------------------------------------------------
void __stdcall Object_Use(CDynamicObj *obj)
{
    uint8_t out[8];
    // 0x0044e1a0: start the target cursor, ECX = "scroll" text (0x004ad1d4), EDX = type, serial on the stack.
    // 0x00462d30: build the double-click packet, ECX = builder (0x0069e0e0).

    if (decomp::vcall(PORT_RET(int), obj, 0x2c) == 0)
        return;
    g_macroTarget = obj->serial;
    if (decomp::vcall(PORT_RET(int), obj, 0x60) != 0) {
        CGump *gump = g_gumpMgr.FindGumpForObject(obj, 0, 0, g_gumpListTail);
        if (gump == nullptr) {
            PORT_THISCALL2(&Build_Packet_0x06, void, g_packetBuilder, uint8_t *, uint32_t)(out, static_cast<uint32_t>(obj->serial));
            pkt::Send(out);
            return;
        }
        gump->BringToFront(0);
        return;
    }

    switch (obj->graphic) {
    case 0xef3:
        BeginTargetCommand(g_scrollText, 1, obj->serial);
        return;
    case 0xef4:
        BeginTargetCommand(g_scrollText, 1, obj->serial);
        return;
    case 0xef5:
        BeginTargetCommand(g_scrollText, 0, obj->serial);
        return;
    case 0xef6:
    case 0xef9:
        BeginTargetCommand(g_scrollText, 0, obj->serial);
        return;
    case 0xef7:
    case 0x383:
        BeginTargetCommand(g_scrollText, 2, obj->serial);
        return;
    case 0xef8:
        BeginTargetCommand(g_scrollText, 2, obj->serial);
        return;
    default: {
        uint8_t out2[8];
        PORT_THISCALL2(&Build_Packet_0x06, void, g_packetBuilder, uint8_t *, uint32_t)(out2, static_cast<uint32_t>(obj->serial));
        pkt::Send(out2);
        return;
    }
    }
}
HOOK_FUNC(0x00456890, Object_Use);

// ---------------------------------------------------------------------------
// 0x00456a10  size 0x166   (RET 4)
// Single click on an object: put its name (with the stack count in front for a stacked item)
// above it as a bark of hue 0x3b2, font 3.
// ---------------------------------------------------------------------------
void __stdcall Object_ShowName(CDynamicObj *obj)
{
    char nameBuf[0x200];
    char textBuf[0x200];
    CGump *contGump = nullptr;
    CGump *owner = contGump;

    if (obj != nullptr && decomp::vcall(PORT_RET(int), obj, 0x2c) != 0)
        Ignored_0x00478cd0();                                  // ? result unused
    if (obj->barkList != nullptr)
        return;

    if (decomp::vcall(PORT_RET(int), obj, 0x2c) != 0 && obj->container != nullptr) {
        contGump = owner = RAWF(CGump *, obj->container, 0xac);
    }
    if (contGump != nullptr) {
        CGump *par = contGump->parent;    // +0x48
        if (par != nullptr)
            owner = par;
    }

    g_barkState = 3;
    sprintf(textBuf, "%d ", static_cast<int>(obj->unk_3c));
    if (decomp::vcall(PORT_RET(int), obj, 0x2c) != 0)
        return;

    uint32_t flags = decomp::vcall(PORT_RET(uint32_t), obj, 0x40);
    int stacked = ((flags & 0x800) != 0 && obj->unk_3c > 1) ? 1 : 0;
    char *name = BuildItemName(static_cast<int>(obj->graphic) + static_cast<int8_t>(obj->unk_3a), stacked);
    strcpy(nameBuf, name);

    flags = decomp::vcall(PORT_RET(uint32_t), obj, 0x40);
    if ((flags & 0x800) != 0 && obj->unk_3c > 1) {
        strcat(textBuf, nameBuf);
        obj->CreateBarkKeepTwo(textBuf, 0x3b2, 3, owner);
        return;
    }
    obj->CreateBarkKeepTwo(nameBuf, 0x3b2, 3, owner);
}
HOOK_FUNC(0x00456a10, Object_ShowName);

// ---------------------------------------------------------------------------
// 0x00456d70  size 0x22   (RET 4)
// ---------------------------------------------------------------------------
CDynamicObj *CObjectManager::FindBySerial(uint32_t serial)
{
    for (CDynamicObj *o = list24; o != nullptr; o = o->listNext) {
        if (static_cast<uint32_t>(o->serial) == serial)
            return o;
    }
    return nullptr;
}
HOOK_METHOD(0x00456d70, CObjectManager, FindBySerial);

// ---------------------------------------------------------------------------
// 0x00457000  size 0xc   (RET 8; ECX = manager, called with a tile x and y)
// Always 1.
// ---------------------------------------------------------------------------
uint32_t __stdcall ObjectManager_Always1(uint32_t tileX, uint32_t tileY)
{
    (void)tileX;
    (void)tileY;
    return 1;
}
HOOK_FUNC(0x00457000, ObjectManager_Always1);

// ---------------------------------------------------------------------------
// 0x00457010  size 0x20   (RET 4)
// ---------------------------------------------------------------------------
uint32_t __stdcall ObjectManager_TileFlagTest_0x200000(uint32_t graphic)
{
    return RAWF(uint32_t, g_tileData, (graphic & 0xffffu) * 0x28u) & 0x200000u;
}
HOOK_FUNC(0x00457010, ObjectManager_TileFlagTest_0x200000);

// ---------------------------------------------------------------------------
// 0x00457050  size 0x28   (RET 4)
// Graphic 0x28e or 0xeed.
// ---------------------------------------------------------------------------
uint32_t __stdcall Graphic_Is_0x28e_Or_0xeed(uint32_t graphic)
{
    uint32_t g = graphic & 0xffffu;
    return (g == 0x28e || g == 0xeed) ? 1u : 0u;
}
HOOK_FUNC(0x00457050, Graphic_Is_0x28e_Or_0xeed);

// ---------------------------------------------------------------------------
// 0x00457320  size 0x84
// Walk the pending label list: bump each record's counter (+0x04); a record whose counter
// passes 4 is unlinked, deleted together with its node, and the walk continues with the
// following node.
// ---------------------------------------------------------------------------
// Unlink `node` from the label list and delete it; returns the record it held, the following node through `next`.
static inline LabelRecord *LabelList_Remove(CObjectManager *m, LabelNode *node, LabelNode *&next)
{
    LabelRecord *rec = nullptr;
    if (node == nullptr) {
        next = nullptr;
    } else {
        LabelNode *prev = node->prev;
        next = node->next;
        if (prev != nullptr)
            prev->next = next;
        else
            m->labels.head = next;
        if (next != nullptr)
            next->prev = prev;
        else
            m->labels.tail = prev;
        rec = node->record;
        node->record = nullptr;
        operator delete(node);
        m->labels.count = m->labels.count - 1;
    }
    return rec;
}

static inline LabelNode *LabelList_Next(LabelNode *n)
{
    if (n == nullptr)
        return nullptr;
    return n->next;
}

void CObjectManager::AgeLabels()
{
    LabelNode *node = labels.head;
    while (node != nullptr) {
        LabelRecord *rec = node->record;
        rec->unk_04 = static_cast<uint8_t>(rec->unk_04 + 1);
        if (rec->unk_04 > 4) {
            LabelNode *next;
            LabelRecord *dead = LabelList_Remove(this, node, next);
            if (dead != nullptr) {
                dead->DtorText();
                operator delete(dead);
            }
            node = next;
        } else {
            node = LabelList_Next(node);
        }
    }
}
HOOK_METHOD(0x00457320, CObjectManager, AgeLabels);

// ---------------------------------------------------------------------------
// 0x00457420  size 0x8   (ECX = record)
// Destroy the string object at +0x08 (0x004703c0).
// ---------------------------------------------------------------------------
void LabelRecord::DtorText()
{
    text.Dtor();
}
HOOK_METHOD(0x00457420, LabelRecord, DtorText);

// ---------------------------------------------------------------------------
// 0x00457430  size 0x25   (ECX = node)
// ---------------------------------------------------------------------------
void LabelNode::FreeRecord()
{
    LabelRecord *rec = record;
    if (rec != nullptr) {
        rec->DtorText();
        operator delete(rec);
        record = nullptr;
    }
}
HOOK_METHOD(0x00457430, LabelNode, FreeRecord);

// ---------------------------------------------------------------------------
// 0x00457460  size 0x80   (ECX = text)
// Profanity filter: overwrite every case-insensitive occurrence of each banned word in the
// text with characters of "!@#$%&*", chosen by the old character (signed) modulo 7.
// ---------------------------------------------------------------------------
void __fastcall Profanity_Apply(char *text)
{
    if (g_ccSel654 <= 0)
        return;
    const char *word = g_ccNames900;
    int count = g_ccSel654;
    do {
        int idx = StrFindNoCase(word, text);
        while (idx != -1) {
            int len = static_cast<int>(strlen(word));
            char *p = text + idx;
            while (len > 0) {
                int c = static_cast<signed char>(*p);
                *p = g_profanityReplace[c % 7];
                p++;
                len--;
            }
            idx = StrFindNoCase(word, text);
        }
        word += 0x1e;
        count--;
    } while (count != 0);
}
HOOK_FUNC(0x00457460, Profanity_Apply);
