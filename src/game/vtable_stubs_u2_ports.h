// Prototypes of the ported procedures in vtable_stubs_u2.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

class CFileManager;

void __fastcall FileManager_Dtor_0043fa40(CFileManager *self, void *);   // 0x0043fa40
void __fastcall MapData_SetVtable_00445410(void *self, void *);   // 0x00445410
void __fastcall WalkManager_SetVtable_004487d0(void *self, void *);   // 0x004487d0
