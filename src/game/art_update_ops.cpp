// Art record update (0x00436b50) and tile art lookup with fallback (0x00436c40). gumps.cxx area.
#include <cstdint>

#include "../runtime/ported.h"
#include "batch02_helpers.h"
#include "file_manager.h"
#include "cell_grid.h"
#include "circle_trans_ports.h"
#include "window_helpers_31_ports.h"
#include "art_record_ops_ports.h"
#include "globals.h"

// Art file loader (0x0068d980); 0x00440210(3, 4, index, arg, data, size) stores one record.
#define AUO_ArtLoader GLOBAL_PTR(void, g_fileManager_storage)
#define AUO_StoreRecord(self, a1, a2, a3, a4, a5, a6) (reinterpret_cast<CFileManager *>(self)->ReplaceRecord(a1, a2, a3, a4, (void *)(a5), a6))
// Object at 0x0069e0c0: 0x004307d0(index) after a gump art record, 0x00430880(index - 0x4000) after a tile art record.
#define AUO_Object GLOBAL_PTR(void, g_worldPicker_storage)
#define AUO_AfterGumpArt(o, a1) PORT_THISCALL1(&CellGrid_InvalidateTerrainTile, void, (o), int)(a1)
#define AUO_AfterTileArt(o, a1) PORT_THISCALL1(&CellGrid_RelinkById, void, (o), int)(a1)
// 0x004242b0: empty procedure with two stack arguments (CircleTrans_Nullsub).
#define AUO_Nullsub(a1, a2) PORT_THISCALL2(&CircleTrans_Nullsub, void, GLOBAL_PTR(void, g_regionMusic_storage), int, int)(a1, a2)
// 0x00482c70: ECX = text, EDX = caption, stack = exit flag, RET 4: message box.
#define AUO_FatalBox(a1, a2, a3) (ShowMessageBoxAndExit(a1, a2, a3))
// 0x00476130 on the tile art table (0x0068d2d8).
#define AUO_TileArtGet(o, a1) PORT_THISCALL1(&ArtRecord_Get, void *, (o), int)(a1)
#define AUO_TileArtTable GLOBAL_PTR(void, g_artCache_storage)

// 0x00436b50  size 0xe7
// Store art record `index` (0..0xffff; below 0x4000 gump art, from 0x4000 tile art) in the art file
// loader. mode == -1 stores an empty record; otherwise data points at the record (0x800 bytes for gump
// art, header size + u16 count * 2 + 8 for tile art).
void __stdcall ArtUpdateRecord(const void *data, int index, int mode, int arg)
{
    if (index < 0 || index >= 0x10000) {
        AUO_FatalBox("Tried to update art less than 0 or greater then 65535.", "damn art bug", 0);   // 0x00482c70
        return;
    }
    AUO_Nullsub(index + AUO_unk75821c, arg);                                // 0x004242b0
    if (mode == -1) {
        AUO_StoreRecord(AUO_ArtLoader, 3, 4, index, arg, nullptr, 0);       // 0x00440210
        if (index < 0x4000) {
            AUO_AfterGumpArt(AUO_Object, index);                            // 0x004307d0
            return;
        }
    } else if (index < 0x4000) {
        AUO_StoreRecord(AUO_ArtLoader, 3, 4, index, arg, data, 0x800);      // 0x00440210
        AUO_AfterGumpArt(AUO_Object, index);                                // 0x004307d0
        return;
    } else {
        int size = *static_cast<const int *>(data) + *(reinterpret_cast<const uint16_t *>(static_cast<const char *>(data) + 6)) * 2 + 8;
        AUO_StoreRecord(AUO_ArtLoader, 3, 4, index, arg, data, size);       // 0x00440210
    }
    AUO_AfterTileArt(AUO_Object, index - 0x4000);                           // 0x00430880
}
HOOK_FUNC(0x00436b50, ArtUpdateRecord);

// 0x00436c40  size 0x29
// Tile art record for key id & 0x7fff; when it is missing the record of key 0 is returned instead.
void *__stdcall TileArtOrDefault(unsigned id)
{
    void *art = AUO_TileArtGet(AUO_TileArtTable, static_cast<int>(id & 0x7fff));   // 0x00476130
    if (art == nullptr)
        art = AUO_TileArtGet(AUO_TileArtTable, 0);                                  // 0x00476130
    return art;
}
HOOK_FUNC(0x00436c40, TileArtOrDefault);
