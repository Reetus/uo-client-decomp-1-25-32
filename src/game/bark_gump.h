// CBarkGump: the floating speech text over a mobile or the player. vtable
// 0x0049c000, gump name "bark gump". Derived fields start at +0x94; the base
// CGump fields are in gump.h. Barks of one speaker form a list through
// older/newer (+0xac/+0xb0); top-level barks hang off the list head at
// 0x006488cc. Layout from decomp/pass2/0x004012a0.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CBarkGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    virtual int IsTag84() { return 1; }
    virtual void RenderText(int a, int *b);
    // 0x004012a0  size 0x6ae  constructor (speaker, text, hue, font, param5), RET 0x14: builds the bark bitmap,
    // positions it above `speaker` and links it into the speaker's bark list. `speaker` is a CObj (null for a
    // system message), `param5` is stored at +0xb4 unchanged. Stack args, RET 0x14.
    CBarkGump(CObj *speaker, char *text, uint16_t hue, uint16_t font, CGump *param5);
protected:
    // CUnicodeBarkGump's constructor (0x00402850) does not call 0x004012a0: it builds only the CGump base itself.
    CBarkGump() {}
public:
    // 0x00401950  destructor (the out-of-line body; the base destructor follows implicitly).
    ~CBarkGump();
    // Batch 00 second pass, bark_gump_ui.cpp.
    void Dtor() { this->CBarkGump::~CBarkGump(); }                 // 0x00401950  the destructor body: unlink from the bark lists, free bitmap and text
    virtual void MouseOver(int x, int y);                // 0x00401b40  bring to front, send slot 0x114 event 0x35 (both stack args unused), RET 8
    virtual void MouseLeave();                       // 0x00401ba0  send slot 0x114 event `hue`
    virtual void Tick();                // 0x00401c40  double-click window expired: single click
    virtual void MouseDown(int x, int y);      // 0x00401ca0  RET 8
    virtual void MouseUp(int x, int y);            // 0x00401cf0  RET 8 (args unused)
    void __thiscall SingleClick(int, int);              // 0x00401d70  target callback on the speaker, RET 8 (args unused)
    virtual void Draw(int *clip);                    // 0x00401e60  blit to the back buffer, RET 4
    void __thiscall Reposition();                       // 0x00402090  re-anchor above the speaker, queue the dirty rect
    void __thiscall StackAndClamp();                    // 0x00402230  keep the speaker's stacked barks on screen
    int __thiscall SpeakerIsSpecial();                  // 0x0043a1e0  speaker set and its slot 0x24 or 0x54 answers true

    int32_t tickStart;   // +0x94  GetTickCount() at creation
    int32_t duration;    // +0x98  display time in ms
    int32_t unk_9c;      // +0x9c  placement scratch (x)
    int32_t unk_a0;      // +0xa0  placement scratch (y)
    int32_t homeX;       // +0xa4  x before clamping to the screen
    int32_t homeY;       // +0xa8  y before clamping; older barks are shifted up when a newer one is added
    CBarkGump *older;    // +0xac  next older bark of the same speaker
    CBarkGump *newer;    // +0xb0
    CGump *param5;       // +0xb4
    CObj *speaker;       // +0xb8  null for top-level barks
    int32_t unk_bc;      // +0xbc
    int32_t unk_c0;      // +0xc0
    int32_t unk_c4;      // +0xc4
    int32_t unk_c8;      // +0xc8
    int32_t unk_cc;      // +0xcc
    int32_t unk_d0;      // +0xd0
    char *text;          // +0xd4  malloc'd copy of the spoken text
    uint16_t hue;        // +0xd8
    uint16_t font;       // +0xda
};

static_assert(offsetof(CBarkGump, tickStart) == 0x94, "CBarkGump::tickStart");
static_assert(offsetof(CBarkGump, duration) == 0x98, "CBarkGump::duration");
static_assert(offsetof(CBarkGump, unk_9c) == 0x9c, "CBarkGump::unk_9c");
static_assert(offsetof(CBarkGump, unk_a0) == 0xa0, "CBarkGump::unk_a0");
static_assert(offsetof(CBarkGump, homeX) == 0xa4, "CBarkGump::homeX");
static_assert(offsetof(CBarkGump, homeY) == 0xa8, "CBarkGump::homeY");
static_assert(offsetof(CBarkGump, older) == 0xac, "CBarkGump::older");
static_assert(offsetof(CBarkGump, newer) == 0xb0, "CBarkGump::newer");
static_assert(offsetof(CBarkGump, param5) == 0xb4, "CBarkGump::param5");
static_assert(offsetof(CBarkGump, speaker) == 0xb8, "CBarkGump::speaker");
static_assert(offsetof(CBarkGump, unk_bc) == 0xbc, "CBarkGump::unk_bc");
static_assert(offsetof(CBarkGump, unk_d0) == 0xd0, "CBarkGump::unk_d0");
static_assert(offsetof(CBarkGump, text) == 0xd4, "CBarkGump::text");
static_assert(offsetof(CBarkGump, hue) == 0xd8, "CBarkGump::hue");
static_assert(offsetof(CBarkGump, font) == 0xda, "CBarkGump::font");
static_assert(sizeof(CBarkGump) == 0xdc, "CBarkGump size (operator new(0xdc))");
