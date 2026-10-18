// CStatusGump: constructor, destructor, left-button-up, bar blit, RedrawFrame.
// 0x00468060, 0x00468480, 0x00468650, 0x00468d40, 0x00468e00.
//
// The SEH frame of the original (try levels around operator new) is omitted.
#include "status_gump.h"

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "gump_art_ops_ports.h"
#include "batch02_helpers.h"
#include "map_world_ops_ports.h"
#include "font_manager.h"
#include "button_gump.h"
#include "mouse_manager.h"
#include "client_misc_ops_ports.h"
#include "book_gump.h"
#include "gump.h"
#include "gump_art_ports.h"
#include "art_row_blits_ports.h"
#include "vtables.h"

// ---------------------------------------------------------------------------
// Globals (single-file use).
// ---------------------------------------------------------------------------
#define g_fontObj GLOBAL_PTR(void, g_fontMgr_storage)
#define g_mouseObj GLOBAL_PTR(void, g_mouse_storage)
#define g_rowPlain PTR_AT(void, PORT_ADDR(&RowBlitCopyNonZero))
#define g_statusVtable reinterpret_cast<uintptr_t>(vtbl_CStatusGump)

// Tag strings the original passes to the tagged allocator.

// ---------------------------------------------------------------------------
// Original code not ported yet.
// ---------------------------------------------------------------------------
static void ArtSize(int artId, int *w, int *h) { GumpArtSizeImpl(artId, w, h); }
static inline void DrawArt(void *renderer, int artId, int x, int y, const void *pixels, int w, int h)
{
    PORT_FUNC7(&DrawGumpArt, void, __fastcall, void *, int, int, int, const void *, int, int)(renderer, artId, x, y, pixels, w, h);
}
static inline void DrawArtClipped(void *renderer, int artId, const int *clip, int x, int y, const void *pixels, int w, int h)
{
    PORT_FUNC8(&DrawGumpArtClipped, void, __fastcall, void *, int, const int *, int, int, const void *, int, int)(renderer, artId, clip, x, y, pixels, w, h);
}
static void *AllocTagged(int size, const char *tag) { return PORT_THISCALL2(&Map_Alloc, void *, g_frameDataAllocator, int, const char *)(size, tag); }
static void FreeTagged(void *p, const char *tag) { PORT_THISCALL2(&Map_Free, void, g_frameDataAllocator, void *, const char *)(p, tag); }
static void TextMeasure(int font, const char *str, int *a, int *b, int *c)
{
    reinterpret_cast<CFontManager *>(g_fontObj)->MeasureText(font, (char *)(str), a, b, c);
}
static void TextDraw(int x, int y, int font, int hue, const char *str, const void *dest, int destW, const int *clip)
{
    reinterpret_cast<CFontManager *>(g_fontObj)->DrawTextPlain(x, y, font, hue, (char *)(str), (uint16_t *)(dest), destW, clip);
}
static void AddChild(void *child, CGump *parent) { CGump_AddChild(static_cast<CGump *>(child), parent, 1); }
static void MousePosition(int *mx, int *my) { reinterpret_cast<CMouseManager *>(g_mouseObj)->GetMousePos(mx, my); }
static inline int IsGameMaster() { return Misc_Return0(); }
static inline void Gump_Notify(CGump *g) { reinterpret_cast<CBookGump *>(g)->Uniconify(); }
static inline void Gump_DetachGroup(CGump *g) { reinterpret_cast<CStatusGump *>(g)->DetachGroup(); }
static inline void Gump_BringToFront(CGump *g) { g->CGump::BringToFront(0); }

static inline int32_t ptr_to_i32(const void *p) { return static_cast<int32_t>(reinterpret_cast<uintptr_t>(p)); }
static inline uint16_t *bitmap_at(const CGump *g, int slot)
{
    return reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(g->frameData[slot])));
}
static inline char *pixels_of(uint16_t *bmp) { return reinterpret_cast<char *>(bmp) + 4; }

// Allocates a frame bitmap (16-bit width, 16-bit height, then w * h pixels), cleared.
static int32_t new_bitmap(int w, int h, const char *tag)
{
    int32_t slot = ptr_to_i32(AllocTagged(h * w * 2 + 4, tag));
    uint16_t *bmp = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(slot)));
    memset(pixels_of(bmp), 0, h * w * 2);
    bmp[0] = static_cast<uint16_t>(w);
    bmp[1] = static_cast<uint16_t>(h);
    return slot;
}

// ---------------------------------------------------------------------------
// 0x00468060  size 0x412
// Constructor. RET 8: (mobile, pinned).
// ---------------------------------------------------------------------------
CStatusGump::CStatusGump(void *mobile, int pinned)
{
    int mx;
    int my;

    if (mobile != static_cast<void *>(g_player)) {
        int count = g_statusRingCount;
        if (count == 8) {
            uint32_t front = g_statusRing[0];
            if (front != 0) {
                decomp::vdelete(reinterpret_cast<void *>(static_cast<uintptr_t>(front)));    // delete the oldest
                count = g_statusRingCount;
            }
            uint32_t *p = g_statusRing;
            do {
                p[0] = p[1];                       // the last copy reads the count word into slot 7
                p++;
            } while (reinterpret_cast<uintptr_t>(p) < GEND(g_statusRing));
            count--;
        }
        g_statusRing[count] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this));
        count++;
        g_statusRingCount = count;
    }
    strcpy(nameText, g_emptyString);
    sex = 0;
    stat_bc = 0;
    stat_be = 0;
    stat_c0 = 0;
    hits = 0;
    hitsMax = 0;
    mana = 0;
    manaMax = 0;
    stam = 0;
    stamMax = 0;
    stat_d0 = 0;
    name = reinterpret_cast<char *>(GADDR(g_statusName));
    dirty = 0;
    unk_e4 = 0;
    dragging = 0;
    unk_dc = 0;
    moved = 0;
    clickState = 0;
    unk_e8 = 0;
    snapTo = nullptr;
    groupNext = nullptr;
    groupHead = nullptr;
    link_108 = nullptr;
    link_104 = nullptr;
    link_100 = nullptr;
    link_fc = nullptr;
    maxY = nullptr;
    minY = nullptr;
    maxX = nullptr;
    minX = nullptr;
    editControl = nullptr;
    owner = static_cast<CObj *>(mobile);
    if (mobile != nullptr) {
        if (RAWF(int32_t, mobile, 0x88) == 0)
            RAWF(void *, mobile, 0x88) = this;
        else
            RAWF(void *, mobile, 0x8c) = this;
    }
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    unk_50 = 0x802;
    ArtSize(0x802, &width, &height);
    int w = (width + 7) & ~7;
    rowStride = w;
    width = w;
    if (owner == g_player)
        ArtSize(0x803, &unk_24, &unk_28);
    else
        ArtSize(0x804, &unk_24, &unk_28);
    MousePosition(&mx, &my);
    int ex = unk_24;
    int cx = mx - ex / 2;
    int h2 = unk_28;
    x = cx;
    int cy = my - h2 / 2;
    y = cy;
    if (cx < 0)
        x = 0;
    if (cy < 0)
        y = 0;
    if (pinned != 0) {
        if (x + ex >= 0x27f)
            x = 0x27f - ex;
        if (y + h2 >= 0x1df)
            y = 0x1df - h2;
    } else {
        if (x + width >= 0x27f)
            x = 0x27f - width;
        if (y + height >= 0x1df)
            y = 0x1df - height;
    }
    if (pinned != 0) {
        unk_60 = 1;
        unk_7c = x;
        unk_80 = y;
    } else {
        unk_60 = 0;
    }
    frameIndex = 0;
    frameCount = 2;
    int32_t *fd = static_cast<int32_t *>(AllocTagged(0x10, g_004a654c));
    frameData = fd;
    memset(fd, 0, frameCount * 8);
    fd[1] = new_bitmap(width, height, s_frame0Tag);
    fd[3] = new_bitmap(unk_24, unk_28, g_004a84c8);
    RedrawFrame(1, 1, 1, 1);
    void *btn = new CButtonGump(0xf0, 0x70, 0x107, 0x88, this, 0, 0, PORT_ADDR(&CBookGump::Iconify), 0);
    AddChild(btn, this);
}
HOOK_CTOR(0x00468060, CStatusGump, (void *, int));

// ---------------------------------------------------------------------------
// 0x00468480  size 0xf8
// Destructor body.
// ---------------------------------------------------------------------------
CStatusGump::~CStatusGump()
{
    int count = g_statusRingCount;
    int idx = 0;
    if (count > 0) {
        uint32_t *p = g_statusRing;
        do {
            if (*p == static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this)))
                break;
            idx++;
            p++;
        } while (idx < count);
    }
    if (idx < count) {
        int newCount = count - 1;
        if (idx < newCount) {
            uint32_t *p = g_statusRing + idx;
            int n = newCount - idx;
            do {
                p[0] = p[1];
                p++;
                n--;
            } while (n != 0);
        }
        g_statusRingCount = newCount;
    }
    if (groupHead != nullptr)
        Gump_DetachGroup(this);
    if (frameData[1] != 0) {
        FreeTagged(bitmap_at(this, 1), s_frame0Free);
        frameData[1] = 0;
    }
    if (frameData[3] != 0) {
        FreeTagged(bitmap_at(this, 3), g_004a84ec);
        frameData[3] = 0;
    }
    if (frameData != nullptr) {
        FreeTagged(frameData, g_004a84d8);
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00468480, CStatusGump);

// ---------------------------------------------------------------------------
// 0x00468650  size 0x411
// Left button released. If the gump was dropped next to another one
// (snapTo), link the four neighbour pointers on the side it snapped to and
// join the other gump's group; then finish the click.
// ---------------------------------------------------------------------------
void __thiscall CStatusGump::MouseUp(int, int)
{
    CStatusGump *o = snapTo;
    CStatusGump *grp;
    CStatusGump *a;
    CStatusGump *b;
    CStatusGump *last;

    if (o != nullptr) {
        uint8_t side = snapSide;
        switch (side - 1) {
        case 0:
            o->link_100 = this;
            o = snapTo;
            link_fc = o;
            a = o->link_104;
            if (a != nullptr) {
                b = a->link_100;
                if (b != nullptr) {
                    link_104 = b;
                    a = o->link_104;
                    b = a->link_100;
                    b->link_108 = this;
                }
            }
            o = snapTo;
            a = o->link_108;
            if (a != nullptr) {
                b = a->link_100;
                if (b != nullptr) {
                    link_108 = b;
                    a = o->link_108;
                    b = a->link_100;
                    b->link_104 = this;
                }
            }
            break;
        case 1:
            o->link_fc = this;
            o = snapTo;
            link_100 = o;
            a = o->link_104;
            if (a != nullptr) {
                b = a->link_fc;
                if (b != nullptr) {
                    link_104 = b;
                    a = o->link_104;
                    b = a->link_fc;
                    b->link_108 = this;
                }
            }
            o = snapTo;
            a = o->link_108;
            if (a != nullptr) {
                b = a->link_fc;
                if (b != nullptr) {
                    link_108 = b;
                    a = o->link_108;
                    b = a->link_fc;
                    b->link_104 = this;
                }
            }
            break;
        case 3:
            o->link_108 = this;
            o = snapTo;
            link_104 = o;
            a = o->link_fc;
            if (a != nullptr) {
                b = a->link_108;
                if (b != nullptr) {
                    link_fc = b;
                    a = o->link_fc;
                    b = a->link_108;
                    b->link_100 = this;
                }
            }
            o = snapTo;
            a = o->link_100;
            if (a != nullptr) {
                b = a->link_108;
                if (b != nullptr) {
                    link_100 = b;
                    a = o->link_100;
                    b = a->link_108;
                    b->link_fc = this;
                }
            }
            break;
        case 7:
            o->link_104 = this;
            o = snapTo;
            link_108 = o;
            a = o->link_fc;
            if (a != nullptr) {
                b = a->link_104;
                if (b != nullptr) {
                    link_fc = b;
                    a = o->link_fc;
                    b = a->link_104;
                    b->link_100 = this;
                }
            }
            o = snapTo;
            a = o->link_100;
            if (a != nullptr) {
                b = a->link_104;
                if (b != nullptr) {
                    link_100 = b;
                    a = o->link_100;
                    b = a->link_104;
                    b->link_fc = this;
                }
            }
            break;
        default:                                  // sides 3, 5, 6, 7 and anything > 8 go straight to the group join
            break;
        }

        // join the snapped-to gump's group
        o = snapTo;
        grp = o->groupHead;
        if (grp == nullptr) {
            groupHead = o;
            groupNext = nullptr;
            o->groupHead = o;
            snapTo->groupNext = this;
            o = snapTo;
            if (x < o->x) {
                groupHead->minX = this;
                groupHead->maxX = snapTo;
            } else {
                groupHead->minX = o;
                groupHead->maxX = this;
            }
            o = snapTo;
            if (y < o->y) {
                groupHead->minY = this;
                groupHead->maxY = snapTo;
            } else {
                groupHead->minY = o;
                grp = groupHead;
                grp->maxY = this;
            }
        } else {
            groupHead = grp;
            groupNext = nullptr;
            last = grp;
            a = grp->groupNext;
            while (a != nullptr) {
                last = a;
                a = last->groupNext;
            }
            last->groupNext = this;
            grp = groupHead;
            a = grp->minX;
            if (x < a->x)
                grp->minX = this;
            grp = groupHead;
            a = grp->maxX;
            if (x > a->x)
                grp->maxX = this;
            grp = groupHead;
            a = grp->minY;
            if (y < a->y)
                grp->minY = this;
            grp = groupHead;
            a = grp->maxY;
            if (y > a->y)
                grp->maxY = this;
        }
        snapTo = nullptr;
    }

    g_dragGump = nullptr;
    int32_t wasMoved = moved;
    dragging = 0;
    if (wasMoved != 0) {
        moved = 0;
        clickState = 0;
        return;
    }
    if (clickState == 2) {
        int32_t pinnedNow = unk_60;               // read before the click state is cleared
        clickState = 0;
        if (pinnedNow != 0) {
            if (owner == g_player || IsGameMaster() != 0)
                Gump_Notify(this);
            if (unk_60 != 0) {
                if (groupHead == nullptr)
                    return;
                Gump_BringToFront(this);
                dragging = 1;
                unk_6c = unk_24 / 2;
                unk_70 = unk_28 / 2;
                g_dragGump = this;
            }
        }
        if (groupHead != nullptr)
            Gump_DetachGroup(this);
    }
}
HOOK_VMETHOD(0x00468650, CStatusGump, MouseUp);

// ---------------------------------------------------------------------------
// 0x00468d40  size 0xb6
// Status-gump bar blit: draw art 0x805 at (x, y), then the same bitmap clipped
// to a rect whose width is width * cur / max (the full width when cur > max or
// max == 0), with the hue `hueId`. The ECX `this` is not read.
// ? The original divides and multiplies on the x87 stack (80-bit); here it is done in double, so the
//   truncated width can differ by one pixel when width * cur / max is an exact integer.
// ---------------------------------------------------------------------------
void __thiscall CStatusGump::DrawBar(int bx, int by, int barWidth, int barHeight, uint16_t cur, uint16_t max,
                                     uint16_t *bmp, int hueId)
{
    int rect[4];

    int w = static_cast<int>(barWidth * ((cur <= max && max != 0) ? static_cast<float>(cur) / max : 1.0f));   // __ftol
    rect[0] = bx;
    rect[1] = by;
    rect[2] = w + bx - 1;
    rect[3] = barHeight + by - 1;
    char *buf = pixels_of(bmp);
    DrawArt(g_rowPlain, 0x805, bx, by, buf, bmp[0], bmp[1]);
    DrawArtClipped(g_rowPlain, hueId, rect, bx, by, buf, bmp[0], bmp[1]);
}
HOOK_METHOD(0x00468d40, CStatusGump, DrawBar);

// ---------------------------------------------------------------------------
// 0x00468e00  size 0x5da
// Redraw the frame: background art, the bars (three for the player's own
// gump, one for others), then the text values. The four stack arguments are
// not read.
// ---------------------------------------------------------------------------
void __thiscall CStatusGump::RedrawFrame(int, int, int, int)
{
    int rect[4];
    int textH;
    int t2;
    int t1;
    char buf[0x28];
    uint16_t *bmp;

    rect[2] = width;
    rect[3] = height;
    rect[0] = 0;
    rect[1] = 0;
    TextMeasure(1, g_refText, &t1, &t2, &textH);

    bmp = bitmap_at(this, 1);
    DrawArt(g_rowPlain, unk_50, 0, 0, pixels_of(bmp), bmp[0], bmp[1]);

    if (owner == g_player) {
        bmp = bitmap_at(this, 3);
        if (RAWF(int32_t, g_player, 0x150) != 0)
            DrawArt(g_rowPlain, 0x807, 0, 0, pixels_of(bmp), bmp[0], bmp[1]);
        else
            DrawArt(g_rowPlain, 0x803, 0, 0, pixels_of(bmp), bmp[0], bmp[1]);
    } else {
        bmp = bitmap_at(this, 3);
        DrawArt(g_rowPlain, 0x804, 0, 0, pixels_of(bmp), bmp[0], bmp[1]);
    }

    char *nm = nameText;
    if (strlen(nm) == 0) {
        if (editControl == nullptr)
            return;
    }

    if (owner == g_player) {
        uint8_t flags = RAWF(uint8_t, g_player, 0x98);
        if ((flags & 4) != 0)
            DrawBar(0x22, 0xc, 0x6d, 0xb, hits, hitsMax, bitmap_at(this, 3), 0x808);
        else if ((flags & 8) != 0)
            DrawBar(0x22, 0xc, 0x6d, 0xb, hits, hitsMax, bitmap_at(this, 3), 0x809);
        else
            DrawBar(0x22, 0xc, 0x6d, 0xb, hits, hitsMax, bitmap_at(this, 3), 0x806);
        DrawBar(0x22, 0x19, 0x6d, 0xb, stam, stamMax, bitmap_at(this, 3), 0x806);
        DrawBar(0x22, 0x26, 0x6d, 0xb, mana, manaMax, bitmap_at(this, 3), 0x806);
    } else {
        DrawBar(0x22, 0x26, 0x6d, 0xb, hits, hitsMax, bitmap_at(this, 3), 0x806);
    }

    char *dest0 = pixels_of(bitmap_at(this, 1));
    strcpy(buf, nm);
    rect[2] = 0x104;
    TextDraw(0x56, textH + 0x2b, 1, 0x386, buf, dest0, width, rect);

    rect[2] = width;
    if (sex == 0)
        TextDraw(0x56, textH + 0x62, 1, 0x386, "M", pixels_of(bitmap_at(this, 1)), rect[2], rect);
    else
        TextDraw(0x56, textH + 0x62, 1, 0x386, "F", pixels_of(bitmap_at(this, 1)), rect[2], rect);

    sprintf(buf, "%u", static_cast<unsigned>(stat_bc));
    TextDraw(0x56, textH + 0x3e, 1, 0x386, buf, pixels_of(bitmap_at(this, 1)), width, rect);

    sprintf(buf, "%u", static_cast<unsigned>(stat_be));
    TextDraw(0x56, textH + 0x4a, 1, 0x386, buf, pixels_of(bitmap_at(this, 1)), width, rect);

    sprintf(buf, "%u", static_cast<unsigned>(stat_c0));
    TextDraw(0x56, textH + 0x56, 1, 0x386, buf, pixels_of(bitmap_at(this, 1)), width, rect);

    sprintf(buf, "%u/%u", static_cast<unsigned>(hits), static_cast<unsigned>(hitsMax));
    TextDraw(0xad, textH + 0x3e, 1, 0x386, buf, pixels_of(bitmap_at(this, 1)), width, rect);

    sprintf(buf, "%u/%u", static_cast<unsigned>(stam), static_cast<unsigned>(stamMax));
    TextDraw(0xad, textH + 0x4a, 1, 0x386, buf, pixels_of(bitmap_at(this, 1)), width, rect);

    sprintf(buf, "%u/%u", static_cast<unsigned>(mana), static_cast<unsigned>(manaMax));
    TextDraw(0xad, textH + 0x56, 1, 0x386, buf, pixels_of(bitmap_at(this, 1)), width, rect);

    sprintf(buf, "%u", static_cast<unsigned>(stat_d0));
    TextDraw(0xad, textH + 0x62, 1, 0x386, buf, pixels_of(bitmap_at(this, 1)), width, rect);

    sprintf(buf, "%u", static_cast<unsigned>(stat_d4));
    TextDraw(0x56, textH + 0x6e, 1, 0x386, buf, pixels_of(bitmap_at(this, 1)), width, rect);

    sprintf(buf, "%u", static_cast<unsigned>(stat_d6));
    TextDraw(0xad, textH + 0x6e, 1, 0x386, buf, pixels_of(bitmap_at(this, 1)), width, rect);

    if (owner != g_player) {
        if (editControl == nullptr) {
            rect[3] = unk_28;
            rect[2] = 0x8a;
            TextDraw(0x10, textH + 0xe, 1, 0x386, nm, pixels_of(bitmap_at(this, 3)), unk_24, rect);
        }
    }
}
HOOK_METHOD(0x00468e00, CStatusGump, RedrawFrame);
