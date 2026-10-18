// skillgrp.mul: default skill names and groups, load and save (skills window data).
#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "globals.h"
#include "crt.h"


// 0x00472940  size 0x20c
// Fill in the default skill names and the single default group, then load skillgrp.mul over them if it exists.
// The inlined strcpy sequences (repne scasb / rep movs) are host strcpy calls.
void __cdecl InitSkillGroups()
{
    g_skillCount = 9;
    strcpy(g_skillNames + 0x00, "Magic Defense");
    strcpy(g_skillNames + 0x50, "Battle Defense");
    strcpy(g_skillNames + 0xa0, "Stealing");
    strcpy(g_skillNames + 0xf0, "Hiding");
    strcpy(g_skillNames + 0x140, "First Aid");
    strcpy(g_skillNames + 0x190, "Peek");
    g_groupCount = 1;
    strcpy(g_skillNames + 0x1e0, "Melee");
    strcpy(g_skillNames + 0x230, "Ranged Weapons");
    strcpy(g_skillNames + 0x280, "Alchemy");
    strcpy(g_groupNames, "Miscellaneous");

    FILE *f = fopen(g_app.ResolveDataPath(const_cast<char *>("skillgrp.mul")), g_fopenReadMode);
    if (f != nullptr) {
        fread(&g_groupCount, 4, 1, f);
        int i = 1;
        if (g_groupCount > 1) {
            char *p = g_groupNames + 0x11;
            do {
                fread(p, 1, 0x11, f);
                i = i + 1;
                p += 0x11;
            } while (i < g_groupCount);
        }
        int32_t *q = g_skillGroup;
        do {
            fread(q, 4, 1, f);
            q = q + 1;
        } while (reinterpret_cast<intptr_t>(q) < static_cast<intptr_t>(GEND(g_skillGroup)));
        fclose(f);
    }
}
HOOK_FUNC(0x00472940, InitSkillGroups);

// 0x00472b50  size 0x8c
// Write skillgrp.mul: group count, group names 1..count-1 (0x11 bytes each), then the skill -> group dwords.
void __cdecl SaveSkillGroups()
{
    FILE *f = fopen(g_app.ResolveDataPath(const_cast<char *>("skillgrp.mul")), "wb");
    fwrite(&g_groupCount, 4, 1, f);
    int i = 1;
    if (g_groupCount > 1) {
        char *p = g_groupNames + 0x11;
        do {
            fwrite(p, 1, 0x11, f);
            i = i + 1;
            p += 0x11;
        } while (i < g_groupCount);
    }
    int32_t *q = g_skillGroup;
    do {
        fwrite(q, 4, 1, f);
        q = q + 1;
    } while (reinterpret_cast<intptr_t>(q) < static_cast<intptr_t>(GEND(g_skillGroup)));
    fclose(f);
}
HOOK_FUNC(0x00472b50, SaveSkillGroups);
