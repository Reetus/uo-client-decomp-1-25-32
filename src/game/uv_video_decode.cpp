// uV intro video player, frame decode dispatcher (porting pass 3, batch v2):
// 0x00486850, ECX = player, EDX = chunk (UvChunk), plain RET.
// Chunk types: 1 header + palette + 8-bit setup, 2 8-bit frame, 3 / 4 DCT frames
// (3 = intra frame with a quality scale table, 4 = frame with motion blocks).
// The decoders it calls in other batches keep their own conventions:
//   0x00487792 0x00487f71 0x00488750 0x00488892 0x004889f2 0x00489193: cdecl (dst or stream, pitch or dst, ...), registers preserved
//   0x00488b52: cdecl (int *, int *), returns EAX
//   0x00489f80 0x0048a3f0: __fastcall (player, data)
//   0x0048a550: __fastcall (player)
//   0x0048a610: __fastcall (player, surface pointer) + one stack argument (RET 4)
#include <cstdint>
#include <cstring>

#include "../runtime/ported.h"
#include "com_call.h"
#include "uv_video.h"
#include "uv_dct_mb_ports.h"
#include "uv_frame8_ports.h"
#include "globals.h"


#define UV_ReadMotion PORT_FUNC2(&Thunk_UvReadTwoNibbles, int32_t, __cdecl, int32_t *, int32_t *)
#define UV_Output32 PORT_FUNC3(&Thunk_UvOutputRowS32, void, __cdecl, uint8_t *, uint32_t, uint32_t *)
#define UV_Output32Alt PORT_FUNC3(&Thunk_UvDecodeBlocksLut32, void, __cdecl, uint8_t *, uint32_t, uint32_t *)
#define UV_Output16 PORT_FUNC3(&Thunk_UvOutputRowS16, void, __cdecl, uint8_t *, uint32_t, uint32_t *)
#define UV_Output16Alt PORT_FUNC3(&Thunk_UvDecodeBlocksPacked, void, __cdecl, uint8_t *, uint32_t, uint32_t *)
#define UV_AllocBuffers(a1, a2) (UvAllocFrameBuffers((UvPlayer8 *)(a1), a2))
#define UV_BuildColorTable(a1) (UvBuildColorTable((UvPlayer8 *)(a1)))
#define UV_SetupFrame(a1, a2) (UvDecodeFrameSetup((UvPlayer8 *)(a1), a2))
#define UV_BlitFrame8(a1, a2, a3) (UvBlitFrame((UvPlayer8 *)(a1), a2, (uint32_t)(a3)))

static inline int32_t uv_fmul16(int32_t a, int32_t b)
{
    int64_t prod = static_cast<int64_t>(a) * static_cast<int64_t>(b);
    uint32_t r = static_cast<uint32_t>(prod >> 16) + static_cast<uint32_t>((prod >> 15) & 1);
    return static_cast<int32_t>(r);
}

// Division by 100 as the compiler's "imul 0x51eb851f; sar edx,5; add sign bit".
static inline int32_t uv_div100(int32_t v)
{
    int64_t prod = static_cast<int64_t>(0x51eb851f) * static_cast<int64_t>(v);
    int32_t hi = static_cast<int32_t>(prod >> 32);
    hi >>= 5;
    return static_cast<int32_t>(static_cast<uint32_t>(hi) + (static_cast<uint32_t>(hi) >> 31));
}

// Division by 14: "imul 0x92492493; add edx,x; sar edx,3; add sign bit".
static inline int32_t uv_div14(int32_t v)
{
    int64_t prod = static_cast<int64_t>(static_cast<int32_t>(0x92492493u)) * static_cast<int64_t>(v);
    int32_t hi = static_cast<int32_t>(prod >> 32);
    hi = static_cast<int32_t>(static_cast<uint32_t>(hi) + static_cast<uint32_t>(v));
    hi >>= 3;
    return static_cast<int32_t>(static_cast<uint32_t>(hi) + (static_cast<uint32_t>(hi) >> 31));
}

static inline int16_t uv_rd16s(const uint8_t *p)
{
    int16_t v;
    memcpy(&v, p, sizeof(v));
    return v;
}

// IDirectDrawSurface::Lock with the surface-lost retry.
static void uv_lock(void *surface, uint32_t *desc)
{
    long r = com_call(PORT_RET(long), surface, 0x64, static_cast<void *>(nullptr), desc, 1, 0);
    while (r != 0) {
        if (r == static_cast<long>(0x887601c2))
            com_call(PORT_RET(long), surface, 0x6c);   // Restore
        r = com_call(PORT_RET(long), surface, 0x64, static_cast<void *>(nullptr), desc, 1, 0);
    }
}

// 0x00486850  size 0x5f4  Decodes one chunk into the offscreen surfaces (or software buffers) and unlocks them.
void __fastcall UV_DecodeChunk(UvVideo *o, UvChunk *chunk)
{
    const uint8_t *data = chunk->data;
    int32_t type = chunk->type;
    uint32_t lockDesc[27];
    memset(lockDesc, 0, sizeof(lockDesc));
    lockDesc[0] = 0x6c;
    if (type == 1) {
        UV_ReadHeaderPalette(o, data);
    } else if (type == 3) {
        o->surf->width = static_cast<uint32_t>(static_cast<int32_t>(uv_rd16s(data)));
        o->surf->height = static_cast<uint32_t>(static_cast<int32_t>(uv_rd16s(data + 2)));
        o->quality = data[4];
    } else if (type == 4) {
        o->surf->width = static_cast<uint32_t>(static_cast<int32_t>(uv_rd16s(data)));
        o->surf->height = static_cast<uint32_t>(static_cast<int32_t>(uv_rd16s(data + 2)));
        o->quality = data[4];
        o->flag20 = data[7] & 2;
        o->flag24 = data[7] & 1;
    }

    UvSurf *p = o->surf;
    if (!(p->flags & 0x800) && p->surf0 == nullptr) {
        uint32_t desc[27];   // DDSURFACEDESC for the offscreen surfaces
        memset(desc, 0, sizeof(desc));
        desc[0] = 0x6c;
        desc[1] = 7;
        desc[26] = 0x40;
        if (p->flags & 0x180)
            desc[26] = 0x840;
        uint32_t width = p->width;
        desc[3] = width;
        desc[2] = p->height;
        if (p->flags & 0x40) {
            width = width + width;
            desc[3] = width;
        }
        com_call(PORT_RET(long), p->dd, 0x18, desc, &p->surf0, static_cast<void *>(nullptr));   // CreateSurface
        p = o->surf;
        com_call(PORT_RET(long), p->dd, 0x18, desc, &p->surf1, static_cast<void *>(nullptr));
    }

    p = o->surf;
    uint8_t *ptr0;   // [ebp-0x14]
    uint8_t *ptr1;   // [ebp-0x1c]
    uint32_t pitch;  // [ebp-0xc]
    if (!(p->flags & 0x800)) {
        uv_lock(p->surf0, lockDesc);
        pitch = lockDesc[4];
        ptr0 = reinterpret_cast<uint8_t *>(lockDesc[9]);
        p = o->surf;
        uv_lock(p->surf1, lockDesc);
        ptr1 = reinterpret_cast<uint8_t *>(lockDesc[9]);
    } else {
        ptr0 = p->buf0;
        ptr1 = p->buf1;
        pitch = p->width << 1;
        if (p->flags & 0x40)
            pitch = pitch << 1;
    }

    if (static_cast<uint32_t>(type - 1) <= 3) {
        switch (type) {
        case 1:
            UV_AllocBuffers(o, data);
            if (o->surf->bpp != 8)
                UV_BuildColorTable(o);
            UV_BlitFrame8(o, ptr0, ptr1);
            break;
        case 2:
            UV_SetupFrame(o, data);
            UV_BlitFrame8(o, ptr0, ptr1);
            break;
        case 3: {
            int32_t c = 100 - o->quality;
            int32_t base = uv_div100((c + (c + c * 4) * 2) * 2) + 8;
            int32_t step = uv_div100(((c + c * 4) + (c + c * 4) * 4) * 2) + 0xa - base;
            int32_t outer = 0;
            int32_t off = 0;
            do {
                int32_t v = outer;
                for (int32_t n = 8; n != 0; n--) {
                    int32_t t = uv_tab_a[off >> 2];
                    int32_t scaled = static_cast<int32_t>(static_cast<uint32_t>(uv_div14(v) + base) << 16);
                    UVD_Quant[off >> 2] = uv_fmul16(t, scaled);
                    v += step;
                    off += 4;
                }
                outer += step;
            } while (off < 0x100);
            if (o->surf->flags & 0x40) {
                UV_WalkMacroblocks32(o, data + 8, ptr0, ptr1, pitch);
            } else {
                UV_WalkMacroblocks16(o, data + 8, ptr0, ptr1, pitch);
            }
            break;
        }
        case 4: {
            int32_t q = o->quality;
            if (q == 0x64) {
                int32_t off = 0;
                do {
                    for (int32_t n = 8; n != 0; n--) {
                        int32_t t = uv_tab_a[off >> 2];
                        UVD_Quant[off >> 2] = uv_fmul16(t, 0x80000);
                        off += 4;
                    }
                } while (off < 0x100);
            } else {
                float scale = static_cast<float>((uv_const_base - static_cast<double>(q)) * static_cast<double>(uv_const_step));
                int32_t off = 0;
                do {
                    for (int32_t n = 8; n != 0; n--) {
                        double t2 = static_cast<double>(uv_tab_b[off >> 2]);
                        int32_t t1 = uv_tab_a[off >> 2];
                        int32_t r = static_cast<int32_t>(t2 * static_cast<double>(scale) * static_cast<double>(uv_const_65536));   // __ftol
                        UVD_Quant[off >> 2] = uv_fmul16(t1, r);
                        off += 4;
                    }
                } while (off < 0x100);
                UVD_Quant[0] = uv_fmul16(uv_tab_a[0], 0x80000);
            }
            if (o->surf->flags & 0x40) {
                uint8_t *dst = ptr0;
                uint8_t *rowBase = ptr0;
                UV_SetBitStream(data + 8);
                o->state[2] = 0;
                o->state[1] = 0;
                o->state[0] = 0;
                p = o->surf;
                uint32_t y = 0;
                if (p->height != 0) {
                    do {
                        uint32_t x = 0;
                        if (p->width != 0) {
                            do {
                                int32_t mvx;   // [ebp-0x18]
                                int32_t mvy;   // [ebp-8]
                                if (o->flag24 == 0 && UV_ReadMotion(&mvx, &mvy) != 0) {
                                    UV_CopyMotionBlock32(reinterpret_cast<uint32_t *>(ptr0), reinterpret_cast<uint32_t *>(ptr1),
                                                         static_cast<int32_t>(x), static_cast<int32_t>(y), static_cast<int32_t>(o->surf->width), mvx, mvy);
                                } else if (o->flag20 != 0) {
                                    UV_Output32Alt(dst, pitch, o->state);
                                } else {
                                    UV_Output32(dst, pitch, o->state);
                                }
                                p = o->surf;
                                x += 0x10;
                                dst += 0x40;
                            } while (x < p->width);
                        }
                        dst = rowBase + ((pitch >> 2) << 6);
                        rowBase = dst;
                        p = o->surf;
                        y += 0x10;
                    } while (y < p->height);
                }
            } else {
                uint8_t *dst = ptr0;
                uint8_t *rowBase = ptr0;
                UV_SetBitStream(data + 8);
                o->state[2] = 0;
                o->state[1] = 0;
                o->state[0] = 0;
                p = o->surf;
                uint32_t y = 0;
                if (p->height != 0) {
                    do {
                        uint32_t x = 0;
                        if (p->width != 0) {
                            do {
                                int32_t mvx;
                                int32_t mvy;
                                if (o->flag24 == 0 && UV_ReadMotion(&mvx, &mvy) != 0) {
                                    UV_CopyMotionBlock16(reinterpret_cast<uint16_t *>(ptr0), reinterpret_cast<uint16_t *>(ptr1),
                                                         static_cast<int32_t>(x), static_cast<int32_t>(y), static_cast<int32_t>(o->surf->width), mvx, mvy);
                                } else if (o->flag20 != 0) {
                                    UV_Output16Alt(dst, pitch, o->state);
                                } else {
                                    UV_Output16(dst, pitch, o->state);
                                }
                                p = o->surf;
                                x += 0x10;
                                dst += 0x20;
                            } while (x < p->width);
                        }
                        dst = rowBase + ((pitch >> 1) << 5);
                        rowBase = dst;
                        p = o->surf;
                        y += 0x10;
                    } while (y < p->height);
                }
            }
            break;
        }
        default:
            break;
        }
    }

    p = o->surf;
    if (!(p->flags & 0x800)) {
        com_call(PORT_RET(long), p->surf1, 0x80, static_cast<void *>(nullptr));   // Unlock
        p = o->surf;
        com_call(PORT_RET(long), p->surf0, 0x80, static_cast<void *>(nullptr));
    }
}
HOOK_FUNC(0x00486850, UV_DecodeChunk);
