// CApp: the application object. One static instance lives at 0x0069e0e8 in
// the original image.
//
// Layout is partial. Only fields that ported code touches are named; the rest
// is opaque padding at the offsets the original code uses. Offsets come from
// decomp/merged (chunk 15) and decomp/pass2/0x0044c030.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"
#include "globals.h"

class CApp {
public:
    // 0x0044c340: dataPath + "\\" + name into the shared buffer at 0x0068ced8.
    char *__thiscall ResolveDataPath(char *name);
    // 0x0044c3f0: the same against dataPath2, result in the buffer at 0x0069fbc8.
    char *__thiscall ResolveDataPath2(char *name);
    // Batch 04, app_startup.cpp. The object is larger than the named fields below (at least 0x1960 bytes);
    // those methods reach the other fields by byte offset.
    CApp *__thiscall Ctor();                    // 0x0044b280  constructor (vtable 0x0049f72c)
    void __thiscall AllocPlayerMobile();        // 0x0044b660  g_player = new CMobile (0x20c bytes)
    void __thiscall Dtor();                     // 0x0044c030  destructor body
    void __thiscall RunMacro(int macroIndex);   // 0x0044c8e0  run the macro with that index (RET 4)
    void __thiscall LoadKeynamesAndMacros();    // 0x0044d780  keynames.txt + macros.txt
    // Second pass, app_support.cpp.
    void *__thiscall ScalarDeletingDtor(int flags);   // 0x0044b620  vtable slot 0, RET 4
    void __thiscall LoadObsceneAndIgnoreLists();      // 0x0044b6d0  obscene.lst (+0x56c / +0x818) and ignore.lst (+0x568 / +0x570)
    void __thiscall SaveObsceneOrIgnoreList(int which);   // 0x0044bd50  RET 4: 0 = obscene.lst, 1 = ignore.lst
    void __thiscall SaveMacros();                     // 0x0044cfa0  write macros.txt
    void __thiscall SaveCharData();             // 0x0044d690  write chardata.mul (two 5-word blocks at +0x3f0, +0x3fa)

    uint8_t pad_000[0x1e8];
    char dataPath[0x250];   // +0x1e8  base directory for data files
    char dataPath2[0x250];  // +0x438  second base directory
};

static_assert(offsetof(CApp, dataPath) == 0x1e8, "CApp::dataPath");
static_assert(offsetof(CApp, dataPath2) == 0x438, "CApp::dataPath2");

#define g_app GLOBAL_AS(CApp, g_app_storage)
