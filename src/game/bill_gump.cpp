#include "bill_gump.h"

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "batch06_helpers.h"
#include "shop_gump.h"

// 0x0046e350 on the bill gump (after the redraw).
#define Bill_AfterRedraw(self) (self->*PORT_METHOD0(CBillGump, void, &CBillGump::SyncLineButtons))()

// 0x0046e5e0  size 0x389  CBillGump::RedrawFrame
// Draws the background art, then one block per bill line (quantity, name, price) and the totals footer.
void __thiscall CBillGump::RedrawFrame()
{
    char text[80];
    int clip[4];   // {left, top, right, bottom}
    int dummyA;
    int dummyB;
    int measA;
    int measB;
    uint8_t *objNode = nullptr;
    int textY;
    BillEntry *e;
    int yy;

    b6_art_size(0x32, &dummyA, &dummyB);   // art size, result unused

    B6Bitmap *bmp = b6_frame_bitmap(this, 0);
    b6_draw_art(B6_ROWBLIT_COPY, unk_50, 0, 0, bmp->px, bmp->w, bmp->h);   // background art into the frame bitmap

    clip[2] = listRight - 2;
    clip[0] = listLeft + 2;
    yy = listTop + 2;
    e = entries;
    clip[3] = listBottom - 2;
    clip[1] = yy;

    if (e != nullptr) {
        for (;;) {
            if (yy > clip[3])
                break;

            if (sellMode == 0) {
                // vendor -> equipment slot e->slot -> container -> first item (+0xa8), next at +0x40, key at +0x78
                char *vendor = reinterpret_cast<char *>(shop->owner);
                char *container = *reinterpret_cast<char **>(vendor + 0xc4 + e->slot * 4);
                objNode = *reinterpret_cast<uint8_t **>(container + 0xa8);
                if (objNode != nullptr) {
                    while (e->key != *reinterpret_cast<int32_t *>(objNode + 0x78)) {
                        objNode = *reinterpret_cast<uint8_t **>(objNode + 0x40);
                        if (objNode == nullptr)
                            break;
                    }
                }
            }

            b6_text_measure(b6_font, 9, "Aq", &measA, &measB, &textY);
            sprintf(text, "%d", static_cast<unsigned int>(e->quantity));
            b6_text_draw(b6_font, clip[0], textY + yy, 9, 0x21f, text, b6_frame_bitmap(this, 0)->px, width, clip);

            if (sellMode != 0) {
                sprintf(text, "%s", shop->items[e->index].name);
            } else if (objNode != nullptr) {
                sprintf(text, "%s", *reinterpret_cast<char **>(objNode + 0xa0));
            }

            clip[2] = clip[2] - 0x28;
            b6_text_draw2(b6_font, clip[0] + 0x1e, textY + yy, 9, 0x21f, text, b6_frame_bitmap(this, 0)->px, width, clip);
            clip[2] = clip[2] + 0x28;

            if (sellMode != 0) {
                sprintf(text, "at %d g.p.", static_cast<unsigned int>(shop->items[e->index].price));
            } else if (objNode != nullptr) {
                sprintf(text, "at %d g.p.", *reinterpret_cast<int32_t *>(objNode + 0xa4));
            }

            b6_text_draw(b6_font, clip[0] + 0x1e, textY + yy + 0xd, 9, 0x21f, text, b6_frame_bitmap(this, 0)->px, width, clip);

            e = e->next;
            yy = yy + 0x1c;
            if (e == nullptr)
                break;
        }
    }

    // footer
    clip[2] = width;
    clip[1] = 0;
    clip[0] = 0;
    clip[3] = height;
    sprintf(text, "%d", total);
    b6_text_measure(b6_font, 9, text, &measA, &measB, &textY);

    if (sellMode != 0) {
        b6_text_draw(b6_font, 0xbc, textY + 0xac, 9, 0x386, text, b6_frame_bitmap(this, 0)->px, width, clip);
    } else {
        b6_text_draw(b6_font, 0x46, textY + 0xab, 9, 0x386, text, b6_frame_bitmap(this, 0)->px, width, clip);
        sprintf(text, "%d", static_cast<unsigned int>(shop->unk_d0));   // player gold
        b6_text_measure(b6_font, 9, text, &measA, &measB, &textY);
        b6_text_draw(b6_font, 0xbc, textY + 0xab, 9, 0x386, text, b6_frame_bitmap(this, 0)->px, width, clip);
    }
    Bill_AfterRedraw(this);
}
HOOK_METHOD(0x0046e5e0, CBillGump, RedrawFrame);
