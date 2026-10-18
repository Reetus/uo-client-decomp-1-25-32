#include "map_world.h"
#include "pos3.h"

#include "../runtime/ported.h"
#include "globals.h"
#include "obj.h"
#include "vcall.h"
#include "file_manager.h"

// 0x00445300  size 0x6f
// Chebyshev-style distance. x1/x2 compare as signed 16-bit. Exactly one x >= 0x1400 means the points are
// on different map halves (9999). The wrap-around only applies when x1 < 0x1400 and is used only when smaller.
int __fastcall MapDistance(const int16_t *a, int bx, int by)
{
    int16_t x1 = a[0];
    if ((bx >= 0x1400) ^ (x1 >= 0x1400))
        return 0x270f;
    int dx = static_cast<int>(x1) - bx;
    if (dx < 0)
        dx = -dx;
    int dy = static_cast<int>(a[1]) - by;
    if (dy < 0)
        dy = -dy;
    if (x1 < 0x1400) {
        int wx = 0x1400 - dx;
        if (dx > wx)
            dx = wx;
        int wy = 0x1000 - dy;
        if (dy > wy)
            dy = wy;
    }
    if (dx <= dy)
        dx = dy;
    return dx;
}
HOOK_FUNC(0x00445300, MapDistance);

// 0x004405d0: fetch a record from a data file pair (thiscall on the file manager at 0x0068d980).
static uint8_t *LoadRecord(int fileA, int fileB, int index, int *len, int *unused)
{
    return reinterpret_cast<CFileManager *>(GLOBAL_PTR(void, g_fileManager_storage))->GetRecord(fileA, fileB, index, len, unused);
}

// 0x00445460  size 0x2c3
// Stream the statics around the view centre: objects of the world list whose 8x8 block is outside the
// 6x6 window are removed, the cache slots outside it are freed, then every block of the window that is
// not cached yet is loaded (static records of 7 bytes: id.w, x.b, y.b, z.b, hue.w) and linked into the
// world with CObj_PoolNew + ToWorld. The SEH frame is omitted.
void __thiscall CMap::StreamStatics()
{
    Pos3 pos;
    int bx0 = (viewX - 0x12) >> 3;
    int by0 = (viewY - 0x12) >> 3;
    if (bx0 == lastBlockX && by0 == lastBlockY)
        return;
    int bx1 = bx0 + 5;
    int by1 = by0 + 5;

    // drop objects whose block lies outside the window; list from DAT_0068ce58 through link40
    CObj *obj = g_worldListTail;
    while (obj != nullptr) {
        CObj *next = obj->link40;
        if (decomp::vcall(PORT_RET(int), obj, 0x2c) == 0 && decomp::vcall(PORT_RET(int), obj, 0x64) == 0) {
            int ox = static_cast<int>(obj->x) >> 3;
            int oy = static_cast<int>(obj->y) >> 3;
            if (ox < bx0 || ox > bx1 || oy < by0 || oy > by1)
                decomp::vcall_void(obj, 0x4);
        }
        obj = next;
    }

    // invalidate cache slots outside the window
    int *slot = &cache[0];
    int *p = slot;
    for (int n = 0x24; n != 0; n--) {
        int id = *p;
        int col = id % blocksY;
        int row = id / blocksY;
        if (row < bx0 || row > bx1 || col < by0 || col > by1)
            *p = -1;
        p++;
    }

    for (int by = by0; by <= by1; by++) {
        if (by >= 0 && by < blocksY) {
            for (int bx = bx0; bx <= bx1; bx++) {
                if (bx >= 0 && bx < blocksX) {
                    int index = bx * blocksY + by;
                    int found = 0;
                    int *q = slot;
                    for (int i = 0; i < 0x24; i++, q++) {
                        if (*q == index) {
                            found = 1;
                            break;
                        }
                    }
                    if (!found) {
                        int count;
                        int tmp;
                        uint8_t *rec = g_fileManager.GetRecord(1, 2, index, &count, &tmp);
                        if (count != 0 && count > 0) {
                            do {
                                uint16_t id = *reinterpret_cast<uint16_t *>(rec);
                                if (id >= 0x4000)
                                    id = 1;
                                int dz = static_cast<int8_t>(rec[4]);
                                int dy = static_cast<int8_t>(rec[3]);
                                uint16_t hue = *reinterpret_cast<uint16_t *>(rec + 5);
                                int dx = static_cast<int8_t>(rec[2]);
                                CObj *o = CObj_PoolNew(id);
                                uint8_t *tile = g_tileData + id * 5 * 8;
                                if ((*reinterpret_cast<uint32_t *>(tile) & 0x800000) != 0)
                                    o->facing = tile[5];
                                o->hue = hue;
                                pos.x = static_cast<int16_t>(dx + bx * 8);
                                pos.y = static_cast<int16_t>(dy + by * 8);
                                pos.z = static_cast<int16_t>(dz);
                                PORT_THISCALL1(&CObj::ToWorld, void, o, Pos3)(pos);   // 0x00452580
                                rec += 7;
                                count -= 7;
                            } while (count > 0);
                        }
                        int k = 0;
                        q = slot;
                        while (k < 0x24 && *q != -1) {
                            k++;
                            q++;
                        }
                        // ? no bounds check on k (can be 0x24, one past the cache), as in the binary
                        *(slot + k) = index;
                    }
                }
            }
        }
    }
    lastBlockX = bx0;
    lastBlockY = by0;
}
HOOK_METHOD(0x00445460, CMap, StreamStatics);
