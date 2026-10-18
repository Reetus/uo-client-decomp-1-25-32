// CCourseGump ("Course gump", vtable 0x0049e6d8): constructor, destructor, mouse handlers and frame blit.
// AddPin (0x00427dd0) and Redraw (0x00428a90) are in course_gump.cpp.
// The SEH frames of the constructor and destructor (vector constructor / `new` try levels) are omitted.
#include <windows.h>

#include <cmath>
#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "batch02_helpers.h"
#include "button_gump.h"
#include "course_gump.h"
#include "globals.h"
#include "crt.h"
#include "packet_builders_batch24_ports.h"
#include "client_socket.h"
#include "obj.h"
#include "window_helpers_31_ports.h"
#include "gump.h"
#include "gump_base_ops_ports.h"
#include "line_draw_ports.h"
#include "art_row_blits_hue_ports.h"
#include "small_methods_u1_ports.h"
#include "spell_icon_gump.h"
#include "ui_callbacks_ports.h"
#include "vtables.h"

#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)
#define g_appObject GLOBAL_PTR(void, g_app_storage)
#define g_fontObject GLOBAL_PTR(void, g_fontMgr_storage)

// 0x00463e40: build the map-pin packet into pkt (map serial, action, pin index, x, y).
#define Packet_MapPin(o, a1, a2, a3, a4, a5, a6) PORT_THISCALL6(&Build_Packet_0x56, void, (o), char *, int, int, int, int, int)(a1, a2, a3, a4, a5, a6)
#define Net_Send(self, a1) (reinterpret_cast<CClientSocket *>(self)->SendPacket((const uint8_t *)(a1)))
// 0x00453c60: apply the pin change to the map item (action, index, x, y). ECX = owner.
#define MapItem_ApplyPin(self, a1, a2, a3, a4) (reinterpret_cast<CObjVt49f960 *>(self)->ListOp(a1, a2, a3, a4))
#define Cursor_Set(o, a1) PORT_THISCALL1(&SetCursorForRegion, void, (o), int)(a1)          // on the object at 0x0069e0e8
#define Gump_LocalToScreen(self, a1, a2) (reinterpret_cast<CGump *>(self)->LocalToScreen(a1, a2))
#define Mgr_Intersects(o, a1, a2) PORT_THISCALL2(&GumpRectIntersects, int, (o), const int *, int *)(a1, a2)
#define Rect_Clip(a1, a2, a3) (GumpRectClip(a1, (const int *)(a2), a3))   // ECX = clip, EDX = &rect, stack = &out
// 0x00432f90 / 0x004332f0: 16-bit line drawers (ECX = font object, not read).
#define Line_Shaded(o, a1, a2, a3, a4, a5) PORT_THISCALL5(&DrawLine16Shaded, int, (o), int, int, int, int, void *)(a1, a2, a3, a4, a5)               // (x0, y0, x1, y1, bmp)
#define Line_FromSource(o, a1, a2, a3, a4, a5, a6) PORT_THISCALL6(&DrawLine16FromSource, int, (o), int, int, int, int, void *, void *)(a1, a2, a3, a4, a5, a6)    // (x0, y0, x1, y1, bmp, src)
#define g_rowCopyHere(a1, a2, a3) (RowBlitCopyAll((uint16_t *)(a1), (uint16_t *)(a2), a3))             // (dst, src, count)

// 0x00427620  size 0x3ec
// Builds the frame (map art, or the density map size + 0x2c x 0x3d), a 0x28 x 0x14 second frame filled with 0x14,
// a copy buffer, and the two buttons (clear pins, toggle plotting).
CCourseGump::CCourseGump(int px, int py, int artId, void *mapItem, uint32_t rx0, uint32_t ry0,
                         uint32_t rx1, uint32_t ry1, uint32_t mw, uint32_t mh)
{
    regionX0 = static_cast<int32_t>(rx0 & 0xffff);
    regionY0 = static_cast<int32_t>(ry0 & 0xffff);
    regionX1 = static_cast<int32_t>(rx1 & 0xffff);
    regionY1 = static_cast<int32_t>(ry1 & 0xffff);
    mapWidth = static_cast<int32_t>(mw & 0xffff);
    mapHeight = static_cast<int32_t>(mh & 0xffff);
    name = g_004a7bd4;                // "Course gump"
    dirty = 0;
    owner = static_cast<CObj *>(mapItem);
    if (mapItem != nullptr) {
        if (PORT_VSLOT0(mapItem, 0x60, int)() != 0)
            RAW_AT(CCourseGump *, owner, 0xb0) = this;
    }
    unk_bc = nullptr;
    unk_ac = 0;
    dragNext = nullptr;
    selectedPin = nullptr;
    dragPin = nullptr;
    x = px;
    plotEnabled = 0;
    cursorActive = 0;
    pinSelected = 0;
    unk_c0 = 0;
    segCount = 0;
    unk_a0 = 0;
    mouseDown = 0;
    unk_98 = 0;
    pinDragging = 0;
    clickState = 0;
    clickTick = 0;
    y = py;
    unk_60 = 0;
    unk_50 = artId;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    if (artId > 0x139d) {
        GumpArtSize(artId, &width, &height);
        rowStride = width;
    } else {
        width = mapWidth + 0x2c;
        rowStride = width;
        height = mapHeight + 0x3d;
    }
    unk_24 = 0x28;
    unk_28 = 0x14;
    frameIndex = 0;
    frameCount = 2;
    frameData = FrameData_Alloc(g_frameDataAllocator, 0x10, "framedata for CCourseGump");
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);

    frameData[1] = ptr_i32(gfx::AllocTagged(width * height * 2 + 4, "course frame0"));
    memset(static_cast<char *>(i32_ptr(frameData[1])) + 4, 0, static_cast<size_t>(width * height) * 2);
    *static_cast<uint16_t *>(i32_ptr(frameData[1])) = static_cast<uint16_t>(width);
    static_cast<uint16_t *>(i32_ptr(frameData[1]))[1] = static_cast<uint16_t>(height);

    frameData[3] = ptr_i32(gfx::AllocTagged(unk_28 * unk_24 * 2 + 4, "course frame1"));
    memset(static_cast<char *>(i32_ptr(frameData[3])) + 4, 0, static_cast<size_t>(unk_28 * unk_24) * 2);
    *static_cast<uint16_t *>(i32_ptr(frameData[3])) = static_cast<uint16_t>(unk_24);
    static_cast<uint16_t *>(i32_ptr(frameData[3]))[1] = static_cast<uint16_t>(unk_28);

    backupFrame = static_cast<uint16_t *>(gfx::AllocTagged(width * height * 2 + 4, "course backupFrame"));
    memset(reinterpret_cast<char *>(backupFrame) + 4, 0, static_cast<size_t>(width * height) * 2);
    backupFrame[0] = static_cast<uint16_t>(width);
    backupFrame[1] = static_cast<uint16_t>(height);

    memset(static_cast<char *>(i32_ptr(frameData[3])) + 4, 0x14, static_cast<size_t>(unk_28 * unk_24) * 2);
    Redraw();

    CButtonGump *button = new CButtonGump(((width - (width >> 31)) >> 1) - 0x37, height - 0x18, ptr_i32(this), 0, 0, PORT_ADDR(&CCourseGump::ClearPins), 0x139a,
                              0x139a, 0);
    clearButton = button;
    CGump_AddChild(button, this, 1);                  // called with a null control when the allocation failed, as in the original
    clearButton->visible = 0;

    button = new CButtonGump(((width - (width >> 31)) >> 1) - 0x46, 5, ptr_i32(this), 1, 0, PORT_ADDR(&UiCb_ToggleChildSend56), 0x1398, 0x1399, 0);
    plotButton = button;
    CGump_AddChild(button, this, 1);
}
HOOK_CTOR(0x00427620, CCourseGump, (int, int, int, void *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t));

HOOK_VTABLE(0x0049e6d8, CCourseGump);
HOOK_SDD(0x00427a10, CCourseGump);

// 0x00427a40  size 0x145
// Sends the "stop plotting" packet (action 6) when plotting was on, detaches from the map item, frees the frames.
CCourseGump::~CCourseGump()
{
    char pkt[0x20];
    if (g_suppressFlag == 0 && plotEnabled != 0) {
        Packet_MapPin(g_packetBuilder, pkt, RAW_AT(int32_t, owner, 0x78), 6, 0, 0, 0);
        Net_Send(g_pktSender, pkt);
    }
    if (owner != nullptr) {
        if (PORT_VSLOT0(owner, 0x60, int)() != 0)
            RAW_AT(int32_t, owner, 0xb0) = 0;
    }
    if (unk_bc != nullptr)
        PORT_VSLOT1(unk_bc, 0x00, void, int)(1);       // deleting dtor
    if (backupFrame != nullptr) {
        gfx::FreeTagged(backupFrame, "Course backupFrame");
        backupFrame = nullptr;
    }
    if (frameData[1] != 0) {
        gfx::FreeTagged(i32_ptr(frameData[1]), "Course frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData[3] != 0) {
        gfx::FreeTagged(i32_ptr(frameData[3]), "Course frame1 pBMP");
        frameData[3] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "Course framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00427a40, CCourseGump);

// 0x00427b90  size 0x1b  vtable slot 0x1c: forget the drag gump and delete this gump.
void __thiscall CCourseGump::RButtonUp(int px, int py)
{
    (void)px;
    (void)py;
    g_dragGump = nullptr;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}
HOOK_VMETHOD(0x00427b90, CCourseGump, RButtonUp);

// 0x00427bb0  size 0xc7  left button pressed: bring to front, count the click, and pick the pin under the point
// (the last matching one in list order) while plotting is on.
void __thiscall CCourseGump::MouseDown(int px, int py)
{
    unk_6c = px;
    unk_70 = py;
    mouseDown = 1;
    CGump_BringToFront(this, 0);
    if (clickState == 1) {
        clickState = 2;
    } else {
        clickState = 1;
        clickTick = GetTickCount();
    }
    if (plotEnabled != 0) {
        MapPin *pin = RAW_AT(MapPin *, owner, 0xa8);
        // px and py are dead after the stores above; the original measures the pin art into their slots
        GumpArtSize(0x139c, &py, &px);
        py >>= 1;
        px >>= 1;
        while (pin != nullptr) {
            int pxx = pin->x;
            if (unk_6c >= pxx - py && unk_6c <= pxx + py) {
                int pyy = pin->y;
                if (unk_70 >= pyy - px && unk_70 <= px + pyy)
                    dragPin = pin;
            }
            pin = pin->next;
        }
    }
}
HOOK_VMETHOD(0x00427bb0, CCourseGump, MouseDown);

// 0x00427c80  size 0x143  left button released: add a pin on a plain click, or finish a pin drag by sending the
// move (inside the gump) or delete (outside) packet.
void __thiscall CCourseGump::MouseUp(int px, int py)
{
    char pkt[0x10];
    int wasDragging = pinDragging;
    mouseDown = 0;
    if (wasDragging != 0) {
        pinDragging = 0;
        clickState = 0;
    } else if (clickState == 2) {
        clickState = 0;
    } else if (unk_a0 == 0 && clickState == 0) {
        AddPin(px, py);
    }
    unk_a0 = 0;
    if (dragPin != nullptr && wasDragging == 1) {
        MapPin *edi = RAW_AT(MapPin *, owner, 0xa8);
        int idx = 0;
        MapPin *want = dragNext;
        if (edi != want) {
            do {
                edi = edi->next;
                idx++;
            } while (edi != want);
        }
        if (px >= 0 && px < width && py >= 0 && py < height) {
            Packet_MapPin(g_packetBuilder, pkt, RAW_AT(int32_t, owner, 0x78), 3, idx, px, py);
            Net_Send(g_pktSender, pkt);
            MapItem_ApplyPin(owner, 2, idx, px, py);
        } else {
            Packet_MapPin(g_packetBuilder, pkt, RAW_AT(int32_t, owner, 0x78), 4, idx, 0, 0);
            Net_Send(g_pktSender, pkt);
        }
        pinSelected = 0;
        selectedPin = nullptr;
        Redraw();
        segCount = 0;
        dragPin = nullptr;
        dragNext = nullptr;
    }
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x00427c80, CCourseGump, MouseUp);

// 0x00427fd0  size 0x3ef
// Start moving the pin under the mouse: remember its list neighbours (positions and 1-based numbers), unlink and
// free it, redraw without it, save frame 0 into the backup buffer and draw the neighbour lines to (x, y).
void __thiscall CCourseGump::StartPinDrag(int px, int py)
{
    int aw, ah;
    char buf[0x10];
    int clip[4];
    MapPin *pred = RAW_AT(MapPin *, owner, 0xa8);
    clip[0] = 0;
    clip[1] = 0;
    clip[2] = width;
    clip[3] = height;
    if (plotEnabled == 0)
        return;
    if (dragPin == nullptr)
        return;
    dragNext = dragPin->next;
    while (pred != nullptr) {
        MapPin *n = pred->next;
        if (n == dragPin)
            break;
        pred = n;
    }
    segCount = 0;
    if (pred != nullptr) {
        pts[0].x = pred->x;
        pts[segCount].y = pred->y;
        int num = 1;
        MapPin *p = RAW_AT(MapPin *, owner, 0xa8);
        if (p != pred) {
            do {
                p = p->next;
                if (p != dragPin)
                    num++;
            } while (p != pred);
        }
        ptNum[segCount] = num;
        segCount++;
    }
    if (dragNext != nullptr) {
        int num = 1;
        pts[segCount].x = dragNext->x;
        pts[segCount].y = dragNext->y;
        MapPin *p = RAW_AT(MapPin *, owner, 0xa8);
        if (p != dragNext) {
            do {
                p = p->next;
                if (p != dragPin)
                    num++;
            } while (p != dragNext);
        }
        ptNum[segCount] = num;
        segCount++;
    }
    // 0x0040caf0 (empty destructor of the pin) is not called.
    if (dragPin != nullptr)
        operator delete(dragPin);
    RAW_AT(int32_t, owner, 0xac) = RAW_AT(int32_t, owner, 0xac) - 1;
    if (pred != nullptr)
        pred->next = dragNext;
    else
        RAW_AT(MapPin *, owner, 0xa8) = dragNext;
    if (segCount == 2) {
        pinSelected = 1;
        selectedPin = pred;
    }
    cursorActive = 0;
    Redraw();
    g_rowCopyHere(reinterpret_cast<char *>(backupFrame) + 4, static_cast<char *>(i32_ptr(frameData[1])) + 4, height * width);
    GumpArtSize(0x139c, &aw, &ah);
    aw >>= 1;
    ah >>= 1;
    if (px < 0 || px >= width)
        return;
    if (py < 0 || py >= height)
        return;
    uint16_t *bmp;
    if (segCount >= 1) {
        Line_Shaded(g_fontObject, px, py, pts[0].x, pts[0].y, i32_ptr(frameData[1]));
        bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
        DrawGumpArt(ROWBLIT_COPY, 0x139c, pts[0].x - aw, pts[0].y - ah, bmp + 2, bmp[0], bmp[1]);
        sprintf(buf, "%d", ptNum[0]);
        bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
        FontDrawText(g_fontObject, pts[0].x - static_cast<int>(strlen(buf) << 4), pts[0].y - 5, 3, 0, buf, bmp + 2, width, clip);
    }
    if (segCount >= 2) {
        Line_Shaded(g_fontObject, px, py, pts[1].x, pts[1].y, i32_ptr(frameData[1]));
        bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
        DrawGumpArt(ROWBLIT_COPY, 0x139c, pts[1].x - aw, pts[1].y - ah, bmp + 2, bmp[0], bmp[1]);
        sprintf(buf, "%d", ptNum[1]);
        bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
        FontDrawText(g_fontObject, pts[1].x - static_cast<int>(strlen(buf) << 4), pts[1].y - 5, 3, 0, buf, bmp + 2, width, clip);
    }
    unk_c0 = 1;
    lastX = px;
    lastY = py;
}
HOOK_METHOD(0x00427fd0, CCourseGump, StartPinDrag);

// 0x004283c0  size 0x2a3  mouse move: normal cursor over a pin or near a plotted segment (which shows the
// segment's midpoint as the cursor pin); the plain cursor and no cursor pin otherwise.
void __thiscall CCourseGump::MouseOver(int px, int py)
{
    int aw, ah;
    int best = 10;
    MapPin *bestPin;
    MapPin *pin;
    MapPin *a;
    MapPin *b;
    int hover = 0;

    if (plotEnabled == 0 || RAW_AT(int32_t, owner, 0xac) >= 0x32) {
        Cursor_Set(g_appObject, 0);
        return;
    }
    pin = RAW_AT(MapPin *, owner, 0xa8);
    GumpArtSize(0x139c, &aw, &ah);
    aw >>= 1;
    ah >>= 1;
    for (a = pin; a != nullptr; a = a->next) {
        int cx = a->x;
        if (px >= cx - aw && px <= cx + aw) {
            int cy = a->y;
            if (py >= cy - ah && py <= cy + ah) {
                Cursor_Set(g_appObject, 0);
                hover = 1;
                break;
            }
        }
    }
    bestPin = nullptr;
    if (!hover && pin != nullptr) {
        a = pin;
        while ((b = a->next) != nullptr) {
            int ax = a->x;
            int bx = b->x;
            int ay = a->y;
            int by = b->y;
            int lo = ax < bx ? ax : bx;
            int hi = ax < bx ? bx : ax;
            if (px >= lo - 9 && px <= hi + 9) {
                lo = ay < by ? ay : by;
                hi = ay < by ? by : ay;
                if (py >= lo - 9 && py <= hi + 9) {
                    int dx = bx - ax;
                    int dy = by - ay;
                    int len = static_cast<int>(std::sqrt(static_cast<double>(dx * dx + dy * dy)));     // FILD / FSQRT / __ftol
                    int sy = ay - py;
                    int sx = ax - px;
                    // no guard against len == 0: IDIV by len as in the binary
                    int t0 = (sy * dx - sx * dy) / len;
                    int d = t0 < 0 ? -t0 : t0;
                    int e1 = ((b->y - py) * dy - (bx - px) * (ax - bx)) / len;
                    int e2 = ((a->y - b->y) * sy - sx * dx) / len;
                    if (e2 < len && e1 < len && d < best) {
                        best = d;
                        bestPin = a;
                    }
                }
            }
            a = b;
        }
    }
    if (bestPin != nullptr) {
        Cursor_Set(g_appObject, 0);
        MapPin *nx = bestPin->next;
        int mx = (bestPin->x + nx->x);
        mx = (mx - (mx >> 31)) >> 1;
        int my = (bestPin->y + nx->y);
        my = (my - (my >> 31)) >> 1;
        if (cursorActive != 0 && cursorX == mx && cursorY == my)
            return;
        cursorX = static_cast<int16_t>(mx);
        cursorActive = 1;
        cursorY = static_cast<int16_t>(my);
        Redraw();
        return;
    }
    if (cursorActive == 0)
        return;
    cursorActive = 0;
    Redraw();
}
HOOK_VMETHOD(0x004283c0, CCourseGump, MouseOver);

// 0x00428670  size 0x3a3
// Mouse drag: move the gump when no pin was grabbed; otherwise redraw the pin being dragged (restoring the
// previous lines from the backup frame, then lines, pin sprite and numbers to the new point), and start the pin
// drag once the pointer moved more than 4 pixels.
void __thiscall CCourseGump::MouseMove(int px, int py)
{
    int aw, ah;
    char buf[8];
    int clip[4];
    uint16_t *bmp;
    clip[0] = 0;
    clip[1] = 0;
    clip[2] = width;
    clip[3] = height;

    if (dragPin != nullptr) {
        if (unk_c0 != 0) {
            if (segCount >= 1)
                Line_FromSource(g_fontObject, lastX, lastY, pts[0].x, pts[0].y, i32_ptr(frameData[1]), backupFrame);
            if (segCount >= 2)
                Line_FromSource(g_fontObject, lastX, lastY, pts[1].x, pts[1].y, i32_ptr(frameData[1]), backupFrame);
            unk_c0 = 0;
        }
        if (px >= 0 && px < width && py >= 0 && py < height) {
            if (selectedPin != nullptr && pinSelected == 0) {
                pinSelected = 1;
                Redraw();
            }
            GumpArtSize(0x139c, &aw, &ah);
            aw >>= 1;
            ah >>= 1;
            if (segCount >= 1) {
                Line_Shaded(g_fontObject, px, py, pts[0].x, pts[0].y, i32_ptr(frameData[1]));
                bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
                DrawGumpArt(ROWBLIT_COPY, 0x139c, pts[0].x - aw, pts[0].y - ah, bmp + 2, bmp[0], bmp[1]);
                sprintf(buf, "%d", ptNum[0]);
                bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
                FontDrawText(g_fontObject, pts[0].x - static_cast<int>(strlen(buf) << 4), pts[0].y - 5, 3, 0, buf, bmp + 2, width,
                             clip);
            }
            if (segCount >= 2) {
                Line_Shaded(g_fontObject, px, py, pts[1].x, pts[1].y, i32_ptr(frameData[1]));
                bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
                DrawGumpArt(ROWBLIT_COPY, 0x139c, pts[1].x - aw, pts[1].y - ah, bmp + 2, bmp[0], bmp[1]);
                sprintf(buf, "%d", ptNum[1]);
                bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
                FontDrawText(g_fontObject, pts[1].x - static_cast<int>(strlen(buf) << 4), pts[1].y - 5, 3, 0, buf, bmp + 2, width,
                             clip);
            }
            unk_c0 = 1;
            lastX = px;
            lastY = py;
        } else if (pinSelected == 1) {
            pinSelected = 0;
            Redraw();
        }

        if (mouseDown != 0 && clickState > 0) {
            if (abs(px - unk_6c) > 4 || abs(py - unk_70) > 4) {
                pinDragging = 1;
                clickState = 0;
                StartPinDrag(px, py);
            }
        }
    } else {
        Cursor_Set(g_appObject, 0);
        if (mouseDown != 0)
            Move(px - unk_6c, py - unk_70, 1, nullptr, 1);
    }
}
HOOK_VMETHOD(0x00428670, CCourseGump, MouseMove);

// 0x00428a20  size 0x69  timer: a single press older than the double-click time becomes an add-pin click, or the
// start of a pin drag when a pin was under the mouse.
void __thiscall CCourseGump::Tick()
{
    DWORD now = GetTickCount();
    DWORD dbl = GetDoubleClickTime();
    if (clickState != 1)
        return;
    if (now - clickTick <= dbl)
        return;
    int down = mouseDown;
    clickState = 0;
    if (down == 0) {
        AddPin(unk_6c, unk_70);
        return;
    }
    pinDragging = 1;
    StartPinDrag(unk_6c, unk_70);
}
HOOK_VMETHOD(0x00428a20, CCourseGump, Tick);

// 0x00429050  size 0xdf
// vtable slot 0x08: blit frame 0 at the gump's screen origin, clipped, then draw the child gumps.
// The rectangle size comes from the current frame (+0x60 selects frame 1) but the pixels are always frame 0.
void __thiscall CCourseGump::Draw(int *clip)
{
    int rect[4];
    int out[4];
    int sx = 0;
    int sy = 0;
    uint16_t *bmp;
    Gump_LocalToScreen(this, &sx, &sy);
    if (unk_60 == 0)
        bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
    else
        bmp = static_cast<uint16_t *>(i32_ptr(frameData[3]));
    rect[0] = sx;
    rect[1] = sy;
    rect[2] = sx + bmp[0];
    rect[3] = sy + bmp[1];
    if (Mgr_Intersects(g_gumpManager, clip, rect) == 0)
        return;
    Rect_Clip(clip, rect, out);
    uint16_t *frame0 = static_cast<uint16_t *>(i32_ptr(frameData[1]));
    GFX_BlitFrame(ROWBLIT_COPY, frame0 + 2, out, frame0[0], frame0[1], sx, sy, g_backBuffer, 0x280, 0x1e0);
    for (CGump *c = firstChild; c != nullptr; c = c->next)
        PORT_VSLOT1(c, 0x08, void, int *)(out);
}
HOOK_VMETHOD(0x00429050, CCourseGump, Draw);

// 0x00429130  size 0x5c  non-zero when the pixel at (x, y) of the current frame bitmap is not 0.
int __thiscall CCourseGump::HitTest(int px, int py)
{
    uint16_t *bmp;
    if (unk_60 == 0)
        bmp = static_cast<uint16_t *>(i32_ptr(frameData[1]));
    else
        bmp = static_cast<uint16_t *>(i32_ptr(frameData[3]));
    int w = bmp[0];
    int h = bmp[1];
    bmp += 2;
    if (px < 0 || py < 0 || px >= w || py >= h)
        return 0;
    return bmp[w * py + px] != 0;
}
HOOK_VMETHOD(0x00429130, CCourseGump, HitTest);
