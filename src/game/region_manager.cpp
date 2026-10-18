#include "region_manager.h"

#include <windows.h>

#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "globals.h"
#include "crt.h"
#include "cstr.h"
#include "circle_trans_ports.h"

#define g_regionMusic GLOBAL_PTR(void, g_regionMusic_storage)

// 0x004703d0 / 0x00470500 / 0x004703c0: 0x10-byte string holder (ctor with a char *, c_str, dtor).
// 0x004242b0 on the region music object: (id + base, music)
#define RegionMusic_Set(o, a1, a2) PORT_THISCALL2(&CircleTrans_Nullsub, void, (o), int, int)(a1, a2)
// 0x0046b0d0 on the new record: fill it from a 0x6b-byte on-disk record.
#define Region_FromDisk(self, a1) (reinterpret_cast<CRegion *>(self)->CopyFromDisk(a1))

#pragma pack(push, 1)
// Format C on-disk record (0x6b bytes), as laid out on the original stack.
struct RegionDiskC {
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
static_assert(sizeof(RegionDiskC) == 0x6b, "RegionDiskC size");

// operator new(0x66) plus the inlined field zeroing (byte +0x63 is left untouched, as in the original).
static CRegion *alloc_region()
{
    CRegion *r = static_cast<CRegion *>(::operator new(0x66));
    if (r != nullptr) {
        memset(r->name, 0, 0x28);
        r->rect[1] = 0;
        r->rect[0] = 0;
        r->rect[3] = 0;
        r->rect[2] = 0;
        r->rect[5] = 0;
        r->rect[4] = 0;
        memset(r->name2, 0, 0x28);
        r->t3 = 0;
        r->t2 = 0;
        r->t1 = 0;
        r->t4 = 0;
        r->t5 = 0;
    }
    return r;
}

// 0x0046b1d0  size 0x84a  CRegionManager::loadRegions  (loads regions.mul into table of 0x2000 region pointers; this = table)
// The first 16-bit word selects the format: < 0xface = A (no header; 8 fields per record), == 0xface = B, > 0xface = C.
// The SEH state around the string holder is omitted.
void __thiscall CRegionManager::loadRegions()
{
    CRegion **table = regions;
    unsigned int header = 0;
    int n;
    CRegion *r;

    CStr holder(g_app.ResolveDataPath(const_cast<char *>("regions.mul")));
    FILE *fp = fopen(holder.GetBuffer(), g_fopenReadMode);
    if (fp == nullptr) {
        OutputDebugStringA("There is no regions.mul file.\n");
    } else {
        header = 0;
        if (fread(&header, 2, 1, fp) == 1) {
            if (static_cast<uint16_t>(header) < 0xface) {
                // ---------------- format A ----------------
                char nameBuf[0x28];
                int16_t aId, aS1, aS2, aS3, aS4, aS5, aS6;
                int32_t aMusic;
                char stage_name1[0x28];
                // ? The original passes two stack slots that this path never writes (id at ebp-0x58 and music at
                // ? ebp-0x4a) and indexes the table with them; they are zero here instead of stale stack bytes.
                int16_t stage_id = 0;
                int32_t stage_music = 0;
                int16_t stage_a, stage_b, stage_c, stage_d, stage_e, stage_f;
                char stage_name2[0x28];
                int16_t stage_t1, stage_t2;
                char stage_t3;

                fseek(fp, 0, 0);
                memset(stage_name2, 0, 0x28);
                stage_t2 = 0;
                stage_t1 = 0;
                stage_t3 = 0;
                n = 0;
                for (;;) {
                    if (fread(nameBuf, 0x28, 1, fp) != 1) break;
                    if (fread(&aId, 2, 1, fp) != 1) break;
                    if (fread(&aS1, 2, 1, fp) != 1) break;
                    if (fread(&aS2, 2, 1, fp) != 1) break;
                    if (fread(&aS3, 2, 1, fp) != 1) break;
                    if (fread(&aS4, 2, 1, fp) != 1) break;
                    if (fread(&aS5, 2, 1, fp) != 1) break;
                    if (fread(&aS6, 2, 1, fp) != 1) break;
                    if (fread(&aMusic, 4, 1, fp) != 1) break;
                    strncpy(stage_name1, nameBuf, 0x28);
                    stage_a = aS1;
                    stage_b = aS2;
                    stage_c = aS3;
                    stage_d = aS4;
                    stage_e = aS5;
                    stage_f = aS6;
                    if (table[stage_id] != nullptr)
                        operator delete(table[stage_id]);
                    r = alloc_region();
                    table[stage_id] = r;
                    r = table[stage_id];
                    strncpy(r->name, stage_name1, 0x28);
                    r->rect[0] = stage_a;
                    r->rect[1] = stage_b;
                    r->rect[2] = stage_c;
                    r->rect[3] = stage_d;
                    r->rect[4] = stage_e;
                    r->rect[5] = stage_f;
                    strncpy(r->name2, stage_name2, 0x28);
                    r->t1 = stage_t1;
                    r->t2 = stage_t2;
                    r->t4 = static_cast<uint8_t>(stage_t3);
                    RegionMusic_Set(g_regionMusic, stage_id + g_regionIdBase, stage_music);
                    n++;
                    if (n >= 0x2000) break;
                }
            } else if (static_cast<uint16_t>(header) == 0xface) {
                // ---------------- format B ----------------
                char bName1[0x28];
                int16_t bId, bA, bB, bC, bD, bE, bF;
                int32_t bMusic;
                char bName2[0x28];
                int16_t bT1, bT2;
                char bT3;
                n = 0;
                for (;;) {
                    if (fread(bName1, 0x28, 1, fp) != 1) break;
                    if (fread(&bId, 2, 1, fp) != 1) break;
                    if (fread(&bA, 2, 1, fp) != 1) break;
                    if (fread(&bB, 2, 1, fp) != 1) break;
                    if (fread(&bC, 2, 1, fp) != 1) break;
                    if (fread(&bD, 2, 1, fp) != 1) break;
                    if (fread(&bE, 2, 1, fp) != 1) break;
                    if (fread(&bF, 2, 1, fp) != 1) break;
                    if (fread(&bMusic, 4, 1, fp) != 1) break;
                    if (fread(bName2, 0x28, 1, fp) != 1) break;
                    if (fread(&bT1, 2, 1, fp) != 1) break;
                    if (fread(&bT2, 2, 1, fp) != 1) break;
                    if (fread(&bT3, 1, 1, fp) != 1) break;
                    if (table[bId] != nullptr)
                        operator delete(table[bId]);
                    r = alloc_region();
                    table[bId] = r;
                    r = table[bId];
                    strncpy(r->name, bName1, 0x28);
                    r->rect[0] = bA;
                    r->rect[1] = bB;
                    r->rect[2] = bC;
                    r->rect[3] = bD;
                    r->rect[4] = bE;
                    r->rect[5] = bF;
                    strncpy(r->name2, bName2, 0x28);
                    r->t1 = bT1;
                    r->t2 = bT2;
                    r->t4 = static_cast<uint8_t>(bT3);
                    RegionMusic_Set(g_regionMusic, bId + g_regionIdBase, bMusic);
                    n++;
                    if (n >= 0x2000) break;
                }
            } else {
                // ---------------- format C (disk record, 0x6b bytes, copied with 0x0046b0d0) ----------------
                RegionDiskC c;
                n = 0;
                for (;;) {
                    if (fread(c.name1, 0x28, 1, fp) != 1) break;
                    if (fread(&c.id, 2, 1, fp) != 1) break;
                    if (fread(&c.s[0], 2, 1, fp) != 1) break;
                    if (fread(&c.s[1], 2, 1, fp) != 1) break;
                    if (fread(&c.s[2], 2, 1, fp) != 1) break;
                    if (fread(&c.s[3], 2, 1, fp) != 1) break;
                    if (fread(&c.s[4], 2, 1, fp) != 1) break;
                    if (fread(&c.s[5], 2, 1, fp) != 1) break;
                    if (fread(&c.music, 4, 1, fp) != 1) break;
                    if (fread(c.name2, 0x28, 1, fp) != 1) break;
                    if (fread(&c.t1, 2, 1, fp) != 1) break;
                    if (fread(&c.t2, 2, 1, fp) != 1) break;
                    if (fread(&c.t3, 2, 1, fp) != 1) break;
                    if (fread(&c.t4, 1, 1, fp) != 1) break;
                    if (fread(&c.t5, 2, 1, fp) != 1) break;
                    if (table[c.id] != nullptr)
                        operator delete(table[c.id]);
                    r = alloc_region();
                    table[c.id] = r;
                    Region_FromDisk(table[c.id], &c);
                    RegionMusic_Set(g_regionMusic, c.id + g_regionIdBase, c.music);
                    n++;
                    if (n >= 0x2000) break;
                }
            }
        }
        fclose(fp);
    }
}
HOOK_METHOD(0x0046b1d0, CRegionManager, loadRegions);

// 0x0046ba20  size 0x154  CRegionManager::writeRegionsToDisk  (this = table of 0x2000 region pointers)
// As in the binary, the staged record is built for every region but never written with fwrite:
// only fopen("wb") and fclose happen. The SEH state around the holder is omitted.
void __thiscall CRegionManager::writeRegionsToDisk()
{
    RegionDiskC st;
    CRegion **p = regions;
    int i;

    CStr holder(g_app.ResolveDataPath(const_cast<char *>("regions.mul")));
    FILE *fp = fopen(holder.GetBuffer(), "wb");
    if (fp == nullptr) {
        OutputDebugStringA("Can not open regions.mul file for writing.\n");
        return;
    }
    i = 0;
    do {
        if (*p != nullptr) {
            strncpy(st.name1, (*p)->name, 0x28);
            st.id = static_cast<int16_t>(i);
            st.s[0] = (*p)->rect[0];
            st.s[1] = (*p)->rect[1];
            st.s[2] = (*p)->rect[2];
            st.s[3] = (*p)->rect[3];
            st.s[4] = (*p)->rect[4];
            st.s[5] = (*p)->rect[5];
            st.music = 0;
            strncpy(st.name2, (*p)->name2, 0x28);
            st.t1 = (*p)->t1;
            st.t2 = (*p)->t2;
            st.t3 = (*p)->t3;
            st.t4 = (*p)->t4;
            st.t5 = (*p)->t5;
        }
        i++;
        p++;
    } while (i < 0x2000);
    fclose(fp);
}
HOOK_METHOD(0x0046ba20, CRegionManager, writeRegionsToDisk);
