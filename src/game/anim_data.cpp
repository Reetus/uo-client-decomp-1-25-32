// CAnimData: animdata.mul loader and the per-tick animation of animated static items.
#include "anim_data.h"

#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "globals.h"
#include "obj.h"
#include "crt.h"
#include "class_0049c690.h"

#define g_animatedList GLOBAL_AS(CObj *, g_objectManager_storage)   // list of animated objects (next at obj+0x50)

// 0x00406720: a lone `ret`, called with ECX = g_loadProgressArg + group index, EDX = the group header.
// Reproduced as a call so the sequence of calls matches the original.
#define AnimData_GroupHook PORT_FUNC2(&Nop_00406720, void, __fastcall, int, int)

// 0x00401000  size 0x102
// Constructor. The animdata.mul file is 0x800 groups of { int32 header; 8 x { 64 frame bytes, 4 bytes } }.
CAnimData *__thiscall CAnimData::Ctor()
{
    int32_t header = 0;

    memset(this, 0, sizeof(CAnimData));
    for (int i = 0; i < 0x4000; i++)
        records[i].interval = 8;

    FILE *fp = fopen(g_app.ResolveDataPath(const_cast<char *>("animdata.mul")), g_fopenReadMode);
    if (fp != nullptr) {
        for (int i = 0; i < 0x4000; i++) {
            AnimRecord *r = &records[i];
            if ((i & 7) == 0) {
                fread(&header, 4, 1, fp);
                AnimData_GroupHook((i >> 3) + g_loadProgressArg, header);
            }
            fread(r->frames, 0x40, 1, fp);
            fread(&r->curFrame, 1, 1, fp);
            fread(&r->frameCount, 1, 1, fp);
            fread(&r->interval, 1, 1, fp);
            fread(&r->startDelay, 1, 1, fp);
        }
        fclose(fp);
    }
    return this;
}
HOOK_METHOD(0x00401000, CAnimData, Ctor);

// 0x00401110  size 0xb0
// Per-tick update of the animated objects (tiledata flag 0x1000000 list). ECX is not read.
// For each object whose virtual slot 0x3c returns 0 and whose record has a non-zero interval, on the
// tick where (startDelay + tick) % interval == 0: virtual slot 0x14, advance the record's frame once
// per tick, store the frame byte at obj+0x3a, virtual slot 0x10.
void __cdecl ObjectManager_UpdateAnimations()
{
    g_animTick = g_animTick + 1;
    for (CObj *obj = g_animatedList; obj != nullptr; obj = obj->link50) {
        if (PORT_VSLOT0(obj, 0x3c, int)() != 0)
            continue;
        AnimRecord *rec = &g_animData.records[obj->graphic];
        if (!(rec->interval > 0))
            continue;
        if ((rec->startDelay + g_animTick) % rec->interval != 0)
            continue;
        PORT_VSLOT0(obj, 0x14, void)();
        if (rec->lastTick != g_animTick) {
            rec->curFrame = static_cast<uint8_t>(rec->curFrame + 1);
            if (rec->curFrame >= rec->frameCount)
                rec->curFrame = 0;
            rec->lastTick = g_animTick;
        }
        obj->unk_3a = rec->frames[rec->curFrame];
        PORT_VSLOT0(obj, 0x10, void)();
    }
}
HOOK_FUNC(0x00401110, ObjectManager_UpdateAnimations);
