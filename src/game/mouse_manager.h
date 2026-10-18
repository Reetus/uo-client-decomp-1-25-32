// CMouseManager: the software mouse cursor (one static instance at 0x0068d2e0, vtable 0x0049f6f8).
// It keeps the 0x2e cursor records (art id ranges and hotspots), the decoded cursor bitmaps, the saved
// background under the cursor and the gump the cursor is over. Four critical sections guard it:
// 0x00528628 (current record), 0x00528610 (position), 0x00528658 (current size) and 0x00528640 (present).
// Layout from the assembly of 0x00446330 .. 0x004484f0.
#pragma once

#include <cstddef>
#include <cstdint>

#include <windows.h>

#include "../runtime/ported.h"
#include "gump.h"
#include "globals.h"

struct CursorRecord {       // 0x18 bytes
    int32_t firstArt;       // +0x00  first art id of the animation (0x6053 ..)
    int32_t lastArt;        // +0x04  last art id
    uint8_t hotX;           // +0x08  hotspot, found as the 0x03e0 marker pixel by LoadCursorArt
    uint8_t hotY;           // +0x09
    uint8_t unk_0a;         // +0x0a  zeroed by the constructor, never read here
    uint8_t pad_0b;
    int32_t maxW;           // +0x0c  largest width of the art ids
    int32_t maxH;           // +0x10
    int32_t loaded;         // +0x14  1 once LoadCursorArt has seen the record
};
static_assert(sizeof(CursorRecord) == 0x18, "CursorRecord size");
static_assert(offsetof(CursorRecord, hotX) == 0x08, "CursorRecord::hotX");
static_assert(offsetof(CursorRecord, maxW) == 0x0c, "CursorRecord::maxW");
static_assert(offsetof(CursorRecord, loaded) == 0x14, "CursorRecord::loaded");

class CMouseManager {
public:
    // 0x00446330  size 0x3dc  constructor (the compiler's vptr, table 0x0049f6f8): clears the records, sets the art ids,
    // creates the critical sections.
    CMouseManager();
    // Frees the decoded bitmaps and the two DIB sections. Defined inline in mouse_manager.cpp so the scalar deleting
    // destructor (0x00446710) expands it as the original's does.
    virtual ~CMouseManager();
    // 0x00446830  size 4  the "cursor changed" flag.
    int __thiscall GetChangedFlag();
    // 0x00446840  RET 4: request cursor `cursor` (0x11 default, 0x14 busy).
    void __thiscall SetMouseCursor(int cursor);
    // 0x004468b0 / 0x004468c0: busy counter up / down (not below 0).
    void __thiscall IncBusy();
    void __thiscall DecBusy();
    // 0x004468e0  size 0x37c  decode the cursor art (ids 0x6053 .. 0x6079) into private bitmaps.
    void __thiscall LoadCursorArt();
    // 0x00446c60  RET 8: cursor hotspot position.
    void __thiscall GetMousePos(int *px, int *py);
    // 0x00446d00  RET 4: forget the hover gump when it is `gump`.
    void __thiscall ForgetHover(CGump *gump);
    // 0x00446d20  direction 0..7 of the cursor from the screen centre.
    int __thiscall GetWalkDirection();
    // 0x00446e30  cursor inside the ellipse around the screen centre.
    int __thiscall IsNearScreenCenter();
    // 0x00446ed0 / 0x00447010 / 0x00447210 / 0x00447340  RET 8: set the cursor position and forward it
    // to the hover gump's virtual slot 0x10 / 0x14 / 0x18 / 0x1c.
    void __thiscall SetPos(int x, int y);
    void __thiscall SetPosSlot14(int x, int y);
    void __thiscall SetPosSlot18(int x, int y);
    void __thiscall SetPosSlot1c(int x, int y);
    // 0x00447460  RET 8: set the cursor position and refresh the cursor shape.
    void __thiscall SetPosUpdateShape(int x, int y);
    // 0x004474f0  find the gump under the cursor and send enter / leave / move to the gumps involved.
    void __thiscall UpdateHover();
    // 0x00447660  RET 4: [+0x10] = value (cursor enabled).
    void __thiscall SetEnabled(int value);
    // 0x00447670  choose the cursor record from the walk direction and state.
    void __thiscall UpdateCursorShape();
    // 0x004477e0  RET 4: restore the old background, save the new one, draw the cursor.
    void __thiscall DrawCursor(int lockPresent);
    // 0x004478f0  save the screen under the cursor into the first DIB.
    void __thiscall SaveBackground();
    // 0x00447a90  compose the cursor over the saved background and put it on the screen.
    void __thiscall DrawCursorImage();
    // 0x00447c20  put the saved background back on the screen.
    void __thiscall RestoreBackground();
    // 0x00447d00  copy the back-buffer pixels under the cursor into the first DIB.
    void __thiscall CaptureBackBufferRect();
    // 0x00447f20  RET 0xc: draw a w x h bitmap at the cursor into the back buffer (ring buffer with wrap).
    void __thiscall BlitToBackBuffer(const uint16_t *src, int srcStride, int skipZero);
    // 0x004482a0  poll the OS cursor position and redraw the cursor when it moved.
    void __thiscall PollCursorPos();
    // 0x004484f0  put the saved background into the back buffer and clear the pending flag.
    void __thiscall RefreshFromSaved();

    int32_t busy;               // +0x004  busy counter (cursor 0x14 while > 0)
    int32_t changed;            // +0x008  cursor changed (InterlockedExchange)
    int32_t cursorId;           // +0x00c  requested cursor, 0x11 default (InterlockedExchange)
    int32_t enabled;            // +0x010
    CGump *hover;               // +0x014  gump under the cursor
    int32_t curRecord;          // +0x018  index into rec[], -1 none
    int32_t unk_1c;             // +0x01c
    CursorRecord rec[0x2e];     // +0x020
    int32_t curArt;             // +0x470  firstArt of the current record
    int32_t posX;               // +0x474  cursor position minus hotspot
    int32_t posY;               // +0x478
    int32_t curW;               // +0x47c  maxW of the current record
    int32_t curH;               // +0x480
    uint16_t *art[0x27];        // +0x484  decoded bitmaps of art ids 0x6053 + i
    int32_t artW[0x27];         // +0x520
    int32_t artH[0x27];         // +0x5bc
    int32_t savedValid;         // +0x658
    void *bits1;                // +0x65c  DIB 1 pixels (saved background)
    HBITMAP hbm1;               // +0x660
    void *bits2;                // +0x664  DIB 2 pixels (cursor over background)
    HBITMAP hbm2;               // +0x668
    int32_t dibW;               // +0x66c  DIB width in pixels (multiple of 4)
    int32_t dibH;               // +0x670
    int32_t savedX;             // +0x674  rectangle currently saved in DIB 1
    int32_t savedY;             // +0x678
    int32_t savedW;             // +0x67c
    int32_t savedH;             // +0x680
    int32_t drawX;              // +0x684  position / size / record / art of the cursor being drawn
    int32_t drawY;              // +0x688
    int32_t drawW;              // +0x68c
    int32_t drawH;              // +0x690
    int32_t drawRecord;         // +0x694
    int32_t drawArt;            // +0x698
};

static_assert(offsetof(CMouseManager, busy) == 0x004, "CMouseManager::busy");
static_assert(offsetof(CMouseManager, changed) == 0x008, "CMouseManager::changed");
static_assert(offsetof(CMouseManager, cursorId) == 0x00c, "CMouseManager::cursorId");
static_assert(offsetof(CMouseManager, enabled) == 0x010, "CMouseManager::enabled");
static_assert(offsetof(CMouseManager, hover) == 0x014, "CMouseManager::hover");
static_assert(offsetof(CMouseManager, curRecord) == 0x018, "CMouseManager::curRecord");
static_assert(offsetof(CMouseManager, rec) == 0x020, "CMouseManager::rec");
static_assert(offsetof(CMouseManager, curArt) == 0x470, "CMouseManager::curArt");
static_assert(offsetof(CMouseManager, posX) == 0x474, "CMouseManager::posX");
static_assert(offsetof(CMouseManager, curW) == 0x47c, "CMouseManager::curW");
static_assert(offsetof(CMouseManager, art) == 0x484, "CMouseManager::art");
static_assert(offsetof(CMouseManager, artW) == 0x520, "CMouseManager::artW");
static_assert(offsetof(CMouseManager, artH) == 0x5bc, "CMouseManager::artH");
static_assert(offsetof(CMouseManager, savedValid) == 0x658, "CMouseManager::savedValid");
static_assert(offsetof(CMouseManager, bits1) == 0x65c, "CMouseManager::bits1");
static_assert(offsetof(CMouseManager, hbm2) == 0x668, "CMouseManager::hbm2");
static_assert(offsetof(CMouseManager, dibW) == 0x66c, "CMouseManager::dibW");
static_assert(offsetof(CMouseManager, savedX) == 0x674, "CMouseManager::savedX");
static_assert(offsetof(CMouseManager, drawX) == 0x684, "CMouseManager::drawX");
static_assert(offsetof(CMouseManager, drawArt) == 0x698, "CMouseManager::drawArt");
static_assert(sizeof(CMouseManager) == 0x69c, "CMouseManager size");

// 0x004467b0  size 0x7d  the destructor for the atexit thunk (a plain function expanding the inline destructor).
void __fastcall MouseManager_Dtor_004467b0(void *self, void *);

#define g_mouse GLOBAL_AS(CMouseManager, g_mouse_storage)

// 0x00447130  fastcall (ECX = x, EDX = y): the gump under the screen point, honouring the scroll-choice gump.
CGump *__fastcall Mouse_FindGumpUnderMouse(int x, int y);
// 0x00447610  RET 0x10: map position of the terrain tile under the screen point (-1, -1 when none).
void __stdcall Mouse_TerrainTileAt(int x, int y, int *outX, int *outY);
