// Bulletin board windows: CBBGump (the board window) and CBBLineGump (one message line). The methods of
// CBBGump that open the composer are in bulletin_board.cpp, the line drawing helpers in bb_msg_gump.cpp.
#include <cstring>
#include <windows.h>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "bb_gump.h"
#include "bb_line_gump.h"
#include "globals.h"
#include "obj.h"
#include "crt.h"
#include "raw_field.h"
#include "scroll_thumb_gump.h"
#include "button_gump.h"
#include "map_world_ops_ports.h"
#include "gump_art_ops_ports.h"
#include "vtables.h"

#define g_bbLineHeight (GLOBAL_AS(uint32_t, g_004a619c) & 0xff)   // row height of a line gump (low byte of the dword)

// 0x00413110: thiscall on new memory, RET 0x24: rectangle button (x1, y1, x2, y2, parent (not read), type, frame,
// callback address, p9).
// 0x00453ad0: thiscall, no args.
#define Obj_DrainList(self) (reinterpret_cast<CContainerObj *>(self)->DestroyChildren())
// 0x00446310: thiscall on the frame allocator, (ptr, tag).
#define FrameData_Free(o, a1, a2) PORT_THISCALL2(&Map_Free, void, (o), void *, const char *)(a1, a2)
// 0x00433e70: ECX = art id, EDX = &width, stack = &height.
#define ArtSize(a1, a2, a3) (GumpArtSizeImpl(a1, a2, a3))

// Virtual slot 0xe0 answers true for the line gumps among the children of a board gump.
static inline int BB_IsLine(CGump *g)
{
    return PORT_VSLOT0(g, 0xe0, int)();
}

// The test the compiler inlined at many sites: a line with no parent post is always shown, otherwise the
// parent post's line gump (post+0x88) must be expanded (+0x98 == 1).
static inline int BB_ParentExpanded(CBBLineGump *l)
{
    if (l->post == nullptr)
        return 1;
    return RAWF(int32_t, RAWF(void *, l->post, 0x88), 0x98) == 1;
}

// 0x00402f00  size 0x371
// Constructor (stack args, RET 8; the SEH frame is omitted). Draws the board frame art (0x87a) and the
// title into frame 0, centres the window, and creates the scrollbar thumb and three rectangle buttons.
CBBGump::CBBGump(CObj *board, const char *title)
{
    int mw;
    int mx;
    int mh;
    int rect[4];

    name = const_cast<char *>("bulletinboard gump");
    dirty = 0;
    dragging = 0;
    closing = 0;
    owner = board;
    if (board != nullptr && PORT_VSLOT0(board, 0x30, int)() != 0)
        RAWF(CBBGump *, owner, 0xac) = this;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    unk_50 = 0x87a;
    ArtSize(0x87a, &width, &height);
    rowStride = width;
    x = (0x280 - width) / 2;
    y = (0x1e0 - height) / 2;
    frameIndex = 0;
    frameCount = 1;
    frameData = FrameData_Alloc(g_frameDataAllocator, 8, "framedata for CBBGump");
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    frameData[1] = static_cast<int32_t>(reinterpret_cast<uintptr_t>(
        FrameData_Alloc(g_frameDataAllocator, height * width * 2 + 4, "bb frame")));
    uint8_t *bmp = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(frameData[1]));
    memset(bmp + 4, 0, static_cast<size_t>(height * width) * 2);
    *reinterpret_cast<uint16_t *>(bmp + 0) = static_cast<uint16_t>(width);
    *reinterpret_cast<uint16_t *>(bmp + 2) = static_cast<uint16_t>(height);
    gfx::DrawArt(gfx::artPlain(), unk_50, 0, 0, bmp + 4, *reinterpret_cast<uint16_t *>(bmp + 0),
                 *reinterpret_cast<uint16_t *>(bmp + 2));

    gfx::TextMeasure(2, title, &mw, &mx, &mh);
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = width;
    rect[3] = height;
    gfx::TextDraw(0xf0 - mw / 2, (mh - mx / 2) + 0x2f, 2, 0x386, title, bmp + 4, width, rect);

    lineCount = 0;
    topLine = nullptr;
    textLeft = 0x6d;
    textRight = 0x162;
    textTop = 0x9f;
    textBottom = 0x144;

    CScrollThumbGump *thumb = new CScrollThumbGump(0x164, 0xa2, 1, 0x13c, (int)(this), 0x828);
    scrollThumb = thumb;
    CGump_AddChild(thumb, this, 1);
    gfx::ScrollThumbSet(static_cast<CScrollThumbGump *>(scrollThumb), 0, 1);

    CGump *btn;
    btn = new CButtonGump(0x15, 0xab, 0x5c, 0xfa, this, 0, 0, PORT_ADDR(&CBBGump::OnNewPostButton), 0);
    CGump_AddChild(btn, this, 1);

    btn = new CButtonGump(0x164, 0x90, 0x176, 0x9c, this, 2, 0, PORT_ADDR(&CBBGump::ScrollUp), 0);
    button_b0 = btn;
    CGump_AddChild(btn, this, 1);

    btn = new CButtonGump(0x164, 0x140, 0x176, 0x14e, this, 2, 0, PORT_ADDR(&CBBGump::ScrollDown), 0);
    button_b4 = btn;
    CGump_AddChild(btn, this, 1);
}
HOOK_CTOR(0x00402f00, CBBGump, (CObj *, const char *));

HOOK_VTABLE(0x0049c348, CBBGump);
HOOK_SDD(0x00403280, CBBGump);

// 0x004032b0  size 0xad
// Destructor body: marks the window closing, drains the board object's list (+0xa8, through virtual slot 4)
// when it answers slot 0x30, frees the frame bitmap and table. The SEH frame is omitted.
CBBGump::~CBBGump()
{
    closing = 1;
    if (owner != nullptr) {
        if (PORT_VSLOT0(owner, 0x30, int)() != 0)
            Obj_DrainList(owner);   // 0x00453ad0
    }
    if (frameData[1] != 0) {
        FrameData_Free(g_frameDataAllocator, reinterpret_cast<void *>(static_cast<uintptr_t>(frameData[1])), "BB frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        FrameData_Free(g_frameDataAllocator, frameData, "BB framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x004032b0, CBBGump);

// 0x00403360  size 0x28
// Left button down: raise the window, start dragging, remember the position in +0x6c / +0x70.
void __thiscall CBBGump::MouseDown(int mouseX, int mouseY)
{
    CGump_BringToFront(this, 0);
    dragging = 1;
    unk_6c = mouseX;
    unk_70 = mouseY;
}
HOOK_VMETHOD(0x00403360, CBBGump, MouseDown);

// 0x00403390  size 0x14
void __thiscall CBBGump::MouseUp(int, int)
{
    dragging = 0;
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x00403390, CBBGump, MouseUp);

// 0x004033b0  size 0x2c
// Mouse move: while dragging, move the window by the distance from the press position.
void __thiscall CBBGump::MouseMove(int mouseX, int mouseY)
{
    if (dragging != 0)
        Move(mouseX - unk_6c, mouseY - unk_70, 1, nullptr, 1);
}
HOOK_VMETHOD(0x004033b0, CBBGump, MouseMove);

// 0x004033e0  size 0x1b7
// Scrollbar callback: make the line at fraction pos / range of the visible lines the top line and shift
// every line gump so that it sits at the top of the list area; lines outside the area are hidden.
void __thiscall CBBGump::OnScroll(int pos, int range, int)
{
    int count = 0;
    CGump *g;

    if (topLine == nullptr)
        return;
    for (g = lastChild; g != nullptr; g = g->prev) {
        if (BB_IsLine(g) != 0) {
            if (BB_ParentExpanded(static_cast<CBBLineGump *>(g)) != 0)
                count++;
        }
    }
    int target = count * pos / range;
    if (target == count)
        target--;

    count = 0;
    for (g = lastChild; g != nullptr; g = g->prev) {
        if (BB_IsLine(g) != 0) {
            if (BB_ParentExpanded(static_cast<CBBLineGump *>(g)) != 0) {
                if (count == target) {
                    topLine = g;
                    break;
                }
                count++;
            }
        }
    }

    CGump *first;
    for (first = lastChild; first != nullptr; first = first->prev) {
        if (BB_IsLine(first) != 0) {
            if (BB_ParentExpanded(static_cast<CBBLineGump *>(first)) != 0)
                break;
        }
    }
    // ? the original dereferences `first` here without a null test
    int delta = (textTop - static_cast<int>(g_bbLineHeight) * count) - first->y;
    for (g = lastChild; g != nullptr; g = g->prev) {
        if (BB_IsLine(g) != 0) {
            g->y = g->y + delta;
            int ly = g->y;
            if (static_cast<int>(g_bbLineHeight) + ly > textBottom || ly < textTop) {
                g->visible = 0;
            } else if (BB_ParentExpanded(static_cast<CBBLineGump *>(g)) != 0) {
                g->visible = 1;
            }
        }
    }
}
HOOK_VMETHOD(0x004033e0, CBBGump, OnScroll);

// 0x004035a0  size 0x40
// Virtual slot 0x0c: true when (px, py) is inside the frame and its pixel in frame 0 is non-zero.
// ? the pixel is read from the bitmap block start (frameData[1]) without skipping its 4-byte size header.
int __thiscall CBBGump::HitTest(int px, int py)
{
    if (px < 0 || py < 0 || px >= width || py >= height)
        return 0;
    const uint16_t *pixels = reinterpret_cast<const uint16_t *>(static_cast<uintptr_t>(frameData[1]));
    return pixels[py * width + px] != 0;
}
HOOK_VMETHOD(0x004035a0, CBBGump, HitTest);

// 0x004035e0  size 0x332
// Create the line gump for `post` and link it into the child list. A top-level line goes to the head of the
// list, one position below the current head line. A reply (parentPost non-null) is inserted behind its
// thread's line: the thread's icon turns to "collapsed" the first time it gets a reply (and the open
// message viewers of this board drop the child at +0xa0), and the reply is shown only while the thread is
// expanded. The SEH frame is omitted.
void __thiscall CBBGump::AddLine(CObj *post, CObj *parentPost, const char *s1, const char *s2, const char *s3)
{
    CBBLineGump *line;
    CGump *g;
    CGump *esi;

    lineCount = lineCount + 1;
    if (parentPost != nullptr)
        line = new CBBLineGump(textLeft + 0x14, 0, (textRight - textLeft) - 0x13, post, parentPost, s1, s2, s3);
    else
        line = new CBBLineGump(textLeft, 0, (textRight - textLeft) + 1, post, nullptr, s1, s2, s3);
    if (topLine == nullptr)
        topLine = line;

    if (parentPost == nullptr) {
        esi = firstChild;
        while (esi != nullptr) {
            if (BB_IsLine(esi) != 0)
                break;
            esi = esi->next;
        }
        if (esi != nullptr)
            line->y = esi->y + static_cast<int>(g_bbLineHeight);
        else
            line->y = textTop;
        if (static_cast<int>(g_bbLineHeight) + line->y > textBottom)
            line->visible = 0;
        line->parent = this;
        line->prev = nullptr;
        firstChild->prev = line;
        line->next = firstChild;
        firstChild = line;
        return;
    }

    line->parent = this;
    esi = lastChild;
    while (esi != nullptr) {
        if (BB_IsLine(esi) != 0 && esi->owner == parentPost)
            break;
        esi = esi->prev;
    }
    line->visible = 0;
    if (esi != nullptr) {
        CBBLineGump *threadLine = static_cast<CBBLineGump *>(esi);
        if (threadLine->iconState == -1) {
            threadLine->iconState = 0;
            threadLine->DrawIcon();   // 0x00404380
            for (g = g_gumpListTail; g != nullptr; g = g->prev) {
                if (PORT_VSLOT0(g, 0xe4, int)() != 0 && g->owner == esi->owner && RAWF(CGump *, g, 0xa0) != nullptr) {
                    CGump *child = RAWF(CGump *, g, 0xa0);
                    PORT_VSLOT1(child, 0x00, void, int)(1);
                    RAWF(CGump *, g, 0xa0) = nullptr;
                }
            }
        } else if (threadLine->iconState == 1) {
            line->visible = 1;
        }
    }

    // Find the top-level line that precedes the thread's line (walk +0x3c while the gump has a parent post).
    if (esi != nullptr) {
        esi = esi->prev;
        while (esi != nullptr && RAWF(void *, esi, 0x94) != nullptr)
            esi = esi->prev;
    }
    line->prev = esi;
    if (esi != nullptr) {
        line->next = esi->next;
        esi->next->prev = line;
        esi->next = line;
        line->y = line->next->y + static_cast<int>(g_bbLineHeight);
        if (line->visible != 0) {
            for (; esi != nullptr; esi = esi->prev) {
                if (BB_IsLine(esi) != 0) {
                    esi->y = esi->y + static_cast<int>(g_bbLineHeight);
                    if (static_cast<int>(g_bbLineHeight) + esi->y > textBottom)
                        esi->visible = 0;
                }
            }
        }
    } else {
        firstChild->prev = line;
        line->next = firstChild;
        firstChild = line;
        line->y = line->next->y + static_cast<int>(g_bbLineHeight);
    }
    if (static_cast<int>(g_bbLineHeight) + line->y > textBottom)
        line->visible = 0;
}
HOOK_METHOD(0x004035e0, CBBGump, AddLine);

// 0x00403a10  size 0x23f
// Remove a line gump: when it was the last reply of its thread, mark the thread "no replies" and redraw its
// icon; pull the lines before it down one row; if it was the top line pick another visible line (or scroll
// up); finally delete it and drop the line count. Does nothing while the window is closing.
void __thiscall CBBGump::RemoveLine(CGump *line)
{
    CGump *esi;

    if (closing != 0)
        return;
    if (RAWF(void *, line, 0x94) != nullptr) {
        int onlyReply = 1;
        for (esi = lastChild; esi != nullptr; esi = esi->prev) {
            if (esi != line && BB_IsLine(esi) != 0 && RAWF(void *, esi, 0x94) == RAWF(void *, line, 0x94))
                onlyReply = 0;
        }
        if (onlyReply != 0) {
            CBBLineGump *threadLine = RAWF(CBBLineGump *, RAWF(void *, line, 0x94), 0x88);
            threadLine->iconState = -1;
            threadLine->DrawIcon();   // 0x00404380
        }
    }
    for (esi = line->prev; esi != nullptr; esi = esi->prev) {
        if (BB_IsLine(esi) != 0) {
            esi->y = esi->y - static_cast<int>(g_bbLineHeight);
            int ly = esi->y;
            if (static_cast<int>(g_bbLineHeight) + ly > textBottom || ly < textTop) {
                esi->visible = 0;
            } else if (BB_ParentExpanded(static_cast<CBBLineGump *>(esi)) != 0) {
                esi->visible = 1;
            }
        }
    }
    if (topLine == line) {
        for (esi = line->prev; esi != nullptr; esi = esi->prev) {
            if (BB_IsLine(esi) != 0 && BB_ParentExpanded(static_cast<CBBLineGump *>(esi)) != 0)
                break;
        }
        if (esi != nullptr) {
            for (esi = line->prev; esi != nullptr; esi = esi->prev) {
                if (BB_IsLine(esi) != 0 && BB_ParentExpanded(static_cast<CBBLineGump *>(esi)) != 0)
                    break;
            }
            topLine = esi;
        } else {
            for (esi = line->next; esi != nullptr; esi = esi->next) {
                if (BB_IsLine(esi) != 0 && BB_ParentExpanded(static_cast<CBBLineGump *>(esi)) != 0)
                    break;
            }
            if (esi != nullptr)
                ScrollUp();   // 0x00410960
            else
                topLine = nullptr;
        }
    }
    if (line != nullptr)
        PORT_VSLOT1(line, 0x00, void, int)(1);
    lineCount = lineCount - 1;
}
HOOK_METHOD(0x00403a10, CBBGump, RemoveLine);

// 0x00403c50  size 0x235
// Line constructor (8 stack args, RET 0x20; the SEH frame is omitted). The line is a bitmap of
// width x row height holding the expander icon background (art 0x1523 at x 0x12) and the caption
// "<s1> - <s2> [<s3>]" drawn after the icon. `owner` (the post) gets this gump as its +0x88.
CBBLineGump::CBBLineGump(int x_, int y_, int width_, CObj *owner_, CObj *parentPost,
                         const char *s1, const char *s2, const char *s3)
{
    char caption[0x80];
    int rect[4];
    int mx;
    int mh;
    int artW;
    int artH;

    post = parentPost;
    name = const_cast<char *>("bbline gump");
    dirty = 0;
    unk_a8 = 0;
    unk_9c = 0;
    clickTime = 0;
    clickState = 0;
    unk_a4 = 0;
    iconState = -1;
    x = x_;
    y = y_;
    owner = owner_;
    if (owner_ != nullptr)
        RAWF(CBBLineGump *, owner_, 0x88) = this;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    unk_50 = 0x1518;
    rowStride = width_;
    width = width_;
    height = static_cast<int>(g_bbLineHeight);
    frameIndex = 0;
    frameCount = 1;
    frameData = FrameData_Alloc(g_frameDataAllocator, 8, "framedata for CBBLineGump");
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    frameData[1] = static_cast<int32_t>(reinterpret_cast<uintptr_t>(
        FrameData_Alloc(g_frameDataAllocator, height * width * 2 + 4, "bbline frame")));
    uint8_t *bmp = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(frameData[1]));
    memset(bmp + 4, 0, static_cast<size_t>(height * width) * 2);
    *reinterpret_cast<uint16_t *>(bmp + 0) = static_cast<uint16_t>(width);
    *reinterpret_cast<uint16_t *>(bmp + 2) = static_cast<uint16_t>(height);
    unk_50 = 0x1523;
    gfx::DrawArt(gfx::artPlain(), unk_50, 0x12, 0, bmp + 4, *reinterpret_cast<uint16_t *>(bmp + 0),
                 *reinterpret_cast<uint16_t *>(bmp + 2));

    int unusedW;
    gfx::TextMeasure(9, g_refText, &unusedW, &mx, &mh);
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = width;
    rect[3] = height;
    sprintf(caption, "%s - %s [%s]", s1, s2, s3);
    ArtSize(unk_50, &artW, &artH);
    if (artW != 0) {
        gfx::TextDrawAlt(artW + 0x18, ((height - mx) / 2) + mh, 9, 0x386, caption, bmp + 4, width, rect);
    }
}
HOOK_CTOR(0x00403c50, CBBLineGump, (int, int, int, CObj *, CObj *, const char *, const char *, const char *));

HOOK_VTABLE(0x0049c460, CBBLineGump);
HOOK_SDD(0x00403e90, CBBLineGump);

// 0x00403ec0  size 0x98
// Destructor body: detach from the post (post+0x88), free the frame bitmap and table. The SEH frame is omitted.
CBBLineGump::~CBBLineGump()
{
    if (owner != nullptr && RAWF(CBBLineGump *, owner, 0x88) == this)
        RAWF(CBBLineGump *, owner, 0x88) = nullptr;
    if (frameData[1] != 0) {
        FrameData_Free(g_frameDataAllocator, reinterpret_cast<void *>(static_cast<uintptr_t>(frameData[1])), "BBLine frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        FrameData_Free(g_frameDataAllocator, frameData, "BBLine framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00403ec0, CBBLineGump);

// 0x00403f60  size 0x5b
// Left button down: raise the board window, remember the position, and run the click state machine
// (1 = first click, 2 = second click inside the double-click time).
void __thiscall CBBLineGump::MouseDown(int mouseX, int mouseY)
{
    if (parent != nullptr)
        CGump_BringToFront(parent, 0);
    unk_a4 = 1;
    unk_6c = mouseX;
    unk_70 = mouseY;
    if (clickState == 1) {
        clickState = 2;
        return;
    }
    clickState = 1;
    clickTime = static_cast<int32_t>(GetTickCount());
}
HOOK_VMETHOD(0x00403f60, CBBLineGump, MouseDown);

// 0x004040b0  size 0x22c
// A click on the expander icon toggles the replies of this thread: flip the expanded state, show or hide the
// consecutive reply lines before this one, shift the lines after them up or down by that many rows, redraw
// the icon and update the scrollbar thumb to the top line's index among the visible lines.
void __thiscall CBBLineGump::ToggleReplies(int mouseX, int mouseY)
{
    int artW;
    int artH;
    int topIndex = 0;
    int rows = 0;
    CGump *esi;

    if (iconState == -1)
        return;
    ArtSize(0x1519, &artW, &artH);
    if (artW == 0)
        return;
    CBBGump *board = static_cast<CBBGump *>(parent);
    CGump *oldTop = board->topLine;
    if (mouseX < 0 || mouseY < 0 || mouseX > artW - 1 || mouseY > artH - 1)
        return;
    iconState = (iconState == 0) ? 1 : 0;

    for (esi = prev; esi != nullptr; esi = esi->prev) {
        if (BB_IsLine(esi) == 0 || RAWF(void *, esi, 0x94) == nullptr)
            break;
        esi->visible = (iconState == 1) ? 1 : 0;
        int ly = esi->y;
        if (static_cast<int>(g_bbLineHeight) + ly > board->textBottom || ly < board->textTop)
            esi->visible = 0;
        rows++;
    }
    for (; esi != nullptr; esi = esi->prev) {
        if (BB_IsLine(esi) == 0)
            break;
        if (iconState == 1)
            esi->y = esi->y + rows * static_cast<int>(g_bbLineHeight);
        else
            esi->y = esi->y - rows * static_cast<int>(g_bbLineHeight);
        int ly = esi->y;
        if (static_cast<int>(g_bbLineHeight) + ly > board->textBottom || ly < board->textTop) {
            esi->visible = 0;
        } else if (BB_ParentExpanded(static_cast<CBBLineGump *>(esi)) != 0) {
            esi->visible = 1;
        }
    }

    DrawIcon();   // 0x00404380
    int shown = 0;
    for (esi = board->lastChild; esi != nullptr; esi = esi->prev) {
        if (BB_IsLine(esi) != 0 && BB_ParentExpanded(static_cast<CBBLineGump *>(esi)) != 0) {
            if (oldTop == esi)
                topIndex = shown;
            shown++;
        }
    }
    if (shown < 2)
        shown = 2;
    gfx::ScrollThumbSet(static_cast<CScrollThumbGump *>(board->scrollThumb), topIndex, shown - 1);
}
HOOK_METHOD(0x004040b0, CBBLineGump, ToggleReplies);

// 0x004042e0  size 0x39
// Mouse move: while the button is down and the line has a board window, drag that window.
void __thiscall CBBLineGump::MouseMove(int mouseX, int mouseY)
{
    if (unk_a4 != 0 && parent != nullptr)
        parent->Move(mouseX - unk_6c, mouseY - unk_70, 1, nullptr, 1);
}
HOOK_VMETHOD(0x004042e0, CBBLineGump, MouseMove);

// 0x00404320  size 0x5a
// Per-tick: when the first click is older than the double-click time, a still-released button means a
// single click (toggle at the stored position), otherwise a drag is pending (unk_9c = 1).
void __thiscall CBBLineGump::Tick()
{
    DWORD now = GetTickCount();
    DWORD dbl = GetDoubleClickTime();
    if (clickState == 1 && static_cast<DWORD>(now - static_cast<DWORD>(clickTime)) > dbl) {
        clickState = 0;
        if (unk_a4 == 0) {
            ToggleReplies(unk_6c, unk_70);   // 0x004040b0
            return;
        }
        unk_9c = 1;
    }
}
HOOK_VMETHOD(0x00404320, CBBLineGump, Tick);
