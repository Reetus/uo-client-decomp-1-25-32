// The inline constructor and destructor of CObj (0x00451be0 / 0x00451c70). The original expands both inside
// the constructor and destructor of the 0xa8-byte class and also has them out of line (the pool slab
// vector-constructs and vector-destroys through the out-of-line copies), so they are defined here for every
// file that constructs or destroys a CObj or a class derived from it.
#pragma once

#include "obj.h"
#include "vcall.h"
#include "vtables.h"

// 0x00451be0  size 0x6d
// Vtable 0x0049f7e0, then the field initialisation, skipped while the slab of the 0x78-byte pool is being
// vector-constructed (DAT_00721044 set).
inline CObj::CObj()
{
    if (g_poolBuilding == 0) {
        link44 = nullptr;
        link40 = nullptr;
        link4c = nullptr;
        link48 = nullptr;
        unk_64 = 0;
        unk_60 = 0;
        onLightList = 0;
        link54 = nullptr;
        link50 = nullptr;
        link5c = nullptr;
        link58 = nullptr;
        graphic = 0;
        hue = 0;
        facing = 0;
        unlinked = 1;
        x = static_cast<int16_t>(0xffff);
        y = static_cast<int16_t>(0xffff);
        z = 0;
        barkList = nullptr;
        flags74 = 0;
        unk_3a = 0;
        unk_3c = 0;
        unk_70 = 0;
    }
}

// The default constructor of CContainerObj (no address of its own: the original expands it inside the constructors
// of the mobile class and the corpse class). The dynamic object constructor 0x00451d80 runs first.
inline CContainerObj::CContainerObj()
{
    firstChild = nullptr;
    contGump = nullptr;
    list_b0 = nullptr;
    unk_b8 = 0;
}

// 0x00451c70  size 0x94
// Destructor of the 0x78-byte pooled object: vtable, then (unless suppressed) take it out of the world, delete
// every child at +0x68 and leave the list at 0x0068ce70/74. The base destructor ~CWorldObj follows.
inline CObj::~CObj()
{
    if (g_suppressFlag == 0) {
        if (unlinked == 0)
            this->CObj::ToVoid();
        g_childDeleteFlag = 1;
        // delete every child linked at +0x68 (each deleting destructor unlinks itself)
        while (barkList != nullptr)
            decomp::vdelete(barkList);
        g_childDeleteFlag = 0;
        if (flags74 & 1)
            this->CObj::UnlinkList70();
    }
}
