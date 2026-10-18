// CRegion / CRegionManager second-pass ports: record constructor and copies, SetRegion.
#include "region_manager.h"

#include <cstring>

#include "../runtime/ported.h"
#include "crt.h"

#pragma pack(push, 1)
// On-disk record as 0x0046b0d0 reads it (0x6b bytes).
struct RegionDiskRec {
    char name1[0x28];
    int16_t id;
    int16_t s[6];
    int32_t music;
    char name2[0x28];
    int16_t t1;
    int16_t t2;
    int16_t t3;
    uint8_t t4;
    int16_t t5;
};
#pragma pack(pop)
static_assert(sizeof(RegionDiskRec) == 0x6b, "RegionDiskRec size");

// 0x0046aff0  size 0x4b  CRegion::CRegion
CRegion *__thiscall CRegion::Ctor()
{
    memset(name, 0, 0x28);
    rect[1] = 0;
    rect[0] = 0;
    rect[3] = 0;
    rect[2] = 0;
    rect[5] = 0;
    rect[4] = 0;
    memset(name2, 0, 0x28);
    t3 = 0;
    t2 = 0;
    t1 = 0;
    t4 = 0;
    t5 = 0;
    return this;
}
HOOK_METHOD(0x0046aff0, CRegion, Ctor);

// 0x0046b040  size 0x86  copy a record laid out like CRegion; RET 4
CRegion *__thiscall CRegion::CopyFrom(const CRegion *src)
{
    strncpy(name, src->name, 0x28);   // 0x0048b750
    rect[0] = src->rect[0];
    rect[1] = src->rect[1];
    rect[2] = src->rect[2];
    rect[3] = src->rect[3];
    rect[4] = src->rect[4];
    rect[5] = src->rect[5];
    strncpy(name2, src->name2, 0x28);   // 0x0048b750
    t1 = src->t1;
    t2 = src->t2;
    t3 = src->t3;
    t4 = src->t4;
    t5 = src->t5;
    return this;
}
HOOK_METHOD(0x0046b040, CRegion, CopyFrom);

// 0x0046b0d0  size 0x86  copy from the packed on-disk record; RET 4
CRegion *__thiscall CRegion::CopyFromDisk(const void *src_)
{
    const RegionDiskRec *src = static_cast<const RegionDiskRec *>(src_);
    strncpy(name, src->name1, 0x28);   // 0x0048b750
    rect[0] = src->s[0];
    rect[1] = src->s[1];
    rect[2] = src->s[2];
    rect[3] = src->s[3];
    rect[4] = src->s[4];
    rect[5] = src->s[5];
    strncpy(name2, src->name2, 0x28);   // 0x0048b750
    t1 = src->t1;
    t2 = src->t2;
    t3 = src->t3;
    t4 = src->t4;
    t5 = src->t5;
    return this;
}
HOOK_METHOD(0x0046b0d0, CRegion, CopyFromDisk);

// 0x0046bb80  size 0x10d  CRegionManager::SetRegion  RET 8  (this = the table)
// Ids 0 through 0x2000 inclusive pass the range check (table[0x2000] is out of the table, as in the original).
void __thiscall CRegionManager::SetRegion(int id, CRegion *src)
{
    if (id < 0 || id > 0x2000)
        return;

    // '_' becomes ' ' in the source name; each scan resumes at the character just replaced (0x0048c940 is memchr).
    char *p = src->name;
    for (;;) {
        p = static_cast<char *>(memchr(p, '_', strlen(p)));
        if (p == nullptr)
            break;
        *p = ' ';
    }

    if (regions[id] != nullptr) {
        operator delete(regions[id]);   // operator delete, 0x0048ad60
        regions[id] = nullptr;
    }
    if (strcmp(src->name, "") != 0) {
        CRegion *r = static_cast<CRegion *>(::operator new(0x66));   // operator new, 0x0048ad70
        if (r != nullptr)
            r->Ctor();
        regions[id] = r;
        r->CopyFrom(src);   // ? the original does not test r for null either
    }
    writeRegionsToDisk();
}
HOOK_METHOD(0x0046bb80, CRegionManager, SetRegion);
