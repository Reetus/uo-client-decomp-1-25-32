// CGumpManager (DrawGumps, CreateGump, LoadDesktop), CGump::Move,
// CGump::HitTest and two gump helpers. gumps.cxx, 0x0043a340..0x0043cd40.
//
// The SEH frames around operator new + ctor in the original are omitted.
#include "gump_manager.h"

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "gump_base_ops_ports.h"
#include "client_socket.h"
#include "gump_art_ops_ports.h"
#include "mobile_anim_queue.h"
#include "present_backbuffer_ports.h"
#include "shop_gump.h"
#include "text_gump.h"
#include "status_gump.h"
#include "client_misc_ops_ports.h"
#include "paperdoll_gump.h"
#include "radar_gump.h"
#include "client_cfg_gump.h"
#include "window_helpers_31_ports.h"
#include "mouse_manager.h"
#include "stack_gump.h"
#include "container_gump.h"
#include "gump.h"
#include "packet_builders_batch24_ports.h"
#include "fighting_gump.h"
#include "spell_icon_gump.h"
#include "waiting_gump.h"
#include "book_gump.h"

// ---------------------------------------------------------------------------
// Globals (single-file use).
// ---------------------------------------------------------------------------
#define g_ringScratchA GLOBAL_AS(int, g_ringBase)
#define g_flushSrc GLOBAL_PTR(char *, g_flushTabB)       // per entry source pointer
#define g_flushDst GLOBAL_PTR(char *, g_flushTabA)       // per entry destination pointer
#define g_deleteCursor GLOBAL_AS(int, g_gumpDeleteCursor)      // iteration cursor shared with gump dtors
#define g_gump6488d4 GLOBAL_AS(void *, g_scrollChoiceGump)
#define g_playerPtr GLOBAL_AS(uint8_t *, g_player)   // g_player as raw bytes

// Network objects.
#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)

// Original functions not ported yet.
static void rect_union(int *dirty, int *rc) { PORT_THISCALL2(&GumpRectUnion, void, g_gumpManager, int *, int *)(dirty, rc); }
static void bring_to_front(void *g) { decomp::thiscall_at_void(PORT_ADDR(&CGump::BringToFront), g, 0); }   // RET 4: (int)
static void minimize(void *g) { decomp::thiscall_at_void(PORT_ADDR(&CBookGump::Iconify), g); }
static void restore_notify(void *g) { decomp::thiscall_at_void(PORT_ADDR(&CBookGump::Uniconify), g); }
static void send_packet(const char *pkt) { reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(pkt)); }
static void add_child(void *g, CGump *parent) { CGump_AddChild(g, parent, 1); }
static void art_size(int type, int *w, int *h) { GumpArtSizeImpl(type, w, h); }

// ---------------------------------------------------------------------------
// 0x0043a340  size 0x3da
// DrawGumps: redraw a screen rect into the ring back buffer.
// ---------------------------------------------------------------------------

// The "flush tail": record the dirty rect and copy the pending rects from the
// ring buffer to the screen. It occurs in the machine code at 0x43a41e and
// 0x43a61e.
static void flush_dirty(CGumpManager *m, const int *rc)
{
    if (m->unk_5c == 0) {
        g_dirtyL = rc[0];
        g_dirtyT = rc[1];
        g_dirtyR = rc[2];
        g_dirtyB = rc[3];
        if (rc[1] < 0)
            g_dirtyT = 0;
        if (rc[0] < 0)
            g_dirtyL = 0;
        if (rc[3] > 0x1e0)
            g_dirtyB = 0x1e0;
        if (rc[2] > 0x280)
            g_dirtyR = 0x280;
        MobileAnim_TickAll();   // ? conventions of 0x00430be0 / 0x00482ca0 not annotated in pass2 (no ECX noted)
        Present_BackBuffer();
    }
    int i = g_flushCount - 1;
    while (i >= 0) {
        int *e = g_flushRects + i * 4;
        if (i > 100)
            break;
        int row = e[1];
        char *src = g_flushSrc[i];
        char *dst = g_flushDst[i];
        if (row < e[3]) {
            do {
                unsigned int n = (unsigned int)(e[2] - e[0]) * 2;
                memcpy(dst, src, n);
                dst += 0x500;
                src += 0x500;
                row++;
            } while (row < e[3]);
        }
        i--;
    }
    g_flushCount = 0;
}

void __thiscall CGumpManager::DrawGumps(int *rc, int flush)
{
    int top = rc[1];
    int bottom = rc[3];
    if (bottom <= top) {
        if (flush != 0)
            flush_dirty(this, rc);
        return;
    }
    int right = rc[2];
    int left = rc[0];
    if (right <= left) {
        if (flush != 0)
            flush_dirty(this, rc);
        return;
    }

    int startOff = left + top * 0x280;
    int endOff = bottom * 0x280 + right - 0x281;
    int wrap = 0x4b000 - g_scrollOffset;

    bool draw;
    if (startOff < wrap)
        draw = (endOff < wrap);
    else
        draw = (endOff >= wrap);

    if (draw) {
        uint16_t *savedBackBuf = g_backBuffer;
        int adj = g_scrollOffset;
        if (startOff >= wrap)
            adj -= 0x4b000;
        adj = adj + adj;   // byte offset (2 bytes per pixel)
        g_backBuffer = reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(g_backBuffer) + adj);
        g_ringScratchA = g_ringScratchB + adj;
        for (CGump *g = listHead; g != nullptr; g = g->next)
            PORT_VSLOT1(g, 0x04, void, int *)(rc);   // vtable slot 1, pass 1
        for (CGump *g = listHead; g != nullptr; g = g->next)
            PORT_VSLOT1(g, 0x08, void, int *)(rc);   // vtable slot 2, pass 2
        g_backBuffer = savedBackBuf;
        if (flush == 0)
            return;
        flush_dirty(this, rc);
        return;
    }

    // split at the ring-buffer wrap point
    int wrapCol = wrap % 0x280;
    int wrapRow = wrap / 0x280;
    int sub[4];
    if (wrapCol > left) {
        if (wrapCol < right) {
            // four-way split
            sub[0] = left;
            sub[1] = top;
            sub[2] = wrapCol;
            sub[3] = wrapRow + 1;
            flush = wrapRow + 1;   // [ebp+0xc] overwritten
            DrawGumps(sub, 0);
            sub[0] = wrapCol;
            sub[2] = rc[2];
            sub[3] = wrapRow;
            DrawGumps(sub, 0);
            sub[3] = rc[3];
            sub[1] = wrapRow;
            DrawGumps(sub, 0);
            sub[0] = rc[0];
            sub[1] = flush;   // wrapRow + 1
            sub[2] = wrapCol;
            DrawGumps(sub, 1);
            return;
        }
        wrapRow = wrapRow + 1;
    } else {
        if (!(wrapCol < right))
            wrapRow = wrapRow + 1;
    }
    sub[0] = left;
    sub[1] = top;
    sub[2] = right;
    sub[3] = wrapRow;
    DrawGumps(sub, 0);
    sub[3] = rc[3];
    sub[1] = wrapRow;
    DrawGumps(sub, 1);
}
HOOK_METHOD(0x0043a340, CGumpManager, DrawGumps);

// ---------------------------------------------------------------------------
// 0x0043ab00  size 0x255
// Item art id -> container gump id. The machine code is a nested range tree
// plus four jump tables; the switch returns the identical value for every
// 16-bit input.
// ---------------------------------------------------------------------------
int __stdcall ContainerGumpIdForItem(unsigned int itemId)
{
    int id = itemId & 0xffff;
    switch (id) {
    case 0x9a8: return 0x4b;
    case 0x990: return 0x41;

    // table 0x43ad58 (0x9a9..0x9b2)
    case 0x9a9: return 0x44;
    case 0x9aa: return 0x43;
    case 0x9ab: return 0x4a;
    case 0x9ac: return 0x41;
    case 0x9ad: case 0x9ae: case 0x9af: return 9;
    case 0x9b0: return 0x3c;
    case 0x9b1: return 0x41;
    case 0x9b2: return 0x3c;

    case 0xa2c: case 0xa2d: case 0xa2e: case 0xa2f:
    case 0xa34: case 0xa35: case 0xa36: case 0xa37:
    case 0xa3c: case 0xa3d: case 0xa3e: case 0xa3f: case 0xa40: case 0xa41: case 0xa42: case 0xa43:
    case 0xa44: case 0xa45: case 0xa46: case 0xa47: case 0xa48: case 0xa49: case 0xa4a: case 0xa4b:
        return 0x51;
    case 0xa30: case 0xa31: case 0xa32: case 0xa33:
    case 0xa38: case 0xa39: case 0xa3a: case 0xa3b:
        return 0x48;

    // table 0x43ad80 (0xa4c..0xa53)
    case 0xa4c: case 0xa4d: case 0xa50: case 0xa51: return 0x4e;
    case 0xa4e: case 0xa4f: case 0xa52: case 0xa53: return 0x4f;

    case 0xa97: case 0xa98: case 0xa99: case 0xa9a: case 0xa9b: case 0xa9c: case 0xa9d: case 0xa9e:
        return 0x4d;

    case 0xe1c: return 0x92e;
    case 0xe3c: case 0xe3d: case 0xe3e: case 0xe3f: return 0x44;

    // table 0x43ada0 (0xe40..0xe43)
    case 0xe40: case 0xe41: return 0x42;
    case 0xe42: case 0xe43: return 0x49;

    case 0xe75: return 0x3c;

    // table 0x43adb0 (0xe76..0xe83)
    case 0xe76: return 0x3d;
    case 0xe77: return 0x3e;
    case 0xe78: return 0x3e;
    case 0xe79: return 0x3c;
    case 0xe7a: return 0x3f;
    case 0xe7b: return 0x3e;
    case 0xe7c: return 0x4a;
    case 0xe7d: return 0x43;
    case 0xe7e: return 0x44;
    case 0xe7f: return 0x3e;
    case 0xe80: return 0x4b;
    case 0xe81: return 9;
    case 0xe82: return 9;
    case 0xe83: return 0x3e;

    case 0xfa6: return 0x91a;
    case 0x1008: return 0x3e;
    case 0xfab: return 0x3e;
    case 0xfad: return 0x92e;
    case 0xfae: return 0x3e;
    case 0x1940: return 0x3e;
    case 0x154d: return 0x3e;
    case 0x1ad6: case 0x1ad7: return 0x3e;
    case 0x1e5e: return 0x52;
    case 0x2af8: return 0x47;
    case 0x3e65: case 0x3e93: case 0x3eae: case 0x3eb9: return 0x4c;
    default: return 9;
    }
}
HOOK_FUNC(0x0043ab00, ContainerGumpIdForItem);

// ---------------------------------------------------------------------------
// 0x0043b140  size 0x394
// CGump::Move: moves a gump, accumulates the dirty rect, moves owned gumps.
// Size fields are +0x1c/+0x20 (or +0x24/+0x28 when unk_60 != 0).
// ---------------------------------------------------------------------------
void __thiscall CGump::Move(int dx, int dy, int isRoot, int *dirtyRect, int clamp)
{
    CGump *scrollOwner = nullptr;
    int sbLeft = 0;
    int sbRight = 0;
    int sbTop = 0;
    int sbBottom = 0;
    int w, h;
    int rc[4];
    int rootRect[4];
    int newY;

    if (unk_60 == 0) {
        w = width;
        h = height;
    } else {
        w = unk_24;
        h = unk_28;
    }

    int sumX = 0, sumY = 0;
    for (CGump *p = this; p != nullptr; p = p->parent) {
        sumX += p->x;
        sumY += p->y;
    }
    rc[0] = sumX;
    rc[1] = sumY;
    rc[2] = sumX + w;
    rc[3] = h + sumY;
    int oldX = x;
    int oldY = y;

    if (isRoot == 0) {
        rect_union(dirtyRect, rc);
    } else {
        rootRect[0] = sumX;
        rootRect[1] = sumY;
        rootRect[2] = rc[2];
        rootRect[3] = rc[3];
        dirtyRect = rootRect;
        if (decomp::vcall(PORT_RET(int), this, 0xac) != 0 && RAWF(void *, this, 0xf0) != nullptr) {
            // scroll bar at this+0xf0 and its four edge gumps
            uint8_t *sb = RAWF(uint8_t *, this, 0xf0);
            sbLeft = RAWF(CGump *, sb, 0x10c)->x;
            sbRight = RAWF(CGump *, sb, 0x110)->x;
            sbTop = RAWF(CGump *, sb, 0x114)->y;
            sbBottom = RAWF(CGump *, sb, 0x118)->y;
            scrollOwner = this;
        }
        int nx = x + dx;
        x = nx;
        int ny = y + dy;
        y = ny;
        newY = ny;
        if (clamp != 0) {
            if (scrollOwner != nullptr) {
                if (sbRight + dx > 0x24d)
                    x = ((nx - dx) - sbRight) + 0x24d;
                if (dx + sbLeft < 0x32 - w)
                    x = x + (((0x32 - dx) - w) - sbLeft);
                if (sbBottom + dy > 0x1ad)
                    y = ((newY - dy) - sbBottom) + 0x1ad;
                if (sbTop + dy < 0x32 - h)
                    y = y + (((0x32 - h) - dy) - sbTop);
            } else {
                int keepX = w;
                if (keepX > 0x32)
                    keepX = 0x32;
                int keepY = 0x32;
                if (h <= 0x32)
                    keepY = h;
                if (nx > 0x280 - keepX)
                    x = 0x280 - keepX;
                if (x < keepX - w)
                    x = keepX - w;
                if (newY > 0x1e0 - keepY)
                    y = 0x1e0 - keepY;
                if (y < keepY - h)
                    y = keepY - h;
            }
        }
    }

    // children
    for (CGump *c = lastChild; c != nullptr; c = c->prev)
        c->Move(dx, dy, 0, dirtyRect, clamp);

    // gumps in the global list owned by this one move with it
    for (CGump *g = g_gumpListTail; g != nullptr; g = g->prev) {
        if (decomp::vcall(PORT_RET(int), g, 0x84) != 0 && RAWF(CGump *, g, 0xb4) == this) {
            rc[0] = g->x;
            rc[1] = g->y;
            rc[2] = g->width + g->x;
            rc[3] = g->height + g->y;
            rect_union(dirtyRect, rc);
            g->x = g->x + (x - oldX);
            g->y = g->y + (y - oldY);
            rc[0] = rc[0] + (x - oldX);
            rc[2] = rc[2] + (x - oldX);
            rc[1] = rc[1] + (y - oldY);
            rc[3] = rc[3] + (y - oldY);
            rect_union(dirtyRect, rc);
        }
    }

    // new rect
    int nx = 0, ny = 0;
    for (CGump *p = this; p != nullptr; p = p->parent) {
        nx += p->x;
        ny += p->y;
    }
    rc[0] = nx;
    rc[1] = ny;
    rc[2] = w + nx;
    rc[3] = h + ny;
    rect_union(dirtyRect, rc);
    if (dirtyRect[0] < 0)
        dirtyRect[0] = 0;
    if (dirtyRect[1] < 0)
        dirtyRect[1] = 0;
    if (dirtyRect[2] > 0x27f)
        dirtyRect[2] = 0x27f;
    if (dirtyRect[3] > 0x1df)
        dirtyRect[3] = 0x1df;
}
HOOK_METHOD(0x0043b140, CGump, Move);

// ---------------------------------------------------------------------------
// 0x0043b970  size 0xf4
// CGump::HitTest: deepest child gump under a point, or 0. Written from the
// asm control flow: the working point is moved to absolute coordinates for
// the current gump and back to local coordinates for each candidate child.
// ---------------------------------------------------------------------------
CGump *__thiscall CGump::FindGumpAt(int px, int py, CGump *skip)
{
    CGump *cur = this;
    int edi = px;
    int ebx = py;

    if (decomp::vcall(PORT_RET(int), cur, 0x0c, edi, ebx) == 0)   // slot 3: contains(x, y)
        return nullptr;

    for (;;) {
        // add the positions of cur and its ancestors to the working point
        if (cur != nullptr) {
            CGump *p = cur;
            do {
                int qx = p->x;
                int qy = p->y;
                p = p->parent;
                edi += qx;
                ebx += qy;
            } while (p != nullptr);
            py = ebx;
            px = edi;
        }

        // scan the children of cur, from the last one back
        CGump *child = cur->lastChild;
        for (;;) {
            if (child == nullptr)
                return cur;   // no deeper child
            int isFlagged = decomp::vcall(PORT_RET(int), child, 0xa8);   // slot 42
            if (child->visible != 0) {
                bool reject = false;
                if (isFlagged != 0) {
                    if (RAWF(int, child, 0xb4) == cur->unk_60)
                        isFlagged = 0;
                    if (isFlagged != 0)
                        reject = true;
                }
                if (!reject) {
                    uint8_t tag = child->layer;
                    if (tag == 0 || tag == cur->layer) {
                        // convert to the child's local space
                        CGump *q = child;
                        do {
                            int qx = q->x;
                            int qy = q->y;
                            q = q->parent;
                            edi -= qx;
                            ebx -= qy;
                        } while (q != nullptr);
                        if (child != skip) {
                            if (decomp::vcall(PORT_RET(int), child, 0x0c, edi, ebx) != 0) {
                                // hit: descend into this child
                                skip = nullptr;
                                py = ebx;
                                px = edi;
                                cur = child;
                                if (decomp::vcall(PORT_RET(int), child, 0x0c, edi, ebx) == 0)
                                    return nullptr;
                                ebx = py;
                                edi = px;
                                break;   // descend
                            }
                        }
                        ebx = py;
                        edi = px;
                    }
                }
            }
            child = child->prev;
        }
    }
}
HOOK_METHOD(0x0043b970, CGump, FindGumpAt);

// ---------------------------------------------------------------------------
// 0x0043bd20  size 0x57d
// Gump factory. SEH state writes omitted (handler 0x49a3a9).
// ---------------------------------------------------------------------------
CGump *__thiscall CGumpManager::CreateGump(CObj *obj, int type, CGump *parent, int containerFlag)
{
    int isPaperdoll = (type == 10);
    int is0x30 = (type == 0x30);
    CGump *g = nullptr;

    if (obj != nullptr) {
        // find an existing gump for this object
        g = this->CGumpManager::FindGumpForObject(obj, isPaperdoll, is0x30, listTail);
        if (g != nullptr) {
            bring_to_front(g);
            if (g->unk_60 != 0)
                restore_notify(g);
            return g;
        }
    }

    if (type == 0x1388) {
        CGump *cfg = configGump;
        if (cfg != nullptr) {
            bring_to_front(cfg);
            cfg = configGump;
            if (cfg->unk_60 != 0)
                restore_notify(cfg);
            return configGump;
        }
    }

    if (type == 0x1392 || type == 0x1393) {
        CGump *radar = radarGump;
        if (radar != nullptr) {
            PORT_VSLOT1(radar, 0x00, void, int)(1);   // deleting dtor
            radarGump = nullptr;
            g_gumpMgr.CreateGump(nullptr, 0x1393, nullptr, 0);
            return radarGump;
        }
    }

    if (type == 7) {
        for (CGump *n = listTail; n != nullptr; n = n->prev) {
            if (decomp::vcall(PORT_RET(int), n, 0x94) != 0) {
                bring_to_front(n);
                if (n->unk_60 != 0)
                    restore_notify(n);
                return n;
            }
        }
    }

    if (type == 0x30) {
        CGump *n = listTail;
        int saved = g_deleteCursor;
        if (n != nullptr) {
            do {
                g_deleteCursor = (int)(intptr_t)n->prev;
                if (decomp::vcall(PORT_RET(int), n, 0xa0) != 0 && n != nullptr)
                    PORT_VSLOT1(n, 0x00, void, int)(1);   // deleting dtor
                n = reinterpret_cast<CGump *>((intptr_t)g_deleteCursor);
            } while (n != nullptr);
        }
        g_deleteCursor = saved;
    }

    int w;
    int h;
    art_size(type, &w, &h);   // art size of gump id `type`
    int ebxType = type;

    // Constructors of the gump classes are not ported; each is called on
    // freshly allocated memory.
    bool useDefault = false;
    if (ebxType <= 0x30) {
        if (ebxType == 0x30) {
            g = new CShopGump(0x96, 5, obj);
        } else if (ebxType == 7) {
            g = new CTextGump(0, 0);
        } else if (ebxType == 8) {
            g = new CStatusGump(obj, (obj != g_player && Misc_Return0() == 0) ? 1 : 0);   // ? calling convention of 0x00478cd0 not annotated
        } else if (ebxType == 10) {
            g = new CPaperdollGump(0x280 - w, 0, (int)(parent), (void *)(obj), 1, w, h, (char *)(g_emptyString), (uint8_t)(0));
        } else {
            useDefault = true;
        }
    } else if (ebxType > 0x1388) {
        if (ebxType == 0x1392) {
            g = new CRadarGump(0, 0, 0x1392);
        } else if (ebxType == 0x1393) {
            g = new CRadarGump(0, 0, 0x1393);
        } else {
            useDefault = true;
        }
    } else if (ebxType == 0x1388) {
        int cfgW, cfgH, cfgW2, cfgH2;
        art_size(0x1388, &cfgW, &cfgH);
        art_size(0x1389, &cfgW2, &cfgH2);
        if (cfgW != 0 && cfgW2 != 0) {
            g = new CClientCfgGump(0x96, 0x96);
        } else {
            ShowMessageBoxAndExit(
                g_004a8374, g_004a81bc, 0);
            return nullptr;
        }
    } else if (ebxType == 0x31) {
        int a, b;
        reinterpret_cast<CMouseManager *>(GLOBAL_PTR(void, g_mouse_storage))->GetMousePos(&a, &b);
        g = new CStackGump(a - 0x32, b - 0x25, obj);
    } else {
        useDefault = true;
    }

    if (useDefault) {
        if (containerFlag != 0) {
            if (g_004a759c != 0) {
                int nx = cascadeX + 0x10;
                int ny = cascadeY + 0x10;
                cascadeX = nx;
                cascadeY = ny;
                if (w + nx >= 0x270)
                    cascadeX = 0x10;
                if (h + ny >= 0x1d0)
                    cascadeY = 0x10;
                g = new CContainerGump(cascadeX, cascadeY, parent, obj, 1, w, h, ebxType);
            } else {
                g = new CContainerGump(0x276 - w, 0xa, parent, obj, 1, w, h, ebxType);
            }
        } else {
            g = new CGump(0x280 - w, 0, (int)(parent), obj, 1, w, h);
        }
    }

    add_child(g, parent);   // runs even when g == 0
    return g;
}
HOOK_METHOD(0x0043bd20, CGumpManager, CreateGump);

// ---------------------------------------------------------------------------
// 0x0043c6c0  size 0x665
// LoadDesktop: load "desktop.uoN" and recreate the saved gumps.
// SEH handler 0x49a3ec omitted. Saved entries live at this+0x70+0x1c*n.
// Record 0 with the player flag set reads x and y only (no flag byte), as in
// the binary.
// ---------------------------------------------------------------------------
void __thiscall CGumpManager::LoadDesktop()
{
    FILE *fp = nullptr;
    int x, y, ix;
    uint32_t serial;
    uint8_t rec;
    uint16_t s;
    char pkt[16];

    switch (g_selectedCharSlot) {
    case 0: fp = fopen(g_app.ResolveDataPath(const_cast<char *>("desktop.uo1")), g_fopenReadMode); break;
    case 1: fp = fopen(g_app.ResolveDataPath(const_cast<char *>("desktop.uo2")), g_fopenReadMode); break;
    case 2: fp = fopen(g_app.ResolveDataPath(const_cast<char *>("desktop.uo3")), g_fopenReadMode); break;
    case 3: fp = fopen(g_app.ResolveDataPath(const_cast<char *>("desktop.uo4")), g_fopenReadMode); break;
    case 4: fp = fopen(g_app.ResolveDataPath(const_cast<char *>("desktop.uo5")), g_fopenReadMode); break;
    default: fp = nullptr; break;
    }
    if (fp == nullptr)
        return;

    int one = 1;
    fread(&serial, 4, 1, fp);
    if (serial != RAWF(uint32_t, g_playerPtr, 0x78)) {
        fclose(fp);
        return;
    }
    desktopLoaded = 1;
    entryCount = 0;

    while (fread(&rec, 1, 1, fp) > 0) {
        switch (rec) {
        case 0: {
            if (decomp::vcall(PORT_RET(int), g_playerPtr, 0x8c) != 0) {
                fread(&ix, 4, 1, fp);
                fread(&ix, 4, 1, fp);
                break;
            }
            int n = entryCount;
            entries[n].type = rec;
            fread(&ix, 4, 1, fp);
            entries[n].x = ix;
            fread(&ix, 4, 1, fp);
            entries[n].y = ix;
            fread(&rec, 1, 1, fp);
            entries[n].flag = (rec == 1);
            entryCount = entryCount + 1;
            PORT_THISCALL2(&Build_Packet_0x06, void, g_packetBuilder, char *, uint32_t)(pkt, RAWF(uint32_t, g_playerPtr, 0x78));
            send_packet(pkt);
            break;
        }
        case 1: {
            fread(&x, 4, 1, fp);
            fread(&y, 4, 1, fp);
            fread(&rec, 1, 1, fp);
            int small = (rec == 1);
            CGump *g = new CStatusGump(g_playerPtr, small);
            g->x = x;   // dereferenced even when g == 0, as in the original
            g->y = y;
            add_child(g, nullptr);
            PORT_THISCALL3(&Build_Packet_0x34, void, g_packetBuilder, char *, int, uint32_t)(pkt, 4, RAWF(uint32_t, g_playerPtr, 0x78));
            send_packet(pkt);
            break;
        }
        case 2: {
            fread(&x, 4, 1, fp);
            fread(&y, 4, 1, fp);
            fread(&rec, 1, 1, fp);
            int minimized = (rec == 1);
            fread(&s, 2, 1, fp);
            unsigned int s1 = s;
            fread(&s, 2, 1, fp);
            unsigned int s2 = s;
            CGump *g = g_gumpMgr.CreateGump(nullptr, 7, nullptr, 0);
            PORT_VSLOT2(g, 0x38, void, unsigned int, unsigned int)(s1 & 0xffff, s2 & 0xffff);
            if (minimized != 0)
                minimize(g);
            g->x = x;
            g->y = y;
            break;
        }
        case 4: {
            int n = entryCount;
            entries[n].type = rec;
            fread(&ix, 4, 1, fp);
            entries[n].x = ix;
            fread(&ix, 4, 1, fp);
            entries[n].y = ix;
            fread(&rec, 1, 1, fp);
            entries[n].flag = (rec == 1);
            fread(&s, 2, 1, fp);
            entries[n].s1 = s;
            fread(&s, 2, 1, fp);
            entries[n].s2 = s;
            entryCount = entryCount + 1;
            PORT_THISCALL3(&Build_Packet_0x34, void, g_packetBuilder, char *, int, uint32_t)(pkt, 5, RAWF(uint32_t, g_playerPtr, 0x78));
            send_packet(pkt);
            g_desktopChanged = one;
            break;
        }
        case 6: {
            fread(&x, 4, 1, fp);
            fread(&y, 4, 1, fp);
            fread(&rec, 1, 1, fp);
            int minimized = (rec == 1);
            CGump *g = g_gumpMgr.CreateGump(nullptr, 0x1388, nullptr, 0);
            if (minimized != 0)
                minimize(g);
            g->x = x;
            g->y = y;
            break;
        }
        case 7: {
            fread(&x, 4, 1, fp);
            fread(&y, 4, 1, fp);
            fread(&rec, 1, 1, fp);
            if (rec == 1) {
                CGump *g = new CFightingGump(0, 0);
                g_fightingGump = g;
                minimize(g);
                g_fightingGump->x = x;
                g_fightingGump->y = y;
            } else {
                CGump *g = new CFightingGump(x, y);
                g_fightingGump = g;
            }
            add_child(g_fightingGump, nullptr);
            break;
        }
        case 8: {
            fread(&x, 4, 1, fp);
            fread(&y, 4, 1, fp);
            fread(&rec, 1, 1, fp);
            CSpellIconGump *g = new CSpellIconGump(x, y, rec);
            add_child(g, nullptr);
            break;
        }
        default:   // types 3, 5 and > 8 are ignored
            break;
        }
    }

    fclose(fp);
    if (entryCount == 0)
        desktopLoaded = 0;
}
HOOK_METHOD(0x0043c6c0, CGumpManager, LoadDesktop);

// ---------------------------------------------------------------------------
// 0x00439f60  size 0x83
// Create a 0x2ac-byte gump, store it in 0x006488d4, add it as a top-level gump.
// ---------------------------------------------------------------------------
void *__stdcall CreateGump2ac(int p1, int p2, int p3, int p4)
{
    // 0x0043d740: thiscall on the new object with 7 stack args
    CGump *g = new CGump_0043de30(-1, -1, (const char *)(p1), p2, 0, p3, p4);
    g_gump6488d4 = g;
    g->LinkInto(nullptr, 1);   // runs even when the allocation failed
    return g;
}
HOOK_FUNC(0x00439f60, CreateGump2ac);
