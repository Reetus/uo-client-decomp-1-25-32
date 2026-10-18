// CBookGump: readable/editable book window. vtable 0x0049c728, gump name
// "book gump". Derived fields start at +0x94. Layout from
// decomp/pass2/0x00406c20.cpp, 0x004071a0.cpp, 0x00407ad0.cpp, 0x00408ca0.cpp.
//
// Page storage: `pages` is an array of 0x648-byte page records, record n at
// pages + n*0x648: u16 line count at +0, up to 20 lines of 0x50 bytes at +2,
// a dword "dirty" flag at +0x644. Page numbers are 1-based, so the record of
// page N is pages + (N-1)*0x648. The left page shown is `page`, the right page
// is page + 1; bufLeft / bufRight point at the records currently displayed.
// The allocation size of the object is not established (CSpellBookGump
// derives from it), so there is no sizeof assert.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"
#include "gump_art_ops_ports.h"
#include "batch02_helpers.h"
#include "sound_manager.h"
#include "map_world_ops_ports.h"
#include "../runtime/ported.h"
#include "gump_art_ports.h"
#include "art_row_blits_ports.h"
#include "art_row_blits_hue_ports.h"
#include "globals.h"

class CBookGump : public CGump {
public:
    virtual int IsTagcc() { return 1; }
    virtual void RedrawSpread();
    virtual void NextPage();
    virtual void PrevPage();
    virtual void OnClickCorner(int x, int y);
    virtual void OnCornerJump(int x, int y);
    // 0x00406c20: build the book. Stack args, RET 0x20.
    CBookGump(int posX, int posY, int artId, CObj *ownerObj, char *titleText, char *authorText,
              uint16_t pageTotal, int isEditable);
    // 0x004071a0: ~CBookGump (the base CGump dtor runs last).
    ~CBookGump();
    void Dtor() { this->CBookGump::~CBookGump(); }
    // 0x00407ad0: word-wrap one line of a page; push overflow to the next line or next page.
    // Returns 1 if text moved on (the caller re-wraps the next line).
    int __thiscall WrapLine(uint16_t pageNo, uint32_t lineNo, int *outWrapped);
    // 0x00408ca0: key handler (virtual slot 0x3c; Delete calls itself through that slot).
    virtual void OnKey(int key);
    // 0x00406ae0: default constructor (no arguments, plain RET): base ctor, vtable, empty title / author.
    CBookGump();
    // 0x00406bf0: virtual slot 0.
    // 0x00407280: virtual slot 0x118. Commit edits, turn the spread forward by two pages and redraw.
    // 0x00407410: turn the spread back by two pages and redraw.
    // 0x004075f0: send the changed title / author and every dirty page record to the server (editable books only).
    void __thiscall CommitEdits();
    // 0x00407730: page `pageNo` has too many lines; move the last one to the first line of the next page
    // (returns 1) or drop it on the last page (returns 0).
    int __thiscall OverflowPage(int pageNo);
    // 0x004078d0: delete line `lineNo` of page `pageNo`, pulling the next page's first line up when the page
    // was full. Returns non-zero when a line was pulled from the next page.
    int __thiscall RemoveLine(int pageNo, int lineNo);
    // 0x00407f60: virtual slot 0x114. Draw the page art, cover labels, both pages' text, carets and page numbers.
    // 0x00408510 (slot 0x10): mouse down; focus the page under the cursor and place the caret.
    virtual void MouseDown(int x, int y);
    // 0x004087c0 (slot 0x14): mouse up; dispatches single (slot 0x120) and double (slot 0x124) clicks.
    virtual void MouseUp(int x, int y);
    // 0x00408860 (slot 0x28): mouse move; drags the window while the button is held.
    virtual void MouseMove(int x, int y);
    // 0x00408890 (slot 0x120): single click on a page corner (previous / next page via slots 0x11c / 0x118).
    // 0x00408920 (slot 0x124): double click on a corner: jump to the first / last spread.
    // 0x0040a280 (slot 0x44): mark the header (title / author) as changed.
    virtual void OnChildChanged(int child);
    // 0x0040a2a0 (slot 0x2c): per-tick; turns a lone click into a single click after the double-click time.
    virtual void Tick();
    // 0x0040a300 (slot 0x1c): clear the drag gump and delete the gump through its deleting destructor.
    virtual void RButtonUp(int x, int y);
    // 0x0040a330 (slot 0x08): blit frame 0 (or frame 1 when unk_60) at the screen origin, then draw the children.
    virtual void Draw(int *clip);
    // 0x0040d390: switch to the small (unk_60) frame at a cascade slot, remembering the position in +0x7c / +0x80.
    void __thiscall Iconify();
    // 0x0040d410: back from the small frame: restore the position and release the cascade slot.
    void __thiscall Uniconify();

    char *bufLeft;       // +0x94  record of the left page (page number `page`)
    char *bufRight;      // +0x98  record of the right page (page + 1)
    char *pageRecords;   // +0x9c
    int32_t editable;    // +0xa0
    int32_t unk_a4;      // +0xa4
    uint16_t page;       // +0xa8  left page number
    uint16_t pageCount;  // +0xaa
    char title[0x3c];    // +0xac
    char author[0x20];   // +0xe8
    CGump *titleCtl;     // +0x108  edit control, text at ctl+0xec
    CGump *authorCtl;    // +0x10c
    int32_t cursorOnLeft; // +0x110  1 = caret on the left page (bufLeft), 0 = right page
    int32_t curLine;     // +0x114
    int32_t curCol;      // +0x118
    int32_t unk_11c;
    int32_t unk_120;
    int32_t unk_124;
    int32_t unk_128;
    int32_t unk_12c;
    int32_t unk_130;
};

static_assert(offsetof(CBookGump, bufLeft) == 0x94, "CBookGump::bufLeft");
static_assert(offsetof(CBookGump, bufRight) == 0x98, "CBookGump::bufRight");
static_assert(offsetof(CBookGump, pageRecords) == 0x9c, "CBookGump::pageRecords");
static_assert(offsetof(CBookGump, editable) == 0xa0, "CBookGump::editable");
static_assert(offsetof(CBookGump, unk_a4) == 0xa4, "CBookGump::unk_a4");
static_assert(offsetof(CBookGump, page) == 0xa8, "CBookGump::page");
static_assert(offsetof(CBookGump, pageCount) == 0xaa, "CBookGump::pageCount");
static_assert(offsetof(CBookGump, title) == 0xac, "CBookGump::title");
static_assert(offsetof(CBookGump, author) == 0xe8, "CBookGump::author");
static_assert(offsetof(CBookGump, titleCtl) == 0x108, "CBookGump::titleCtl");
static_assert(offsetof(CBookGump, authorCtl) == 0x10c, "CBookGump::authorCtl");
static_assert(offsetof(CBookGump, cursorOnLeft) == 0x110, "CBookGump::cursorOnLeft");
static_assert(offsetof(CBookGump, curLine) == 0x114, "CBookGump::curLine");
static_assert(offsetof(CBookGump, curCol) == 0x118, "CBookGump::curCol");
static_assert(offsetof(CBookGump, unk_11c) == 0x11c, "CBookGump::unk_11c");
static_assert(offsetof(CBookGump, unk_130) == 0x130, "CBookGump::unk_130");
static_assert(sizeof(CBookGump) == 0x134, "CBookGump size (operator new(0x134))");

// Text rectangles of the two book pages (ints in the original image):
// left {0x26,0x20,0xc2,0xd2}, right {0xe0,0x20,0x17c,0xd2} = {x, y, right, bottom}.

// Original helpers shared by CBookGump and CSpellBookGump (all still run from the image).
// Gump art objects passed in ECX: 0x004764c0 plain, 0x004761e0 spell icons.
#define BOOK_ART_PLAIN PTR_AT(void, PORT_ADDR(&RowBlitCopyNonZero))
#define BOOK_ART_ICON PTR_AT(void, PORT_ADDR(&RowBlitCopyAll))
#define BOOK_FONT_OBJ GLOBAL_PTR(void, g_fontMgr_storage)
#define BOOK_MUSIC_OBJ GLOBAL_PTR(void, g_soundManager_storage)
// 0x00433e70: ECX = art id, EDX = &w, stack = &h.
#define BOOK_ArtSize(a1, a2, a3) (GumpArtSizeImpl(a1, a2, a3))
// 0x00433ed0: ECX = art object, EDX = art id, stack = (x, y, pixels, w, h).
#define BOOK_DrawArt PORT_FUNC7(&DrawGumpArt, void, __fastcall, void *, int, int, int, void *, int, int)
// 0x00434060: ECX = art object, EDX = art id, stack = (clip rect, x, y, pixels, w, h).
#define BOOK_DrawArtClipped PORT_FUNC8(&DrawGumpArtClipped, void, __fastcall, void *, int, int *, int, int, void *, int, int)
// 0x004800f0: thiscall on the music object (sound/music trigger 0x55).
#define BOOK_Music(self, a1, a2, a3, a4, a5, a6) (reinterpret_cast<CSoundManager *>(self)->PlaySound(a1, (long)(a2), (long)(a3), a4, a5, a6))
// 0x00446310: thiscall on the frame allocator, (ptr, name).
#define BOOK_FrameFree(o, a1, a2) PORT_THISCALL2(&Map_Free, void, (o), void *, const char *)(a1, a2)
