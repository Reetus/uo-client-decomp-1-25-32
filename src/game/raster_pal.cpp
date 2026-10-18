// Palette-mapped variant of the software triangle rasterizer (second pass, batch 19).
// Companion of raster.cpp: 0x004503f0 draws with the texel as it is; this one first maps the texel
// through the 16-bit colour table at 0x0068dd9c.
#include "raster.h"

#include "../runtime/ported.h"
#include "globals.h"


// 0x00450c60  size 0x87d
// Draws a Gouraud-lit textured triangle into a 16bpp surface (row stride 0x500 bytes = 640 pixels), the
// same way as 0x004503f0 (see raster.cpp) except for the pixel: the texel is mapped through the colour
// table, then each channel (r = colour >> 10, g = (colour >> 5) & 0x1f, b = colour & 0x1f) goes through
// the light table row (light >> 12) * 32, result ((lut[r] << 5 | lut[g]) << 5) | lut[b].
// ECX = dest (row 0), EDX = texture, stack = (sizeShift, v0, v1, v2, light0, light1, light2, clip), RET 0x20.
// Vertex records are {int x, int y, signed char texIdx}; clip is {left, top, right, bottom}.
// The vertex-order and slope set-up were checked against the assembly (0x00450d31 .. 0x00451030).
// ? when all three y are equal the original picks vertex index 3 as the middle one and reads stack
// garbage there; V has zeroed room for it.
void __fastcall DrawTexturedTrianglePal(void *dest, const uint16_t *tex, int sizeShift, const int *pv0, const int *pv1,
                                        const int *pv2, int light0, int light1, int light2, const int *clip)
{
    int V[4][5] = {0};   // per vertex {x, y, u, v, light}; row 3 only for the degenerate case
    int shift;
    int maxIdx;
    int minIdx;
    int midIdx;
    int yMax;
    int yMin;
    int yMid;
    int yRange;
    int target1;
    int target2;
    int aIdx;
    int bIdx;
    int xMin;
    int xMax;
    int xMid;
    int slopeMid;
    int slopeMax;
    int xl;
    int ul;
    int ll;
    int vl;
    int xr;
    int lr;
    int ur;
    int vr;
    int dxA;
    int dlA;
    int duA;
    int dvA;
    int dxB;
    int dlB;
    int duB;
    int dvB;
    int cury;
    char *row;
    int dy;
    int curU;
    int curV;
    int curL;
    int dU;
    int dV;
    int dL;
    int xL;
    int xR;
    int inv;
    int x;
    uint16_t *px;
    uint16_t colour;
    const uint8_t *lut;
    int r;
    int g;
    int b;

    shift = sizeShift + 6;
    V[0][0] = pv0[0];
    V[0][1] = pv0[1];
    V[0][2] = g_texU[*reinterpret_cast<const signed char *>(reinterpret_cast<const char *>(pv0) + 8) + sizeShift * 4];
    V[0][3] = g_texV[*reinterpret_cast<const signed char *>(reinterpret_cast<const char *>(pv0) + 8) + sizeShift * 4];
    V[0][4] = light0;
    V[1][0] = pv1[0];
    V[1][1] = pv1[1];
    V[1][2] = g_texU[*reinterpret_cast<const signed char *>(reinterpret_cast<const char *>(pv1) + 8) + sizeShift * 4];
    V[1][3] = g_texV[*reinterpret_cast<const signed char *>(reinterpret_cast<const char *>(pv1) + 8) + sizeShift * 4];
    V[1][4] = light1;
    V[2][0] = pv2[0];
    V[2][1] = pv2[1];
    V[2][2] = g_texU[*reinterpret_cast<const signed char *>(reinterpret_cast<const char *>(pv2) + 8) + sizeShift * 4];
    V[2][3] = g_texV[*reinterpret_cast<const signed char *>(reinterpret_cast<const char *>(pv2) + 8) + sizeShift * 4];
    V[2][4] = light2;

    // find the vertices with greatest and least y
    maxIdx = 0;
    minIdx = 0;
    if (V[1][1] > V[0][1])
        maxIdx = 1;
    if (V[1][1] < V[0][1])
        minIdx = 1;
    if (V[2][1] > V[maxIdx][1])
        maxIdx = 2;
    if (V[2][1] < V[minIdx][1])
        minIdx = 2;
    midIdx = 3 - (maxIdx | minIdx);
    yMax = V[maxIdx][1];
    yMin = V[minIdx][1];
    yRange = yMax - yMin;
    if (yRange < 0)
        return;

    // target1 / aIdx: the left edge, target2 / bIdx: the right edge
    target1 = maxIdx;
    target2 = maxIdx;
    aIdx = minIdx;
    bIdx = minIdx;
    yMid = V[midIdx][1];
    xMin = V[minIdx][0];
    if (yMin == yMid) {
        if (xMin < V[midIdx][0])
            bIdx = midIdx;
        else
            aIdx = midIdx;
    } else {
        xMax = V[maxIdx][0];
        if (xMin == xMax) {
            if (xMin >= V[midIdx][0])
                target1 = midIdx;
            else
                target2 = midIdx;
        } else {
            xMid = V[midIdx][0];
            if (xMin == xMid) {
                if (xMin < xMax)
                    target1 = midIdx;
                else
                    target2 = midIdx;
            } else if (yMax == yMid) {
                if (xMax >= xMid)
                    target1 = midIdx;
                else
                    target2 = midIdx;
            } else {
                slopeMid = ((xMid - xMin) << 12) / (yMid - yMin);
                slopeMax = ((xMax - xMin) << 12) / yRange;
                if (slopeMax < slopeMid)
                    target2 = midIdx;
                else
                    target1 = midIdx;
            }
        }
    }

    // edge B start (right edge): x, light, u, v
    xr = V[bIdx][0] << 12;
    lr = V[bIdx][4] << 12;
    ur = V[bIdx][2] << 12;
    vr = V[bIdx][3] << 12;
    // edge A start (left edge)
    xl = (V[aIdx][0] << 12) - 0x1000;
    ll = V[aIdx][4] << 12;
    ul = V[aIdx][2] << 12;
    vl = V[aIdx][3] << 12;

    // edge A slopes toward target1
    dy = V[target1][1] - V[aIdx][1];
    dxA = (((V[target1][0] << 12) - ((V[aIdx][0] << 12) - 0x1000)) - 0x1000) / dy;
    dlA = ((V[target1][4] - V[aIdx][4]) << 12) / dy;
    duA = ((V[target1][2] - V[aIdx][2]) << 12) / dy;
    dvA = ((V[target1][3] - V[aIdx][3]) << 12) / dy;

    // edge B slopes toward target2
    dy = V[target2][1] - V[bIdx][1];
    dxB = ((V[target2][0] - V[bIdx][0]) << 12) / dy;
    dlB = ((V[target2][4] - V[bIdx][4]) << 12) / dy;
    duB = ((V[target2][2] - V[bIdx][2]) << 12) / dy;
    dvB = ((V[target2][3] - V[bIdx][3]) << 12) / dy;

    cury = yMin;
    row = static_cast<char *>(dest) + yMin * 0x500;

    // ---- first half: from yMin to yMid ----
    if (cury <= yMid) {
        do {
            if (cury >= clip[1] && cury < clip[3]) {
                xR = xr >> 12;
                xL = xl >> 12;
                inv = 0x1000 / (xR - xL + 1);
                dU = ((ur - ul) * inv) >> 12;
                dL = ((lr - ll) * inv) >> 12;
                dV = ((vr - vl) * inv) >> 12;
                curU = ul;
                curV = vl;
                curL = ll;
                x = xL;
                if (xL < xR) {
                    px = reinterpret_cast<uint16_t *>(row + xL * 2);
                    do {
                        if (x >= clip[0] && x < clip[2]) {
                            colour = g_colourMap[tex[((curV >> 12) << (shift & 0x1f)) + (curU >> 12)]];
                            lut = g_lightLut + (curL >> 12) * 32;
                            r = colour >> 10;
                            g = (colour >> 5) & 0x1f;
                            b = colour & 0x1f;
                            *px = static_cast<uint16_t>((((lut[r] << 5) | lut[g]) << 5) | lut[b]);
                        }
                        curL += dL;
                        curU += dU;
                        curV += dV;
                        x++;
                        px++;
                    } while (x < xR);
                }
            }
            row += 0x500;
            xl += dxA;
            xr += dxB;
            ll += dlA;
            lr += dlB;
            ul += duA;
            ur += duB;
            vl += dvA;
            vr += dvB;
            cury++;
        } while (cury <= yMid);
    }

    // ---- second half: from yMid to yMax ----
    if (cury > yMax)
        return;

    if (target1 != maxIdx) {
        // edge A ended at the middle vertex: continue it from target1 to the bottom vertex
        dy = yMax - V[target1][1];
        dxA = ((V[maxIdx][0] - V[target1][0]) << 12) / dy;
        dlA = ((V[maxIdx][4] - V[target1][4]) << 12) / dy;
        duA = ((V[maxIdx][2] - V[target1][2]) << 12) / dy;
        dvA = ((V[maxIdx][3] - V[target1][3]) << 12) / dy;
        xl = (V[target1][0] << 12) + dxA - 0x1000;
        ll = (V[target1][4] << 12) + dlA;
        ul = (V[target1][2] << 12) + duA;
        vl = (V[target1][3] << 12) + dvA;
    } else if (target2 != maxIdx) {
        // edge B ended at the middle vertex
        dy = yMax - V[target2][1];
        dxB = ((V[maxIdx][0] - V[target2][0]) << 12) / dy;
        dlB = ((V[maxIdx][4] - V[target2][4]) << 12) / dy;
        duB = ((V[maxIdx][2] - V[target2][2]) << 12) / dy;
        dvB = ((V[maxIdx][3] - V[target2][3]) << 12) / dy;
        xr = (V[target2][0] << 12) + dxB;
        lr = (V[target2][4] << 12) + dlB;
        ur = (V[target2][2] << 12) + duB;
        vr = (V[target2][3] << 12) + dvB;
    }

    do {
        if (cury >= clip[1] && cury < clip[3]) {
            xR = xr >> 12;
            xL = xl >> 12;
            inv = 0x1000 / (xR - xL + 1);
            dL = ((lr - ll) * inv) >> 12;
            dU = ((ur - ul) * inv) >> 12;
            dV = ((vr - vl) * inv) >> 12;
            curU = ul;
            curV = vl;
            curL = ll;
            x = xL;
            if (xL < xR) {
                px = reinterpret_cast<uint16_t *>(row + xL * 2);
                do {
                    if (x >= clip[0] && x < clip[2]) {
                        colour = g_colourMap[tex[((curV >> 12) << (shift & 0x1f)) + (curU >> 12)]];
                        lut = g_lightLut + (curL >> 12) * 32;
                        r = colour >> 10;
                        g = (colour >> 5) & 0x1f;
                        b = colour & 0x1f;
                        *px = static_cast<uint16_t>((((lut[r] << 5) | lut[g]) << 5) | lut[b]);
                    }
                    curL += dL;
                    curU += dU;
                    curV += dV;
                    x++;
                    px++;
                } while (x < xR);
            }
        }
        row += 0x500;
        xl += dxA;
        xr += dxB;
        ll += dlA;
        lr += dlB;
        ul += duA;
        ur += duB;
        vl += dvA;
        vr += dvB;
        cury++;
    } while (cury <= yMax);
}
HOOK_FUNC(0x00450c60, DrawTexturedTrianglePal);
