// CMouseManager: construction, cursor selection, cursor art loading, position queries and the position
// setters that forward to the hover gump. Procedures 0x00446330 .. 0x004474f0 of chunk_14_00445800.
#include "mouse_manager.h"

#include <windows.h>

#include <cstring>

#include "../runtime/ported.h"
#include "ddraw_display.h"
#include "globals.h"
#include "gump_manager.h"
#include "obj.h"
#include "crt.h"
#include "art_record_ops_ports.h"
#include "art_row_blits_ports.h"
#include "art_blit_ports.h"
#include "art_row_blits_hue_ports.h"
#include "vtables.h"

#define CS_CURSOR GLOBAL_PTR(CRITICAL_SECTION, g_mouseCursorLock_storage)    // guards curRecord
#define CS_POS GLOBAL_PTR(CRITICAL_SECTION, g_mousePosLock_storage)       // guards posX / posY
#define CS_SIZE GLOBAL_PTR(CRITICAL_SECTION, g_mouseSizeLock_storage)      // guards curW / curH
#define CS_PRESENT GLOBAL_PTR(CRITICAL_SECTION, g_presentLock_storage)   // guards the whole draw

#define g_tileArtTable GLOBAL_PTR(void, g_artCache_storage)
#define g_displayObj GLOBAL_AS(DDrawDisplay, g_clientState_storage)

// 0x00476130 on the tile art table: the art record for an id.
#define Mouse_TileArtGet(o, a1) PORT_THISCALL1(&ArtRecord_Get, uint8_t *, (o), int)(a1)
// 0x004767c0: ECX = buffer, EDX = width (target of the row blits).
#define Mouse_SetBlitTarget(a1, a2) (SetBlitSurface((uint16_t *)(a1), a2))
// 0x004767d0: ECX = row blit, EDX = art record, stack (clip, x, y).
#define Mouse_RleArtWalk PORT_FUNC5(&RleArtWalk, void, __fastcall, void *, void *, int *, int, int)
// 0x0047e2d0 on the display object: DeleteObject(hbitmap); the second argument is not read.
#define Mouse_DeleteGdiObject(self, a1, a2) (reinterpret_cast<DDrawDisplay *>(self)->DeleteGdiObject((HGDIOBJ)(a1), (int)(a2)))
// 0x00482c70: message box; ECX = text, EDX = caption.

// 0x00446330  size 0x3dc
// Constructor: clears the 0x2e cursor records and the bitmap tables, sets the vtable, initialises the four
// critical sections and gives each record its art id range.
CMouseManager::CMouseManager()
{
    busy = 0;
    enabled = 0;
    hover = nullptr;
    curRecord = -1;
    unk_1c = 0;

    CursorRecord *r = &rec[0];
    int n = 0x2e;
    do {
        r->firstArt = -1;
        r->lastArt = -1;
        r->hotX = 0;
        r->hotY = 0;
        r->unk_0a = 0;
        r->maxW = 0;
        r->maxH = 0;
        r->loaded = 0;
        r++;
        n--;
    } while (n != 0);

    curArt = 0;
    posX = 0;
    posY = 0;
    curW = 0;
    curH = 0;
    savedValid = 0;
    bits1 = nullptr;
    hbm1 = nullptr;
    bits2 = nullptr;
    hbm2 = nullptr;
    dibW = 0;
    dibH = 0;
    savedX = 0;
    savedY = 0;
    savedW = 0;
    savedH = 0;
    if (cursorId != 0) {
        InterlockedExchange(reinterpret_cast<LONG *>(&cursorId), 0);
        InterlockedExchange(reinterpret_cast<LONG *>(&changed), 1);
    }
    InterlockedExchange(reinterpret_cast<LONG *>(&changed), 1);
    InitializeCriticalSection(CS_PRESENT);
    InitializeCriticalSection(CS_POS);
    InitializeCriticalSection(CS_SIZE);
    InitializeCriticalSection(CS_CURSOR);
    for (int i = 0; i < 0x27; i++)
        art[i] = nullptr;

    rec[0].firstArt = 0x6053;
    rec[0].lastArt = 0x6053;
    rec[1].firstArt = 0x6054;
    rec[1].lastArt = 0x6054;
    rec[2].firstArt = 0x6055;
    rec[2].lastArt = 0x6055;
    rec[3].firstArt = 0x6056;
    rec[3].lastArt = 0x6056;
    rec[4].firstArt = 0x6057;
    rec[4].lastArt = 0x6057;
    rec[5].firstArt = 0x6058;
    rec[5].lastArt = 0x6058;
    rec[6].firstArt = 0x6059;
    rec[6].lastArt = 0x6059;
    rec[7].firstArt = 0x605a;
    rec[7].lastArt = 0x605a;
    rec[8].firstArt = 0x6053;
    rec[8].lastArt = 0x6053;
    rec[9].firstArt = 0x6054;
    rec[9].lastArt = 0x6054;
    rec[10].firstArt = 0x6055;
    rec[10].lastArt = 0x6055;
    rec[11].firstArt = 0x6056;
    rec[11].lastArt = 0x6056;
    rec[12].firstArt = 0x6057;
    rec[12].lastArt = 0x6057;
    rec[13].firstArt = 0x6058;
    rec[13].lastArt = 0x6058;
    rec[14].firstArt = 0x6059;
    rec[14].lastArt = 0x6059;
    rec[15].firstArt = 0x605a;
    rec[15].lastArt = 0x605a;
    rec[16].firstArt = 0x605b;
    rec[16].lastArt = 0x605b;
    rec[17].firstArt = 0x605c;
    rec[17].lastArt = 0x605d;
    rec[18].firstArt = 0x605e;
    rec[18].lastArt = 0x605e;
    rec[19].firstArt = 0x605f;
    rec[19].lastArt = 0x605f;
    rec[20].firstArt = 0x6060;
    rec[20].lastArt = 0x6060;
    rec[21].firstArt = 0x6061;
    rec[21].lastArt = 0x6061;
    rec[22].firstArt = 0x6062;
    rec[22].lastArt = 0x6062;
    rec[23].firstArt = 0x606a;
    rec[23].lastArt = 0x606a;
    rec[24].firstArt = 0x606b;
    rec[24].lastArt = 0x606b;
    rec[25].firstArt = 0x606c;
    rec[25].lastArt = 0x606c;
    rec[26].firstArt = 0x606d;
    rec[26].lastArt = 0x606d;
    rec[27].firstArt = 0x606e;
    rec[27].lastArt = 0x606e;
    rec[28].firstArt = 0x606f;
    rec[28].lastArt = 0x606f;
    rec[29].firstArt = 0x6070;
    rec[29].lastArt = 0x6070;
    rec[30].firstArt = 0x6071;
    rec[30].lastArt = 0x6071;
    rec[38].firstArt = 0x6071;
    rec[38].lastArt = 0x6071;
    rec[39].firstArt = 0x6072;
    rec[39].lastArt = 0x6072;
    rec[35].firstArt = 0x606e;
    rec[41].firstArt = 0x6075;
    rec[41].lastArt = 0x6075;
    rec[35].lastArt = 0x606e;
    rec[42].firstArt = 0x6076;
    rec[42].lastArt = 0x6076;
    rec[31].firstArt = 0x606a;
    rec[43].firstArt = 0x6077;
    rec[43].lastArt = 0x6077;
    rec[31].lastArt = 0x606a;
    rec[44].firstArt = 0x6078;
    rec[44].lastArt = 0x6078;
    rec[32].firstArt = 0x606b;
    rec[45].firstArt = 0x6079;
    rec[45].lastArt = 0x6079;
    rec[32].lastArt = 0x606b;
    rec[33].firstArt = 0x606c;
    rec[33].lastArt = 0x606c;
    rec[34].firstArt = 0x606d;
    rec[34].lastArt = 0x606d;
    rec[36].firstArt = 0x606f;
    rec[36].lastArt = 0x606f;
    rec[37].firstArt = 0x6070;
    rec[37].lastArt = 0x6070;
    rec[40].firstArt = 0x6073;
    rec[40].lastArt = 0x6074;
}
HOOK_CTOR(0x00446330, CMouseManager, ());

inline CMouseManager::~CMouseManager()
{
    for (int i = 0; i < 0x27; i++)
        operator delete(art[i]);
    if (hbm1 != nullptr && bits1 != nullptr) {
        Mouse_DeleteGdiObject(GLOBAL_PTR(void, g_clientState_storage), hbm1, bits1);
        Mouse_DeleteGdiObject(GLOBAL_PTR(void, g_clientState_storage), hbm2, bits2);
        hbm2 = nullptr;
        hbm1 = nullptr;
        bits2 = nullptr;
        bits1 = nullptr;
    }
}
HOOK_VTABLE(0x0049f6f8, CMouseManager);
HOOK_SDD(0x00446710, CMouseManager);   // slot 0

// 0x004467b0  size 0x7d
// The destructor as the atexit thunk 0x0044a970 jumps to it (ECX = g_mouse): a plain function that expands the inline
// destructor.
void __fastcall MouseManager_Dtor_004467b0(void *self, void *)
{
    reinterpret_cast<CMouseManager *>(self)->CMouseManager::~CMouseManager();
}
HOOK_FUNC(0x004467b0, MouseManager_Dtor_004467b0);

// 0x00446830  size 4
int __thiscall CMouseManager::GetChangedFlag()
{
    return changed;
}
HOOK_METHOD(0x00446830, CMouseManager, GetChangedFlag);

// 0x00446840  size 0x6b
// Request a cursor: 0x11 unless the enable flag (0x005285c8) is set; cursor 0x14 raises the busy counter to
// at least 1, and a busy counter forces 0x14. Publishes the id and the changed flag only when the id differs.
void __thiscall CMouseManager::SetMouseCursor(int cursor)
{
    int want = 0x11;
    if (g_inputActive != 0)
        want = cursor;
    if (want == 0x14 && busy <= 0)
        busy = busy + 1;
    int value;
    if (busy > 0) {
        if (cursorId == 0x14)
            return;
        value = 0x14;
    } else if (want == 0x11) {
        if (cursorId == 0x11)
            return;
        value = 0x11;
    } else {
        if (cursorId == want)
            return;
        value = want;
    }
    InterlockedExchange(reinterpret_cast<LONG *>(&cursorId), value);
    InterlockedExchange(reinterpret_cast<LONG *>(&changed), 1);
}
HOOK_METHOD(0x00446840, CMouseManager, SetMouseCursor);

// 0x004468b0  size 4
void __thiscall CMouseManager::IncBusy()
{
    busy = busy + 1;
}
HOOK_METHOD(0x004468b0, CMouseManager, IncBusy);

// 0x004468c0  size 0x11
void __thiscall CMouseManager::DecBusy()
{
    int v = busy - 1;
    busy = v;
    if (v < 0)
        busy = 0;
}
HOOK_METHOD(0x004468c0, CMouseManager, DecBusy);

// 0x004468e0  size 0x37c
// Pass 1: for every record, find the hotspot (the first pixel equal to 0x03e0 in the first row, and in the
// first column of the rows) of every art id of its range and the largest size. Pass 2: copy every art id
// 0x6053 .. 0x6079 into a private bitmap (even width, clipped to {1, 1, w-1, h}), tint the first 0x17 with
// 0x00528674 and convert to 565 when the display is 565. Then create the two DIB sections.
void __thiscall CMouseManager::LoadCursorArt()
{
    CursorRecord *r = &rec[0];
    int n = 0x2e;
    do {
        r->maxW = 0;
        r->maxH = 0;
        int id = r->firstArt;
        if (id <= r->lastArt) {
            do {
                uint8_t *a = Mouse_TileArtGet(g_tileArtTable, id);   // 0x00476130
                int h = *reinterpret_cast<uint16_t *>(a + 6);
                int w = *reinterpret_cast<uint16_t *>(a + 4);
                uint16_t *rowOff = reinterpret_cast<uint16_t *>(a + 8);
                uint8_t *data = a + h * 2 + 8;
                uint16_t *p = reinterpret_cast<uint16_t *>(data + rowOff[0] * 2 + 4);
                int k = 0;
                if (w > 0) {
                    do {
                        if (*p == 0x3e0) {
                            r->hotX = static_cast<uint8_t>(k);
                            break;
                        }
                        k++;
                        p++;
                    } while (k < w);
                }
                k = 0;
                if (h > 0) {
                    do {
                        if (*reinterpret_cast<uint16_t *>(data + rowOff[k] * 2 + 4) == 0x3e0) {
                            r->hotY = static_cast<uint8_t>(k);
                            break;
                        }
                        k++;
                    } while (k < h);
                }
                r->loaded = 1;
                if (w > r->maxW)
                    r->maxW = w;
                if (h > r->maxH)
                    r->maxH = h;
                id++;
            } while (id <= r->lastArt);
        }
        r++;
        n--;
    } while (n != 0);

    for (int k = 0; k < 0x27; k++) {
        uint8_t *a = Mouse_TileArtGet(g_tileArtTable, k + 0x6053);   // 0x00476130
        artW[k] = *reinterpret_cast<uint16_t *>(a + 4);
        if (artW[k] & 1)
            artW[k] = artW[k] - 1;
        artH[k] = *reinterpret_cast<uint16_t *>(a + 6);
        int clip[4];
        clip[0] = 1;
        clip[1] = 1;
        clip[2] = artW[k] - 1;
        clip[3] = artH[k];
        uint16_t *buf = static_cast<uint16_t *>(::operator new(static_cast<size_t>(artW[k]) * artH[k] * 2));   // operator new 0x0048ad70
        art[k] = buf;
        memset(buf, 0, static_cast<size_t>(artW[k]) * artH[k] * 2);
        Mouse_SetBlitTarget(art[k], artW[k]);                 // 0x004767c0
        Mouse_RleArtWalk(PTR_AT(void, PORT_ADDR(&RowBlitCopyAll)), a, clip, 0, 0);   // 0x004767d0 (RleArtWalk)
        if (k < 0x17 && g_tintColor != 0) {
            uint16_t *px = art[k];
            int c = g_tintColor;
            for (int row = 0; row < artH[k]; row++) {
                for (int col = 0; col < artW[k]; col++) {
                    uint16_t v = *px;
                    if (v != 0) {
                        int red = static_cast<uint16_t>(v) >> 10;
                        int lo = (c & 0x1f) * red;
                        int mid = ((c >> 5) & 0x1f) * red;
                        int hi = (v & 0xfc00) * (c >> 10);
                        lo = lo >> 5;
                        mid = mid & 0xffe0;
                        hi = static_cast<int>(static_cast<uint32_t>(hi) >> 5);
                        hi = hi & 0x400;
                        *px = static_cast<uint16_t>(lo | mid | hi);
                        c = g_tintColor;
                    }
                    px++;
                }
            }
        }
        if (g_is565 != 0) {
            uint16_t *px = art[k];
            for (int row = 0; row < artH[k]; row++) {
                for (int col = 0; col < artW[k]; col++) {
                    uint16_t v = *px;
                    if (v != 0)
                        *px = static_cast<uint16_t>((v & 0x1f) | ((v & 0xffe0) << 1));
                    px++;
                }
            }
        }
    }

    dibW = 4;
    dibH = 4;
    for (int k = 0; k < 0x27; k++) {
        if (artW[k] > dibW)
            dibW = artW[k];
        if (artH[k] > dibH)
            dibH = artH[k];
    }
    dibW = (dibW + 3) & ~3;
    hbm1 = g_displayObj.CreateTopDownDib16(dibW, dibH, &bits1);   // 0x0047e2f0
    hbm2 = g_displayObj.CreateTopDownDib16(dibW, dibH, &bits2);   // 0x0047e2f0
}
HOOK_METHOD(0x004468e0, CMouseManager, LoadCursorArt);

// 0x00446c60  size 0x9a
// Cursor hotspot position: posX/posY plus the hotspot of the current record when it is loaded.
void __thiscall CMouseManager::GetMousePos(int *px, int *py)
{
    EnterCriticalSection(CS_CURSOR);
    int r = curRecord;
    LeaveCriticalSection(CS_CURSOR);
    EnterCriticalSection(CS_POS);
    if (r >= 0 && rec[r].loaded != 0) {
        *px = posX + rec[r].hotX;
        *py = posY + rec[r].hotY;
        LeaveCriticalSection(CS_POS);
        return;
    }
    *px = posX;
    *py = posY;
    LeaveCriticalSection(CS_POS);
}
HOOK_METHOD(0x00446c60, CMouseManager, GetMousePos);

// 0x00446d00  size 0x18
void __thiscall CMouseManager::ForgetHover(CGump *gump)
{
    if (hover == gump)
        hover = nullptr;
}
HOOK_METHOD(0x00446d00, CMouseManager, ForgetHover);

// 0x00446d20  size 0x102
// Sector of the cursor around the screen centre (0x140, 0xf0) in eighths: 0 up, then clockwise by the
// slope tests (|dy| against 2|dx| and |dx| against 2|dy|).
int __thiscall CMouseManager::GetWalkDirection()
{
    int x;
    int y;
    EnterCriticalSection(CS_CURSOR);
    int r = curRecord;
    LeaveCriticalSection(CS_CURSOR);
    EnterCriticalSection(CS_POS);
    if (r >= 0 && rec[r].loaded != 0) {
        x = posX + rec[r].hotX;
        y = posY + rec[r].hotY;
    } else {
        x = posX;
        y = posY;
    }
    LeaveCriticalSection(CS_POS);
    x = x - 0x140;
    y = y - 0xf0;
    if (x > 0) {
        if (y < -x * 2)
            return 0;
        if (y * 2 < -x)
            return 1;
        if (y * 2 < x)
            return 2;
        return (y >= x * 2) + 3;
    }
    if (y > -x * 2)
        return 4;
    if (y * 2 > -x)
        return 5;
    if (y * 2 > x)
        return 6;
    return ((y <= x * 2) - 1) & 7;
}
HOOK_METHOD(0x00446d20, CMouseManager, GetWalkDirection);

// 0x00446e30  size 0x99
// 9 dx^2 + 16 dy^2 <= 0x64000 around the screen centre (0x140, 0xf0).
int __thiscall CMouseManager::IsNearScreenCenter()
{
    int x;
    int y;
    EnterCriticalSection(CS_CURSOR);
    int r = curRecord;
    LeaveCriticalSection(CS_CURSOR);
    EnterCriticalSection(CS_POS);
    if (r >= 0 && rec[r].loaded != 0) {
        x = posX + rec[r].hotX;
        y = posY + rec[r].hotY;
    } else {
        x = posX;
        y = posY;
    }
    LeaveCriticalSection(CS_POS);
    x = x - 0x140;
    y = y - 0xf0;
    return x * x * 9 + (y * y << 4) <= 0x64000;
}
HOOK_METHOD(0x00446e30, CMouseManager, IsNearScreenCenter);

// 0x00446ed0  size 0x136
// Set the cursor position (the hotspot is subtracted when the record is loaded), find the gump under the
// hotspot when there is none, and send the hotspot in its local coordinates to its virtual slot 0x10.
void __thiscall CMouseManager::SetPos(int x, int y)
{
    int gx;
    int gy;
    EnterCriticalSection(CS_CURSOR);
    int r = curRecord;
    LeaveCriticalSection(CS_CURSOR);
    EnterCriticalSection(CS_POS);
    if (r >= 0 && rec[r].loaded != 0) {
        posX = x - rec[r].hotX;
        posY = y - rec[r].hotY;
    } else {
        posX = x;
        posY = y;
    }
    LeaveCriticalSection(CS_POS);
    EnterCriticalSection(CS_CURSOR);
    r = curRecord;
    LeaveCriticalSection(CS_CURSOR);
    EnterCriticalSection(CS_POS);
    if (r >= 0 && rec[r].loaded != 0) {
        gx = posX + rec[r].hotX;
        gy = posY + rec[r].hotY;
    } else {
        gx = posX;
        gy = posY;
    }
    LeaveCriticalSection(CS_POS);
    if (hover == nullptr)
        hover = g_gumpMgr.GumpAtPoint(gx, gy, nullptr, 1);   // 0x0043a220
    if (hover != nullptr) {
        hover->ScreenToLocal(&gx, &gy);   // 0x0043b510
        PORT_VSLOT2(hover, 0x10, void, int, int)(gx, gy);
    }
}
HOOK_METHOD(0x00446ed0, CMouseManager, SetPos);

// 0x00447010  size 0x116
// As SetPos, but only forwards to a hover gump that already exists, through slot 0x14.
void __thiscall CMouseManager::SetPosSlot14(int x, int y)
{
    int gx;
    int gy;
    EnterCriticalSection(CS_CURSOR);
    int r = curRecord;
    LeaveCriticalSection(CS_CURSOR);
    EnterCriticalSection(CS_POS);
    if (r >= 0 && rec[r].loaded != 0) {
        posX = x - rec[r].hotX;
        posY = y - rec[r].hotY;
    } else {
        posX = x;
        posY = y;
    }
    LeaveCriticalSection(CS_POS);
    EnterCriticalSection(CS_CURSOR);
    r = curRecord;
    LeaveCriticalSection(CS_CURSOR);
    EnterCriticalSection(CS_POS);
    if (r >= 0 && rec[r].loaded != 0) {
        gx = posX + rec[r].hotX;
        gy = posY + rec[r].hotY;
    } else {
        gx = posX;
        gy = posY;
    }
    LeaveCriticalSection(CS_POS);
    if (hover != nullptr) {
        hover->ScreenToLocal(&gx, &gy);   // 0x0043b510
        PORT_VSLOT2(hover, 0x14, void, int, int)(gx, gy);
    }
}
HOOK_METHOD(0x00447010, CMouseManager, SetPosSlot14);

// 0x00447130  size 0xd9
// The gump under a screen point. While the scroll-choice gump (0x006488d4) exists it is the only candidate
// (0 when the point misses it); otherwise the top-level list is walked from the tail through +0x3c for the
// first visible gump that answers slot 0x0c and returns 0 from slot 0x84, and the deepest child at the point
// is returned. ECX = x, EDX = y; the object is not used.
CGump *__fastcall Mouse_FindGumpUnderMouse(int x, int y)
{
    int lx;
    int ly;
    if (g_scrollChoiceGump != nullptr) {
        lx = x;
        ly = y;
        g_scrollChoiceGump->ScreenToLocal(&lx, &ly);   // 0x0043b510
        if (PORT_VSLOT2(g_scrollChoiceGump, 0x0c, int, int, int)(lx, ly) == 0)
            return nullptr;
        return g_scrollChoiceGump->FindGumpAt(lx, ly, nullptr);   // 0x0043b970
    }
    for (CGump *g = g_gumpListTail; g != nullptr; g = g->prev) {
        lx = x;
        ly = y;
        g->ScreenToLocal(&lx, &ly);   // 0x0043b510
        if (g->visible != 0 && PORT_VSLOT2(g, 0x0c, int, int, int)(lx, ly) != 0 &&
            PORT_VSLOT0(g, 0x84, int)() == 0)
            return g->FindGumpAt(lx, ly, nullptr);   // 0x0043b970
    }
    return nullptr;
}
HOOK_FUNC(0x00447130, Mouse_FindGumpUnderMouse);

// 0x00447210  size 0x12b
// As SetPos, but the missing hover gump is looked up with Mouse_FindGumpUnderMouse and the position goes to
// slot 0x18.
void __thiscall CMouseManager::SetPosSlot18(int x, int y)
{
    int gx;
    int gy;
    EnterCriticalSection(CS_CURSOR);
    int r = curRecord;
    LeaveCriticalSection(CS_CURSOR);
    EnterCriticalSection(CS_POS);
    if (r >= 0 && rec[r].loaded != 0) {
        posX = x - rec[r].hotX;
        posY = y - rec[r].hotY;
    } else {
        posX = x;
        posY = y;
    }
    LeaveCriticalSection(CS_POS);
    EnterCriticalSection(CS_CURSOR);
    r = curRecord;
    LeaveCriticalSection(CS_CURSOR);
    EnterCriticalSection(CS_POS);
    if (r >= 0 && rec[r].loaded != 0) {
        gx = posX + rec[r].hotX;
        gy = posY + rec[r].hotY;
    } else {
        gx = posX;
        gy = posY;
    }
    LeaveCriticalSection(CS_POS);
    if (hover == nullptr)
        hover = Mouse_FindGumpUnderMouse(gx, gy);   // 0x00447130
    if (hover != nullptr) {
        hover->ScreenToLocal(&gx, &gy);   // 0x0043b510
        PORT_VSLOT2(hover, 0x18, void, int, int)(gx, gy);
    }
}
HOOK_METHOD(0x00447210, CMouseManager, SetPosSlot18);

// 0x00447340  size 0x116
// As SetPosSlot14 with slot 0x1c.
void __thiscall CMouseManager::SetPosSlot1c(int x, int y)
{
    int gx;
    int gy;
    EnterCriticalSection(CS_CURSOR);
    int r = curRecord;
    LeaveCriticalSection(CS_CURSOR);
    EnterCriticalSection(CS_POS);
    if (r >= 0 && rec[r].loaded != 0) {
        posX = x - rec[r].hotX;
        posY = y - rec[r].hotY;
    } else {
        posX = x;
        posY = y;
    }
    LeaveCriticalSection(CS_POS);
    EnterCriticalSection(CS_CURSOR);
    r = curRecord;
    LeaveCriticalSection(CS_CURSOR);
    EnterCriticalSection(CS_POS);
    if (r >= 0 && rec[r].loaded != 0) {
        gx = posX + rec[r].hotX;
        gy = posY + rec[r].hotY;
    } else {
        gx = posX;
        gy = posY;
    }
    LeaveCriticalSection(CS_POS);
    if (hover != nullptr) {
        hover->ScreenToLocal(&gx, &gy);   // 0x0043b510
        PORT_VSLOT2(hover, 0x1c, void, int, int)(gx, gy);
    }
}
HOOK_METHOD(0x00447340, CMouseManager, SetPosSlot1c);

// 0x00447460  size 0x89
// Set the cursor position only (hotspot subtracted when loaded) and refresh the cursor shape.
void __thiscall CMouseManager::SetPosUpdateShape(int x, int y)
{
    EnterCriticalSection(CS_CURSOR);
    int r = curRecord;
    LeaveCriticalSection(CS_CURSOR);
    EnterCriticalSection(CS_POS);
    if (r >= 0 && rec[r].loaded != 0) {
        posX = x - rec[r].hotX;
        posY = y - rec[r].hotY;
    } else {
        posX = x;
        posY = y;
    }
    LeaveCriticalSection(CS_POS);
    UpdateCursorShape();   // 0x00447670
}
HOOK_METHOD(0x00447460, CMouseManager, SetPosUpdateShape);
