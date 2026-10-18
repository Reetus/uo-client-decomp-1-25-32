// Prototypes of the ported procedures in skill_groups.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __cdecl InitSkillGroups(void);   // 0x00472940
void __cdecl SaveSkillGroups(void);   // 0x00472b50
