// uV intro video player: the DCT codec (0x00486e77 - 0x00489c0a, hand-written assembly).
// Bit reader state, coefficient/IDCT scratch buffers and the colour lookup tables all live in the UVA_DATA section at
// their original addresses. (Porting pass 3, batch v3.)
#pragma once

#include <cstdint>

#include "../runtime/ported.h"
#include "globals.h"

// Bit reader globals shared by every decoder here.

// Scratch buffers.

// One 8x8 block: eight column passes (pass 1, integer result) then eight row passes (pass 2, result >> 17).
// ESI = input row (8 dwords), the stack argument = output.
void uv_dct_idct_pass1(const int32_t *in, int32_t *out);  // 0x00489c42; output element k lands at out[9 * k]
void uv_dct_idct_pass2(const int32_t *in, int32_t *out);  // 0x00489e6f; output element k lands at out[k]

// Coefficient decoders. Both fill UVD_Coeff.
void uv_dct_decode_block_sparse();                                   // 0x00486e77; byte-oriented stream (UVD_Bit*)
void uv_dct_decode_block_vlc(int32_t dc, const uint32_t *base, uint32_t pos);  // 0x00487017; EAX = dc, EBP = base, EDI = pos

// Colour conversion of one 16-pixel row (two 8-pixel luma runs, 8 chroma values from `chroma`, 8 from `chroma + 0x40`).
void uv_dct_output_block_s32(const int32_t *luma, const uint32_t *chroma, uint32_t *dst);  // 0x0048732c
void uv_dct_output_block_s16(const int32_t *luma, const uint32_t *chroma, uint16_t *dst);  // 0x00487557
void uv_dct_output_block_e(const int32_t *luma, const uint32_t *lut, uint32_t *dst);       // 0x00488bed
void uv_dct_output_block_f(const int32_t *luma, const uint32_t *lut, uint32_t *dst);       // 0x00488ec0

// DC prediction + VLC decode + IDCT of the six blocks of a macroblock into 0x0075d220 (0x004892d5).
// `dc` is the three-element predictor array (luma, Cb, Cr).
void uv_dct_decode_dc_and_block(int32_t *dc);
