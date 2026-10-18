#include "text_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch06_helpers.h"
#include "scroll_thumb_gump.h"

// 0x0047a0b0 / 0x0047a210 on the gump, 0x00413b20 on the child control.
#define TextGump_Fix1(self) (self->*PORT_METHOD0(CTextGump, void, &CTextGump::ScrollToEnd))()
#define TextGump_Fix2(self) (self->*PORT_METHOD2(CTextGump, void, &CTextGump::RedrawText, int, int))(1, 1)
#define Child_Refresh(self) (reinterpret_cast<CScrollThumbGump *>(self)->PlaceFromTextList())

// 0x00479eb0  size 0x1fa  CTextGump::Resize(int width, int height)  [name low-confidence]
// Children whose vtable slot 0x98 answers non-zero are re-anchored by their own +0x9c / +0xa0 / +0xa8 fields;
// the child at +0xc4 is shifted by the size change; any other child is shifted when it lies in the far half.
// Then the frame bitmap and the background copy are reallocated.
void __thiscall CTextGump::Resize(int width_, int height_)
{
    CGump *child;
    int oldX;

    rowStride = (width_ + 7) & ~7;   // AND AL,0xf8
    child = lastChild;
    while (child != nullptr) {
        char *c = reinterpret_cast<char *>(child);
        if (PORT_VSLOT0(child, 0x98, int)() != 0) {
            oldX = child->x;
            *reinterpret_cast<int32_t *>(c + 0x9c) = oldX - width + rowStride;
            child->x = *reinterpret_cast<int32_t *>(c + 0x9c);
            *reinterpret_cast<int32_t *>(c + 0xa0) = oldX - width + rowStride;
            *reinterpret_cast<int32_t *>(c + 0xa8) = *reinterpret_cast<int32_t *>(c + 0xa8) + (height_ - height);
        } else if (child == childC4) {
            child->x = child->x + (rowStride - width);
            child->y = child->y + (height_ - height);
        } else {
            if (child->x > width / 2) {
                child->x = child->x - width + rowStride;
            }
            if (child->y > height / 2) {
                child->y = height_ + child->y - height;
            }
        }
        child = child->prev;
    }

    if (frameData[1] != 0) {
        b6_free(g_frameDataAllocator, b6_frame_bitmap(this, 0), "text frame0 pBMP");
        b6_set_frame_bitmap(this, 0, nullptr);
    }
    if (unk_58 != 0) {
        b6_free(g_frameDataAllocator, b6_ptr(unk_58), "gumpbak textgump");
        unk_58 = 0;
    }

    height = height_;
    width = rowStride;
    b6_set_frame_bitmap(this, 0, b6_alloc(g_frameDataAllocator, height_ * rowStride * 2 + 4, "text frame0"));

    unsigned int bytes = (height * width) * 2;
    memset(b6_frame_bitmap(this, 0)->px, 0, bytes);   // rep stosd + rep stosb

    b6_frame_bitmap(this, 0)->w = static_cast<uint16_t>(width);
    b6_frame_bitmap(this, 0)->h = static_cast<uint16_t>(height);

    unk_58 = static_cast<int32_t>(reinterpret_cast<uintptr_t>(b6_alloc(g_frameDataAllocator, height * width * 2, "gump background")));

    unk_b8 = width - 0x31;
    if ((unk_b8 & 1) == 0) {   // even: make it odd
        unk_b8 = unk_b8 - 1;
    }
    unk_bc = height - 0x4e;
    if (*reinterpret_cast<int32_t *>(reinterpret_cast<char *>(childC4) + 0x10) == 0) {
        TextGump_Fix1(this);
    }
    Child_Refresh(childC0);
    TextGump_Fix2(this);
}
HOOK_VMETHOD(0x00479eb0, CTextGump, Resize);
