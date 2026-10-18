// CBBGump: bulletin board window. ctor 0x00402f00, vtable 0x0049c348.
// Layout is partial: only the base CGump fields are used by the ported
// methods (owner at +0x34 is the board object).
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CBBGump : public CGump {
public:
    virtual int IsTagdc() { return 1; }
    // Gump prep P5: own copy of a slot body the original shares between gumps.
    virtual void Draw(int *clip);
    // Batch u1 (small_methods_u1.cpp).
    // 0x00410870: "new post" button callback: OnPostNew(0, "").
    void __thiscall OnNewPostButton();
    // 0x00403920: open a CBBMsgGump to compose a new post on this board.
    // `parentSerial` is the serial of the message being replied to (stack arg 1),
    // `subject` the subject line prefilled in the editor. RET 8.
    void __thiscall OnPostNew(uint32_t parentSerial, char *subject);
    // 0x00410960  scroll the list by one visible line towards the previous line (virtual slot 0xe0 marks line gumps)
    void __thiscall ScrollUp();
    // 0x00410a50  scroll the list by one visible line towards the next line
    void __thiscall ScrollDown();
    // Batch 00 second pass, bb_gump.cpp.
    // 0x00402f00: RET 8. `board` is the board object (stored at +0x34), `title` the board name drawn on the frame.
    CBBGump(CObj *board, const char *title);
    ~CBBGump();                                                       // 0x004032b0
    void Dtor() { this->CBBGump::~CBBGump(); }
    virtual void MouseDown(int x, int y);          // 0x00403360  RET 8: raise, start dragging
    virtual void MouseUp(int x, int y);                          // 0x00403390  RET 8: stop dragging
    virtual void MouseMove(int x, int y);          // 0x004033b0  RET 8: drag
    virtual void OnScroll(int a, int b, int c);                // 0x004033e0  RET 0xc (third arg not read)
    virtual int HitTest(int x, int y);                      // 0x004035a0  RET 8 (vtable slot 0x0c): frame pixel is non-zero
    // 0x004035e0: RET 0x14. Add a line gump for `post` (child of `parentPost` when that is non-null) and place it.
    void __thiscall AddLine(CObj *post, CObj *parentPost, const char *s1, const char *s2, const char *s3);
    void __thiscall RemoveLine(CGump *line);                          // 0x00403a10  RET 4

    int32_t textLeft;      // +0x94  0x6d: the list area inside the frame
    int32_t textTop;       // +0x98  0x9f
    int32_t textRight;     // +0x9c  0x162
    int32_t textBottom;    // +0xa0  0x144
    int32_t dragging;      // +0xa4  1 while the window is being dragged
    int32_t closing;       // +0xa8  set by the destructor; RemoveLine does nothing then
    CGump *topLine;        // +0xac  first visible line gump
    CGump *button_b0;      // +0xb0  rect button created by the constructor
    CGump *button_b4;      // +0xb4  rect button created by the constructor
    CGump *scrollThumb;    // +0xb8  scrollbar thumb
    int32_t lineCount;     // +0xbc  number of line gumps
    virtual void RButtonUp(int x, int y);   // slot RButtonUp, copy of CCourseGump::CloseAndDelete
};

static_assert(offsetof(CBBGump, textLeft) == 0x94, "CBBGump::textLeft");
static_assert(offsetof(CBBGump, textTop) == 0x98, "CBBGump::textTop");
static_assert(offsetof(CBBGump, textRight) == 0x9c, "CBBGump::textRight");
static_assert(offsetof(CBBGump, textBottom) == 0xa0, "CBBGump::textBottom");
static_assert(offsetof(CBBGump, dragging) == 0xa4, "CBBGump::dragging");
static_assert(offsetof(CBBGump, closing) == 0xa8, "CBBGump::closing");
static_assert(offsetof(CBBGump, topLine) == 0xac, "CBBGump::topLine");
static_assert(offsetof(CBBGump, button_b0) == 0xb0, "CBBGump::button_b0");
static_assert(offsetof(CBBGump, button_b4) == 0xb4, "CBBGump::button_b4");
static_assert(offsetof(CBBGump, scrollThumb) == 0xb8, "CBBGump::scrollThumb");
static_assert(offsetof(CBBGump, lineCount) == 0xbc, "CBBGump::lineCount");
static_assert(sizeof(CBBGump) == 0xc0, "CBBGump size (operator new(0xc0))");
