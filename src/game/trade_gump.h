// CTradeGump: the secure trade window (packet 0x6F). It shows the other player's name above their offered
// container and the player's own name above theirs, each side with an accept check box.
// vtable 0x004a0b88, operator new(0xc8). Constructor 0x0047b8f0, allocated in packet_handlers_dialogs.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "container_gump.h"
#include "gump.h"
#include "toggle_control.h"

class CTradeGump : public CGump {
public:
    virtual int IsTag80() { return 0; }
    virtual int IsTag110() { return 1; }
    // slot override (gump virtual-slot prep): own copy of the shared body
    virtual int HitTest(int x, int y);
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void MouseMove(int x, int y);
    // 0x0047b8f0: RET 0x10, (trade partner mobile, own container serial, partner container serial, partner name).
    // The mobile (CGump::owner) learns about the gump through its field +0x1d4.
    CTradeGump(CObj *partner, uint32_t serial1, uint32_t serial2, const char *partnerName);
    // 0x0047bd70: destructor.
    ~CTradeGump();
    void Dtor() { this->CTradeGump::~CTradeGump(); }
    // 0x0047be70: RET 8 (arguments not read): the close button. Sends the cancel (0x6F action 1) when the first
    // container is known, otherwise just deletes the gump.
    virtual void RButtonUp(int x, int y);
    // 0x0047bed0: RET 4 (control): the own accept check box was clicked; sends 0x6F action 2.
    virtual void OnChildChanged(int child);
    // 0x0047bf90: vtable slot 0x08, RET 4.
    virtual void Draw(int *clip);

    int32_t unk_94;
    char partnerName[0x20];         // +0x98
    CContainerGump *container1;     // +0xb8  the first container (serial argument 1)
    CContainerGump *container2;     // +0xbc
    CToggleControl *acceptOwn;      // +0xc0
    CToggleControl *acceptOther;    // +0xc4
};

static_assert(offsetof(CTradeGump, partnerName) == 0x98, "CTradeGump::partnerName");
static_assert(offsetof(CTradeGump, container1) == 0xb8, "CTradeGump::container1");
static_assert(offsetof(CTradeGump, container2) == 0xbc, "CTradeGump::container2");
static_assert(offsetof(CTradeGump, acceptOwn) == 0xc0, "CTradeGump::acceptOwn");
static_assert(offsetof(CTradeGump, acceptOther) == 0xc4, "CTradeGump::acceptOther");
static_assert(sizeof(CTradeGump) == 0xc8, "CTradeGump size (operator new(0xc8))");
