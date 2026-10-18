// Unicode (unifont) text helpers and the Unicode speech packet (0xAE).
//
// Glyph table: DAT_007581b0 is the loaded unifont.mul offset table
// (0x10000 dwords; glyph = {xoff, yoff, width, height, bitmap}); DAT_007581a8
// is the "unifont loaded" flag; FUN_0047c920 lazily loads it.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"

// The object whose method 0x0047c690 sends a typed Unicode line as an 8-bit
// speech packet. Only +8 is touched.
// ? the real class of this object is not identified; the name is a placeholder
class CSpeechInput {
public:
    // 0x0047c690: narrow `text` (chars >= 0x80 dropped), build a speech packet, send it, clear this+8.
    void __thiscall SendAsSpeechPacket(int speechMode, uint16_t *text);

    uint8_t pad_000[8];
    int unk_08;   // +0x08  cleared after sending
};

static_assert(offsetof(CSpeechInput, unk_08) == 0x08, "CSpeechInput::unk_08");
