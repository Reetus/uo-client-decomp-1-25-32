// CBBMsgGump: bulletin board message viewer/composer. ctor 0x00404610,
// vtable 0x0049c578, allocated with operator new(0x2b4). Layout from
// decomp/pass2/0x004054e0.cpp: the text is an array of up to 0x50 malloc'd
// 0x51-byte lines at +0x174.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CBBMsgGump : public CGump {
public:
    virtual int IsTage4() { return 1; }
    // 0x004054e0: keyboard input for the composer (Home/End/arrows/Backspace/Enter/characters).
    virtual void OnKey(int key);
    // 0x00410880  ? Reply: open a composer on the board for "RE: <subject at +0xde>" (class inferred; fields +0xbc / +0xde are read raw)
    void __thiscall OnReply();
    // 0x00410b40  scroll the text up by one line
    void __thiscall ScrollUp();
    // 0x00410b80  scroll the text down by one line
    void __thiscall ScrollDown();

    // 0x00404610: build the viewer / composer. Stack args, RET 0x28. `bodyId`, `hue`,
    // `nEquip` and `equip` are pushed by the caller but never read by the constructor.
    CBBMsgGump(CObj *board, uint32_t postSerial, char *authorText, char *subjectText, char *timeText,
               uint16_t bodyId, uint16_t hue, uint8_t nEquip, uint16_t *equip, int composing);
    // 0x00404ff0: virtual slot 0.
    // 0x00405020: ~CBBMsgGump (the base CGump dtor runs last).
    ~CBBMsgGump();
    void Dtor() { this->CBBMsgGump::~CBBMsgGump(); }
    // 0x00405130 (virtual slot 0x10): start a drag; in the composer a click in the text area moves the caret.
    virtual void MouseDown(int x, int y);
    // 0x00405220 (virtual slot 0x14): end the drag. Both stack arguments are ignored.
    virtual void MouseUp(int x, int y);
    // 0x00405240: while dragging, move the gump by the mouse delta.
    virtual void MouseMove(int x, int y);
    // 0x00405270: scrollbar callback; the third stack argument is ignored.
    virtual void OnScroll(int a, int b, int c);
    // 0x004052b0: word-wrap line `lineIdx` (low 16 bits) of the composer. Returns 1 if text moved to the next line.
    int __thiscall WrapLine(int lineIdx);
    // 0x00405de0: resize to `newHeight` (the first stack argument is ignored) and repaint both frames.
    virtual void Resize(int w, int h);
    // 0x00406270: copy frame 1 over frame 0 and draw the visible text lines and the caret.
    void __thiscall RedrawText();
    // 0x00406450 (virtual slot 0x08): blit frame 0 to the back buffer when the clip intersects, then draw the children.
    virtual void Draw(int *clip);
    // 0x00406540: append a malloc'd copy of `text` as lines[lastLine - 1] after incrementing lastLine.
    void __thiscall AddLine(const char *text);

    CGump *subjectEdit;   // +0x94  edit control for the subject (composer only)
    CGump *scrollbar;     // +0x98  scrollbar child (thiscall 0x00413c00 updates it)
    CGump *btnReply;      // +0x9c  Post / Reply button
    CGump *btnDelete;     // +0xa0  Delete button, only for the player's own unanswered post
    int32_t textLeft;     // +0xa4
    int32_t textTop;      // +0xa8
    int32_t textRight;    // +0xac
    int32_t textBottom;   // +0xb0
    int32_t dragging;     // +0xb4
    int32_t composing;    // +0xb8  non-zero: new post being written
    int32_t postSerial;   // +0xbc
    char author[0x1e];    // +0xc0
    char subject[0x50];   // +0xde
    char time[0x2a];      // +0x12e
    int32_t lastLine;     // +0x158  index of the virtual last line; lines[lastLine] exists
    int32_t top;          // +0x15c  first visible line
    int32_t cur;          // +0x160  line holding the caret
    int32_t caret;        // +0x164  column
    int32_t unk_168;
    int32_t textOrigin;   // +0x16c
    int32_t lineHeight;   // +0x170
    char *lines[0x50];    // +0x174
    virtual void RButtonUp(int x, int y);   // slot RButtonUp, copy of CCourseGump::CloseAndDelete
    virtual int HitTest(int x, int y);   // slot HitTest, copy of CGump::ContainsLocalPoint
};

static_assert(offsetof(CBBMsgGump, subjectEdit) == 0x94, "CBBMsgGump::subjectEdit");
static_assert(offsetof(CBBMsgGump, btnReply) == 0x9c, "CBBMsgGump::btnReply");
static_assert(offsetof(CBBMsgGump, btnDelete) == 0xa0, "CBBMsgGump::btnDelete");
static_assert(offsetof(CBBMsgGump, dragging) == 0xb4, "CBBMsgGump::dragging");
static_assert(offsetof(CBBMsgGump, composing) == 0xb8, "CBBMsgGump::composing");
static_assert(offsetof(CBBMsgGump, postSerial) == 0xbc, "CBBMsgGump::postSerial");
static_assert(offsetof(CBBMsgGump, author) == 0xc0, "CBBMsgGump::author");
static_assert(offsetof(CBBMsgGump, subject) == 0xde, "CBBMsgGump::subject");
static_assert(offsetof(CBBMsgGump, time) == 0x12e, "CBBMsgGump::time");
static_assert(offsetof(CBBMsgGump, scrollbar) == 0x98, "CBBMsgGump::scrollbar");
static_assert(offsetof(CBBMsgGump, textLeft) == 0xa4, "CBBMsgGump::textLeft");
static_assert(offsetof(CBBMsgGump, textTop) == 0xa8, "CBBMsgGump::textTop");
static_assert(offsetof(CBBMsgGump, textRight) == 0xac, "CBBMsgGump::textRight");
static_assert(offsetof(CBBMsgGump, textBottom) == 0xb0, "CBBMsgGump::textBottom");
static_assert(offsetof(CBBMsgGump, lastLine) == 0x158, "CBBMsgGump::lastLine");
static_assert(offsetof(CBBMsgGump, top) == 0x15c, "CBBMsgGump::top");
static_assert(offsetof(CBBMsgGump, cur) == 0x160, "CBBMsgGump::cur");
static_assert(offsetof(CBBMsgGump, caret) == 0x164, "CBBMsgGump::caret");
static_assert(offsetof(CBBMsgGump, textOrigin) == 0x16c, "CBBMsgGump::textOrigin");
static_assert(offsetof(CBBMsgGump, lineHeight) == 0x170, "CBBMsgGump::lineHeight");
static_assert(offsetof(CBBMsgGump, lines) == 0x174, "CBBMsgGump::lines");
static_assert(sizeof(CBBMsgGump) == 0x2b4, "CBBMsgGump size (operator new(0x2b4))");
