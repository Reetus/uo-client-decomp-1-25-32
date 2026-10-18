// Login-flow gumps. The SEH frame of the original (try levels around the
// operator new calls) is omitted throughout.
#include "login_gumps.h"

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "batch02_helpers.h"
#include "globals.h"
#include "crt.h"
#include "input_control_gump.h"
#include "button_gump.h"
#include "toggle_control.h"
#include "font_manager.h"
#include "small_methods_u1_ports.h"
#include "vtables.h"
#include "class_0049c690.h"

// Constructors of the original control classes (all __thiscall on freshly allocated memory).
#define InputCtl_Redraw(self) (reinterpret_cast<CInputControlGump *>(self)->Redraw())
// button (0xcc bytes): x, y, parent, 0, 0, handler, up, down, 0
// check box (0xb8 bytes): x, y, parent, art, art2, 1, hue, label, ?, 0, 0
#define CheckBox_Ctor(self, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) ((void *)reinterpret_cast<CToggleControl *>(self)->Ctor(a1, a2, (int)(a3), a4, a5, a6, a7, a8, a9, a10, a11))
// 0x00432260: draw wrapped text (x, y, font, hue, text, wrapWidth, dst, stride, clip, 0, 0)
#define FontDrawTextWrapped(self, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) (reinterpret_cast<CFontManager *>(self)->DrawTextWrapped(a1, a2, a3, a4, (char *)(a5), a6, a7, a8, (const int *)(a9), a10, a11))

#define g_activeChgPW GLOBAL_AS(void *, g_scrollChoiceGump)        // the live CChgPWGump

// Pixel start of frame 0's bitmap (the bitmap has a 4-byte {w, h} header).
static inline uint16_t *frame0_pixels(CGump *g)
{
    return reinterpret_cast<uint16_t *>(static_cast<char *>(i32_ptr(g->frameData[1])) + 4);
}

// Free frame 0 bitmap and the frame data, as the dtors do.
static void free_frame(CGump *g, const char *bmpTag, const char *dataTag)
{
    if (g->frameData[1] != 0) {
        FrameData_Free(g_frameDataAllocator, i32_ptr(g->frameData[1]), bmpTag);
        g->frameData[1] = 0;
    }
    if (g->frameData != nullptr) {
        FrameData_Free(g_frameDataAllocator, g->frameData, dataTag);
        g->frameData = nullptr;
    }
}

// 0x004212d0  size 0xb2
// CLoginGump::~CLoginGump body.
CLoginGump::~CLoginGump()
{
    CInputControlGump **slot;
    int n;

    slot = children;
    n = 5;
    do {
        if (*slot != nullptr) {
            PORT_VSLOT1(*slot, 0, void, int)(1);     // child deleting dtor
            *slot = nullptr;
        }
        slot = slot + 1;
        n = n - 1;
    } while (n != 0);
    if (frameData[1] != 0) {
        FrameData_Free(g_frameDataAllocator, i32_ptr(frameData[1]), "Login frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        FrameData_Free(g_frameDataAllocator, frameData, "Login framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x004212d0, CLoginGump);

// 0x00421660  size 0x415
// CAcctLoginGump::CAcctLoginGump
CAcctLoginGump::CAcctLoginGump()
{
    void *mem;
    int16_t *bmp;

    name = const_cast<char *>("AcctLogin gump");
    dirty = 0;
    unk_50 = 0x410;                       // background art id
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    GumpArtSize(0x410, &width, &height);
    x = 0x80;
    y = 0x6c;
    frameIndex = 0;
    frameCount = 1;
    frameData = FrameData_Alloc(g_frameDataAllocator, 8, "framedata for CAcctLoginGump");
    memset(frameData, 0, frameCount * 8);
    frameData[1] = ptr_i32(FrameData_Alloc(g_frameDataAllocator, width * height * 2 + 4, "dumb frame0"));
    bmp = static_cast<int16_t *>(i32_ptr(frameData[1]));
    bmp[0] = static_cast<int16_t>(width);
    bmp = static_cast<int16_t *>(i32_ptr(frameData[1]));
    bmp[1] = static_cast<int16_t>(height);
    memset(frame0_pixels(this), 0, width * height * 2);
    DrawGumpArt(ROWBLIT_COPY, unk_50, 0, 0, frame0_pixels(this), width, height);

    // ---- account name input ----
    mem = new CInputControlGump(0xdb - x, 0x8a - y, this, 0x41f, 0x41f, 0, 0x1e, 2, 0x386, 1, 0, 0);
    nameInput = mem;
    RAW_AT(int16_t, nameInput, 0xde) = 0x412;
    RAW_AT(int16_t, nameInput, 0xe0) = 0x420;
    RAW_AT(int16_t, nameInput, 0xdc) = 0x411;
    RAW_AT(uint8_t, nameInput, 0xe2) = 5;
    RAW_AT(uint8_t, nameInput, 0xe3) = 1;
    InputCtl_Redraw(nameInput);
    CGump_AddChild(nameInput, this, 1);

    // ---- password input ----
    mem = new CInputControlGump(0xd9 - x, 0xad - y, this, 0x421, 0x421, 0, 0x1e, 2, 0x386, 1, 0, 0);
    passwordInput = mem;
    RAW_AT(int16_t, passwordInput, 0xde) = 0x414;
    RAW_AT(int16_t, passwordInput, 0xe0) = 0x422;
    RAW_AT(int16_t, passwordInput, 0xdc) = 0x413;
    RAW_AT(uint8_t, passwordInput, 0xe2) = 7;
    RAW_AT(uint8_t, passwordInput, 0xe3) = 2;
    RAW_AT(int32_t, passwordInput, 0xb0) = 1;       // mask with '*'
    InputCtl_Redraw(passwordInput);
    CGump_AddChild(passwordInput, this, 1);

    // ---- button 0 ----
    mem = new CButtonGump(0x173 - x, 0xde - y, (int)(this), 0, 0, (int)(reinterpret_cast<void *>(PORT_ADDR(&CAcctLoginGump::Connect))), 0x418, 0x419, 0);
    RAW_AT(int32_t, mem, 0xa4) = 0x41a;
    CGump_AddChild(mem, this, 1);

    // ---- button 1 ----
    mem = new CButtonGump(0xa1 - x, 0xde - y, (int)(this), 0, 0, (int)(reinterpret_cast<void *>(PORT_ADDR(&CAcctLoginGump::Cancel))), 0x415, 0x416, 0);
    RAW_AT(int32_t, mem, 0xa4) = 0x417;
    CGump_AddChild(mem, this, 1);

    // ---- check-box control (size 0xb8) ----
    mem = new CToggleControl(0x105 - x, 0xd4 - y, (int)(this), 0x41b, 0x41d, 1, 0x386, g_emptyString, g_rememberPassword, 0, 0);
    checkBox = mem;
    RAW_AT(int32_t, checkBox, 0xa0) = 0x41c;
    RAW_AT(int32_t, checkBox, 0xa4) = 0x41e;
    CGump_AddChild(checkBox, this, 1);
}
HOOK_CTOR(0x00421660, CAcctLoginGump, ());

// 0x00421ab0  size 0xc5
// CAcctLoginGump::~CAcctLoginGump body.
CAcctLoginGump::~CAcctLoginGump()
{

    if (nameInput != nullptr) {
        PORT_VSLOT1(nameInput, 0, void, int)(1);
        nameInput = nullptr;
    }
    if (passwordInput != nullptr) {
        PORT_VSLOT1(passwordInput, 0, void, int)(1);
        passwordInput = nullptr;
    }
    if (checkBox != nullptr) {
        PORT_VSLOT1(checkBox, 0, void, int)(1);
        checkBox = nullptr;
    }
    if (frameData[1] != 0) {
        FrameData_Free(g_frameDataAllocator, i32_ptr(frameData[1]), "AcctLogin frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        FrameData_Free(g_frameDataAllocator, frameData, "AcctLogin framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00421ab0, CAcctLoginGump);

// 0x00421ce0  size 0x4cc
// CChgPWGump::CChgPWGump(char *unused, char *account)
CChgPWGump::CChgPWGump(char *unused, char *accountName)
{
    (void)unused;
    int rect[4];
    int16_t *bmp;
    void *ctl;

    g_activeChgPW = this;
    strcpy(account, accountName);
    unk_94 = 0;
    dirty = 0;
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    name = const_cast<char *>("ChgPW gump");
    unk_50 = 0x474;                       // background art id
    GumpArtSize(0x474, &width, &height);
    x = 0x79;
    y = 0x66;
    frameIndex = 0;
    frameCount = 1;

    frameData = FrameData_Alloc(g_frameDataAllocator, 8, "framedata for CChgPWGump");
    memset(frameData, 0, frameCount * 8);

    frameData[1] = ptr_i32(FrameData_Alloc(g_frameDataAllocator, width * height * 2 + 4, "dumb frame0"));
    bmp = static_cast<int16_t *>(i32_ptr(frameData[1]));
    bmp[0] = static_cast<int16_t>(width);
    bmp = static_cast<int16_t *>(i32_ptr(frameData[1]));
    bmp[1] = static_cast<int16_t>(height);
    memset(frame0_pixels(this), 0, (width * height) << 1);

    DrawGumpArt(ROWBLIT_COPY, unk_50, 0, 0, frame0_pixels(this), width, height);

    rect[0] = 0;
    rect[1] = 0;
    rect[2] = width;
    rect[3] = height;
    FontDrawText(g_fontManager, 0x3c, 0x37, 2, 0x386, "Old Password", frame0_pixels(this), width, rect);
    FontDrawText(g_fontManager, 0x3c, 0x63, 2, 0x386, "New Password", frame0_pixels(this), width, rect);
    FontDrawText(g_fontManager, 0x3c, 0x8f, 2, 0x386, "Verify Password", frame0_pixels(this), width, rect);

    // ---- control 1: old password (y = 0x3a), focused ----
    ctl = new CInputControlGump(0x3c, 0x3a, this, 0x475, 0x475, 0, 0x1e, 1, 0x386, 1, 0, 0);
    oldPassword = ctl;
    RAW_AT(int16_t, ctl, 0xde) = 0x476;
    RAW_AT(int16_t, oldPassword, 0xe0) = 0x477;
    RAW_AT(uint8_t, oldPassword, 0xe2) = 0xc;
    RAW_AT(uint8_t, oldPassword, 0xe3) = 5;
    RAW_AT(int32_t, oldPassword, 0xd8) = 1;         // focused
    RAW_AT(int32_t, oldPassword, 0xb0) = 1;         // masked
    g_focusedInput = oldPassword;
    InputCtl_Redraw(oldPassword);
    CGump_AddChild(oldPassword, this, 1);

    // ---- control 2: new password (y = 0x66) ----
    ctl = new CInputControlGump(0x3c, 0x66, this, 0x475, 0x475, 0, 0x1e, 1, 0x386, 1, 0, 0);
    newPassword = ctl;
    RAW_AT(int16_t, ctl, 0xde) = 0x476;
    RAW_AT(int16_t, newPassword, 0xe0) = 0x477;
    RAW_AT(uint8_t, newPassword, 0xe2) = 0xc;
    RAW_AT(uint8_t, newPassword, 0xe3) = 5;
    CGump_AddChild(newPassword, this, 1);
    RAW_AT(int32_t, newPassword, 0xb0) = 1;         // masked
    InputCtl_Redraw(newPassword);

    // ---- control 3: verify password (y = 0x92) ----
    ctl = new CInputControlGump(0x3c, 0x92, this, 0x475, 0x475, 0, 0x1e, 1, 0x386, 1, 0, 0);
    verifyPassword = ctl;
    RAW_AT(int16_t, ctl, 0xde) = 0x476;
    RAW_AT(int16_t, verifyPassword, 0xe0) = 0x477;
    RAW_AT(uint8_t, verifyPassword, 0xe2) = 0xc;
    RAW_AT(uint8_t, verifyPassword, 0xe3) = 5;
    CGump_AddChild(verifyPassword, this, 1);
    RAW_AT(int32_t, verifyPassword, 0xb0) = 1;      // masked
    InputCtl_Redraw(verifyPassword);

    // ---- OK button (handler 0x0040f250) ----
    ctl = new CButtonGump(0xee - x, 0x124 - y, (int)(this), 0, 0, (int)(reinterpret_cast<void *>(PORT_ADDR(&CChgPWGump::Confirm))), 0x47b, 0x47c, 0);
    RAW_AT(int32_t, ctl, 0xa4) = 0x47d;
    CGump_AddChild(ctl, this, 1);

    // ---- Cancel button (handler 0x0040cf60) ----
    ctl = new CButtonGump(0x145 - x, 0x124 - y, (int)(this), 0, 0, (int)(reinterpret_cast<void *>(PORT_ADDR(&DeleteGumpCallback))), 0x478, 0x479, 0);
    RAW_AT(int32_t, ctl, 0xa4) = 0x47a;
    CGump_AddChild(ctl, this, 1);
}
HOOK_CTOR(0x00421ce0, CChgPWGump, (char *, char *));

// 0x004221e0  size 0xce
// CChgPWGump dtor body.
CChgPWGump::~CChgPWGump()
{
    if (g_activeChgPW == this)
        g_activeChgPW = nullptr;
    if (g_prevGump != nullptr)
        g_prevGump->visible = 1;

    if (oldPassword != nullptr) {
        PORT_VSLOT1(oldPassword, 0, void, int)(1);
        oldPassword = nullptr;
    }
    if (newPassword != nullptr) {
        PORT_VSLOT1(newPassword, 0, void, int)(1);
        newPassword = nullptr;
    }
    free_frame(this, "ChgPW frame0 pBMP", "ChgPW framedata");
}
HOOK_DTOR(0x004221e0, CChgPWGump);

// 0x00422590  size 0x346
// CDeleteCharGump::CDeleteCharGump(char *charName, char *unused)
CDeleteCharGump::CDeleteCharGump(char *charName, char *unused)
{
    (void)unused;
    char msg[80];
    int rect[4];
    int16_t *bmp;
    void *ctl;

    unk_94 = 0;
    name = const_cast<char *>("DeleteChar gump");
    dirty = 0;
    unk_50 = 0x474;                       // background art id
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    GumpArtSize(0x474, &width, &height);
    x = 0x79;
    y = 0x66;
    frameIndex = 0;
    frameCount = 1;

    frameData = FrameData_Alloc(g_frameDataAllocator, 8, "framedata for CDeleteCharGump");
    memset(frameData, 0, frameCount * 8);

    frameData[1] = ptr_i32(FrameData_Alloc(g_frameDataAllocator, height * width * 2 + 4, "dumb frame0"));
    bmp = static_cast<int16_t *>(i32_ptr(frameData[1]));
    bmp[0] = static_cast<int16_t>(width);
    bmp = static_cast<int16_t *>(i32_ptr(frameData[1]));
    bmp[1] = static_cast<int16_t>(height);
    memset(frame0_pixels(this), 0, (height * width) << 1);

    DrawGumpArt(ROWBLIT_COPY, unk_50, 0, 0, frame0_pixels(this), width, height);

    rect[0] = 0;
    rect[1] = 0;
    rect[2] = width;
    rect[3] = height;
    FontDrawText(g_fontManager, 0x3c, 0x7d, 2, 0x386, "Password", frame0_pixels(this), width, rect);

    sprintf(msg, "Permanently delete %s?", charName);
    FontDrawTextWrapped(g_fontManager, 0x3c, 0x32, 2, 0x386, msg, width - 0x78, frame0_pixels(this), width, rect, 0, 0);

    // password edit control (0x158 bytes)
    ctl = new CInputControlGump(0x3c, 0x82, this, 0x475, 0x475, 0, 0x1e, 1, 0x386, 1, 0, 0);
    password = ctl;
    RAW_AT(int16_t, password, 0xde) = 0x476;
    RAW_AT(int16_t, password, 0xe0) = 0x477;
    RAW_AT(uint8_t, password, 0xe2) = 0xc;
    RAW_AT(uint8_t, password, 0xe3) = 5;
    CGump_AddChild(password, this, 1);
    RAW_AT(int32_t, password, 0xb0) = 1;            // masked input
    RAW_AT(int32_t, password, 0xd8) = 1;            // focused
    g_focusedInput = password;
    InputCtl_Redraw(password);

    // OK button (0xcc bytes), handler 0x0040f020
    ctl = new CButtonGump(0xee - x, 0x124 - y, (int)(this), 0, 0, (int)(reinterpret_cast<void *>(PORT_ADDR(&CDeleteCharGump::DelChar))), 0x47b, 0x47c, 0);
    RAW_AT(int32_t, ctl, 0xa4) = 0x47d;
    CGump_AddChild(ctl, this, 1);

    // Cancel button (0xcc bytes), handler 0x0040cf60
    ctl = new CButtonGump(0x145 - x, 0x124 - y, (int)(this), 0, 0, (int)(reinterpret_cast<void *>(PORT_ADDR(&DeleteGumpCallback))), 0x478, 0x479, 0);
    RAW_AT(int32_t, ctl, 0xa4) = 0x47a;
    CGump_AddChild(ctl, this, 1);
}
HOOK_CTOR(0x00422590, CDeleteCharGump, (char *, char *));

// 0x00422910  size 0xa9
// CDeleteCharGump dtor body.
CDeleteCharGump::~CDeleteCharGump()
{
    if (g_prevGump != nullptr)
        g_prevGump->visible = 1;

    if (password != nullptr) {
        PORT_VSLOT1(password, 0, void, int)(1);
        password = nullptr;
    }
    free_frame(this, "DeleteChar frame0 pBMP", "DeleteChar framedata");
}
HOOK_DTOR(0x00422910, CDeleteCharGump);
