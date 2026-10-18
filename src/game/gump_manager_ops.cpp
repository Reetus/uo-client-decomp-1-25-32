// CGumpManager methods, second pass: gump list sweeps, the memory DC, object lookup, the
// screen hit test, the per-frame update and the desktop save. gumps.cxx, 0x00439e90..0x0043cd60.
#include "gump_manager.h"

#include <windows.h>

#include <cstdio>

#include "../runtime/ported.h"
#include "app.h"
#include "bark_gump.h"
#include "globals.h"
#include "missile_gump.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "display_ops_ports.h"
#include "radar_gump.h"

#define g_charSlot GLOBAL_AS(unsigned, g_selectedCharSlot)
#define Mo_CreateBackBuffer() (CreateBackBuffer())
#define Mo_RadarUpdate(self) (reinterpret_cast<CRadarGump *>(self)->RedrawIfDue())
#define Mo_RadarBlink(self) (reinterpret_cast<CRadarGump *>(self)->BlinkPlayerPixel())

// 0x00439e90  size 0x79
// Shift every gump that answers slot 0xf0 (the missile gumps) by (dx, dy): its position, its
// sub-pixel position and its destination. RET 8.
void __thiscall CGumpManager::ShiftMissileGumps(int dx, int dy)
{
    for (CGump *g = listTail; g != nullptr; g = g->prev) {
        if (decomp::vcall(PORT_RET(int), g, 0xf0) != 0) {
            CMissileGump *m = static_cast<CMissileGump *>(g);
            m->posX = static_cast<double>(dx) + m->posX;
            m->x = m->x + dx;
            m->y = m->y + dy;
            m->destScreenX = m->destScreenX + dx;
            m->destScreenY = m->destScreenY + dy;
            m->posY = static_cast<double>(dy) + m->posY;
        }
    }
}
HOOK_METHOD(0x00439e90, CGumpManager, ShiftMissileGumps);

// 0x00439f10  size 0x44
// Delete every gump that answers slot 0xf0. The cursor 0x0064cd7c is advanced before each
// delete so the gump destructors can keep the sweep valid.
void __thiscall CGumpManager::DeleteMissileGumps()
{
    CGump *saved = g_gumpDeleteCursor;
    CGump *g = listTail;
    if (g != nullptr) {
        do {
            g_gumpDeleteCursor = g->prev;
            if (decomp::vcall(PORT_RET(int), g, 0xf0) != 0) {
                if (g != nullptr)
                    PORT_VSLOT1(g, 0x00, void, int)(1);
            }
            g = g_gumpDeleteCursor;
        } while (g != nullptr);
    }
    g_gumpDeleteCursor = saved;
}
HOOK_METHOD(0x00439f10, CGumpManager, DeleteMissileGumps);

// 0x00439ff0  size 0x44
// As 0x00439f10, for the gumps that answer slot 0x80.
void __thiscall CGumpManager::DeleteGumpsSlot80()
{
    CGump *saved = g_gumpDeleteCursor;
    CGump *g = listTail;
    if (g != nullptr) {
        do {
            g_gumpDeleteCursor = g->prev;
            if (decomp::vcall(PORT_RET(int), g, 0x80) != 0) {
                if (g != nullptr)
                    PORT_VSLOT1(g, 0x00, void, int)(1);
            }
            g = g_gumpDeleteCursor;
        } while (g != nullptr);
    }
    g_gumpDeleteCursor = saved;
}
HOOK_METHOD(0x00439ff0, CGumpManager, DeleteGumpsSlot80);

// 0x0043a040  size 0x5d
// When +0x5c is set: rebuild the back buffer, create a memory DC compatible with the main window
// (with the palette selected when 0x0069e0ac is set) and select the desktop bitmap at +0x4474 into it.
void __thiscall CGumpManager::CreateMemDC()
{
    if (unk_5c != 0) {
        Mo_CreateBackBuffer();   // 0x0047e1a0
        HDC wnd = GetDC(static_cast<HWND>(g_mainWindow));
        HDC dc = CreateCompatibleDC(wnd);
        g_memDC = dc;
        if (g_paletteOn != 0) {
            SelectPalette(dc, g_palette, 0);
            dc = g_memDC;
        }
        SelectObject(dc, RAWF(HGDIOBJ, this, 0x4474));
        unk_5c = 0;
    }
}
HOOK_METHOD(0x0043a040, CGumpManager, CreateMemDC);

// 0x0043a0a0  size 0x138
// Find the gump of `owner` among the siblings starting at `node` (following +0x3c) and, recursively,
// their children (starting at +0x44). A gump matches when its +0x34 is `owner` and one of the type
// predicates agrees with the owner's kind and the two flags. RET 0x10.
CGump *__thiscall CGumpManager::FindGumpForObject(CObj *obj, int flagA, int flagB, CGump *node)
{
    for (CGump *g = node; g != nullptr; g = g->prev) {
        if (obj == g->owner) {
            if (decomp::vcall(PORT_RET(int), g, 0xb4) != 0)
                return g;
            if (obj == g->owner) {
                if (decomp::vcall(PORT_RET(int), g, 0x9c) != 0)
                    return g;
            }
        }
        if (decomp::vcall(PORT_RET(int), obj, 0x30) != 0 && obj == g->owner && flagA == 0 && flagB == 0) {
            if (decomp::vcall(PORT_RET(int), g, 0x90) != 0)
                return g;
        }
        if (decomp::vcall(PORT_RET(int), obj, 0x24) != 0 && obj == g->owner && flagA != 0) {
            if (decomp::vcall(PORT_RET(int), g, 0x78) != 0)
                return g;
        }
        if (decomp::vcall(PORT_RET(int), obj, 0x24) != 0 && obj == g->owner && flagB != 0) {
            if (decomp::vcall(PORT_RET(int), g, 0xa0) != 0)
                return g;
        }
        if (decomp::vcall(PORT_RET(int), obj, 0x24) != 0 && obj == g->owner && flagA == 0 && flagB == 0) {
            if (decomp::vcall(PORT_RET(int), g, 0x78) == 0) {
                if (decomp::vcall(PORT_RET(int), g, 0xa0) == 0)
                    return g;
            }
        }
        if (g->lastChild != nullptr) {
            CGump *r = FindGumpForObject(obj, flagA, flagB, g->lastChild);   // 0x0043a0a0
            if (r != nullptr)
                return r;
        }
    }
    return nullptr;
}
HOOK_METHOD(0x0043a0a0, CGumpManager, FindGumpForObject);

// 0x0043a220  size 0x112
// Top-level gump hit test at screen (x, y), RET 0x10. With a captured gump (+0x14) only that gump
// is tried. Otherwise walk the list from the tail: a visible gump other than `exclude` whose slot
// 0x0c accepts the local point wins (via CGump::HitTest for the deepest child); gumps that answer
// slot 0x84 only count when `flag` is zero or their +0xb8 object answers slot 0x24 or 0x54.
CGump *__thiscall CGumpManager::GumpAtPoint(int px, int py, CGump *exclude, int flag)
{
    if (RAWF(CGump *, this, 0x14) != nullptr) {
        int lx = px;
        int ly = py;
        for (CGump *p = RAWF(CGump *, this, 0x14); p != nullptr; p = p->parent) {
            lx -= p->x;
            ly -= p->y;
        }
        if (decomp::vcall(PORT_RET(int), RAWF(CGump *, this, 0x14), 0x0c, lx, ly) != 0)
            return RAWF(CGump *, this, 0x14)->FindGumpAt(lx, ly, exclude);   // 0x0043b970
        return nullptr;
    }
    CGump *g = listTail;
    if (g == nullptr)
        return nullptr;
    for (;;) {
        int lx = px;
        int ly = py;
        for (CGump *p = g; p != nullptr; p = p->parent) {
            lx -= p->x;
            ly -= p->y;
        }
        if (g != exclude && g->visible != 0 && decomp::vcall(PORT_RET(int), g, 0x0c, lx, ly) != 0) {
            if (decomp::vcall(PORT_RET(int), g, 0x84) != 0) {
                if (flag == 0)
                    return g->FindGumpAt(lx, ly, exclude);   // 0x0043b970
                void *o = RAWF(void *, g, 0xb8);
                if (o != nullptr) {
                    if (decomp::vcall(PORT_RET(int), o, 0x24) != 0 || decomp::vcall(PORT_RET(int), RAWF(void *, g, 0xb8), 0x54) != 0)
                        return g->FindGumpAt(lx, ly, exclude);   // 0x0043b970
                }
            } else {
                return g->FindGumpAt(lx, ly, exclude);   // 0x0043b970
            }
        }
        g = g->prev;
        if (g == nullptr)
            return nullptr;
    }
}
HOOK_METHOD(0x0043a220, CGumpManager, GumpAtPoint);

// 0x0043aa10  size 0x8a
// Per-frame update. Delete the timed gumps (slot 0x84 true) whose age (now - +0x94) exceeds their
// lifetime (+0x98); update the radar and blink the player pixel; then call slot 0x5c of every gump,
// with the next gump kept in 0x004d333c so a gump may delete itself.
void __thiscall CGumpManager::Update()
{
    DWORD now = GetTickCount();
    CGump *g = listTail;
    while (g != nullptr) {
        CGump *nx = g->prev;
        if (decomp::vcall(PORT_RET(int), g, 0x84) != 0) {
            if (now - RAWF(uint32_t, g, 0x94) > RAWF(uint32_t, g, 0x98)) {
                if (g != nullptr)
                    PORT_VSLOT1(g, 0x00, void, int)(1);
            }
        }
        g = nx;
    }
    if (radarGump != nullptr) {
        Mo_RadarUpdate(radarGump);   // 0x0046afd0
        Mo_RadarBlink(radarGump);    // 0x0046adf0
    }
    CGump *c = listTail;
    if (c != nullptr) {
        do {
            g_updateCursor = c->prev;
            decomp::vcall_void(c, 0x5c);
            c = g_updateCursor;
        } while (c != nullptr);
    }
}
HOOK_METHOD(0x0043aa10, CGumpManager, Update);

// 0x0043aaa0  size 0x53
// For every top-level bark gump that is the newest of its speaker (+0xb0 null): re-anchor each bark
// of the chain (CBarkGump::Reposition, then +0xac), and when it has a speaker stack and clamp them.
void __thiscall CGumpManager::RepositionBarks()
{
    for (CGump *g = listTail; g != nullptr; g = g->prev) {
        if (decomp::vcall(PORT_RET(int), g, 0x84) != 0) {
            CBarkGump *b = static_cast<CBarkGump *>(g);
            if (b->newer == nullptr) {
                CBarkGump *s = b;
                if (b != nullptr) {
                    do {
                        s->Reposition();   // 0x00402090
                        s = s->older;
                    } while (s != nullptr);
                }
                if (b->speaker != nullptr)
                    b->StackAndClamp();   // 0x00402230
            }
        }
    }
}
HOOK_METHOD(0x0043aaa0, CGumpManager, RepositionBarks);

// 0x0043c670  size 0x4c
// Delete every gump that answers neither slot 0x70 nor slot 0x74 (cursor 0x0064cd7c as above).
void __thiscall CGumpManager::DeleteTransientGumps()
{
    CGump *saved = g_gumpDeleteCursor;
    CGump *g = listTail;
    if (g != nullptr) {
        do {
            g_gumpDeleteCursor = g->prev;
            if (decomp::vcall(PORT_RET(int), g, 0x70) == 0) {
                if (decomp::vcall(PORT_RET(int), g, 0x74) == 0) {
                    if (g != nullptr)
                        PORT_VSLOT1(g, 0x00, void, int)(1);
                }
            }
            g = g_gumpDeleteCursor;
        } while (g != nullptr);
    }
    g_gumpDeleteCursor = saved;
}
HOOK_METHOD(0x0043c670, CGumpManager, DeleteTransientGumps);

// 0x0043cd60  size 0x325
// Write the desktop layout file desktop.uoN (N = character slot 1..5; no file for other slots, the
// writes then go to a null FILE as in the original): the player's serial, then one record per
// gump in the list from the tail: type byte, x, y, a byte from +0x60, and for the resizable types
// the width and height as 16-bit values.
// Types: 0 player's paperdoll, 1 player's status, 2 slot 0x94, 8 slot 0x104 (byte from +0xac),
// 4 skill gump, 6 client config gump, 7 fighting gump.
void __thiscall CGumpManager::SaveDesktop()
{
    if (g_flag6f479c == 0)
        return;

    FILE *f = nullptr;
    char *name = nullptr;
    switch (g_charSlot) {
    case 0:
        name = const_cast<char *>("desktop.uo1");
        break;
    case 1:
        name = const_cast<char *>("desktop.uo2");
        break;
    case 2:
        name = const_cast<char *>("desktop.uo3");
        break;
    case 3:
        name = const_cast<char *>("desktop.uo4");
        break;
    case 4:
        name = const_cast<char *>("desktop.uo5");
        break;
    default:
        break;
    }
    if (name != nullptr)
        f = fopen(g_app.ResolveDataPath(name), "wb");   // 0x0044c340, 0x0048ad40

    uint8_t type;
    int32_t tmp4;
    int32_t tmp2;
    fwrite(&RAWF(int32_t, g_player, 0x78), 4, 1, f);   // 0x0048ba70
    for (CGump *g = listTail; g != nullptr; g = g->prev) {
        if (decomp::vcall(PORT_RET(int), g, 0x78) != 0 && g->owner == g_player) {
            type = 0;
        } else if (decomp::vcall(PORT_RET(int), g, 0xac) != 0 && g->owner == g_player) {
            type = 1;
        } else if (decomp::vcall(PORT_RET(int), g, 0x94) != 0) {
            type = 2;
            fwrite(&type, 1, 1, f);
            tmp4 = g->x;
            fwrite(&tmp4, 4, 1, f);
            tmp4 = g->y;
            fwrite(&tmp4, 4, 1, f);
            type = RAWF(uint8_t, g, 0x60);
            fwrite(&type, 1, 1, f);
            tmp2 = static_cast<uint16_t>(g->width);
            fwrite(&tmp2, 2, 1, f);
            tmp2 = static_cast<uint16_t>(g->height);
            fwrite(&tmp2, 2, 1, f);
            continue;
        } else if (decomp::vcall(PORT_RET(int), g, 0x104) != 0) {
            type = 8;
            fwrite(&type, 1, 1, f);
            tmp4 = g->x;
            fwrite(&tmp4, 4, 1, f);
            tmp4 = g->y;
            fwrite(&tmp4, 4, 1, f);
            type = RAWF(uint8_t, g, 0xac);
            fwrite(&type, 1, 1, f);
            continue;
        } else if (g == g_pSkillWindow) {
            type = 4;
            fwrite(&type, 1, 1, f);
            tmp4 = g->x;
            fwrite(&tmp4, 4, 1, f);
            tmp4 = g->y;
            fwrite(&tmp4, 4, 1, f);
            type = RAWF(uint8_t, g, 0x60);
            fwrite(&type, 1, 1, f);
            tmp2 = static_cast<uint16_t>(g->width);
            fwrite(&tmp2, 2, 1, f);
            tmp2 = static_cast<uint16_t>(g->height);
            fwrite(&tmp2, 2, 1, f);
            continue;
        } else if (g == GLOBAL_AS(CGump *, g_clientCfgGump)) {
            type = 6;
            fwrite(&type, 1, 1, f);
            tmp4 = g->x;
            fwrite(&tmp4, 4, 1, f);
            tmp4 = g->y;
            fwrite(&tmp4, 4, 1, f);
            type = RAWF(uint8_t, g, 0x60);
            fwrite(&type, 1, 1, f);
            continue;
        } else if (g == g_fightingGump) {
            type = 7;
        } else {
            continue;
        }
        // types 0, 1 and 7 share the plain record.
        fwrite(&type, 1, 1, f);
        tmp4 = g->x;
        fwrite(&tmp4, 4, 1, f);
        tmp4 = g->y;
        fwrite(&tmp4, 4, 1, f);
        type = RAWF(uint8_t, g, 0x60);
        fwrite(&type, 1, 1, f);
    }
    fclose(f);   // 0x0048aad0
}
HOOK_METHOD(0x0043cd60, CGumpManager, SaveDesktop);

// 0x0043a1e0  size 0x34
// Non-zero when the bark has a speaker and the speaker answers slot 0x24 or slot 0x54.
int __thiscall CBarkGump::SpeakerIsSpecial()
{
    void *s = speaker;
    if (s != nullptr) {
        if (decomp::vcall(PORT_RET(int), s, 0x24) != 0)
            return 1;
        return decomp::vcall(PORT_RET(int), speaker, 0x54) != 0 ? 1 : 0;
    }
    return 0;
}
HOOK_METHOD(0x0043a1e0, CBarkGump, SpeakerIsSpecial);
