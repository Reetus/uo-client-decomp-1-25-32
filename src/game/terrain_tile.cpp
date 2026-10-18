// CTerrainTile.
#include "terrain_tile.h"

#include <cmath>

#include "../runtime/ported.h"
#include "batch02_helpers.h"
#include "globals.h"
#include "terrain_tile_draw_ports.h"
#include "map_world.h"
#include "dirty_rects_ports.h"

// 0x0042eec0: set the position from {a, b} (ECX = &x).
#define Pos_Set(o, a1, a2) PORT_THISCALL2(&Pos_SetXYZ, void, (o), int, int)(a1, a2)
// 0x00446030: the four corner heights of map cell (x, y) (ECX = the map object).
#define Map_CellHeights(self, a1, a2) ((signed char *)reinterpret_cast<CMap *>(self)->GetNeighborZ(a1, a2))
// 0x0044fe30: ECX = &heights; selects between the two triangulations.
#define Heights_Test(o) PORT_THISCALL0(&RectIsTall, int, (o))()

// Normalised vertex normal scaled to 256, as the original does it: scale in single
// precision, each component through __ftol (truncation).
static void normalise(int a, int b, int c, int *out)
{
    float scale = static_cast<float>((1.0 / std::sqrt(static_cast<double>(a * a + b * b + c * c))) * 256.0);
    out[0] = static_cast<int>(static_cast<double>(a) * static_cast<double>(scale));
    out[1] = static_cast<int>(static_cast<double>(b) * static_cast<double>(scale));
    out[2] = static_cast<int>(static_cast<double>(c) * static_cast<double>(scale));
}

// 0x0042efa0  size 0x6a0
void __thiscall CTerrainTile::Init(int tileIdArg, TileCoords *coords, signed char *corners)
{
    int n[27];                       // 9 cells * 3 components
    int v[12];                       // 4 vertex normals * 3 components
    int idx, tx, ty, k;
    int X, Y, H0, H1, H2, H3;
    int rowBase;
    signed char *h;
    char *self = reinterpret_cast<char *>(this);

    tileId = tileIdArg;
    Pos_Set(&x, coords->a, coords->b);       // 8-byte struct by value: a, then b in the second dword  // ?

    height[0] = corners[0];
    height[1] = corners[1];
    height[2] = corners[2];
    height[3] = corners[3];

    if (height[0] + 0xb > height[2])
        flat = 0;
    else
        flat = 1;

    idx = 0;
    for (ty = y - 1; ty <= y + 1; ty++) {
        for (tx = x - 1; tx <= x + 1; tx++) {
            rowBase = (tx + ty) * 0x16;
            h = Map_CellHeights(g_mapObject, tx, ty);
            X = ((tx - ty) + g_scrollBase) * 0x16;
            Y = rowBase - 0x16;
            H0 = h[0] * 4;
            H1 = h[1] * 4;
            H2 = h[2] * 4;
            H3 = h[3] * 4;

            int d02 = h[0] - h[2];
            if (d02 < 0)
                d02 = -d02;
            int d13 = h[1] - h[3];
            if (d13 < 0)
                d13 = -d13;

            if (d02 <= d13) {
                int dyA = (Y + 0x2c) - Y;
                int d30 = H3 - H0;
                int q = H2 - H0;
                int p = H1 - H0;
                int dxL = (X - 0x16) - X;
                int dxR = (X + 0x16) - X;
                n[idx * 3 + 0] = ((d30 - p) * dyA) >> 1;
                n[idx * 3 + 1] = ((0 * p) + dxL * q - d30 * 0 - dxR * q) >> 1;
                n[idx * 3 + 2] = ((dxR - dxL) * dyA) >> 1;
            } else {
                int r = H3 - H1;
                int dy0 = (Y + 0x2c) - (Y + 0x16);
                int s = H2 - H1;
                int dyC = (Y + 0x16) - Y;
                int d30 = H3 - H0;
                int p = H1 - H0;
                int dxM = X - (X + 0x16);
                int dxLR = (X - 0x16) - (X + 0x16);
                int dxL = (X - 0x16) - X;
                int dxR = (X + 0x16) - X;
                n[idx * 3 + 0] = (((d30 - p) * dyC) + dy0 * r - s * 0) >> 1;
                n[idx * 3 + 1] = ((dxLR * s) + dxL * p - d30 * dxR - dxM * r) >> 1;
                n[idx * 3 + 2] = (((dxR - dxL) * dyC) + dxM * 0 - dxLR * dy0) >> 1;
            }
            idx++;
        }
    }

    // vertex 0: cells 0,1,3,4
    normalise((n[0] + n[3] + n[9] + n[12]) >> 2, (n[1] + n[4] + n[10] + n[13]) >> 2,
              (n[2] + n[5] + n[11] + n[14]) >> 2, &v[0]);
    // vertex 1: cells 1,2,4,5
    normalise((n[3] + n[6] + n[12] + n[15]) >> 2, (n[4] + n[7] + n[13] + n[16]) >> 2,
              (n[5] + n[8] + n[14] + n[17]) >> 2, &v[3]);
    // vertex 2: cells 4,5,7,8
    normalise((n[12] + n[15] + n[21] + n[24]) >> 2, (n[13] + n[16] + n[22] + n[25]) >> 2,
              (n[14] + n[17] + n[23] + n[26]) >> 2, &v[6]);
    // vertex 3: cells 3,4,6,7
    normalise((n[9] + n[12] + n[18] + n[21]) >> 2, (n[10] + n[13] + n[19] + n[22]) >> 2,
              (n[11] + n[14] + n[20] + n[23]) >> 2, &v[9]);

    // light level per vertex
    for (k = 0; k < 4; k++) {
        int lv = (g_lightDirY * v[k * 3 + 1] + v[k * 3 + 2] * g_lightDirZ + v[k * 3 + 0] * g_lightDirX) / 1000;
        if (lv < 0)
            lv = 0;
        else if (lv > 0xf)
            lv = 0xf;
        lightLevel[k] = lv;
    }

    // choose triangulation
    diag0 = 0;
    diag1 = 2;
    if (height[1] > height[0])
        diag0 = 1;
    if (height[3] > height[2])
        diag1 = 3;

    if (Heights_Test(height) != 0) {
        int i;
        if (height[2] > self[0x6c + diag0])
            diag0 = 2;
        i = 0;
        if (height[0] > self[0x6c + diag1]) {
            i = 0;
            diag1 = 0;
        }
        triangle[0] = i;
        triangle[1] = 1;
        triangle[2] = 2;
        triangle[3] = 2;
        triangle[4] = 3;
        triangle[5] = i;
    } else {
        if (height[3] > self[0x6c + diag0])
            diag0 = 3;
        if (height[1] > self[0x6c + diag1])
            diag1 = 1;
        triangle[0] = 0;
        triangle[1] = 1;
        triangle[2] = 3;
        triangle[3] = 1;
        triangle[4] = 2;
        triangle[5] = 3;
    }
}
HOOK_METHOD(0x0042efa0, CTerrainTile, Init);
