// Software triangle rasterizers. decomp/pass2/0x004503f0.cpp.
// (0x00450c60, the palette-mapped variant, is not ported: its pass2 file is marked "partial".)
#include "raster.h"

#include "../runtime/ported.h"
#include "globals.h"


// 0x004503f0  size 0x869
// Draws a Gouraud-lit textured triangle into a 16bpp 555 surface (row stride 0x500 bytes = 640 pixels).
// Per vertex {x, y, texIdx(char)} u/v come from the two int tables above. Vertices are sorted by y; edge A
// (left) runs from the left start vertex to target1, edge B (right) from the right start vertex to target2;
// when the scan passes the middle vertex the edge that ended there is re-derived from the middle vertex to
// the bottom one. Fixed point is 20.12. Output pixel: ((lut[r] << 5 | lut[g]) << 5) | lut[b] with
// lut = 0x6f92a8 + (light >> 12) * 32 and r = texel >> 10.
// ? the exact meaning of the 12-byte vertex record and of the tables is inferred from use only.
void __fastcall DrawLitTexturedTriangle(void *dest, const uint16_t *tex, int sizeShift, const int *pv0, const int *pv1,
                                        const int *pv2, int light0, int light1, int light2, const int *clip)
{
    int V[3][5];   // per vertex {x, y, u, v, light}
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
    uint16_t texel;
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
                curU = ul;
                xL = xl >> 12;
                inv = 0x1000 / (xR - xL + 1);
                dU = ((ur - ul) * inv) >> 12;
                dL = ((lr - ll) * inv) >> 12;
                dV = ((vr - vl) * inv) >> 12;
                curV = vl;
                curL = ll;
                x = xL;
                if (xL < xR) {
                    px = reinterpret_cast<uint16_t *>(row + xL * 2);
                    do {
                        if (x >= clip[0] && x < clip[2]) {
                            texel = tex[((curV >> 12) << shift) + (curU >> 12)];
                            lut = g_lightLut + (curL >> 12) * 32;
                            r = texel >> 10;
                            g = (texel >> 5) & 0x1f;
                            b = texel & 0x1f;
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
        ll = (V[target1][4] << 12) + dlA;
        ul = (V[target1][2] << 12) + duA;
        xl = (V[target1][0] << 12) + dxA - 0x1000;
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
            curU = ul;
            xL = xl >> 12;
            inv = 0x1000 / (xR - xL + 1);
            curV = vl;
            dL = ((lr - ll) * inv) >> 12;
            dU = ((ur - ul) * inv) >> 12;
            dV = ((vr - vl) * inv) >> 12;
            curL = ll;
            x = xL;
            if (xL < xR) {
                px = reinterpret_cast<uint16_t *>(row + xL * 2);
                do {
                    if (x >= clip[0] && x < clip[2]) {
                        texel = tex[((curV >> 12) << shift) + (curU >> 12)];
                        lut = g_lightLut + (curL >> 12) * 32;
                        r = texel >> 10;
                        g = (texel >> 5) & 0x1f;
                        b = texel & 0x1f;
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
HOOK_FUNC(0x004503f0, DrawLitTexturedTriangle);
