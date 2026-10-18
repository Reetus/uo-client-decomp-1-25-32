// uV intro video player, DCT codec: the macroblock drivers (batch v3, hand-written assembly).
//   0x00487792 / 0x00487f71  decode six sparse blocks of a macroblock and write 16 rows of 32 / 16-bit pixels
//   0x004892d5               DC prediction + VLC decode + IDCT of six blocks (internal)
//   0x00488750 / 0x00489193  decode a VLC macroblock and write 16 rows of 32 / 16-bit pixels
//   0x00488892 / 0x004889f2  decode a VLC macroblock and write 16 rows through the pointer LUTs (packed / 32-bit)
//   0x00488b52               read the optional pair of 4-bit values from the bit stream
// The seven entries called from the frame decoder (0x00486850) and the macroblock walkers (0x004857a0 /
// 0x00485aa0) keep a hook. They are cdecl procedures that preserve every register but EAX, so each hook is a
// naked thunk that re-pushes the stack arguments and also keeps ECX / EDX around the C++ call.
#include "uv_dct.h"

#include "../runtime/ported.h"
#include "globals.h"

// Call wrappers: each names the original address it stands for (for check_port) and nothing else.
static inline void UvSparse() { uv_dct_decode_block_sparse(); /* 0x00486e77 */ }
static inline void UvPass1(const int32_t *in, int32_t *out) { uv_dct_idct_pass1(in, out); /* 0x00489c42 */ }
static inline void UvPass2(const int32_t *in, int32_t *out) { uv_dct_idct_pass2(in, out); /* 0x00489e6f */ }
static inline void UvOutS32(const int32_t *l, const uint32_t *c, uint8_t *d) { uv_dct_output_block_s32(l, c, reinterpret_cast<uint32_t *>(d)); /* 0x0048732c */ }
static inline void UvOutS16(const int32_t *l, const uint32_t *c, uint8_t *d) { uv_dct_output_block_s16(l, c, reinterpret_cast<uint16_t *>(d)); /* 0x00487557 */ }
static inline void UvOutE(const int32_t *l, const uint32_t *c, uint8_t *d) { uv_dct_output_block_e(l, c, reinterpret_cast<uint32_t *>(d)); /* 0x00488bed */ }
static inline void UvOutF(const int32_t *l, const uint32_t *c, uint8_t *d) { uv_dct_output_block_f(l, c, reinterpret_cast<uint32_t *>(d)); /* 0x00488ec0 */ }
static inline void UvDcBlock(int32_t *dc) { uv_dct_decode_dc_and_block(dc); /* 0x004892d5 */ }


// 32 bits of the stream at bit `pos` (mov/mov/shld).
static inline uint32_t Peek32(const uint32_t *base, uint32_t pos)
{
    uint32_t w0 = base[pos >> 5];
    uint32_t w1 = base[(pos >> 5) + 1];
    return static_cast<uint32_t>(((static_cast<uint64_t>(w0) << 32) | w1) << (pos & 31) >> 32);
}

// DC difference: table lookup of (size << 4 | codeLength), then `size` magnitude bits (a clear top bit means a
// negative value, v - (2^size - 1)); adds it to *pred and returns the new predictor.
static int32_t ReadDc(const uint32_t *base, uint32_t &pos, unsigned shift, uint32_t table, int32_t *pred)
{
    uint32_t entry = *reinterpret_cast<const uint8_t *>(static_cast<uintptr_t>(table + (Peek32(base, pos) >> shift)));
    uint32_t size = entry >> 4;
    pos += entry & 0xf;
    uint32_t v = 0;
    if (size != 0) {
        uint32_t window = Peek32(base, pos);
        pos += size;
        v = window >> ((0x20 - size) & 31);
        if (((v >> ((size - 1) & 31)) & 1) == 0)
            v -= (1u << (size & 31)) - 1;
    }
    uint32_t sum = v + static_cast<uint32_t>(*pred);
    *pred = static_cast<int32_t>(sum);
    return static_cast<int32_t>(sum);
}

// Destination of the pass-2 output rows of each of the six blocks of a macroblock: luma blocks 0/1 side by side in
// rows of 0x40 bytes, blocks 2/3 the same 8 rows below, then Cb and Cr with rows of 0x20 bytes.

// 0x00487792  size 0x7df
// Macroblock of six sparse-coded blocks, 16 rows of 32-bit pixels. cdecl(src, dst, pitch); src = stream,
// dst = first row, pitch in bytes. Preserves every register but EAX (the hook keeps ECX / EDX).
static void UvDecodeBlocksS32(const uint8_t *src, uint8_t *dst, int32_t pitch)
{
    UVD_BitPtr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(src + 1));
    UVD_BitBuf = src[0];
    UVD_BitPos = 8;
    // block 0
    UvSparse();
    UvPass1(UVD_Coeff, uv_0075d100);
    UvPass1(uv_0075d020, uv_0075d104);
    UvPass1(uv_0075d040, uv_0075d108);
    UvPass1(uv_0075d060, uv_0075d10c);
    UvPass1(uv_0075d080, uv_0075d110);
    UvPass1(uv_0075d0a0, uv_0075d114);
    UvPass1(uv_0075d0c0, uv_0075d118);
    UvPass1(uv_0075d0e0, uv_0075d11c);
    UvPass2(uv_0075d100, uv_0075d220);
    UvPass2(uv_0075d124, uv_0075d260);
    UvPass2(uv_0075d148, uv_0075d2a0);
    UvPass2(uv_0075d16c, uv_0075d2e0);
    UvPass2(uv_0075d190, uv_0075d320);
    UvPass2(uv_0075d1b4, uv_0075d360);
    UvPass2(uv_0075d1d8, uv_0075d3a0);
    UvPass2(uv_0075d1fc, uv_0075d3e0);
    // block 1
    UvSparse();
    UvPass1(UVD_Coeff, uv_0075d100);
    UvPass1(uv_0075d020, uv_0075d104);
    UvPass1(uv_0075d040, uv_0075d108);
    UvPass1(uv_0075d060, uv_0075d10c);
    UvPass1(uv_0075d080, uv_0075d110);
    UvPass1(uv_0075d0a0, uv_0075d114);
    UvPass1(uv_0075d0c0, uv_0075d118);
    UvPass1(uv_0075d0e0, uv_0075d11c);
    UvPass2(uv_0075d100, uv_0075d240);
    UvPass2(uv_0075d124, uv_0075d280);
    UvPass2(uv_0075d148, uv_0075d2c0);
    UvPass2(uv_0075d16c, uv_0075d300);
    UvPass2(uv_0075d190, uv_0075d340);
    UvPass2(uv_0075d1b4, uv_0075d380);
    UvPass2(uv_0075d1d8, uv_0075d3c0);
    UvPass2(uv_0075d1fc, uv_0075d400);
    // block 2
    UvSparse();
    UvPass1(UVD_Coeff, uv_0075d100);
    UvPass1(uv_0075d020, uv_0075d104);
    UvPass1(uv_0075d040, uv_0075d108);
    UvPass1(uv_0075d060, uv_0075d10c);
    UvPass1(uv_0075d080, uv_0075d110);
    UvPass1(uv_0075d0a0, uv_0075d114);
    UvPass1(uv_0075d0c0, uv_0075d118);
    UvPass1(uv_0075d0e0, uv_0075d11c);
    UvPass2(uv_0075d100, uv_0075d420);
    UvPass2(uv_0075d124, uv_0075d460);
    UvPass2(uv_0075d148, uv_0075d4a0);
    UvPass2(uv_0075d16c, uv_0075d4e0);
    UvPass2(uv_0075d190, uv_0075d520);
    UvPass2(uv_0075d1b4, uv_0075d560);
    UvPass2(uv_0075d1d8, uv_0075d5a0);
    UvPass2(uv_0075d1fc, uv_0075d5e0);
    // block 3
    UvSparse();
    UvPass1(UVD_Coeff, uv_0075d100);
    UvPass1(uv_0075d020, uv_0075d104);
    UvPass1(uv_0075d040, uv_0075d108);
    UvPass1(uv_0075d060, uv_0075d10c);
    UvPass1(uv_0075d080, uv_0075d110);
    UvPass1(uv_0075d0a0, uv_0075d114);
    UvPass1(uv_0075d0c0, uv_0075d118);
    UvPass1(uv_0075d0e0, uv_0075d11c);
    UvPass2(uv_0075d100, uv_0075d440);
    UvPass2(uv_0075d124, uv_0075d480);
    UvPass2(uv_0075d148, uv_0075d4c0);
    UvPass2(uv_0075d16c, uv_0075d500);
    UvPass2(uv_0075d190, uv_0075d540);
    UvPass2(uv_0075d1b4, uv_0075d580);
    UvPass2(uv_0075d1d8, uv_0075d5c0);
    UvPass2(uv_0075d1fc, uv_0075d600);
    // block 4
    UvSparse();
    UvPass1(UVD_Coeff, uv_0075d100);
    UvPass1(uv_0075d020, uv_0075d104);
    UvPass1(uv_0075d040, uv_0075d108);
    UvPass1(uv_0075d060, uv_0075d10c);
    UvPass1(uv_0075d080, uv_0075d110);
    UvPass1(uv_0075d0a0, uv_0075d114);
    UvPass1(uv_0075d0c0, uv_0075d118);
    UvPass1(uv_0075d0e0, uv_0075d11c);
    UvPass2(uv_0075d100, GLOBAL_PTR(int32_t, uv_0075d620));
    UvPass2(uv_0075d124, GLOBAL_PTR(int32_t, uv_0075d640));
    UvPass2(uv_0075d148, GLOBAL_PTR(int32_t, uv_0075d660));
    UvPass2(uv_0075d16c, GLOBAL_PTR(int32_t, uv_0075d680));
    UvPass2(uv_0075d190, GLOBAL_PTR(int32_t, uv_0075d6a0));
    UvPass2(uv_0075d1b4, GLOBAL_PTR(int32_t, uv_0075d6c0));
    UvPass2(uv_0075d1d8, GLOBAL_PTR(int32_t, uv_0075d6e0));
    UvPass2(uv_0075d1fc, GLOBAL_PTR(int32_t, uv_0075d700));
    // block 5
    UvSparse();
    UvPass1(UVD_Coeff, uv_0075d100);
    UvPass1(uv_0075d020, uv_0075d104);
    UvPass1(uv_0075d040, uv_0075d108);
    UvPass1(uv_0075d060, uv_0075d10c);
    UvPass1(uv_0075d080, uv_0075d110);
    UvPass1(uv_0075d0a0, uv_0075d114);
    UvPass1(uv_0075d0c0, uv_0075d118);
    UvPass1(uv_0075d0e0, uv_0075d11c);
    UvPass2(uv_0075d100, uv_0075d720);
    UvPass2(uv_0075d124, uv_0075d740);
    UvPass2(uv_0075d148, uv_0075d760);
    UvPass2(uv_0075d16c, uv_0075d780);
    UvPass2(uv_0075d190, uv_0075d7a0);
    UvPass2(uv_0075d1b4, uv_0075d7c0);
    UvPass2(uv_0075d1d8, uv_0075d7e0);
    UvPass2(uv_0075d1fc, uv_0075d800);
    // 16 rows of output
    UvOutS32(uv_0075d220, uv_0075d620, dst);
    dst += pitch;
    UvOutS32(uv_0075d260, uv_0075d620, dst);
    dst += pitch;
    UvOutS32(uv_0075d2a0, uv_0075d640, dst);
    dst += pitch;
    UvOutS32(uv_0075d2e0, uv_0075d640, dst);
    dst += pitch;
    UvOutS32(uv_0075d320, uv_0075d660, dst);
    dst += pitch;
    UvOutS32(uv_0075d360, uv_0075d660, dst);
    dst += pitch;
    UvOutS32(uv_0075d3a0, uv_0075d680, dst);
    dst += pitch;
    UvOutS32(uv_0075d3e0, uv_0075d680, dst);
    dst += pitch;
    UvOutS32(uv_0075d420, uv_0075d6a0, dst);
    dst += pitch;
    UvOutS32(uv_0075d460, uv_0075d6a0, dst);
    dst += pitch;
    UvOutS32(uv_0075d4a0, uv_0075d6c0, dst);
    dst += pitch;
    UvOutS32(uv_0075d4e0, uv_0075d6c0, dst);
    dst += pitch;
    UvOutS32(uv_0075d520, uv_0075d6e0, dst);
    dst += pitch;
    UvOutS32(uv_0075d560, uv_0075d6e0, dst);
    dst += pitch;
    UvOutS32(uv_0075d5a0, uv_0075d700, dst);
    dst += pitch;
    UvOutS32(uv_0075d5e0, uv_0075d700, dst);
}
__declspec(naked) void Thunk_UvDecodeBlocksS32()
{
    __asm {
        push ecx
        push edx
        push dword ptr [esp + 0x14]
        push dword ptr [esp + 0x14]
        push dword ptr [esp + 0x14]
        call UvDecodeBlocksS32
        add esp, 12
        pop edx
        pop ecx
        ret
    }
}
HOOK_FUNC(0x00487792, Thunk_UvDecodeBlocksS32);

// 0x00487f71  size 0x7df
// Macroblock of six sparse-coded blocks, 16 rows of 16-bit pixels. cdecl(src, dst, pitch); src = stream,
// dst = first row, pitch in bytes. Preserves every register but EAX (the hook keeps ECX / EDX).
static void UvDecodeBlocksS16(const uint8_t *src, uint8_t *dst, int32_t pitch)
{
    UVD_BitPtr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(src + 1));
    UVD_BitBuf = src[0];
    UVD_BitPos = 8;
    // block 0
    UvSparse();
    UvPass1(UVD_Coeff, uv_0075d100);
    UvPass1(uv_0075d020, uv_0075d104);
    UvPass1(uv_0075d040, uv_0075d108);
    UvPass1(uv_0075d060, uv_0075d10c);
    UvPass1(uv_0075d080, uv_0075d110);
    UvPass1(uv_0075d0a0, uv_0075d114);
    UvPass1(uv_0075d0c0, uv_0075d118);
    UvPass1(uv_0075d0e0, uv_0075d11c);
    UvPass2(uv_0075d100, uv_0075d220);
    UvPass2(uv_0075d124, uv_0075d260);
    UvPass2(uv_0075d148, uv_0075d2a0);
    UvPass2(uv_0075d16c, uv_0075d2e0);
    UvPass2(uv_0075d190, uv_0075d320);
    UvPass2(uv_0075d1b4, uv_0075d360);
    UvPass2(uv_0075d1d8, uv_0075d3a0);
    UvPass2(uv_0075d1fc, uv_0075d3e0);
    // block 1
    UvSparse();
    UvPass1(UVD_Coeff, uv_0075d100);
    UvPass1(uv_0075d020, uv_0075d104);
    UvPass1(uv_0075d040, uv_0075d108);
    UvPass1(uv_0075d060, uv_0075d10c);
    UvPass1(uv_0075d080, uv_0075d110);
    UvPass1(uv_0075d0a0, uv_0075d114);
    UvPass1(uv_0075d0c0, uv_0075d118);
    UvPass1(uv_0075d0e0, uv_0075d11c);
    UvPass2(uv_0075d100, uv_0075d240);
    UvPass2(uv_0075d124, uv_0075d280);
    UvPass2(uv_0075d148, uv_0075d2c0);
    UvPass2(uv_0075d16c, uv_0075d300);
    UvPass2(uv_0075d190, uv_0075d340);
    UvPass2(uv_0075d1b4, uv_0075d380);
    UvPass2(uv_0075d1d8, uv_0075d3c0);
    UvPass2(uv_0075d1fc, uv_0075d400);
    // block 2
    UvSparse();
    UvPass1(UVD_Coeff, uv_0075d100);
    UvPass1(uv_0075d020, uv_0075d104);
    UvPass1(uv_0075d040, uv_0075d108);
    UvPass1(uv_0075d060, uv_0075d10c);
    UvPass1(uv_0075d080, uv_0075d110);
    UvPass1(uv_0075d0a0, uv_0075d114);
    UvPass1(uv_0075d0c0, uv_0075d118);
    UvPass1(uv_0075d0e0, uv_0075d11c);
    UvPass2(uv_0075d100, uv_0075d420);
    UvPass2(uv_0075d124, uv_0075d460);
    UvPass2(uv_0075d148, uv_0075d4a0);
    UvPass2(uv_0075d16c, uv_0075d4e0);
    UvPass2(uv_0075d190, uv_0075d520);
    UvPass2(uv_0075d1b4, uv_0075d560);
    UvPass2(uv_0075d1d8, uv_0075d5a0);
    UvPass2(uv_0075d1fc, uv_0075d5e0);
    // block 3
    UvSparse();
    UvPass1(UVD_Coeff, uv_0075d100);
    UvPass1(uv_0075d020, uv_0075d104);
    UvPass1(uv_0075d040, uv_0075d108);
    UvPass1(uv_0075d060, uv_0075d10c);
    UvPass1(uv_0075d080, uv_0075d110);
    UvPass1(uv_0075d0a0, uv_0075d114);
    UvPass1(uv_0075d0c0, uv_0075d118);
    UvPass1(uv_0075d0e0, uv_0075d11c);
    UvPass2(uv_0075d100, uv_0075d440);
    UvPass2(uv_0075d124, uv_0075d480);
    UvPass2(uv_0075d148, uv_0075d4c0);
    UvPass2(uv_0075d16c, uv_0075d500);
    UvPass2(uv_0075d190, uv_0075d540);
    UvPass2(uv_0075d1b4, uv_0075d580);
    UvPass2(uv_0075d1d8, uv_0075d5c0);
    UvPass2(uv_0075d1fc, uv_0075d600);
    // block 4
    UvSparse();
    UvPass1(UVD_Coeff, uv_0075d100);
    UvPass1(uv_0075d020, uv_0075d104);
    UvPass1(uv_0075d040, uv_0075d108);
    UvPass1(uv_0075d060, uv_0075d10c);
    UvPass1(uv_0075d080, uv_0075d110);
    UvPass1(uv_0075d0a0, uv_0075d114);
    UvPass1(uv_0075d0c0, uv_0075d118);
    UvPass1(uv_0075d0e0, uv_0075d11c);
    UvPass2(uv_0075d100, GLOBAL_PTR(int32_t, uv_0075d620));
    UvPass2(uv_0075d124, GLOBAL_PTR(int32_t, uv_0075d640));
    UvPass2(uv_0075d148, GLOBAL_PTR(int32_t, uv_0075d660));
    UvPass2(uv_0075d16c, GLOBAL_PTR(int32_t, uv_0075d680));
    UvPass2(uv_0075d190, GLOBAL_PTR(int32_t, uv_0075d6a0));
    UvPass2(uv_0075d1b4, GLOBAL_PTR(int32_t, uv_0075d6c0));
    UvPass2(uv_0075d1d8, GLOBAL_PTR(int32_t, uv_0075d6e0));
    UvPass2(uv_0075d1fc, GLOBAL_PTR(int32_t, uv_0075d700));
    // block 5
    UvSparse();
    UvPass1(UVD_Coeff, uv_0075d100);
    UvPass1(uv_0075d020, uv_0075d104);
    UvPass1(uv_0075d040, uv_0075d108);
    UvPass1(uv_0075d060, uv_0075d10c);
    UvPass1(uv_0075d080, uv_0075d110);
    UvPass1(uv_0075d0a0, uv_0075d114);
    UvPass1(uv_0075d0c0, uv_0075d118);
    UvPass1(uv_0075d0e0, uv_0075d11c);
    UvPass2(uv_0075d100, uv_0075d720);
    UvPass2(uv_0075d124, uv_0075d740);
    UvPass2(uv_0075d148, uv_0075d760);
    UvPass2(uv_0075d16c, uv_0075d780);
    UvPass2(uv_0075d190, uv_0075d7a0);
    UvPass2(uv_0075d1b4, uv_0075d7c0);
    UvPass2(uv_0075d1d8, uv_0075d7e0);
    UvPass2(uv_0075d1fc, uv_0075d800);
    // 16 rows of output
    UvOutS16(uv_0075d220, uv_0075d620, dst);
    dst += pitch;
    UvOutS16(uv_0075d260, uv_0075d620, dst);
    dst += pitch;
    UvOutS16(uv_0075d2a0, uv_0075d640, dst);
    dst += pitch;
    UvOutS16(uv_0075d2e0, uv_0075d640, dst);
    dst += pitch;
    UvOutS16(uv_0075d320, uv_0075d660, dst);
    dst += pitch;
    UvOutS16(uv_0075d360, uv_0075d660, dst);
    dst += pitch;
    UvOutS16(uv_0075d3a0, uv_0075d680, dst);
    dst += pitch;
    UvOutS16(uv_0075d3e0, uv_0075d680, dst);
    dst += pitch;
    UvOutS16(uv_0075d420, uv_0075d6a0, dst);
    dst += pitch;
    UvOutS16(uv_0075d460, uv_0075d6a0, dst);
    dst += pitch;
    UvOutS16(uv_0075d4a0, uv_0075d6c0, dst);
    dst += pitch;
    UvOutS16(uv_0075d4e0, uv_0075d6c0, dst);
    dst += pitch;
    UvOutS16(uv_0075d520, uv_0075d6e0, dst);
    dst += pitch;
    UvOutS16(uv_0075d560, uv_0075d6e0, dst);
    dst += pitch;
    UvOutS16(uv_0075d5a0, uv_0075d700, dst);
    dst += pitch;
    UvOutS16(uv_0075d5e0, uv_0075d700, dst);
}
__declspec(naked) void Thunk_UvDecodeBlocksS16()
{
    __asm {
        push ecx
        push edx
        push dword ptr [esp + 0x14]
        push dword ptr [esp + 0x14]
        push dword ptr [esp + 0x14]
        call UvDecodeBlocksS16
        add esp, 12
        pop edx
        pop ecx
        ret
    }
}
HOOK_FUNC(0x00487f71, Thunk_UvDecodeBlocksS16);

// 0x00488b52  size 0x9b
// cdecl(int *first, int *second): reads one flag bit; when it is set, skips one more bit and stores two 4-bit values
// into *first and *second. Returns 1 when the values were read, else 0. Updates the stream cursor (0x75f120/4).
static int UvReadTwoNibbles(int32_t *first, int32_t *second)
{
    const uint32_t *base = reinterpret_cast<const uint32_t *>(static_cast<uintptr_t>(UVD_BitPtr));
    uint32_t pos = UVD_BitPos;
    uint32_t result = Peek32(base, pos) >> 31;
    pos += 1;
    if (result == 1) {
        pos += 1; // a second flag bit is read and ignored
        *first = static_cast<int32_t>(Peek32(base, pos) >> 28);
        pos += 4;
        *second = static_cast<int32_t>(Peek32(base, pos) >> 28);
        pos += 4;
        result = 1;
    }
    UVD_BitPtr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(base));
    UVD_BitPos = pos;
    return static_cast<int>(result);
}
__declspec(naked) void Thunk_UvReadTwoNibbles()
{
    __asm {
        push ecx
        push edx
        push dword ptr [esp + 0x10]
        push dword ptr [esp + 0x10]
        call UvReadTwoNibbles
        add esp, 8
        pop edx
        pop ecx
        ret
    }
}
HOOK_FUNC(0x00488b52, Thunk_UvReadTwoNibbles);

// 0x00488750  size 0x142
// Decodes one VLC macroblock (0x004892d5) and writes 16 rows of 32-bit pixels. cdecl(dst, pitch, dc): dst = first row,
// pitch in bytes, dc = the three DC predictors. Preserves every register but EAX (the hook keeps ECX / EDX).
static void UvOutputRowS32(uint8_t *dst, int32_t pitch, int32_t *dc)
{
    UvDcBlock(dc);
    UvOutS32(uv_0075d220, uv_0075d620, dst);
    dst += pitch;
    UvOutS32(uv_0075d260, uv_0075d620, dst);
    dst += pitch;
    UvOutS32(uv_0075d2a0, uv_0075d640, dst);
    dst += pitch;
    UvOutS32(uv_0075d2e0, uv_0075d640, dst);
    dst += pitch;
    UvOutS32(uv_0075d320, uv_0075d660, dst);
    dst += pitch;
    UvOutS32(uv_0075d360, uv_0075d660, dst);
    dst += pitch;
    UvOutS32(uv_0075d3a0, uv_0075d680, dst);
    dst += pitch;
    UvOutS32(uv_0075d3e0, uv_0075d680, dst);
    dst += pitch;
    UvOutS32(uv_0075d420, uv_0075d6a0, dst);
    dst += pitch;
    UvOutS32(uv_0075d460, uv_0075d6a0, dst);
    dst += pitch;
    UvOutS32(uv_0075d4a0, uv_0075d6c0, dst);
    dst += pitch;
    UvOutS32(uv_0075d4e0, uv_0075d6c0, dst);
    dst += pitch;
    UvOutS32(uv_0075d520, uv_0075d6e0, dst);
    dst += pitch;
    UvOutS32(uv_0075d560, uv_0075d6e0, dst);
    dst += pitch;
    UvOutS32(uv_0075d5a0, uv_0075d700, dst);
    dst += pitch;
    UvOutS32(uv_0075d5e0, uv_0075d700, dst);
}
__declspec(naked) void Thunk_UvOutputRowS32()
{
    __asm {
        push ecx
        push edx
        push dword ptr [esp + 0x14]
        push dword ptr [esp + 0x14]
        push dword ptr [esp + 0x14]
        call UvOutputRowS32
        add esp, 12
        pop edx
        pop ecx
        ret
    }
}
HOOK_FUNC(0x00488750, Thunk_UvOutputRowS32);

// 0x00489193  size 0x142
// Decodes one VLC macroblock (0x004892d5) and writes 16 rows of 16-bit pixels. cdecl(dst, pitch, dc): dst = first row,
// pitch in bytes, dc = the three DC predictors. Preserves every register but EAX (the hook keeps ECX / EDX).
static void UvOutputRowS16(uint8_t *dst, int32_t pitch, int32_t *dc)
{
    UvDcBlock(dc);
    UvOutS16(uv_0075d220, uv_0075d620, dst);
    dst += pitch;
    UvOutS16(uv_0075d260, uv_0075d620, dst);
    dst += pitch;
    UvOutS16(uv_0075d2a0, uv_0075d640, dst);
    dst += pitch;
    UvOutS16(uv_0075d2e0, uv_0075d640, dst);
    dst += pitch;
    UvOutS16(uv_0075d320, uv_0075d660, dst);
    dst += pitch;
    UvOutS16(uv_0075d360, uv_0075d660, dst);
    dst += pitch;
    UvOutS16(uv_0075d3a0, uv_0075d680, dst);
    dst += pitch;
    UvOutS16(uv_0075d3e0, uv_0075d680, dst);
    dst += pitch;
    UvOutS16(uv_0075d420, uv_0075d6a0, dst);
    dst += pitch;
    UvOutS16(uv_0075d460, uv_0075d6a0, dst);
    dst += pitch;
    UvOutS16(uv_0075d4a0, uv_0075d6c0, dst);
    dst += pitch;
    UvOutS16(uv_0075d4e0, uv_0075d6c0, dst);
    dst += pitch;
    UvOutS16(uv_0075d520, uv_0075d6e0, dst);
    dst += pitch;
    UvOutS16(uv_0075d560, uv_0075d6e0, dst);
    dst += pitch;
    UvOutS16(uv_0075d5a0, uv_0075d700, dst);
    dst += pitch;
    UvOutS16(uv_0075d5e0, uv_0075d700, dst);
}
__declspec(naked) void Thunk_UvOutputRowS16()
{
    __asm {
        push ecx
        push edx
        push dword ptr [esp + 0x14]
        push dword ptr [esp + 0x14]
        push dword ptr [esp + 0x14]
        call UvOutputRowS16
        add esp, 12
        pop edx
        pop ecx
        ret
    }
}
HOOK_FUNC(0x00489193, Thunk_UvOutputRowS16);

// 0x00488892  size 0x160
// Decodes one VLC macroblock (0x004892d5) and writes 16 rows through the pointer LUTs, packed (two 16-bit pixels per dword).
// cdecl(dst, pitch, dc) as 0x00488750. Preserves every register but EAX (the hook keeps ECX / EDX).
static void UvDecodeBlocksPacked(uint8_t *dst, int32_t pitch, int32_t *dc)
{
    UvDcBlock(dc);
    UvOutF(uv_0075d220, uv_0075d620, dst);
    dst += pitch;
    UvOutF(uv_0075d260, uv_0075d620, dst);
    dst += pitch;
    UvOutF(uv_0075d2a0, uv_0075d640, dst);
    dst += pitch;
    UvOutF(uv_0075d2e0, uv_0075d640, dst);
    dst += pitch;
    UvOutF(uv_0075d320, uv_0075d660, dst);
    dst += pitch;
    UvOutF(uv_0075d360, uv_0075d660, dst);
    dst += pitch;
    UvOutF(uv_0075d3a0, uv_0075d680, dst);
    dst += pitch;
    UvOutF(uv_0075d3e0, uv_0075d680, dst);
    dst += pitch;
    UvOutF(uv_0075d420, uv_0075d6a0, dst);
    dst += pitch;
    UvOutF(uv_0075d460, uv_0075d6a0, dst);
    dst += pitch;
    UvOutF(uv_0075d4a0, uv_0075d6c0, dst);
    dst += pitch;
    UvOutF(uv_0075d4e0, uv_0075d6c0, dst);
    dst += pitch;
    UvOutF(uv_0075d520, uv_0075d6e0, dst);
    dst += pitch;
    UvOutF(uv_0075d560, uv_0075d6e0, dst);
    dst += pitch;
    UvOutF(uv_0075d5a0, uv_0075d700, dst);
    dst += pitch;
    UvOutF(uv_0075d5e0, uv_0075d700, dst);
}
__declspec(naked) void Thunk_UvDecodeBlocksPacked()
{
    __asm {
        push ecx
        push edx
        push dword ptr [esp + 0x14]
        push dword ptr [esp + 0x14]
        push dword ptr [esp + 0x14]
        call UvDecodeBlocksPacked
        add esp, 12
        pop edx
        pop ecx
        ret
    }
}
HOOK_FUNC(0x00488892, Thunk_UvDecodeBlocksPacked);

// 0x004889f2  size 0x160
// Decodes one VLC macroblock (0x004892d5) and writes 16 rows through the pointer LUTs, 32-bit.
// cdecl(dst, pitch, dc) as 0x00488750. Preserves every register but EAX (the hook keeps ECX / EDX).
static void UvDecodeBlocksLut32(uint8_t *dst, int32_t pitch, int32_t *dc)
{
    UvDcBlock(dc);
    UvOutE(uv_0075d220, uv_0075d620, dst);
    dst += pitch;
    UvOutE(uv_0075d260, uv_0075d620, dst);
    dst += pitch;
    UvOutE(uv_0075d2a0, uv_0075d640, dst);
    dst += pitch;
    UvOutE(uv_0075d2e0, uv_0075d640, dst);
    dst += pitch;
    UvOutE(uv_0075d320, uv_0075d660, dst);
    dst += pitch;
    UvOutE(uv_0075d360, uv_0075d660, dst);
    dst += pitch;
    UvOutE(uv_0075d3a0, uv_0075d680, dst);
    dst += pitch;
    UvOutE(uv_0075d3e0, uv_0075d680, dst);
    dst += pitch;
    UvOutE(uv_0075d420, uv_0075d6a0, dst);
    dst += pitch;
    UvOutE(uv_0075d460, uv_0075d6a0, dst);
    dst += pitch;
    UvOutE(uv_0075d4a0, uv_0075d6c0, dst);
    dst += pitch;
    UvOutE(uv_0075d4e0, uv_0075d6c0, dst);
    dst += pitch;
    UvOutE(uv_0075d520, uv_0075d6e0, dst);
    dst += pitch;
    UvOutE(uv_0075d560, uv_0075d6e0, dst);
    dst += pitch;
    UvOutE(uv_0075d5a0, uv_0075d700, dst);
    dst += pitch;
    UvOutE(uv_0075d5e0, uv_0075d700, dst);
}
__declspec(naked) void Thunk_UvDecodeBlocksLut32()
{
    __asm {
        push ecx
        push edx
        push dword ptr [esp + 0x14]
        push dword ptr [esp + 0x14]
        push dword ptr [esp + 0x14]
        call UvDecodeBlocksLut32
        add esp, 12
        pop edx
        pop ecx
        ret
    }
}
HOOK_FUNC(0x004889f2, Thunk_UvDecodeBlocksLut32);

// 0x004892d5  size 0x935
// Six blocks of VLC coded data. For each: the DC difference (a 7-bit lookup for luma through the byte table at
// 0x00760ac0, an 8-bit lookup for chroma through 0x00760b40; size bits of magnitude, JPEG style) is added to the
// predictor (dc[0] for the four luma blocks, dc[1] Cb, dc[2] Cr), then the AC coefficients are decoded by
// 0x00487017 and the block is transformed into 0x0075d220... See the layout comment above. The original is entered
// with the caller's own stack argument [esp+0x3c] = dc.
void uv_dct_decode_dc_and_block(int32_t *dc)
{
    const uint32_t *base;
    uint32_t pos;
    // block 0
    base = reinterpret_cast<const uint32_t *>(static_cast<uintptr_t>(UVD_BitPtr));
    pos = UVD_BitPos;
    int32_t dc0 = ReadDc(base, pos, 25, GADDR(dat_00760ac0), &dc[0]);
    uv_dct_decode_block_vlc(dc0, base, pos); // 0x00487017
    UvPass1(UVD_Coeff, uv_0075d100);
    UvPass1(uv_0075d020, uv_0075d104);
    UvPass1(uv_0075d040, uv_0075d108);
    UvPass1(uv_0075d060, uv_0075d10c);
    UvPass1(uv_0075d080, uv_0075d110);
    UvPass1(uv_0075d0a0, uv_0075d114);
    UvPass1(uv_0075d0c0, uv_0075d118);
    UvPass1(uv_0075d0e0, uv_0075d11c);
    UvPass2(uv_0075d100, uv_0075d220);
    UvPass2(uv_0075d124, uv_0075d260);
    UvPass2(uv_0075d148, uv_0075d2a0);
    UvPass2(uv_0075d16c, uv_0075d2e0);
    UvPass2(uv_0075d190, uv_0075d320);
    UvPass2(uv_0075d1b4, uv_0075d360);
    UvPass2(uv_0075d1d8, uv_0075d3a0);
    UvPass2(uv_0075d1fc, uv_0075d3e0);
    // block 1
    base = reinterpret_cast<const uint32_t *>(static_cast<uintptr_t>(UVD_BitPtr));
    pos = UVD_BitPos;
    int32_t dc1 = ReadDc(base, pos, 25, GADDR(dat_00760ac0), &dc[0]);
    uv_dct_decode_block_vlc(dc1, base, pos); // 0x00487017
    UvPass1(UVD_Coeff, uv_0075d100);
    UvPass1(uv_0075d020, uv_0075d104);
    UvPass1(uv_0075d040, uv_0075d108);
    UvPass1(uv_0075d060, uv_0075d10c);
    UvPass1(uv_0075d080, uv_0075d110);
    UvPass1(uv_0075d0a0, uv_0075d114);
    UvPass1(uv_0075d0c0, uv_0075d118);
    UvPass1(uv_0075d0e0, uv_0075d11c);
    UvPass2(uv_0075d100, uv_0075d240);
    UvPass2(uv_0075d124, uv_0075d280);
    UvPass2(uv_0075d148, uv_0075d2c0);
    UvPass2(uv_0075d16c, uv_0075d300);
    UvPass2(uv_0075d190, uv_0075d340);
    UvPass2(uv_0075d1b4, uv_0075d380);
    UvPass2(uv_0075d1d8, uv_0075d3c0);
    UvPass2(uv_0075d1fc, uv_0075d400);
    // block 2
    base = reinterpret_cast<const uint32_t *>(static_cast<uintptr_t>(UVD_BitPtr));
    pos = UVD_BitPos;
    int32_t dc2 = ReadDc(base, pos, 25, GADDR(dat_00760ac0), &dc[0]);
    uv_dct_decode_block_vlc(dc2, base, pos); // 0x00487017
    UvPass1(UVD_Coeff, uv_0075d100);
    UvPass1(uv_0075d020, uv_0075d104);
    UvPass1(uv_0075d040, uv_0075d108);
    UvPass1(uv_0075d060, uv_0075d10c);
    UvPass1(uv_0075d080, uv_0075d110);
    UvPass1(uv_0075d0a0, uv_0075d114);
    UvPass1(uv_0075d0c0, uv_0075d118);
    UvPass1(uv_0075d0e0, uv_0075d11c);
    UvPass2(uv_0075d100, uv_0075d420);
    UvPass2(uv_0075d124, uv_0075d460);
    UvPass2(uv_0075d148, uv_0075d4a0);
    UvPass2(uv_0075d16c, uv_0075d4e0);
    UvPass2(uv_0075d190, uv_0075d520);
    UvPass2(uv_0075d1b4, uv_0075d560);
    UvPass2(uv_0075d1d8, uv_0075d5a0);
    UvPass2(uv_0075d1fc, uv_0075d5e0);
    // block 3
    base = reinterpret_cast<const uint32_t *>(static_cast<uintptr_t>(UVD_BitPtr));
    pos = UVD_BitPos;
    int32_t dc3 = ReadDc(base, pos, 25, GADDR(dat_00760ac0), &dc[0]);
    uv_dct_decode_block_vlc(dc3, base, pos); // 0x00487017
    UvPass1(UVD_Coeff, uv_0075d100);
    UvPass1(uv_0075d020, uv_0075d104);
    UvPass1(uv_0075d040, uv_0075d108);
    UvPass1(uv_0075d060, uv_0075d10c);
    UvPass1(uv_0075d080, uv_0075d110);
    UvPass1(uv_0075d0a0, uv_0075d114);
    UvPass1(uv_0075d0c0, uv_0075d118);
    UvPass1(uv_0075d0e0, uv_0075d11c);
    UvPass2(uv_0075d100, uv_0075d440);
    UvPass2(uv_0075d124, uv_0075d480);
    UvPass2(uv_0075d148, uv_0075d4c0);
    UvPass2(uv_0075d16c, uv_0075d500);
    UvPass2(uv_0075d190, uv_0075d540);
    UvPass2(uv_0075d1b4, uv_0075d580);
    UvPass2(uv_0075d1d8, uv_0075d5c0);
    UvPass2(uv_0075d1fc, uv_0075d600);
    // block 4
    base = reinterpret_cast<const uint32_t *>(static_cast<uintptr_t>(UVD_BitPtr));
    pos = UVD_BitPos;
    int32_t dc4 = ReadDc(base, pos, 24, GADDR(dat_00760b40), &dc[1]);
    uv_dct_decode_block_vlc(dc4, base, pos); // 0x00487017
    UvPass1(UVD_Coeff, uv_0075d100);
    UvPass1(uv_0075d020, uv_0075d104);
    UvPass1(uv_0075d040, uv_0075d108);
    UvPass1(uv_0075d060, uv_0075d10c);
    UvPass1(uv_0075d080, uv_0075d110);
    UvPass1(uv_0075d0a0, uv_0075d114);
    UvPass1(uv_0075d0c0, uv_0075d118);
    UvPass1(uv_0075d0e0, uv_0075d11c);
    UvPass2(uv_0075d100, GLOBAL_PTR(int32_t, uv_0075d620));
    UvPass2(uv_0075d124, GLOBAL_PTR(int32_t, uv_0075d640));
    UvPass2(uv_0075d148, GLOBAL_PTR(int32_t, uv_0075d660));
    UvPass2(uv_0075d16c, GLOBAL_PTR(int32_t, uv_0075d680));
    UvPass2(uv_0075d190, GLOBAL_PTR(int32_t, uv_0075d6a0));
    UvPass2(uv_0075d1b4, GLOBAL_PTR(int32_t, uv_0075d6c0));
    UvPass2(uv_0075d1d8, GLOBAL_PTR(int32_t, uv_0075d6e0));
    UvPass2(uv_0075d1fc, GLOBAL_PTR(int32_t, uv_0075d700));
    // block 5
    base = reinterpret_cast<const uint32_t *>(static_cast<uintptr_t>(UVD_BitPtr));
    pos = UVD_BitPos;
    int32_t dc5 = ReadDc(base, pos, 24, GADDR(dat_00760b40), &dc[2]);
    uv_dct_decode_block_vlc(dc5, base, pos); // 0x00487017
    UvPass1(UVD_Coeff, uv_0075d100);
    UvPass1(uv_0075d020, uv_0075d104);
    UvPass1(uv_0075d040, uv_0075d108);
    UvPass1(uv_0075d060, uv_0075d10c);
    UvPass1(uv_0075d080, uv_0075d110);
    UvPass1(uv_0075d0a0, uv_0075d114);
    UvPass1(uv_0075d0c0, uv_0075d118);
    UvPass1(uv_0075d0e0, uv_0075d11c);
    UvPass2(uv_0075d100, uv_0075d720);
    UvPass2(uv_0075d124, uv_0075d740);
    UvPass2(uv_0075d148, uv_0075d760);
    UvPass2(uv_0075d16c, uv_0075d780);
    UvPass2(uv_0075d190, uv_0075d7a0);
    UvPass2(uv_0075d1b4, uv_0075d7c0);
    UvPass2(uv_0075d1d8, uv_0075d7e0);
    UvPass2(uv_0075d1fc, uv_0075d800);
}
PORT_NOHOOK_FUNC(0x004892d5, uv_dct_decode_dc_and_block);

