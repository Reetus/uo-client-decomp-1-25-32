// Third porting pass, batch u3: CRegionManager constructor (0x0046b160), called by the static initialiser 0x0044acd0.
#include "region_manager.h"

#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"

#define RM_GOTO_NAME reinterpret_cast<const char *>(GADDR(str_004ae528))   // "goto"

// 0x0046b160  size 0x64
// Clear the 0x2000 region pointers, load regions.mul (0x0046b1d0), then look for "goto" in the dot-command table and
// remember its index at +0x10000 (-1 when absent). Returns this.
CRegionManager *__thiscall CRegionManager::Ctor()
{
    memset(this, 0, 0x2000 * sizeof(uint32_t));
    this->CRegionManager::loadRegions();   // loadRegions
    // The original stores the index at +0x10000 (the declared gotoIndex sits at +0x8000, inside the slot table).
    RAWF(int32_t, this, 0x10000) = -1;
    for (int index = 0; g_dotCommandTable[index * 2] != -1; index++) {
        if (_stricmp(RM_GOTO_NAME, reinterpret_cast<const char *>(g_dotCommandTable[index * 2 + 1])) == 0) {
            RAWF(int32_t, this, 0x10000) = index;
            break;
        }
    }
    return this;
}
HOOK_METHOD(0x0046b160, CRegionManager, Ctor);
