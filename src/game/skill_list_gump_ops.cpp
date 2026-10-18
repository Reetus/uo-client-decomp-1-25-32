// CSkillListGump (the skills window, vtable 0x004a0600): constructor, destructor, mouse and key handling, draw,
// hit test, and the vertically tiled frame painter 0x00472be0. Batch 28.
// The SEH frames of the constructor and destructor (try levels around each operator new) are omitted; every
// "operator new returned NULL" branch of the original is kept (the object pointer becomes 0).
// The sequence "write skillgrp.mul" is inlined at several places of the original; it is written out at each.
#include "skill_list_gump.h"

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "batch01_helpers.h"
#include "button_gump.h"
#include "edit_group_gump.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "scroll_thumb_gump.h"
#include "char_creation_events_ports.h"
#include "gump_manager.h"
#include "text_gump_ctor_ports.h"
#include "book_gump.h"
#include "ui_callbacks_ports.h"
#include "char_creation_pages.h"
#include "vtables.h"
#include "input_control_gump.h"

#define g_focusedControl GLOBAL_AS(CGump *, g_focusedInput)
#define g_journal GLOBAL_PTR(void, g_msgTarget_storage)

// 0x00418bd0: plain RET, does not read ECX (called right after the base constructor).
#define Fn_00418bd0() (LoadSkillNameTable())
// 0x00439f60 on the gump manager: alert gump (text, 0, 0, 0), RET 0x10.
#define Gump_Alert(o, a1, a2, a3, a4) PORT_THISCALL4(&CreateGump2ac, void, (o), const char *, int, int, int)(a1, a2, a3, a4)
// 0x00479480 on the journal object: system message (a, b, text), RET 0xc.
#define Journal_Message(o, a1, a2, a3) PORT_THISCALL3(&CreateBarkGump, void, (o), int, int, const char *)(a1, a2, a3)
// 0x0040d410: CBookGump::Uniconify, ECX = the gump.
#define Gump_Uniconify(self) (reinterpret_cast<CBookGump *>(self)->Uniconify())

int __stdcall GumpRectIntersects(const int *a, const int *b);
void __cdecl SaveSkillGroups();   // skill_groups.cpp, 0x00472b50

static inline CScrollThumbGump *skill_thumb(CSkillListGump *g)
{
    return reinterpret_cast<CScrollThumbGump *>(static_cast<uintptr_t>(static_cast<uint32_t>(g->unk_94)));
}

// frame0 (unk_60 == 0) or frame1 bitmap.
static inline uint16_t *skill_frame(CSkillListGump *g)
{
    if (g->unk_60 == 0)
        return reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(g->frameData[1])));
    return reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(g->frameData[3])));
}

// The first half of the inlined "commit the group rename" sequence (before its redraw): copy the edit text over
// the group name when it is not empty, delete the edit control, clear the rename state.
static inline void skill_commit_name(CSkillListGump *g)
{
    char *nameText = reinterpret_cast<char *>(g->renameEdit + 0xec);
    if (strlen(nameText) != 0)
        strcpy(g_groupNames + g->renameGroup * 0x11, nameText);
    if (g->renameEdit != nullptr)
        PORT_VSLOT1(g->renameEdit, 0x00, void, int)(1);
    g->renameEdit = nullptr;
    g->renaming = 0;
    g->renameGroup = -1;
}

// 0x00472be0  size 0x1b5
// Paint a vertically tiled frame into a bitmap. ECX = art id of the top piece, EDX = number of repeating middle
// pieces, stack = (bitmap, y, bottom padding), RET 0xc. Pieces are art ids a (top), a+1 .. a+b (middle, cycled)
// and a+b+1 (bottom).
void __fastcall SkillList_DrawTiledFrame(int artTop, int middleCount, uint16_t *bmp, int y, int bottomPad)
{
    int w0, h0, w1, h1, w2, h2;
    int clip[4];
    int xOff = 0;
    int w = bmp[0];
    int h = bmp[1];
    int idx;
    uint16_t *pixels;
    int bottom;
    int artBottom;

    gfx::ArtSize(artTop, &w0, &h0);
    artBottom = middleCount + artTop + 1;
    gfx::ArtSize(artBottom, &w2, &h2);
    bottom = h - h2 - bottomPad;
    pixels = bmp + 2;
    gfx::DrawArt(gfx::artPlain(), artTop, 0, y, pixels, w, h);
    int mid = artTop + 1;
    gfx::ArtSize(mid, &w1, &h1);
    if (w1 < w0)
        xOff = (w0 - w1) / 2;
    if (w1 != 0) {
        clip[0] = 0;
        clip[1] = h0;
        clip[2] = w;
        clip[3] = bottom + 1;
        idx = 0;
        int cy = h0 + y - 1;
        if (cy < bottom) {
            do {
                int art = mid + idx;
                gfx::ArtSize(art, &w1, &h1);
                if (cy + h1 > bottom)
                    gfx::DrawArtClipped(gfx::artPlain(), art, clip, xOff, cy, pixels, w, h);
                else
                    gfx::DrawArt(gfx::artPlain(), art, xOff, cy, pixels, w, h);
                cy += h1;
                idx++;
                if (idx == middleCount)
                    idx = 0;
            } while (cy < bottom);
        }
        gfx::ArtSize(artBottom, &w1, &h1);
        if (w1 == w0)
            xOff = 0;
        gfx::DrawArt(gfx::artPlain(), artBottom, xOff, bottom - 1, pixels, w, h);
    }
}
HOOK_FUNC(0x00472be0, SkillList_DrawTiledFrame);

// 0x00472da0  size 0x5e1
// Constructor of the skills window: frame0 (art 0x820 wide, height 300) and frame1 (art 0x839), the iconify,
// up, add-group, down and list buttons, the scroll thumb (unk_94); restores a saved desktop position.
CSkillListGump::CSkillListGump(int px, int py)
{
    int restored = 0;
    int savedA = 0;
    int savedB = 0;
    int artW, artH;

    Fn_00418bd0();
    memset(skillValue, 0, sizeof(skillValue));
    useOwnValues = 0;
    name = const_cast<char *>("skill gump");
    dirty = 0;
    unk_114 = 0;
    unk_110 = 0;
    unk_10c = 0;
    dragging = 0;
    clickPending = 0;
    clickTick = 0;

    if (g_deskQueueActive != 0) {
        int count = g_deskQueueCount;
        int i = 0;
        if (count > 0) {
            for (; i < count; i++) {
                if (g_deskQueue[i * 0x1c] == 4) {
                    restored = 1;
                    x = RAWF(int32_t, g_deskQueue, i * 0x1c + 0x14);
                    y = RAWF(int32_t, g_deskQueue, i * 0x1c + 0x18);
                    unk_60 = RAWF(int32_t, g_deskQueue, i * 0x1c + 0x08);
                    unk_7c = px;
                    unk_80 = py;
                    savedA = RAWF(uint16_t, g_deskQueue, i * 0x1c + 0x0e);
                    count = g_deskQueueCount;
                    savedB = RAWF(uint16_t, g_deskQueue, i * 0x1c + 0x10);
                    break;
                }
            }
        }
        if (i < count) {
            int j = i;
            if (i < count - 1) {
                uint8_t *dst = g_deskQueue + i * 0x1c;
                do {
                    memcpy(dst, dst + 0x1c, 7 * 4);
                    j++;
                    count = g_deskQueueCount;
                    dst += 0x1c;
                } while (j < count - 1);
            }
            count--;
            g_deskQueueCount = count;
            if (count == 0)
                g_deskQueueActive = 0;
        }
    } else {
        x = px;
        y = py;
    }

    owner = nullptr;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    if (g_groupCount > 0) {
        int i = 0;
        do {
            groupExpanded[i] = 0;
            i++;
        } while (i < g_groupCount);
        if (g_groupCount > 0)
            memset(g_groupFlags, 0, static_cast<size_t>(g_groupCount) * 4);
    }
    onGroupHeader = 1;
    renaming = 0;
    cursor = 0;
    scrollRange = g_groupCount;
    scrollPos = 0;
    for (int i = 0; i < g_skillCount; i++)
        g_skillLockState[i] = ((g_skillFlags[i] & 2) != 0) - 1;
    renameGroup = -1;
    selIndex = -1;
    selGroup = -2;
    unk_50 = 0x820;
    gfx::ArtSize(0x820, &width, &height);
    rowStride = width;
    height = 300;
    gfx::ArtSize(0x839, &unk_24, &unk_28);
    frameIndex = 0;
    frameCount = 2;

    frameData = static_cast<int32_t *>(gfx::AllocTagged(0x10, "framedata for CSkillGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    gfx::setBitmap(this, gfx::AllocTagged(height * width * 2 + 4, "shopgump frame"));   // sic: tag copied from the shop gump
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(height * width * 2));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);
    {
        uint16_t *bmp1 = static_cast<uint16_t *>(gfx::AllocTagged(unk_28 * unk_24 * 2 + 4, "cfg frame1"));   // sic: tag copied from the config gump
        frameData[3] = static_cast<int32_t>(reinterpret_cast<uintptr_t>(bmp1));
        memset(reinterpret_cast<uint8_t *>(bmp1) + 4, 0, static_cast<size_t>(unk_28 * unk_24 * 2));
        bmp1[0] = static_cast<uint16_t>(unk_24);
        bmp1[1] = static_cast<uint16_t>(unk_28);
    }

    const int self = static_cast<int>(reinterpret_cast<uintptr_t>(this));

    // iconify button
    CButtonGump *b1 = new CButtonGump(0x89, 0, self, 0, 0, PORT_ADDR(&CBookGump::Iconify), 0x82d, 0x82d, 0);
    gfx::AddChild(b1, this);

    // up button
    CButtonGump *b2 = new CButtonGump(0xf9, 0x38, self, 2, 0, PORT_ADDR(&UiCb_SkillListUp), 0x824, 0x824, 0);
    gfx::AddChild(b2, this);

    // add-group button
    CButtonGump *b3 = new CButtonGump(0xb4, height - 0x38, self, 0, 0, PORT_ADDR(&CSkillListGump::AddGroup), 0x83a, 0x83a, 0);
    b3->unk_c0 = 0;
    gfx::AddChild(b3, this);

    gfx::ArtSize(0x82e, &artW, &artH);
    listLeft = 0x1c;
    listRight = 0xff;
    listTop = 0x4a;
    listBottom = height - artH - 0x46;

    // down button
    CButtonGump *b4 = new CButtonGump(0xf9, height - artH - 0x42, self, 2, 0, PORT_ADDR(&UiCb_SkillListDown), 0x825, 0x825, 0);
    gfx::AddChild(b4, this);

    // list button
    CButtonGump *b5 = new CButtonGump(0x89, height - artH - 3, self, 0, 0, PORT_ADDR(&CGumpHandlers04::AddBackdropChildDc), 0x82e, 0x82e, 0);
    b5->artHover = 0x82f;
    gfx::AddChild(b5, this);

    // scroll thumb
    CScrollThumbGump *thumb = new CScrollThumbGump(0x100, 0x48, 1, height - artH - 0x46, self, 0x828);
    unk_94 = static_cast<int32_t>(reinterpret_cast<uintptr_t>(thumb));
    gfx::AddChild(thumb, this);
    skill_thumb(this)->SetPosition(0, 1);

    {
        uint16_t *bmp1 = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[3])));
        gfx::DrawArt(gfx::artPlain(), 0x839, 0, 0, bmp1 + 2, bmp1[0], bmp1[1]);
    }
    Redraw(-1);
    if (restored != 0)
        Resize(savedA, savedB);
}
HOOK_CTOR(0x00472da0, CSkillListGump, (int, int));

HOOK_VTABLE(0x004a0600, CSkillListGump);
HOOK_SDD(0x00473390, CSkillListGump);

// 0x004733c0  size 0x1ef
// Destructor body: commit a pending rename (and save skillgrp.mul), clear the window global, free both frames.
CSkillListGump::~CSkillListGump()
{
    if (renaming != 0) {
        skill_commit_name(this);
        Redraw(-1);
{
    FILE *f = fopen(g_app.ResolveDataPath(const_cast<char *>("skillgrp.mul")), "wb");
    fwrite(&g_groupCount, 4, 1, f);
    int i = 1;
    if (g_groupCount > 1) {
        char *p = g_groupNames + 0x11;
        do {
            fwrite(p, 1, 0x11, f);
            i = i + 1;
            p += 0x11;
        } while (i < g_groupCount);
    }
    int32_t *q = g_skillGroup;
    do {
        fwrite(q, 4, 1, f);
        q = q + 1;
    } while (reinterpret_cast<uintptr_t>(q) < GEND(g_skillGroup));
    fclose(f);
}
    }
    if (renameGroup != -1) {
        renameGroup = -1;
        Redraw(-1);
    }
    if (g_pSkillWindow == this)
        g_pSkillWindow = nullptr;
    if (frameData[1] != 0) {
        gfx::FreeTagged(reinterpret_cast<void *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[1]))), "skill frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData[3] != 0) {
        gfx::FreeTagged(reinterpret_cast<void *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[3]))), "skill frame1 pBMP");
        frameData[3] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "skill framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x004733c0, CSkillListGump);

// 0x004735b0  size 0x11
// Vtable slot 0x1c (arguments not read): delete `this` through its own slot 0 when non-null.
void __thiscall CSkillListGump::RButtonUp(int a, int b)
{
    (void)a;
    (void)b;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}
HOOK_VMETHOD(0x004735b0, CSkillListGump, RButtonUp);

// 0x004735d0  size 0xca
// Mouse down: raise the window, start the click timer (second press = double click), remember what was hit.
void __thiscall CSkillListGump::MouseDown(int mx, int my)
{
    int isGroup;
    int index;

    if (g_focusedControl == this)
        BringToFront(1);
    else
        BringToFront(0);
    unk_110 = 1;
    unk_6c = mx;
    unk_70 = my;
    if (clickPending == 1) {
        clickPending = 2;
    } else {
        clickPending = 1;
        clickTick = GetTickCount();
    }
    unk_e0 = 0;
    unk_e4 = 0;
    if (HitTestRow(mx, my, &isGroup, &index) != 0) {
        if (isGroup != 0)
            unk_e0 = 1;
        else
            unk_e4 = 1;
        unk_f0 = index;
    }
    if (unk_e0 != 0)
        clickPending = 0;
}
HOOK_VMETHOD(0x004735d0, CSkillListGump, MouseDown);

// 0x004736a0  size 0x212
// Mouse up: finish a skill drag (move the skill to the target group, save skillgrp.mul), else deliver the
// click or double click.
void __thiscall CSkillListGump::MouseUp(int mx, int my)
{
    unk_110 = 0;
    if (dragging != 0) {
        int from = selIndex;
        dragging = 0;
        clickPending = 0;
        if (from >= 0 && selGroup >= -1) {
            int target = selGroup;
            int row;
            int grp;
            if (onGroupHeader != 0) {
                grp = cursor;
                row = -1;
            } else {
                row = cursor;
                grp = g_skillGroup[g_skillOrder[row]];
            }
            int fromGrp = g_skillGroup[g_skillOrder[from]];
            if ((fromGrp > grp || (fromGrp == grp && from > row)) && (target < grp || (target == grp && from < row)))
                scrollPos++;
            if (from == row) {
                int rows = g_skillCount;
                row++;
                if (row < rows) {
                    do {
                        if (g_skillGroup[g_skillOrder[row]] == grp)
                            break;
                        row++;
                    } while (row < rows);
                }
                if (row >= rows) {
                    row = grp + 1;
                    if (row < g_groupCount) {
                        onGroupHeader = 1;
                        cursor = row;
                    }
                } else {
                    cursor = row;
                }
            }
            g_skillGroup[g_skillOrder[from]] = target;
            if (RAWF(int32_t, this, static_cast<unsigned>(0xb0 + selGroup * 4)) == 0)
                scrollRange--;
    {
        FILE *f = fopen(g_app.ResolveDataPath(const_cast<char *>("skillgrp.mul")), "wb");
        fwrite(&g_groupCount, 4, 1, f);
        int i = 1;
        if (g_groupCount > 1) {
            char *p = g_groupNames + 0x11;
            do {
                fwrite(p, 1, 0x11, f);
                i = i + 1;
                p += 0x11;
            } while (i < g_groupCount);
        }
        int32_t *q = g_skillGroup;
        do {
            fwrite(q, 4, 1, f);
            q = q + 1;
        } while (reinterpret_cast<uintptr_t>(q) < GEND(g_skillGroup));
        fclose(f);
    }
        }
        Redraw(-1);
        skill_thumb(this)->SetPosition(scrollPos, scrollRange);
    } else {
        int pending = clickPending;
        if (pending == 2) {
            clickPending = 0;
            OnDoubleClick(mx, my);
        } else if (unk_114 == 0 && pending == 0) {
            OnClick(mx, my);
        }
    }
    unk_114 = 0;
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x004736a0, CSkillListGump, MouseUp);

// 0x004738c0  size 0x2c3
// Mouse move: a press that moved more than 4 pixels starts a drag (dropping any rename); a drag of the window
// itself moves the gump; a skill drag tracks the group under the cursor.
void __thiscall CSkillListGump::MouseMove(int mx, int my)
{
    if (unk_110 != 0) {
        if (clickPending != 0 && (abs(mx - unk_6c) > 4 || abs(my - unk_70) > 4)) {
            dragging = 1;
            clickPending = 0;
            if (renaming != 0) {
                skill_commit_name(this);
                Redraw(-1);
{
    FILE *f = fopen(g_app.ResolveDataPath(const_cast<char *>("skillgrp.mul")), "wb");
    fwrite(&g_groupCount, 4, 1, f);
    int i = 1;
    if (g_groupCount > 1) {
        char *p = g_groupNames + 0x11;
        do {
            fwrite(p, 1, 0x11, f);
            i = i + 1;
            p += 0x11;
        } while (i < g_groupCount);
    }
    int32_t *q = g_skillGroup;
    do {
        fwrite(q, 4, 1, f);
        q = q + 1;
    } while (reinterpret_cast<uintptr_t>(q) < GEND(g_skillGroup));
    fclose(f);
}
            }
            if (renameGroup != -1) {
                renameGroup = -1;
                Redraw(-1);
            }
            if (unk_e0 != 0)
                unk_e0 = 0;
            if (unk_e4 != 0) {
                selIndex = unk_f0;
                selGroup = g_skillGroup[g_skillOrder[selIndex]];
                Redraw(-1);
            } else {
                selIndex = -1;
                selGroup = -2;
            }
        }
        if (unk_110 != 0) {
            if (dragging == 0)
                return;
            if (unk_e4 == 0 && unk_e0 == 0)
                Move(mx - unk_6c, my - unk_70, 1, 0, 1);
        }
    }
    if (dragging != 0 && selIndex >= 0) {
        int target = selGroup;
        int isGroup = my;
        int index = mx;
        if (HitTestRow(mx, my, &isGroup, &index) != 0) {
            if (isGroup == 0 && index == selIndex) {
                // still on the dragged row: keep the target
            } else if (isGroup != 0) {
                target = index;
            } else {
                target = g_skillGroup[g_skillOrder[index]];
            }
        } else {
            target = g_skillGroup[g_skillOrder[selIndex]];
        }
        if (target != selGroup) {
            selGroup = target;
            Redraw(-1);
        }
    }
}
HOOK_VMETHOD(0x004738c0, CSkillListGump, MouseMove);

// 0x00473b90  size 0x471
// Click at (mx, my). Not on a group: finish any rename. On a group: left of its +/- icon toggles the group open
// or closed (adjusting the scroll range); elsewhere selects the group, and a second click starts a rename.
void __thiscall CSkillListGump::OnClick(int mx, int my)
{
    (void)my;
    int moved = 0;

    if (unk_e0 == 0) {
        if (renaming != 0) {
            skill_commit_name(this);
            Redraw(-1);
            SaveSkillGroups();
        }
        if (renameGroup != -1) {
            renameGroup = -1;
            Redraw(-1);
        }
        return;
    }

    if (mx > listLeft + 0x14) {
        if (renaming != 0) {
            skill_commit_name(this);
            Redraw(-1);
{
    FILE *f = fopen(g_app.ResolveDataPath(const_cast<char *>("skillgrp.mul")), "wb");
    fwrite(&g_groupCount, 4, 1, f);
    int i = 1;
    if (g_groupCount > 1) {
        char *p = g_groupNames + 0x11;
        do {
            fwrite(p, 1, 0x11, f);
            i = i + 1;
            p += 0x11;
        } while (i < g_groupCount);
    }
    int32_t *q = g_skillGroup;
    do {
        fwrite(q, 4, 1, f);
        q = q + 1;
    } while (reinterpret_cast<uintptr_t>(q) < GEND(g_skillGroup));
    fclose(f);
}
            if (renameGroup != -1) {
                renameGroup = -1;
                Redraw(-1);
            }
        }
        if (renameGroup != unk_f0) {
            renameGroup = unk_f0;
            Gump_SetFocus(this);
            Redraw(-1);
            return;
        }
        BeginRename(renameGroup);
        return;
    }

    if (renaming != 0) {
        skill_commit_name(this);
        Redraw(-1);
{
    FILE *f = fopen(g_app.ResolveDataPath(const_cast<char *>("skillgrp.mul")), "wb");
    fwrite(&g_groupCount, 4, 1, f);
    int i = 1;
    if (g_groupCount > 1) {
        char *p = g_groupNames + 0x11;
        do {
            fwrite(p, 1, 0x11, f);
            i = i + 1;
            p += 0x11;
        } while (i < g_groupCount);
    }
    int32_t *q = g_skillGroup;
    do {
        fwrite(q, 4, 1, f);
        q = q + 1;
    } while (reinterpret_cast<uintptr_t>(q) < GEND(g_skillGroup));
    fclose(f);
}
    }
    if (renameGroup != -1) {
        renameGroup = -1;
        Redraw(-1);
    }
    int rows = g_skillCount;
    int empty = 1;
    int i = 0;
    if (rows > 0) {
        int group = unk_f0;
        do {
            if (g_skillGroup[g_skillOrder[i]] == group) {
                empty = 0;
                break;
            }
            i++;
        } while (i < rows);
    }
    if (empty != 0)
        return;
    i = 0;
    if (rows > 0) {
        int group = unk_f0;
        do {
            if (g_skillGroup[g_skillOrder[i]] == group)
                moved++;
            i++;
        } while (i < rows);
    }
    int32_t *flag = &groupExpanded[unk_f0];
    if (*flag != 0)
        scrollRange = scrollRange - moved;
    else
        scrollRange = scrollRange + moved;
    *flag = (*flag == 0);
    Redraw(-1);
    skill_thumb(this)->SetPosition(scrollPos, scrollRange);
}
HOOK_METHOD(0x00473b90, CSkillListGump, OnClick);

// 0x00474010  size 0x9d
// Double click (arguments not read): when iconified, restore; on a group other than the first, open the EditGroup gump.
void __thiscall CSkillListGump::OnDoubleClick(int mx, int my)
{
    (void)mx;
    (void)my;
    if (unk_60 != 0) {
        Gump_Uniconify(this);
        return;
    }
    if (unk_e0 != 0 && unk_f0 > 0) {
        CEditGroupGump *dlg = new CEditGroupGump(unk_f0);
        gfx::AddChild(dlg, nullptr);
    }
}
HOOK_METHOD(0x00474010, CSkillListGump, OnDoubleClick);

// 0x004740b0  size 0x155
// Resize to (width, height): shift the children that sit in the right / bottom half, reallocate frame0 and
// reposition the list.
void __thiscall CSkillListGump::Resize(int newWidth, int newHeight)
{
    int artW, artH;

    rowStride = (newWidth + 7) & ~7;
    for (CGump *c = lastChild; c != nullptr; c = c->prev) {
        if (PORT_VSLOT0(c, 0x98, int)() != 0) {
            RAWF(int32_t, c, 0xa8) += newHeight - height;
        } else {
            if (c->x > width / 2)
                c->x = c->x - (width - rowStride);
            if (c->y > height / 2)
                c->y = newHeight + (c->y - height);
        }
    }
    if (frameData[1] != 0) {
        gfx::FreeTagged(reinterpret_cast<void *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[1]))), "skill frame0 pBMP");
        frameData[1] = 0;
    }
    height = newHeight;
    width = rowStride;
    gfx::setBitmap(this, gfx::AllocTagged(newHeight * rowStride * 2 + 4, "text frame0"));   // sic
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(width * height * 2));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);
    gfx::ArtSize(0x82e, &artW, &artH);
    listBottom = height - artH - 0x46;
    Redraw(-1);
    skill_thumb(this)->SetPosition(scrollPos, scrollRange);
}
HOOK_VMETHOD(0x004740b0, CSkillListGump, Resize);

// 0x004744b0  size 0x393
// Key press. Delete (0xe02e) with a group selected removes it: its skills go to group 0 ("Miscellaneous"), later
// groups shift down, the scroll range is corrected and skillgrp.mul is saved.
void __thiscall CSkillListGump::OnKey(int key)
{
    int moved = 0;

    if (key != 0xe02e || renameGroup == -1)
        return;
    if (renameGroup == 0) {
        Gump_Alert(g_gumpManager, "Cannot delete this group.", 0, 0, 0);
        return;
    }
    int rows = g_skillCount;
    int wasExpanded = groupExpanded[renameGroup];
    if (rows > 0) {
        int i = 0;
        do {
            int skill = g_skillOrder[i];
            if (g_skillGroup[skill] == renameGroup) {
                g_skillGroup[skill] = 0;
                moved++;
            }
            i++;
        } while (i < rows);
    }
    int g = renameGroup + 1;
    if (g < g_groupCount) {
        int32_t *dstFlag = &groupExpanded[g - 1];
        char *namePtr = g_groupNames + g * 0x11;
        for (;;) {
            if (rows > 0) {
                int n = rows;
                int32_t *p = g_skillGroup;
                do {
                    int v = *p;
                    if (v == g) {
                        v--;
                        *p = v;
                    }
                    p++;
                    n--;
                } while (n != 0);
            }
            strcpy(namePtr - 0x11, namePtr);
            dstFlag++;
            dstFlag[-1] = dstFlag[0];
            g++;
            namePtr += 0x11;
            if (!(g < g_groupCount))
                break;
            rows = g_skillCount;
        }
    }
    g_groupCount = g_groupCount - 1;
    if (wasExpanded != 0) {
        if (groupExpanded[0] == 0)
            scrollRange = scrollRange + (-1 - moved);
        else
            scrollRange = scrollRange - 1;
    } else {
        if (groupExpanded[0] != 0)
            scrollRange = scrollRange + (moved - 1);
        else
            scrollRange = scrollRange - 1;
    }
    if (renaming != 0) {
        skill_commit_name(this);
        Redraw(-1);
{
    FILE *f = fopen(g_app.ResolveDataPath(const_cast<char *>("skillgrp.mul")), "wb");
    fwrite(&g_groupCount, 4, 1, f);
    int i = 1;
    if (g_groupCount > 1) {
        char *p = g_groupNames + 0x11;
        do {
            fwrite(p, 1, 0x11, f);
            i = i + 1;
            p += 0x11;
        } while (i < g_groupCount);
    }
    int32_t *q = g_skillGroup;
    do {
        fwrite(q, 4, 1, f);
        q = q + 1;
    } while (reinterpret_cast<uintptr_t>(q) < GEND(g_skillGroup));
    fclose(f);
}
    }
    if (renameGroup != -1) {
        renameGroup = -1;
        Redraw(-1);
    }
    Redraw(-1);
    CScrollThumbGump *thumb = skill_thumb(this);
    PORT_VSLOT3(this, 0x30, void, int, int, int)(thumb->y - thumb->minY, thumb->maxY - thumb->minY,
                                                 static_cast<int>(reinterpret_cast<uintptr_t>(thumb)));
{
    FILE *f = fopen(g_app.ResolveDataPath(const_cast<char *>("skillgrp.mul")), "wb");
    fwrite(&g_groupCount, 4, 1, f);
    int i = 1;
    if (g_groupCount > 1) {
        char *p = g_groupNames + 0x11;
        do {
            fwrite(p, 1, 0x11, f);
            i = i + 1;
            p += 0x11;
        } while (i < g_groupCount);
    }
    int32_t *q = g_skillGroup;
    do {
        fwrite(q, 4, 1, f);
        q = q + 1;
    } while (reinterpret_cast<uintptr_t>(q) < GEND(g_skillGroup));
    fclose(f);
}
    if (moved > 0)
        Journal_Message(g_journal, 0, 3, "Skills moved to Miscellaneous.");
}
HOOK_VMETHOD(0x004744b0, CSkillListGump, OnKey);

// 0x004753c0  size 0x108
// Draw (vtable slot 0x08): blit the current frame, then the children that belong to the current page.
void __thiscall CSkillListGump::Draw(int *clip)
{
    int rect[4];
    uint16_t *bmp = skill_frame(this);
    int px = 0;
    int py = 0;

    LocalToScreen(&px, &py);
    rect[0] = px;
    rect[1] = py;
    rect[2] = px + bmp[0];
    rect[3] = py + bmp[1];
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, int *, int *)(clip, rect) != 0) {
        GFX_BlitFrame(gfx::artPlain(), bmp + 2, clip, bmp[0], bmp[1], px, py, g_backBuffer, 0x280, 0x1e0);
        for (CGump *c = firstChild; c != nullptr; c = c->next) {
            if (PORT_VSLOT0(c, 0xa8, int)() != 0 && RAWF(int32_t, c, 0xb4) == unk_60) {
                PORT_VSLOT1(c, 0x08, void, int *)(clip);
            } else if (PORT_VSLOT0(c, 0x98, int)() != 0 && unk_60 == 0) {
                PORT_VSLOT1(c, 0x08, void, int *)(clip);
            } else if (PORT_VSLOT0(c, 0xc8, int)() != 0 && unk_60 == 0) {
                PORT_VSLOT1(c, 0x08, void, int *)(clip);
            }
        }
    }
}
HOOK_VMETHOD(0x004753c0, CSkillListGump, Draw);

// 0x004754d0  size 0xd7
// Hit test (vtable slot 0x0c): an opaque pixel of the current frame, else any child that answers the point.
int __thiscall CSkillListGump::HitTest(int mx, int my)
{
    uint16_t *bmp = skill_frame(this);
    int w = bmp[0];
    int h = bmp[1];
    bmp += 2;
    if (mx < 0 || my < 0 || mx >= w || my >= h)
        return 0;
    if (bmp[w * my + mx] != 0)
        return 1;
    LocalToScreen(&mx, &my);
    for (CGump *c = lastChild; c != nullptr; c = c->prev) {
        int cx = mx;
        int cy = my;
        c->ScreenToLocal(&cx, &cy);
        if (PORT_VSLOT2(c, 0x0c, int, int, int)(cx, cy) != 0)
            return 1;
    }
    return 0;
}
HOOK_VMETHOD(0x004754d0, CSkillListGump, HitTest);

// 0x004758e0  size 0x132
// Start renaming group `renameGroup` (argument not read): an edit control over the group's row, filled with
// the current name and given the keyboard focus.
void __thiscall CSkillListGump::BeginRename(int group)
{
    (void)group;
    int textW, textH, textX;

    gfx::TextMeasure(6, "Aq", &textW, &textH, &textX);
    int rowY = g_skillRowTop - 2;   // y of the row last hit-tested
    int h = textH;
    // ? the original pushes the width as a 32-bit value whose upper half is whatever ECX held (a stack address);
    //   the constructor takes it as a 16-bit parameter, so only the low 16 bits count.
    renameEdit = reinterpret_cast<uint8_t *>(new CInputControlGump(
        listLeft + 0xd, rowY - 6, this, static_cast<uint16_t>(listRight - listLeft + 1), static_cast<uint16_t>(h + 4), 0, 0x10, 6, 0, 1, 0, 0));
    gfx::AddChild(reinterpret_cast<CGump *>(renameEdit), this);
    g_focusedControl = reinterpret_cast<CGump *>(renameEdit);
    strcpy(reinterpret_cast<char *>(renameEdit + 0xec), g_groupNames + renameGroup * 0x11);
    PORT_VSLOT1(renameEdit, 0x3c, void, int)(0xe023);
    renaming = 1;
    Redraw(-1);
}
HOOK_METHOD(0x004758e0, CSkillListGump, BeginRename);

// 0x00475a20  size 0x149
// Vtable slot 0x4c: commit a pending rename (saving skillgrp.mul) and clear the selected group.
void __thiscall CSkillListGump::OnOk()
{
    if (renaming != 0) {
        skill_commit_name(this);
        Redraw(-1);
{
    FILE *f = fopen(g_app.ResolveDataPath(const_cast<char *>("skillgrp.mul")), "wb");
    fwrite(&g_groupCount, 4, 1, f);
    int i = 1;
    if (g_groupCount > 1) {
        char *p = g_groupNames + 0x11;
        do {
            fwrite(p, 1, 0x11, f);
            i = i + 1;
            p += 0x11;
        } while (i < g_groupCount);
    }
    int32_t *q = g_skillGroup;
    do {
        fwrite(q, 4, 1, f);
        q = q + 1;
    } while (reinterpret_cast<intptr_t>(q) < static_cast<intptr_t>(GEND(g_skillGroup)));
    fclose(f);
}
    }
    if (renameGroup != -1) {
        renameGroup = -1;
        Redraw(-1);
    }
}
HOOK_VMETHOD(0x00475a20, CSkillListGump, OnOk);

// 0x004760f0  size 0x10
// Choose whether the window shows skillValue[] (non-zero) or the player's skills.
void __thiscall CSkillListGump::SetUseOwnValues(int v)
{
    useOwnValues = v;
}
HOOK_METHOD(0x004760f0, CSkillListGump, SetUseOwnValues);

// 0x00476100  size 0x1f
// skillValue[skill] = value for 0 <= skill < 50.
void __thiscall CSkillListGump::SetSkillValue(int skill, uint16_t value)
{
    if (skill >= 0 && skill < 0x32)
        skillValue[skill] = value;
}
HOOK_METHOD(0x00476100, CSkillListGump, SetSkillValue);
