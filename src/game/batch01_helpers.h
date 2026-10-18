// Helpers shared by the batch-01 ports (character creation, client config,
// login, input controls, ...). Everything here is a typed wrapper around an
// entry point that still runs from the original image; nothing is ported
// here. All wrappers live in namespace gfx so they cannot collide with the
// macros other batches put in gump.h / their own helper headers.
//
// "Art object" arguments: the gump art drawers take a pointer in ECX that the
// original code passes as the constants 0x004764c0 / 0x004765a0 / 0x004761e0.
// Those are addresses inside the image (row blitter routines in batch-02's
// reading); this tree only forwards them.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"
#include "gump.h"
#include "gump_art_ops_ports.h"
#include "batch02_helpers.h"
#include "font_manager.h"
#include "bitmap16_draw_ports.h"
#include "map_world_ops_ports.h"
#include "gump_art_ports.h"
#include "art_row_blits_ports.h"
#include "art_row_blits_hue_ports.h"
#include "globals.h"

namespace gfx {

inline void *artPlain() { return PTR_AT(void, PORT_ADDR(&RowBlitCopyNonZero)); }
inline void *artHued() { return PTR_AT(void, PORT_ADDR(&RowBlitLookupNonZero)); }
inline void *artAlt() { return PTR_AT(void, PORT_ADDR(&RowBlitCopyAll)); }
inline void *textObj() { return GLOBAL_PTR(void, g_fontMgr_storage); }   // font / text drawer object

// 0x00433e70: size of gump art `artId` (width, height); 0 when missing.
// ECX = art id, EDX = &w, stack = &h, RET 4.
inline void ArtSize(int artId, int *w, int *h)
{
    GumpArtSizeImpl(artId, w, h);
}

// 0x00433ed0: draw gump art. ECX = art object, EDX = art id, stack (x, y, pixels, w, h), RET 0x14.
inline void DrawArt(void *artObject, int artId, int x, int y, void *pixels, int w, int h)
{
    PORT_FUNC7(&DrawGumpArt, void, __fastcall, void *, int, int, int, void *, int, int)(artObject, artId, x, y, pixels, w, h);
}

// 0x00434060: as DrawArt with a leading clip rect {left, top, right, bottom} on the stack, RET 0x18.
inline void DrawArtClipped(void *artObject, int artId, const int *clip, int x, int y, void *pixels, int w, int h)
{
    PORT_FUNC8(&DrawGumpArtClipped, void, __fastcall, void *, int, const int *, int, int, void *, int, int)(artObject, artId, clip, x, y, pixels, w, h);
}

// 0x004342e0: blit a frame bitmap to a surface. ECX = art object, EDX = source pixels,
// stack (clip, w, h, ox, oy, dest, destW, destH), RET 0x20.
inline void BlitFrame(void *artObject, void *pixels, void *clip, int w, int h, int ox, int oy, void *dest, int destW, int destH)
{
    PORT_FUNC10(&GumpBlitBitmapClipped, void, __fastcall, void *, void *, void *, int, int, int, int, void *, int, int)(artObject, pixels, clip, w, h, ox, oy, dest, destW, destH);
}

// call written out at each site (a wrapper function changes VC5's argument evaluation order)
#define GFX_BlitFrame PORT_FUNC10(&GumpBlitBitmapClipped, void, __fastcall, void *, void *, void *, int, int, int, int, void *, int, int)

// 0x004327b0: measure text. thiscall on the text object, (font, str, &w, &x, &h), RET 0x14.
// ? which of the three outputs is which beyond width (first) and line height (last) is not settled.
inline void TextMeasure(int font, const char *str, int *w, int *x, int *h)
{
    reinterpret_cast<CFontManager *>(textObj())->MeasureText(font, (char *)(str), w, x, h);
}

// 0x00432130 / 0x00432550: draw text. thiscall on the text object,
// (x, y, font, hue, str, dest pixels, destW, clip), RET 0x20.
inline void TextDraw(int x, int y, int font, int hue, const char *str, void *dest, int destW, const int *clip)
{
    reinterpret_cast<CFontManager *>(textObj())->DrawTextPlain(x, y, font, hue, (char *)(str), (uint16_t *)(dest), destW, clip);
}
inline void TextDrawAlt(int x, int y, int font, int hue, const char *str, void *dest, int destW, const int *clip)
{
    reinterpret_cast<CFontManager *>(textObj())->DrawTextClipped(x, y, font, hue, (char *)(str), (uint16_t *)(dest), destW, clip);
}

// 0x00432260: wrapped multi-line text draw. thiscall on the text object,
// (x, y, font, hue, str, wrapWidth, dest pixels, destW, clip, a10, a11), RET 0x2c.
inline void TextDrawWrapped(int x, int y, int font, int hue, const char *str, int wrapWidth, void *dest, int destW,
                            const int *clip, int a10, int a11)
{
    reinterpret_cast<CFontManager *>(textObj())->DrawTextWrapped(x, y, font, hue, (char *)(str), wrapWidth, (uint16_t *)(dest), destW, clip, a10, a11);
}

// 0x00433620: pixel width of the first n characters of str. thiscall on the text object, (font, n, str), RET 0xc.
inline int TextWidth(int font, int n, const char *str)
{
    return reinterpret_cast<CFontManager *>(textObj())->GetTextWidthN(font, n, str);
}

// 0x004336c0: locate the caret in wrapped text. thiscall on the text object,
// (font, n, str, wrapWidth, &caretX, &caretYExtra), RET 0x18.
inline void TextCaretLocate(int font, int n, const char *str, int wrapWidth, int *caretX, int *caretY)
{
    reinterpret_cast<CFontManager *>(textObj())->GetCaretPos(font, n, (char *)(str), wrapWidth, caretX, caretY);
}

// 0x00432c70: thiscall on the text object, (x1, y1, x2, y2, color, bitmap), RET 0x18.
// ? pass2 calls it "clear frame0"; the meaning of the 5th argument is not settled.
inline void FillRect(int x1, int y1, int x2, int y2, int color, void *bitmap)
{
    PORT_THISCALL6(&Bitmap16_FillRect, void, textObj(), int, int, int, int, int, void *)(x1, y1, x2, y2, color, bitmap);
}

// 0x004462f0 / 0x00446310: tagged allocator at 0x0064cdac, (size, tag) and (ptr, tag), both RET 8.
inline void *AllocTagged(int size, const char *tag)
{
    return PORT_THISCALL2(&Map_Alloc, void *, g_frameDataAllocator, int, const char *)(size, tag);
}
inline void FreeTagged(void *ptr, const char *tag)
{
    PORT_THISCALL2(&Map_Free, void, g_frameDataAllocator, void *, const char *)(ptr, tag);
}

// 0x0043b4e0: thiscall on a gump, (&x, &y), RET 8. With (0, 0) in the outputs it yields the
// gump's absolute screen origin (DrawFrame0); with a mouse position it is used as screen -> local.
// ? the in/out direction for a non-zero input is not settled by pass2.
inline void GumpOrigin(CGump *g, int *x, int *y)
{
    g->CGump::LocalToScreen(x, y);
}

// 0x0043bcc0 (CGump_AddChild in gump.h): link `child` under `parent` (or the top-level list when 0).
inline void AddChild(CGump *child, CGump *parent)
{
    child->CGump::LinkInto(parent, 1);
}

// Pointer stored in a 32-bit frame data slot.
inline uint16_t *bitmapOf(const CGump *g)
{
    return reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(g->frameData[1])));
}
inline void setBitmap(CGump *g, void *bmp)
{
    g->frameData[1] = static_cast<int32_t>(reinterpret_cast<uintptr_t>(bmp));
}

} // namespace gfx
