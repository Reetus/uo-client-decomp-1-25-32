// Small art-record helpers at 0x00476120 .. 0x00476160 (the first-level art / sprite record accessors).
#include <cstdint>

#include "../runtime/ported.h"
#include "file_manager.h"

// 0x00476120  size 0x3
// Returns ECX (a thiscall that does nothing but hand back its object). Followed by padding, so it can be hooked.
void *__fastcall ArtRecord_ReturnThis(void *self)
{
    return self;
}
HOOK_FUNC(0x00476120, ArtRecord_ReturnThis);

// 0x00476130  size 0x24
// Fetch record `index` of the data file pair 3 / 4 (art) from the file manager at 0x0068d980; RET 4 (ECX not read).
// The two out pointers go to the callee as the address of a local and of the argument slot itself.
uint8_t *__stdcall ArtRecord_Get(int index)
{
    int outLen;
    return g_fileManager.GetRecord(3, 4, index, &outLen, &index);
}
HOOK_FUNC(0x00476130, ArtRecord_Get);

// 0x00476160  size 0x76
// Pixel of an RLE art record at (x, y), or 0 when the point is outside every run; RET 0xc (ECX not read).
// Record: +6 height (ushort), +8 ushort rowOffset[height], then rows: {ushort startX, ushort len, len pixels}
// repeated, ending with a run of length 0 (the next header's startX is relative to the end of the previous run).
// Callers read only AX of the result.
uint16_t __stdcall ArtRecord_PixelAt(const uint8_t *rec, int x, int y)
{
    if (rec == nullptr)
        return 0;
    uint32_t rowOff = *reinterpret_cast<const uint16_t *>(rec + y * 2 + 8);
    uint32_t height = *reinterpret_cast<const uint16_t *>(rec + 6);
    const uint16_t *p = reinterpret_cast<const uint16_t *>(rec + (rowOff + height) * 2 + 8);
    uint32_t startX = *p++;
    uint32_t len = *p++;
    while (len != 0) {
        x -= static_cast<int>(startX);
        if (x < 0)
            return 0;
        if (x < static_cast<int>(len))
            return p[x];
        p += len;
        x -= static_cast<int>(len);
        startX = *p++;
        len = *p++;
    }
    return 0;
}
HOOK_FUNC(0x00476160, ArtRecord_PixelAt);
