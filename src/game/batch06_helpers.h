// Small helpers shared by the batch-06 ports (radar, shop, picker, skill and
// text gumps, ...): the original frame-data allocator, gump-art and text
// entry points that are still reached through the original image.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"
#include "gump.h"
#include "map_world_ops_ports.h"
#include "gump_art_ops_ports.h"
#include "batch02_helpers.h"
#include "font_manager.h"
#include "bitmap16_draw_ports.h"
#include "palette_manager_ports.h"
#include "gump_art_ports.h"
#include "art_row_blits_ports.h"
#include "globals.h"

// Frame bitmap: u16 width, u16 height, then width*height 16-bit pixels.
struct B6Bitmap {
    uint16_t w;
    uint16_t h;
    uint16_t px[1];
};

// Bitmap pointer stored in frame slot `slot` of a gump (frameData[1 + slot * 2]).
inline B6Bitmap *b6_frame_bitmap(const CGump *g, int slot = 0)
{
    return reinterpret_cast<B6Bitmap *>(static_cast<uintptr_t>(static_cast<uint32_t>(g->frameData[1 + slot * 2])));
}
inline void b6_set_frame_bitmap(CGump *g, int slot, void *bmp)
{
    g->frameData[1 + slot * 2] = static_cast<int32_t>(reinterpret_cast<uintptr_t>(bmp));
}

// Frame-data allocator object at 0x0064cdac: alloc(size, tag), free(ptr, tag).
// Call as b6_alloc(g_frameDataAllocator, size, "tag").
#define b6_alloc(o, a1, a2) PORT_THISCALL2(&Map_Alloc, void *, (o), int, const char *)(a1, a2)
#define b6_free(o, a1, a2) PORT_THISCALL2(&Map_Free, void, (o), void *, const char *)(a1, a2)

// Row blitter passed to the gump art drawers: __fastcall(dst, src, count), callee pops.
typedef void(__fastcall *B6RowBlit)(uint16_t *dst, uint16_t *src, int count);
#define B6_ROWBLIT_COPY reinterpret_cast<B6RowBlit>(static_cast<uintptr_t>(PORT_ADDR(&RowBlitCopyNonZero)))

// 0x00433e70: width/height of a gump art. ECX = art id, EDX = &w, stack = &h.
#define b6_art_size(a1, a2, a3) (GumpArtSizeImpl(a1, a2, a3))
// 0x00433ed0: draw gump art into a bitmap. ECX = row blit, EDX = art id, stack = (x, y, pixels, w, h).
#define b6_draw_art PORT_FUNC7(&DrawGumpArt, void, __fastcall, B6RowBlit, int, int, int, void *, int, int)
// 0x00434060: composite gump art. ECX = row blit, EDX = art id, stack = (clip, x, y, pixels, w, h).
#define b6_draw_art_clip PORT_FUNC8(&DrawGumpArtClipped, void, __fastcall, B6RowBlit, int, int *, int, int, void *, int, int)

// Font manager object at 0x0064cde8: the `this` of the text calls.
#define b6_font GLOBAL_PTR(void, g_fontMgr_storage)
// 0x004327b0: measure (font index, text, &w, &h, &yOffset)
#define b6_text_measure(self, a1, a2, a3, a4, a5) (reinterpret_cast<CFontManager *>(self)->MeasureText(a1, (char *)(a2), a3, a4, a5))
// 0x00432130 / 0x00432550: draw text (x, y, font index, color, text, pixels, stride, clip)
#define b6_text_draw(self, a1, a2, a3, a4, a5, a6, a7, a8) (reinterpret_cast<CFontManager *>(self)->DrawTextPlain(a1, a2, a3, a4, (char *)(a5), (uint16_t *)(a6), a7, (const int *)(a8)))
#define b6_text_draw2(self, a1, a2, a3, a4, a5, a6, a7, a8) (reinterpret_cast<CFontManager *>(self)->DrawTextClipped(a1, a2, a3, a4, (char *)(a5), (uint16_t *)(a6), a7, (const int *)(a8)))
// 0x00432c70: fill rect (x1, y1, x2, y2, color)
#define b6_fill_rect(o, a1, a2, a3, a4, a5) PORT_THISCALL5(&Bitmap16_FillRect, void, (o), int, int, int, int, int)(a1, a2, a3, a4, a5)

// 0x00436870 on the pixel-format object 0x0068dd98: (r, g, b) -> 16-bit pixel. RET 0xc.
#define b6_pixel_format GLOBAL_PTR(void, g_clientState_storage)
#define b6_make_color(o, a1, a2, a3) PORT_THISCALL3(&PackRGB16, uint32_t, (o), int, int, int)(a1, a2, a3)

// Operator new / delete of the original image.

// Call a virtual scalar-deleting destructor (slot 0, arg 1).
inline void b6_delete_gump(void *g)
{
    PORT_VSLOT1(g, 0x00, void, int)(1);
}

// Read a 32-bit slot as a pointer.
inline void *b6_ptr(int32_t v)
{
    return reinterpret_cast<void *>(static_cast<uintptr_t>(static_cast<uint32_t>(v)));
}
