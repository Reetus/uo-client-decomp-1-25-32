// Software triangle rasterizers. decomp/pass2/0x004503f0.cpp.
#pragma once

#include <cstdint>

// 0x004503f0  size 0x869
// ECX = destination 16bpp surface (row stride 640 pixels), EDX = texture (16bpp, row length
// 1 << (sizeShift + 6) texels). Stack (RET 0x20): sizeShift, v0, v1, v2 (each {int x, int y, char texIdx}),
// light0, light1, light2, clip {xmin, ymin, xmax, ymax}.
void __fastcall DrawLitTexturedTriangle(void *dest, const uint16_t *tex, int sizeShift, const int *pv0, const int *pv1,
                                        const int *pv2, int light0, int light1, int light2, const int *clip);
