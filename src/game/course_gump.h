// CCourseGump: the cartography map window ("Course gump", vtable 0x0049e6d8): art 0x139d,
// pins plotted over the multimap.rle density image. (Not the world-view "map gump" of
// map_gump.h.) Layout partial; see decomp/pass2/0x00427dd0.cpp and 0x00428a90.cpp.
// The owner (CGump::owner, +0x34) is the map item object: pin list head at +0xa8,
// pin count at +0xac, serial at +0x78.
#pragma once

#include <cstddef>
#include <cstdint>
#include <new>

#include "gump.h"

// One plotted pin of a map item pin list.
struct MapPin {
    int16_t x;          // +0x00
    int16_t y;          // +0x02
    uint8_t pad_04[4];
    MapPin *next;       // +0x08
};

static_assert(offsetof(MapPin, next) == 0x08, "MapPin::next");

class CCourseGump : public CGump {
public:
    virtual int IsTagb4() { return 1; }
    // 0x00427dd0: click at (x, y): add a pin on the nearest plotted segment (within 10),
    // else a new end point. Sends the add-pin packet, updates the owner, redraws.
    void __thiscall AddPin(int x, int y);
    // 0x00428a90: redraw the frame bitmap: background art or density map, pin lines,
    // pin sprites with numbers, cursor sprite, corner overlay.
    void __thiscall Redraw();
    // 0x0040e490  send the clear-pins packet (0x56, action 5) for the owner map item, clear it, redraw
    void __thiscall ClearPins();

    // 0x00427620: RET 0x28. (x, y, art id, map item object, then six region / size words: regionX0, regionY0,
    // regionX1, regionY1, mapWidth, mapHeight, each masked with 0xffff)
    CCourseGump(int px, int py, int artId, void *mapItem, uint32_t rx0, uint32_t ry0, uint32_t rx1,
                uint32_t ry1, uint32_t mw, uint32_t mh);
    // 0x00427a10: vtable slot 0, RET 4.
    // 0x00427a40: destructor (the CGump base destructor follows).
    ~CCourseGump();
    void Dtor() { this->CCourseGump::~CCourseGump(); }
    // 0x00427b90: vtable slot 0x1c, RET 8 (x, y not read): clear the drag gump pointer and delete this gump.
    virtual void RButtonUp(int x, int y);
    // 0x00427bb0: vtable slot 0x10, RET 8.
    virtual void MouseDown(int x, int y);
    // 0x00427c80: vtable slot 0x14, RET 8.
    virtual void MouseUp(int x, int y);
    // 0x00427fd0: RET 8. Start moving the pin under the mouse: unlink and free it, remember its neighbours.
    void __thiscall StartPinDrag(int px, int py);
    // 0x004283c0: vtable slot 0x20, RET 8.
    virtual void MouseOver(int x, int y);
    // 0x00428670: vtable slot 0x28, RET 8.
    virtual void MouseMove(int x, int y);
    // 0x00428a20: vtable slot 0x2c, plain RET.
    virtual void Tick();
    // 0x00429050: vtable slot 0x08, RET 4 (clip rect).
    virtual void Draw(int *clip);
    // 0x00429130: vtable slot 0x0c, RET 8.
    virtual int HitTest(int x, int y);

    int32_t pinDragging;        // +0x94  1 once a pin drag has started
    int32_t unk_98;
    int32_t mouseDown;          // +0x9c
    int32_t unk_a0;             // +0xa0  cleared by the button-up handler
    uint32_t clickTick;         // +0xa4  GetTickCount at the first press
    uint32_t clickState;        // +0xa8  1 after the first press, 2 after the second
    uint8_t unk_ac;             // +0xac
    uint8_t pad_ad[3];
    MapPin *dragNext;           // +0xb0  successor of the pin being dragged
    MapPin *dragPin;            // +0xb4  pin under the mouse at the press / being dragged
    MapPin *selectedPin;        // +0xb8  pin being moved; its outgoing line is not drawn
    CGump *unk_bc;              // +0xbc  gump deleted in the destructor
    int32_t unk_c0;             // +0xc0  cleared by Redraw
    int32_t pinSelected;        // +0xc4  nonzero: selectedPin is skipped when drawing lines
    int32_t cursorActive;       // +0xc8  nonzero: draw the cursor pin sprite
    int32_t plotEnabled;        // +0xcc  "plotting enabled" flag
    int16_t cursorX;            // +0xd0
    int16_t cursorY;            // +0xd2
    uint8_t pad_d4[4];
    int32_t lastX;              // +0xd8  last drag position
    int32_t lastY;              // +0xdc
    uint8_t segCount;           // +0xe0  number of pin points (0..2) around the dragged pin
    uint8_t pad_e1[3];
    struct PinPoint {
        PinPoint() {}
        ~PinPoint() {}
        int16_t x;
        int16_t y;
        uint8_t pad_04[8];
    } pts[2];                   // +0xe4  neighbours of the dragged pin
    int32_t ptNum[2];           // +0xfc  their 1-based pin numbers
    uint16_t *backupFrame;      // +0x104  copy of frame 0 without the dragged pin's lines
    CGump *clearButton;         // +0x108
    CGump *plotButton;          // +0x10c
    int32_t regionX0;           // +0x110  map region the image shows, in 0x1400 x 0x1000 units
    int32_t regionY0;           // +0x114
    int32_t regionX1;           // +0x118
    int32_t regionY1;           // +0x11c
    int32_t mapWidth;           // +0x120  density image size
    int32_t mapHeight;          // +0x124
};

static_assert(offsetof(CCourseGump, selectedPin) == 0xb8, "CCourseGump::selectedPin");
static_assert(offsetof(CCourseGump, unk_c0) == 0xc0, "CCourseGump::unk_c0");
static_assert(offsetof(CCourseGump, pinSelected) == 0xc4, "CCourseGump::pinSelected");
static_assert(offsetof(CCourseGump, cursorActive) == 0xc8, "CCourseGump::cursorActive");
static_assert(offsetof(CCourseGump, plotEnabled) == 0xcc, "CCourseGump::plotEnabled");
static_assert(offsetof(CCourseGump, cursorX) == 0xd0, "CCourseGump::cursorX");
static_assert(offsetof(CCourseGump, cursorY) == 0xd2, "CCourseGump::cursorY");
static_assert(offsetof(CCourseGump, regionX0) == 0x110, "CCourseGump::regionX0");
static_assert(offsetof(CCourseGump, regionY1) == 0x11c, "CCourseGump::regionY1");
static_assert(offsetof(CCourseGump, mapWidth) == 0x120, "CCourseGump::mapWidth");
static_assert(offsetof(CCourseGump, mapHeight) == 0x124, "CCourseGump::mapHeight");
static_assert(offsetof(CCourseGump, pinDragging) == 0x94, "CCourseGump::pinDragging");
static_assert(offsetof(CCourseGump, mouseDown) == 0x9c, "CCourseGump::mouseDown");
static_assert(offsetof(CCourseGump, clickTick) == 0xa4, "CCourseGump::clickTick");
static_assert(offsetof(CCourseGump, clickState) == 0xa8, "CCourseGump::clickState");
static_assert(offsetof(CCourseGump, unk_ac) == 0xac, "CCourseGump::unk_ac");
static_assert(offsetof(CCourseGump, dragNext) == 0xb0, "CCourseGump::dragNext");
static_assert(offsetof(CCourseGump, dragPin) == 0xb4, "CCourseGump::dragPin");
static_assert(offsetof(CCourseGump, unk_bc) == 0xbc, "CCourseGump::unk_bc");
static_assert(offsetof(CCourseGump, lastX) == 0xd8, "CCourseGump::lastX");
static_assert(offsetof(CCourseGump, segCount) == 0xe0, "CCourseGump::segCount");
static_assert(offsetof(CCourseGump, pts) == 0xe4, "CCourseGump::pts");
static_assert(offsetof(CCourseGump, ptNum) == 0xfc, "CCourseGump::ptNum");
static_assert(offsetof(CCourseGump, backupFrame) == 0x104, "CCourseGump::backupFrame");
static_assert(offsetof(CCourseGump, plotButton) == 0x10c, "CCourseGump::plotButton");
static_assert(offsetof(CCourseGump, regionX0) == 0x110, "CCourseGump::regionX0");
static_assert(sizeof(CCourseGump) == 0x128, "CCourseGump size (operator new(0x128))");
