#include "scroll_choice_gump.h"

#include <windows.h>

#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "button_gump.h"
#include "globals.h"
#include "gump_manager.h"
#include "crt.h"
#include "scroll_thumb_gump.h"
#include "main_menu_gump.h"
#include "waiting_gump.h"
#include "client_socket.h"
#include "packet_builders_b25_ports.h"
#include "main_window_ports.h"
#include "char_creation_pages.h"
#include "vtables.h"

// The SEH frame of the original (try-level bookkeeping around each operator new) is omitted; every
// "operator new returned NULL" branch of the original is kept (the object pointer becomes 0).

#define g_pDragGump GLOBAL_AS(CGump *, g_dragGump)

// 0x00413110: rectangle hit button, RET 0x24, (x1, y1, x2, y2, parent, mode, 0, handler, 0). Not ported.
// 0x00476ad0: close the connection object, plain thiscall.
#define Orig_NetClose(self) (reinterpret_cast<CSocketBase *>(self)->Close())
// 0x00465ab0: build the select-server packet (0xa0), thiscall on 0x0069e0e0, RET 8, (packet, server id).
#define Orig_BuildSelectServer(o, a1, a2) PORT_THISCALL2(&Pkt_Build_0xA0_SelectServer, void, (o), char *, uint16_t)(a1, a2)
// 0x00478610: send a built packet.
#define Orig_SendPacket(self, a1) (reinterpret_cast<CClientSocket *>(self)->SendPacket((const uint8_t *)(a1)))
// 0x00480d60: set the main window title suffix, ECX = text.
#define Orig_SetMainWindowTitle(a1) (SetMainWindowTitle(a1))

// 0x0041c930  size 0x3ed
// Constructor with a target edit box: the list rectangle and the scroll thumb / arrow buttons depend on the
// background art (0x4ca narrow, 0x4cc wide). +0x22c8 is not initialised here.
CScrollChoiceGump::CScrollChoiceGump(CGump *ownerArg, CInputControlGump *choiceBoxArg, int artId)
{
    g_scrollChoiceHoverRow = -1;
    count = 0;
    firstRow = 0;
    clickCount = 0;
    mark[2] = -1;
    mark[1] = -1;
    mark[0] = -1;
    name = const_cast<char *>("scrollchoice gump");
    strcpy(reinterpret_cast<char *>(pad_226c), g_emptyString);
    owner_22bc = ownerArg;
    dirty = 0;
    unk_58 = 0;
    unk_a0 = 0;
    pendingClick = 0;
    unk_98 = 0;
    unk_94 = 0;
    waitingDouble = 0;
    tickStart = 0;
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    choiceBox_22c4 = choiceBoxArg;
    unk_50 = artId;

    gfx::ArtSize(unk_50, &width, &height);
    x = 0x140 - width / 2;
    y = 0xf0 - height / 2;
    frameIndex = 0;
    frameCount = 1;

    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CScrollChoiceGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    gfx::setBitmap(this, gfx::AllocTagged(width * height * 2 + 4, "text frame0"));
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(width * height * 2));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);

    if (unk_50 == 0x4ca) {
        listLeft = 0x22;
        listTop = 0x22;
        listRight = 0xcc;
        listBottom = 0xfd;
        thumb_22c0 = new CScrollThumbGump(0xc6, 0x24, 1, 0xf8, (int)(this), 0x4cb);
        gfx::AddChild(thumb_22c0, this);
        gfx::AddChild(new CButtonGump(0xb9, 2, 0xd2, 0x14, this, 2, 0, PORT_ADDR(&CGumpHandlers04::ScrollChoicePrev), 0), this);
        gfx::AddChild(new CButtonGump(0xb2, 0x108, 0xcc, 0x11c, this, 2, 0, PORT_ADDR(&CGumpHandlers04::ScrollChoiceNext), 0), this);
    } else if (unk_50 == 0x4cc) {
        listLeft = 0x22;
        listTop = 0x22;
        listRight = 0x168;
        listBottom = 0xfd;
        thumb_22c0 = new CScrollThumbGump(0x16a, 0x24, 1, 0xf8, (int)(this), 0x4cb);
        gfx::AddChild(thumb_22c0, this);
        gfx::AddChild(new CButtonGump(0x162, 4, 0x17a, 0x16, this, 2, 0, PORT_ADDR(&CGumpHandlers04::ScrollChoicePrev), 0), this);
        gfx::AddChild(new CButtonGump(0x15e, 0x108, 0x174, 0x11e, this, 2, 0, PORT_ADDR(&CGumpHandlers04::ScrollChoiceNext), 0), this);
    }
    Redraw();   // 0x0041d830
}
HOOK_CTOR(0x0041c930, CScrollChoiceGump, (CGump *, CInputControlGump *, int));

// 0x0041cd50  size 0x53f
// Constructor for the server list (no target edit box): as Ctor3 plus the "Missing gump!" check and the cancel
// button (and, on the wide variant, two more buttons).
CScrollChoiceGump::CScrollChoiceGump(CGump *ownerArg, int artId)
{
    g_scrollChoiceHoverRow = -1;
    count = 0;
    firstRow = 0;
    clickCount = 0;
    mark[2] = -1;
    mark[1] = -1;
    mark[0] = -1;
    name = const_cast<char *>("scrollchoice gump");
    strcpy(reinterpret_cast<char *>(pad_226c), g_emptyString);
    owner_22bc = ownerArg;
    dirty = 0;
    unk_58 = 0;
    choiceBox_22c4 = nullptr;
    unk_a0 = 0;
    pendingClick = 0;
    unk_98 = 0;
    unk_94 = 0;
    waitingDouble = 0;
    tickStart = 0;
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    serverList_22c8 = 1;
    unk_50 = artId;

    gfx::ArtSize(unk_50, &width, &height);
    x = 0x140 - width / 2;
    y = 0xf0 - height / 2;
    frameIndex = 0;
    frameCount = 1;

    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CScrollChoiceGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    gfx::setBitmap(this, gfx::AllocTagged(width * height * 2 + 4, "text frame0"));
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(width * height * 2));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);

    if (unk_50 == 0x4ca) {
        listLeft = 0x22;
        listTop = 0x22;
        listRight = 0xcc;
        listBottom = 0xfd;
        thumb_22c0 = new CScrollThumbGump(0xc6, 0x24, 1, 0xf8, (int)(this), 0x4cb);
        gfx::AddChild(thumb_22c0, this);
        gfx::AddChild(new CButtonGump(0xb9, 2, 0xd2, 0x14, this, 2, 0, PORT_ADDR(&CGumpHandlers04::ScrollChoicePrev), 0), this);
        gfx::AddChild(new CButtonGump(0xb2, 0x108, 0xcc, 0x11c, this, 2, 0, PORT_ADDR(&CGumpHandlers04::ScrollChoiceNext), 0), this);
    } else if (unk_50 == 0x4cc) {
        listLeft = 0x4a;
        listTop = 0x22;
        listRight = 0x168;
        listBottom = 0xfd;
        thumb_22c0 = new CScrollThumbGump(0x16a, 0x24, 1, 0xf8, (int)(this), 0x4cb);
        gfx::AddChild(thumb_22c0, this);
        gfx::AddChild(new CButtonGump(0x162, 4, 0x17a, 0x16, this, 2, 0, PORT_ADDR(&CGumpHandlers04::ScrollChoicePrev), 0), this);
        gfx::AddChild(new CButtonGump(0x15e, 0x108, 0x174, 0x11e, this, 2, 0, PORT_ADDR(&CGumpHandlers04::ScrollChoiceNext), 0), this);
    }

    int w1, h1;
    gfx::ArtSize(0x93b, &w1, &h1);
    if (w1 == 0)
        CreateGump2ac(static_cast<int>(reinterpret_cast<uintptr_t>("Missing gump!")), 0, 0, 0);   // 0x00439f60

    {
        CButtonGump *b = new CButtonGump(0x6c, 0x108, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 2, 0, PORT_ADDR(&CGumpHandlers04::SortServersByKeyA), 0x93b, 0x93c, 0);
        b->artHover = 0x93d;
        gfx::AddChild(b, this);
    }
    if (unk_50 == 0x4cc) {
        {
            CButtonGump *b = new CButtonGump(0xb9, 0x108, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 2, 0, PORT_ADDR(&CGumpHandlers04::SortServersByKeyB), 0x93e, 0x93f, 0);
            b->artHover = 0x940;
            gfx::AddChild(b, this);
        }
        {
            CButtonGump *b = new CButtonGump(0xfa, 0x108, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 2, 0, PORT_ADDR(&CGumpHandlers04::SortServersByPing), 0x941, 0x942, 0);
            b->artHover = 0x943;
            gfx::AddChild(b, this);
        }
    }
    Redraw();   // 0x0041d830
}
HOOK_CTOR(0x0041cd50, CScrollChoiceGump, (CGump *, int));

// 0x0041d290  size 0x175
// Destructor body: frees frame0 and the frame data; without a target edit box the gump was the server list, so
// the main menu is shown again (or, when the choice was made, a "Connecting... " waiting gump is opened).
CScrollChoiceGump::~CScrollChoiceGump()
{
    void *mem;
    if (g_scrollChoiceGump == this)
        g_scrollChoiceGump = nullptr;
    if (frameData[1] != 0) {
        gfx::FreeTagged(gfx::bitmapOf(this), "scrollchoice frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "scrollchoice framedata");
        frameData = nullptr;
    }
    if (owner_22bc != nullptr && choiceBox_22c4 != nullptr)
        PORT_VSLOT0(owner_22bc, 0x60, void)();
    if (choiceBox_22c4 == nullptr) {
        if (serverList_22c8 != 0) {
            CGump *mm = g_pMainMenu;
            if (mm != nullptr) {
                mm->visible = 1;
            } else {
                mm = new CMainMenuGump();
                g_pMainMenu = mm;
                gfx::AddChild(mm, nullptr);
            }
            if (g_pktSender != nullptr)
                Orig_NetClose(g_pktSender);
            g_flag6f493c = 0;
            Orig_SetMainWindowTitle(g_emptyString);
        } else {
            CGump *w = new CGump_0043de30(-1, -1, "Connecting... ", 0, 1, 1, 0);
            g_waitingGump = w;
            g_waitingGump->lastChild->visible = 0;
            gfx::AddChild(g_waitingGump, nullptr);
        }
    }
}
HOOK_DTOR(0x0041d290, CScrollChoiceGump);

HOOK_VTABLE(0x0049d8a0, CScrollChoiceGump);
HOOK_SDD(0x0041cd20, CScrollChoiceGump);

// 0x0041d440  size 0x5d
// Mouse down: remember the position, start a click and bring the gump to the front; the double-click wait
// moves 0 -> 1 (with the tick count) -> 2.
void __thiscall CScrollChoiceGump::MouseDown(int mx, int my)
{
    unk_6c = mx;
    unk_70 = my;
    pendingClick = 1;
    clickCount = 1;
    CGump_BringToFront(this, 0);
    if (waitingDouble == 1) {
        waitingDouble = 2;
        return;
    }
    waitingDouble = 1;
    tickStart = GetTickCount();
}
HOOK_VMETHOD(0x0041d440, CScrollChoiceGump, MouseDown);

// 0x0041d410  size 0x25
// Vtable slot 0x1c: release the drag capture, mark +0x22c8 and delete the gump (virtual slot 0, flags 1).
// ? which input event reaches this slot is not settled.
void __thiscall CScrollChoiceGump::RButtonUp(int a1, int a2)
{
    (void)a1;
    (void)a2;
    g_pDragGump = nullptr;
    serverList_22c8 = 1;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void *, int)(1);
}
HOOK_VMETHOD(0x0041d410, CScrollChoiceGump, RButtonUp);

// 0x0041d4a0  size 0x175
// Click at a local position: when it is inside the list rectangle the entry under it is chosen. With a target edit
// box its value and text are set and the owner is notified (slot 0x44); otherwise a select-server packet is sent
// and the server id and name are stored in globals. The gump then deletes itself.
void __thiscall CScrollChoiceGump::OnLeftClick(int px, int py)
{
    int lineW;
    int lineH;
    if (px < listLeft || px > listRight || py < listTop || py > listBottom)
        return;
    // the second output of the measure call overwrites px (the original passes &px)
    gfx::TextMeasure(5, g_refText, &lineW, &px, &lineH);
    int row = (py - listTop) / px + firstRow;
    if (row >= count)
        return;
    if (choiceBox_22c4 != nullptr) {
        choiceBox_22c4->value = ids[row];
        strcpy(choiceBox_22c4->text, names[row]);
        choiceBox_22c4->Redraw();
        if (owner_22bc != nullptr)
            PORT_VSLOT1(owner_22bc, 0x44, void, CGump *)(choiceBox_22c4);
    } else {
        char pkt[4];
        Orig_BuildSelectServer(GLOBAL_PTR(void, g_packetBuilder_storage), pkt, static_cast<uint16_t>(ids[row]));
        Orig_SendPacket(g_pktSender, pkt);
        serverList_22c8 = 0;
        g_selectedServer = static_cast<uint8_t>(ids[row]);
        strcpy(g_titlePartB, names[row]);
    }
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void *, int)(1);
}
HOOK_METHOD(0x0041d4a0, CScrollChoiceGump, OnLeftClick);
