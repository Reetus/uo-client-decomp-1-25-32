// Small helpers shared by the batch-02 ports (login gumps, map gump, generic
// gump, ...). Raw byte-offset access for objects whose class is not defined
// in this tree yet, and the original gump-art / frame-data entry points.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"
#include "gump.h"
#include "map_world_ops_ports.h"
#include "gump_art_ops_ports.h"
#include "font_manager.h"
#include "art_row_blits_ports.h"
#include "globals.h"

// Field of type T at byte offset `off` inside an object of an undefined class: an lvalue, RAW_AT(uint16_t, base, 0x38).
// A macro because Visual C++ 5.0 accepts no explicit function-template argument.
#define RAW_AT(T, base, off) (*(T *)((char *)(base) + (off)))

// A frame-data pointer slot holds a 32-bit address.
inline void *i32_ptr(int32_t v)
{
    return reinterpret_cast<void *>(static_cast<uintptr_t>(static_cast<uint32_t>(v)));
}
inline int32_t ptr_i32(const void *p)
{
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(p));
}

#ifndef FrameData_Free
#define FrameData_Free(o, a1, a2) PORT_THISCALL2(&Map_Free, void, (o), void *, const char *)(a1, a2)   // (ptr, tag) on the allocator object at 0x0064cdac
#endif

// Row blitter passed to the gump art drawers: __fastcall(dst, src, count), callee pops.
typedef void(__fastcall *RowBlitFn)(uint16_t *dst, uint16_t *src, int count);
#define ROWBLIT_COPY reinterpret_cast<RowBlitFn>(static_cast<uintptr_t>(PORT_ADDR(&RowBlitCopyNonZero)))
#define ROWBLIT_ALT reinterpret_cast<RowBlitFn>(static_cast<uintptr_t>(PORT_ADDR(&RowBlitBlendNonZero)))

// 0x00433e70: width/height of a gump art. ECX = art id, EDX = &w, stack = &h.
#define GumpArtSize(a1, a2, a3) (GumpArtSizeImpl(a1, a2, a3))

// 0x00433ed0 / 0x00434060 are ported in gump_art.cpp.
void __fastcall DrawGumpArt(RowBlitFn rowBlit, int artId, int x, int y, uint16_t *dest, int stride, int destH);

// Font manager object at 0x0064cde8 (this for the text calls).
#define g_fontManager GLOBAL_PTR(void, g_fontMgr_storage)
// 0x00432130: draw text. (x, y, font, hue, text, dst, stride, clipRect)
#define FontDrawText(self, a1, a2, a3, a4, a5, a6, a7, a8) (reinterpret_cast<CFontManager *>(self)->DrawTextPlain(a1, a2, a3, a4, (char *)(a5), a6, a7, (const int *)(a8)))

// 16-bit pixel format: non-zero when the display is RGB565 (else 555).
