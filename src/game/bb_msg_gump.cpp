// Bulletin board line and message gumps (0x00404380-0x00406a80), plus the neighbouring
// CObj0049c690 methods and two small helpers that sit in the same address range.
#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "bb_line_gump.h"
#include "bb_msg_gump.h"
#include "button_gump.h"
#include "class_0049c690.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "scroll_thumb_gump.h"
#include "skill_list_gump_ops_ports.h"
#include "font_manager.h"
#include "gump.h"
#include "gump_base_ops_ports.h"
#include "input_control_gump.h"
#include "world_obj_ops.h"
#include "char_creation_pages.h"
#include "char_ui_gumps.h"
#include "vtables.h"


// 0x00472be0: ECX = art id, EDX = 2, stack = (bitmap, 0, art height). RET 0xc.
#define BB_DrawFrame(a1, a2, a3, a4, a5) (SkillList_DrawTiledFrame(a1, a2, (uint16_t *)(a3), a4, a5))
// 0x00432ba0: thiscall on the font object, (font, text, width); index of the last character that fits, or -1.
#define BB_FindWrap(text, width) PORT_THISCALL3(&CFontManager::FindWrapBreak, int, gfx::textObj(), int, const char *, int)(9, text, width)
// 0x00433a50: thiscall on the font object, (font, text, x offset); character index under that x. RET 0xc.
#define BB_CaretFromX(text, dx) PORT_THISCALL3(&CFontManager::CharsFitting, int, gfx::textObj(), int, const char *, int)(9, text, dx)
// 0x0043a820: thiscall on the gump manager, (gump): give it the keyboard focus.
#define BB_Focus(g) PORT_THISCALL1(&Gump_SetFocus, void, g_gumpManager, CGump *)(g)
#define BB_SELF static_cast<int>(reinterpret_cast<uintptr_t>(this))

namespace {

inline uint16_t *Bmp(int32_t slot) { return reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(slot))); }

} // namespace

// 0x00404380  size 0xb0
// CBBLineGump::DrawIcon. Clears frame 0 over the size of art 0x1519, then draws the collapsed (0x37)
// or expanded (0x38) icon at the top left.
void __thiscall CBBLineGump::DrawIcon()
{
    int w, h;

    gfx::ArtSize(0x1519, &w, &h);
    if (w != 0) {
        gfx::FillRect(0, 0, w - 1, h - 1, 0, gfx::bitmapOf(this));
        if (iconState == 0) {
            uint16_t *bmp = gfx::bitmapOf(this);
            gfx::DrawArt(gfx::artPlain(), 0x37, 0, 0, bmp + 2, bmp[0], bmp[1]);
            return;
        }
        if (iconState == 1) {
            uint16_t *bmp = gfx::bitmapOf(this);
            gfx::DrawArt(gfx::artPlain(), 0x38, 0, 0, bmp + 2, bmp[0], bmp[1]);
        }
    }
}
HOOK_METHOD(0x00404380, CBBLineGump, DrawIcon);

// 0x00404430  size 0xc4
// CBBLineGump::Draw (virtual slot 0x08). Blits frame 0 at the screen origin when the clip rect
// intersects it, then draws the children front to back along +0x40.
void __thiscall CBBLineGump::Draw(int *clip)
{
    int ox, oy;
    if (visible == 0)
        return;
    ox = 0;
    oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);

    int rect[4];
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = ox + gfx::bitmapOf(this)[0];
    rect[3] = gfx::bitmapOf(this)[1] + oy;
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, void *, int *)(clip, rect) == 0)
        return;
    uint16_t *bmp = gfx::bitmapOf(this);
    GFX_BlitFrame(gfx::artPlain(), bmp + 2, clip, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
    for (CGump *c = firstChild; c != nullptr; c = c->next)
        PORT_VSLOT1(c, kGumpSlot_Draw, void, void *)(clip);
}
HOOK_VMETHOD(0x00404430, CBBLineGump, Draw);

// 0x00404500  size 0x31
// CBBLineGump::HitTest (virtual slot 0x0c).
int __thiscall CBBLineGump::HitTest(int px, int py)
{
    if (visible != 0 && px >= 0 && py >= 0 && px < width && py < height)
        return 1;
    return 0;
}
HOOK_VMETHOD(0x00404500, CBBLineGump, HitTest);

// 0x00404540  size 0x25
// CBBLineGump::IsVisibleByParent. Top-level lines (no message object) are always shown; otherwise the
// thread object at post+0x88 must be expanded (+0x98 == 1).
int __thiscall CBBLineGump::IsVisibleByParent()
{
    if (post == nullptr)
        return 1;
    return RAWF(int32_t, RAWF(void *, post, 0x88), 0x98) == 1;
}
HOOK_METHOD(0x00404540, CBBLineGump, IsVisibleByParent);

// 0x00404570  size 0x4b
// CBBLineGump::NextVisibleLine. Walks the +0x3c chain (the counterpart of PrevVisibleLine, which walks +0x40).
CBBLineGump *__thiscall CBBLineGump::NextVisibleLine()
{
    CBBLineGump *l;
    for (l = static_cast<CBBLineGump *>(prev); l != nullptr; l = static_cast<CBBLineGump *>(l->prev)) {
        if (PORT_VSLOT0(l, 0xe0, int)() != 0) {
            int isVisible;
            if (l->post == nullptr)
                isVisible = 1;
            else
                isVisible = (RAWF(int32_t, RAWF(void *, l->post, 0x88), 0x98) == 1);
            if (isVisible != 0)
                break;
        }
    }
    return l;
}
HOOK_METHOD(0x00404570, CBBLineGump, NextVisibleLine);

// 0x004069f0  size 0x90
// Tiles gump art `artId` horizontally from x to the right edge of `rect` ({left, top, right, bottom}),
// at row y of the bitmap `bmp`; the last tile that reaches the right edge is drawn clipped.
// ECX = x, EDX = y, stack = (artId, bmp, rect), RET 0xc.
void __fastcall BBMsg_TileArt(int x, int y, int artId, void *bmp, int *rect)
{
    int w, h;
    gfx::ArtSize(artId, &w, &h);
    int right = rect[2];
    if (x > right)
        return;
    uint16_t *b = static_cast<uint16_t *>(bmp);
    do {
        if (w + x < right)
            gfx::DrawArt(gfx::artPlain(), artId, x, y, b + 2, b[0], b[1]);
        else
            gfx::DrawArtClipped(gfx::artPlain(), artId, rect, x, y, b + 2, b[0], b[1]);
        x += w;
    } while (x <= rect[2]);
}
HOOK_FUNC(0x004069f0, BBMsg_TileArt);

// 0x00404610  size 0x9d1
// CBBMsgGump constructor (stack args, RET 0x28). The SEH frame is omitted. Builds a 300-high window
// with two frames (frame 1 holds the static header and text-box art, frame 0 the composed result),
// the header texts, the scrollbar, the close / scroll buttons and the Post, Reply or Delete button.
// ? bodyId, hue, nEquip and equip are never read here.
CBBMsgGump::CBBMsgGump(CObj *board, uint32_t serial, char *authorText, char *subjectText, char *timeText,
                       uint16_t, uint16_t, uint8_t, uint16_t *, int isNew)
{
    int v10 = 0, v14 = 0, v18 = 0, v1c = 0, v20 = 0, v24 = 0, v28 = 0;
    int rect[4];

    name = const_cast<char *>("bbmsg gump");
    dirty = 0;
    dragging = 0;
    composing = isNew;
    caret = 0;
    cur = 0;
    top = 0;
    lastLine = 0;
    postSerial = static_cast<int32_t>(serial);
    if (composing != 0) {
        lines[0] = static_cast<char *>(::operator new(0x51));
        strcpy(lines[0], g_emptyString);
    }
    owner = board;
    if (board != nullptr)
        RAWF(CBBMsgGump *, board, 0x8c) = this;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    unk_50 = 0x820;
    gfx::ArtSize(0x820, &width, &height);
    rowStride = width;
    height = 0x12c;
    unk_24 = width;
    unk_28 = 0x12c;
    frameIndex = 0;
    frameCount = 2;
    frameData = static_cast<int32_t *>(gfx::AllocTagged(0x10, "framedata for CBBMsgGump"));
    memset(frameData, 0, frameCount * 8);

    frameData[1] = static_cast<int32_t>(
        reinterpret_cast<uintptr_t>(gfx::AllocTagged(width * height * 2 + 4, "bbmsg frame")));
    memset(Bmp(frameData[1]) + 2, 0, width * height * 2);
    Bmp(frameData[1])[0] = static_cast<uint16_t>(width);
    Bmp(frameData[1])[1] = static_cast<uint16_t>(height);

    frameData[3] = static_cast<int32_t>(
        reinterpret_cast<uintptr_t>(gfx::AllocTagged(unk_28 * unk_24 * 2 + 4, "bbmsg frame")));
    memset(Bmp(frameData[3]) + 2, 0, unk_28 * unk_24 * 2);
    Bmp(frameData[3])[0] = static_cast<uint16_t>(unk_24);
    Bmp(frameData[3])[1] = static_cast<uint16_t>(unk_28);

    gfx::ArtSize(0x82e, &v28, &v18);
    textBottom = height - v18 - 0x32;
    gfx::ArtSize(0x882, &v20, &v1c);
    textLeft = 0x1e;
    textTop = 0x78;
    textRight = 0xf5;
    strcpy(subject, subjectText);
    strcpy(author, authorText);
    strcpy(time, timeText);
    gfx::TextMeasure(9, "Aq", &v14, &lineHeight, &v10);
    textOrigin = textTop;
    unk_168 = 0x14;
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = width;
    rect[3] = height;
    gfx::ArtSize(0x82e, &v28, &v18);
    BB_DrawFrame(unk_50, 2, Bmp(frameData[3]), 0, v18);
    if (composing != 0) {
        uint16_t *b = Bmp(frameData[3]);
        gfx::DrawArt(gfx::artPlain(), 0x883, 0x61, 0xc, b + 2, b[0], b[1]);
    }
    {
        uint16_t *b = Bmp(frameData[3]);
        gfx::DrawArt(gfx::artPlain(), 0x82b, 0x1e, 0x22, b + 2, b[0], b[1]);
    }
    {
        uint16_t *b = Bmp(frameData[3]);
        gfx::DrawArt(gfx::artPlain(), 0x82b, 0x1e, height - v18 - 0x30, b + 2, b[0], b[1]);
    }
    rect[2] = textRight;

    gfx::TextMeasure(6, "Author:", &v14, &v1c, &v10);
    gfx::TextDrawAlt(textLeft, v10 + 0x28, 6, 0, "Author:", Bmp(frameData[3]) + 2, width, rect);
    gfx::TextMeasure(9, author, &v20, &v1c, &v10);
    gfx::TextDrawAlt(v14 + textLeft + 4, v10 + 0x2e, 9, 0x386, author, Bmp(frameData[3]) + 2, width, rect);
    gfx::TextMeasure(6, "Time:", &v14, &v1c, &v10);
    gfx::TextDrawAlt(textLeft, v10 + 0x38, 6, 0, "Time:", Bmp(frameData[3]) + 2, width, rect);
    gfx::TextMeasure(9, time, &v20, &v1c, &v10);
    gfx::TextDrawAlt(v14 + textLeft + 4, v10 + 0x3e, 9, 0x386, time, Bmp(frameData[3]) + 2, width, rect);
    gfx::TextMeasure(6, "Subject:", &v14, &v1c, &v10);
    gfx::TextDrawAlt(textLeft, v10 + 0x48, 6, 0, "Subject:", Bmp(frameData[3]) + 2, width, rect);

    if (composing == 0) {
        gfx::TextMeasure(9, subject, &v20, &v1c, &v10);
        gfx::TextDrawAlt(v14 + textLeft + 4, v10 + 0x4e, 9, 0x386, subject, Bmp(frameData[3]) + 2, width, rect);
    } else {
        // Editable subject line: the width argument is only used through its low 16 bits by 0x0041de20.
        int editW = static_cast<uint16_t>(textRight - v14 - textLeft) - 4;
        CGump *ctl;
        ctl = new CInputControlGump(v14 + textLeft + 4, 0x4c, this, static_cast<uint16_t>(editW), static_cast<uint16_t>(0x10), 0, 0x3c, 9, 8, 1, 0, 0);
        subjectEdit = ctl;
        RAWF(int32_t, ctl, 0xcc) = 1;
        strcpy(&RAWF(char, ctl, 0xec), subjectText);
        PORT_VSLOT1(ctl, 0x3c, void, int)(0xe023);
        reinterpret_cast<CInputControlGump *>(ctl)->Redraw();   // CInputControlGump::Redraw
        BB_Focus(ctl);
        reinterpret_cast<CInputControlGump *>(ctl)->Redraw();
        CGump_AddChild(ctl, this, 1);
    }

    BBMsg_TileArt(textLeft, 0x6e, 0x835, Bmp(frameData[3]), &textLeft);
    RedrawText();

    CScrollThumbGump *thumb;
    thumb = new CScrollThumbGump(0xfe, 0x78, 1, height - v18 - 0x32, (int)(this), 0x828);
    scrollbar = thumb;
    CGump_AddChild(thumb, this, 1);
    gfx::ScrollThumbSet(thumb, 0, 1);

    CButtonGump *btn;
    btn = new CButtonGump(0x8b, height - v18 - 3, BB_SELF, 0, 0, PORT_ADDR(&CGumpHandlers04::AddBackdropChild104), 0x82e, 0x82e, 0);
    btn->artHover = 0x82f;
    CGump_AddChild(btn, this, 1);

    btn = new CButtonGump(0xf6, 0x6a, BB_SELF, 2, 0, PORT_ADDR(&CBBMsgGump::ScrollUp), 0x824, 0x824, 0);
    CGump_AddChild(btn, this, 1);

    btn = new CButtonGump(0xf6, height - v18 - 0x34, BB_SELF, 2, 0, PORT_ADDR(&CBBMsgGump::ScrollDown), 0x825, 0x825, 0);
    CGump_AddChild(btn, this, 1);

    btnDelete = nullptr;
    btnReply = nullptr;
    if (composing != 0) {
        gfx::ArtSize(0x886, &v14, &v24);
        btn = new CButtonGump(0x25, height - v18 - 0x18, BB_SELF, 0, 0, PORT_ADDR(&CGump_00410820::SendAndDelete), 0x886, 0x886, 0);
    } else {
        gfx::ArtSize(0x884, &v14, &v24);
        btn = new CButtonGump(0x25, height - v18 - 0x18, BB_SELF, 0, 0, PORT_ADDR(&CBBMsgGump::OnReply), 0x884, 0x884, 0);
    }
    btnReply = btn;
    btn->unk_c0 = 0;
    CGump_AddChild(btn, this, 1);

    if (composing == 0) {
        // The author may delete the post while it has no replies (+0x98 == -1 on the thread object).
        if (strcmp(g_playerName, authorText) == 0 && RAWF(int32_t, RAWF(void *, owner, 0x88), 0x98) == -1) {
            gfx::ArtSize(0x885, &v14, &v24);
            btn = new CButtonGump(0xeb, height - v18 - 0x18, BB_SELF, 0, 0, PORT_ADDR(&CGump_00410910::SendOwnerPacket), 0x885, 0x885, 0);
            btnDelete = btn;
            btn->unk_c0 = 0;
            CGump_AddChild(btn, this, 1);
        }
    }
}
HOOK_CTOR(0x00404610, CBBMsgGump, (CObj *, uint32_t, char *, char *, char *, uint16_t, uint16_t, uint8_t, uint16_t *, int));

HOOK_VTABLE(0x0049c578, CBBMsgGump);
HOOK_SDD(0x00404ff0, CBBMsgGump);

// 0x00405020  size 0x10d
// ~CBBMsgGump. The SEH frame is omitted. Unhooks itself from the board line, frees the text lines
// (and the line being composed), the two frame bitmaps and the frame table, then runs the CGump dtor.
CBBMsgGump::~CBBMsgGump()
{
    if (owner != nullptr) {
        if (RAWF(CBBMsgGump *, owner, 0x8c) == this)
            RAWF(CBBMsgGump *, owner, 0x8c) = nullptr;
    }
    if (lastLine > 0) {
        int i = 0;
        char **p = &lines[0];
        do {
            operator delete(*p);
            i++;
            p++;
        } while (i < lastLine);
    }
    if (composing != 0)
        operator delete(lines[lastLine]);
    if (frameData[1] != 0) {
        gfx::FreeTagged(Bmp(frameData[1]), "BBMsg frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData[3] != 0) {
        gfx::FreeTagged(Bmp(frameData[3]), "BBMsg frame1 pBMP");
        frameData[3] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "BBMsg framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00405020, CBBMsgGump);

// 0x00405130  size 0xf0
// CBBMsgGump::MouseDown (virtual slot 0x10). Brings the gump to the front and starts a drag; in the
// composer a click inside the text area focuses the gump and puts the caret under the mouse.
void __thiscall CBBMsgGump::MouseDown(int mouseX, int mouseY)
{
    CGump_BringToFront(this, 0);
    dragging = 1;
    unk_6c = mouseX;
    unk_70 = mouseY;
    if (composing == 0)
        return;
    if (mouseX < textLeft || mouseX > textRight || mouseY < textTop || mouseY > textBottom || mouseY < textOrigin)
        return;
    BB_Focus(this);
    int n = lastLine;
    cur = (mouseY - textOrigin) / lineHeight;
    if (cur > n)
        cur = n;
    if (cur < n) {
        caret = BB_CaretFromX(lines[cur], mouseX - textLeft);
        RedrawText();
        return;
    }
    caret = 0;
    RedrawText();
}
HOOK_VMETHOD(0x00405130, CBBMsgGump, MouseDown);

// 0x00405220  size 0x14
// CBBMsgGump::MouseUp (virtual slot 0x14). Ends the drag and clears the global drag gump.
void __thiscall CBBMsgGump::MouseUp(int, int)
{
    dragging = 0;
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x00405220, CBBMsgGump, MouseUp);

// 0x00405240  size 0x2c
// CBBMsgGump::MouseMove. Moves the gump by the mouse delta while dragging.
void __thiscall CBBMsgGump::MouseMove(int mouseX, int mouseY)
{
    if (dragging != 0)
        Move(mouseX - unk_6c, mouseY - unk_70, 1, nullptr, 1);
}
HOOK_VMETHOD(0x00405240, CBBMsgGump, MouseMove);

// 0x00405270  size 0x33
// CBBMsgGump::OnScroll. Scrollbar callback: first visible line = pos * lines / range, clamped.
void __thiscall CBBMsgGump::OnScroll(int pos, int range, int)
{
    int n = lastLine;
    if (n != 0) {
        top = (pos * lastLine) / range;
        if (top >= n)
            top = n - 1;
    }
    RedrawText();
}
HOOK_VMETHOD(0x00405270, CBBMsgGump, OnScroll);

// 0x004052b0  size 0x223
// CBBMsgGump::WrapLine. Breaks line `lineIdx` at the width of the text box; the overflow is prepended to
// the next line (a new last line is added when needed, up to 0x4f). Returns 1 when text moved, and
// follows the caret and the scroll position.
int __thiscall CBBMsgGump::WrapLine(int lineArg)
{
    char tmp[0x51];
    int idx = lineArg & 0xffff;
    int split = BB_FindWrap(lines[idx], textRight - textLeft - 2);
    char *line;

    if (split == -1) {
        line = lines[idx];
        split = static_cast<int>(strlen(line));
    } else {
        split = split + 1;
        line = lines[idx];
        if (line[split] == ' ')
            split = split + 1;
    }
    if (split < static_cast<int>(strlen(line))) {
        int n = lastLine;
        if (idx == n - 1) {
            if (n + 1 >= 0x50) {
                line[split] = '\0';
                g_bbWrapFull = 1;
                return 0;
            }
            lastLine = n + 1;
            lines[lastLine] = static_cast<char *>(::operator new(0x51));
            strcpy(lines[lastLine], g_emptyString);
        }

        strcpy(tmp, lines[idx] + split);
        strcat(tmp, lines[idx + 1]);
        strcpy(lines[idx + 1], tmp);
        lines[idx][split] = '\0';
        if (cur == idx && caret >= split) {
            caret = caret - split;
            if (caret < 0)
                caret = 0;
            int oldTop = top;
            cur = cur + 1;
            if ((cur - oldTop + 1) * lineHeight + textOrigin > textBottom) {
                top = oldTop + 1;
                reinterpret_cast<CScrollThumbGump *>(scrollbar)->SetPosition(top, lastLine);
            }
        }
        return 1;
    }
    return 0;
}
HOOK_METHOD(0x004052b0, CBBMsgGump, WrapLine);

// 0x00405de0  size 0x48d
// CBBMsgGump::Resize. Shifts the children for the new height (children answering virtual slot 0x98 are
// the buttons: their x/y fields at +0x9c/+0xa0/+0xa8 are updated, the others move down when in the
// lower half), reallocates both frame bitmaps and repaints the static art and header texts.
// The first stack argument is not read.
// ? the original frees both bitmaps under the tag "bbmsg frame0 pBMP" (the second looks like a copy-paste).
void __thiscall CBBMsgGump::Resize(int, int newHeight)
{
    int v04 = 0, v08 = 0, v0c = 0, v10 = 0, v14 = 0, v18 = 0;
    int rect[4];

    for (CGump *c = lastChild; c != nullptr; c = c->prev) {
        if (PORT_VSLOT0(c, 0x98, int)() != 0) {
            RAWF(int32_t, c, 0x9c) = (rowStride - width) + c->x;
            RAWF(int32_t, c, 0xa0) = (rowStride - width) + c->x;
            c->x = RAWF(int32_t, c, 0x9c);
            RAWF(int32_t, c, 0xa8) = RAWF(int32_t, c, 0xa8) + (newHeight - height);
        } else {
            if (c->y > height / 2)
                c->y = newHeight - height + c->y;
        }
    }

    if (frameData[1] != 0) {
        gfx::FreeTagged(Bmp(frameData[1]), "bbmsg frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData[3] != 0) {
        gfx::FreeTagged(Bmp(frameData[3]), "bbmsg frame0 pBMP");
        frameData[3] = 0;
    }
    height = newHeight;
    frameData[1] = static_cast<int32_t>(
        reinterpret_cast<uintptr_t>(gfx::AllocTagged(height * width * 2 + 4, "bbmsg frame0")));
    memset(Bmp(frameData[1]) + 2, 0, height * width * 2);
    Bmp(frameData[1])[0] = static_cast<uint16_t>(width);
    Bmp(frameData[1])[1] = static_cast<uint16_t>(height);
    frameData[3] = static_cast<int32_t>(
        reinterpret_cast<uintptr_t>(gfx::AllocTagged(height * width * 2 + 4, "bbmsg frame1")));
    memset(Bmp(frameData[3]) + 2, 0, height * width * 2);
    Bmp(frameData[3])[0] = static_cast<uint16_t>(width);
    Bmp(frameData[3])[1] = static_cast<uint16_t>(height);

    gfx::ArtSize(0x82e, &v18, &v10);
    textBottom = height - v10 - 0x32;
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = textRight;
    rect[3] = height;
    BB_DrawFrame(unk_50, 2, Bmp(frameData[3]), 0, v10);
    if (composing != 0) {
        uint16_t *b = Bmp(frameData[3]);
        gfx::DrawArt(gfx::artPlain(), 0x883, 0x61, 0xc, b + 2, b[0], b[1]);
    }
    {
        uint16_t *b = Bmp(frameData[3]);
        gfx::DrawArt(gfx::artPlain(), 0x82b, 0x1e, 0x22, b + 2, b[0], b[1]);
    }
    {
        uint16_t *b = Bmp(frameData[3]);
        gfx::DrawArt(gfx::artPlain(), 0x82b, 0x1e, height - v10 - 0x30, b + 2, b[0], b[1]);
    }

    gfx::TextMeasure(6, "Author:", &v0c, &v08, &v04);
    gfx::TextDrawAlt(textLeft, v04 + 0x28, 6, 0, "Author:", Bmp(frameData[3]) + 2, width, rect);
    gfx::TextMeasure(9, author, &v14, &v08, &v04);
    gfx::TextDrawAlt(v0c + textLeft + 4, v04 + 0x2e, 9, 0x386, author, Bmp(frameData[3]) + 2, width, rect);
    gfx::TextMeasure(6, "Time:", &v0c, &v08, &v04);
    gfx::TextDrawAlt(textLeft, v04 + 0x38, 6, 0, "Time:", Bmp(frameData[3]) + 2, width, rect);
    gfx::TextMeasure(9, time, &v14, &v08, &v04);
    gfx::TextDrawAlt(v0c + textLeft + 4, v04 + 0x3e, 9, 0x386, time, Bmp(frameData[3]) + 2, width, rect);
    gfx::TextMeasure(6, "Subject:", &v0c, &v08, &v04);
    gfx::TextDrawAlt(textLeft, v04 + 0x48, 6, 0, "Subject:", Bmp(frameData[3]) + 2, width, rect);
    if (composing == 0) {
        gfx::TextMeasure(9, subject, &v14, &v08, &v04);
        gfx::TextDrawAlt(v0c + textLeft + 4, v04 + 0x4e, 9, 0x386, subject, Bmp(frameData[3]) + 2, width, rect);
    }
    BBMsg_TileArt(textLeft, 0x6e, 0x835, Bmp(frameData[3]), &textLeft);
    RedrawText();
}
HOOK_VMETHOD(0x00405de0, CBBMsgGump, Resize);

// 0x00406270  size 0x1e0
// CBBMsgGump::RedrawText. Copies the static frame 1 over frame 0, draws the visible lines (font 9,
// hue 0x386) from the first visible one down to the text-box bottom and, when this gump has the focus,
// a "_" caret (hue 8) on the current line, or on the line after the last one.
void __thiscall CBBMsgGump::RedrawText()
{
    int v08, v0c, v14;
    int rect[4];

    memcpy(Bmp(frameData[1]) + 2, Bmp(frameData[3]) + 2, height * width * 2);
    gfx::TextMeasure(4, "Aq", &v14, &v08, &v0c);
    rect[0] = 0;
    rect[1] = textTop;
    rect[2] = width;
    rect[3] = textBottom;
    gfx::TextMeasure(9, "Aq", &v14, &v08, &v0c);

    int rowY = textOrigin;
    char **p = &lines[top];
    for (int idx = top; idx < lastLine; idx++, p++, rowY += v08) {
        if (v08 + rowY >= textBottom)
            break;
        gfx::TextDrawAlt(textLeft, v0c + rowY, 9, 0x386, *p, Bmp(frameData[1]) + 2, width, &textLeft);
        if (g_focusedInput == this && cur == idx) {
            int w = gfx::TextWidth(9, caret, *p);
            gfx::TextDraw(w + textLeft, v0c + rowY + 4, 9, 8, "_", Bmp(frameData[1]) + 2, width, rect);
        }
    }
    if (g_focusedInput == this && cur == lastLine) {
        gfx::TextDraw(textLeft, v08 * cur + v0c + textOrigin + 4, 9, 8, "_", Bmp(frameData[1]) + 2, width, rect);
    }
}
HOOK_METHOD(0x00406270, CBBMsgGump, RedrawText);

// 0x00406450  size 0xbb
// CBBMsgGump::Draw (virtual slot 0x08). Blits frame 0 at the screen origin when the clip rect intersects
// it, then draws the children along +0x40. Unlike CBBLineGump::Draw it does not test the visible flag.
void __thiscall CBBMsgGump::Draw(int *clip)
{
    int ox = 0;
    int oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);

    int rect[4];
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = ox + gfx::bitmapOf(this)[0];
    rect[3] = gfx::bitmapOf(this)[1] + oy;
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, void *, int *)(clip, rect) == 0)
        return;
    uint16_t *bmp = gfx::bitmapOf(this);
    GFX_BlitFrame(gfx::artPlain(), bmp + 2, clip, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
    for (CGump *c = firstChild; c != nullptr; c = c->next)
        PORT_VSLOT1(c, kGumpSlot_Draw, void, void *)(clip);
}
HOOK_VMETHOD(0x00406450, CBBMsgGump, Draw);

// 0x00406540  size 0x6a
// CBBMsgGump::AddLine. Appends a malloc'd copy of `text` as lines[lastLine - 1] (after incrementing lastLine).
void __thiscall CBBMsgGump::AddLine(const char *text)
{
    lastLine = lastLine + 1;
    char *copy = static_cast<char *>(::operator new(strlen(text) + 1));
    lines[lastLine - 1] = copy;
    strcpy(lines[lastLine - 1], text);
}
HOOK_METHOD(0x00406540, CBBMsgGump, AddLine);

// 0x004065b0  size 0x88
// CObj0049c690 constructor (RET 0x14): base ctor 0x0042ad20, then links the object at the head of
// owner's list (owner+0xb0) and inherits owner+0x04 / +0x0c / +0x28 / +0x30.
CObj0049c690::CObj0049c690(int16_t w20, int16_t w22, int p40, int p44, CObj0049c690 *ownerObj)
{
    x = w20;
    y = w22;
    top40 = p40;
    bottom44 = p44;
    owner = ownerObj;
    CObj0049c690 *head = RAWF(CObj0049c690 *, ownerObj, 0xb0);
    prev = head;
    if (head != nullptr)
        head->next = this;
    next = nullptr;
    RAWF(CObj0049c690 *, owner, 0xb0) = this;
    RAWF(int32_t, this, 0x08) = p40;
    RAWF(int32_t, this, 0x04) = RAWF(int32_t, owner, 0x04);
    RAWF(int32_t, this, 0x10) = p44;
    RAWF(int32_t, this, 0x0c) = RAWF(int32_t, owner, 0x0c);
    RAWF(int32_t, this, 0x28) = RAWF(int32_t, owner, 0x28);
    RAWF(int32_t, this, 0x2c) = x + y;
    RAWF(int32_t, this, 0x30) = RAWF(int32_t, owner, 0x30);
}
HOOK_CTOR(0x004065b0, CObj0049c690, (int16_t, int16_t, int, int, CObj0049c690 *));

// 0x00406650  size 0xb
// Returns the 16-bit value at +0x24. Shared virtual slot; RET 4 for an argument it ignores.
uint16_t __thiscall CWorldObj::GetHeightAt(int)
{
    return RAWF(uint16_t, this, 0x24);
}
HOOK_VMETHOD(0x00406650, CWorldObj, GetHeightAt);

// 0x00406720  size 1
// Empty virtual slot 0x0c.
void __thiscall CWorldObj::ToVoid()
{
}
HOOK_VMETHOD(0x00406720, CWorldObj, ToVoid);

// Slot 0x0c of the 0x0049c690 class: the same empty body (the linker folds the two).
void __thiscall CObj0049c690::ComputeScreenRect()
{
}

// The empty procedure as a plain callback / destructor stub (a pointer to the virtual member would be its call thunk).
void __cdecl Nop_00406720()
{
}

// 0x00406730  size 0x68
// CObj0049c690::DrawClipped (virtual slot 0x08). Clips the rect's y range to [+0x40, +0x44] less the
// scroll offset, then draws through the owner's slot 0x08 with the busy flag 0x004ba30c raised.
void __thiscall CObj0049c690::Draw(int *rect)
{
    int r[4];

    r[0] = rect[0];
    r[1] = rect[1];
    r[2] = rect[2];
    r[3] = rect[3];
    int lo = top40 - g_scrollY;
    if (r[1] <= lo)
        r[1] = lo;
    int hi = bottom44 - g_scrollY;
    if (r[3] >= hi)
        r[3] = hi;
    g_clipBusyBB = 1;
    PORT_VSLOT1(owner, 0x08, void, int *)(r);
    g_clipBusyBB = 0;
}
HOOK_VMETHOD(0x00406730, CObj0049c690, Draw);

// 0x00406a80  size 0x5e
// StripParenSuffix (ECX = text). Copies the text up to the first '(' into the buffer at 0x004b70e0 and
// returns it. The terminator is written at index count - 1, so the last copied character (the space
// before the '(' in "Name (n)") is dropped; with no '(' the last character is dropped, and an empty or
// null text writes the byte before the buffer. Reproduced as is.
const char *__fastcall StripParenSuffix(const char *text)
{
    int n = 0;

    if (text != nullptr) {
        for (const char *p = text; n < static_cast<int>(strlen(text)); n++, p++) {
            if (*p == '(')
                break;
            g_parenBuf[n] = *p;
        }
    }
    g_parenBuf[n - 1] = '\0';
    return g_parenBuf;
}
HOOK_FUNC(0x00406a80, StripParenSuffix);
