#include "skill_list_gump.h"

#include <windows.h>

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "batch06_helpers.h"
#include "globals.h"
#include "crt.h"
#include "mouse_manager.h"
#include "button_gump.h"
#include "skill_list_gump_ops_ports.h"
#include "bb_msg_gump_ports.h"
#include "gump.h"
#include "ui_callbacks_ports.h"
#include "gump_callbacks.h"

#define g_skillGroupEnd GEND(g_skillGroup)

// 0x00473b90 on the gump (x, y)
#define SkillList_OnClick(self, a, b) (self->*PORT_METHOD2(CSkillListGump, void, &CSkillListGump::OnClick, int, int))(a, b)
// 0x00446c60 on the mouse object 0x0068d2e0: current position
#define Mouse_GetPos(self, a1, a2) (reinterpret_cast<CMouseManager *>(self)->GetMousePos(a1, a2))
#undef g_mouse   // the class header's typed form; this file addresses the object by offset
#define g_mouse GLOBAL_PTR(void, g_mouse_storage)
// 0x00412fb0 button ctor: (x, y, parent, 0, 0, callback, upArt, downArt, 0)
// 0x00472be0: ECX = art id, EDX = 2, stack = (bitmap, h1, h2)
#define SkillList_DrawFrame(a1, a2, a3, a4, a5) (SkillList_DrawTiledFrame(a1, a2, (uint16_t *)(a3), a4, a5))
// 0x004069f0: ECX = x, EDX = y, stack = (art id, bitmap, clip)
#define SkillList_DrawIcon(a1, a2, a3, a4, a5) (BBMsg_TileArt(a1, a2, a3, a4, a5))

// Commit the pending group rename: store the typed name, delete the edit control, redraw.
static void commit_rename(CSkillListGump *g)
{
    char *nameText = reinterpret_cast<char *>(g->renameEdit + 0xec);
    if (strlen(nameText) != 0) {
        strcpy(g_groupNames + g->renameGroup * 0x11, nameText);
    }
    if (g->renameEdit != nullptr)
        b6_delete_gump(g->renameEdit);
    g->renameEdit = nullptr;
    g->renaming = 0;
    g->renameGroup = -1;
    g->Redraw(-1);
}

// Write skillgrp.mul: group count, group names 1..count-1 (0x11 each), then the 50 skill -> group dwords.
static void save_skill_groups()
{
    FILE *f = fopen(g_app.ResolveDataPath(const_cast<char *>("skillgrp.mul")), "wb");
    fwrite(&g_groupCount, 4, 1, f);
    int i = 1;
    if (g_groupCount > 1) {
        char *p = g_groupNames + 0x11;
        do {
            fwrite(p, 0x11, 1, f);
            i = i + 1;
            p += 0x11;
        } while (i < g_groupCount);
    }
    int32_t *q = g_skillGroup;
    do {
        fwrite(q, 4, 1, f);
        q = q + 1;
    } while (reinterpret_cast<uintptr_t>(q) < g_skillGroupEnd);
    fclose(f);
}

// 0x00474210  size 0x294  CSkillGump scroll-to (num, den): commits any pending group-rename, then moves the display cursor num steps through the group/skill list
// Two cursor modes via onGroupHeader (1 = on a group header, 0 = on a skill row); cursor is the group or row number.
// The third stack argument is unused (RET 0xc).
void __thiscall CSkillListGump::OnScroll(int num, int den, int unused)
{
    (void)unused;
    int count;
    int nSkills;
    int i;
    int grp, idx, k;

    if (renaming != 0) {
        commit_rename(this);
        save_skill_groups();
    }

    if (renameGroup != -1) {
        renameGroup = -1;
        Redraw(-1);
    }

    count = (scrollRange * num) / den;
    onGroupHeader = 1;
    cursor = 0;
    scrollPos = count;
    nSkills = g_skillCount;
    i = 0;
    if (nSkills > 0) {
        do {
            if (g_skillGroup[g_skillOrder[i]] == -1) {
                onGroupHeader = 0;
                cursor = i;
                nSkills = g_skillCount;
                break;
            }
            i++;
        } while (i < nSkills);
    }

    if (count > 0) {
        int left = count;
        do {
            if (onGroupHeader == 0) {
                // on a skill row: find the next skill of the same group
                idx = cursor;
                grp = g_skillGroup[g_skillOrder[idx]];
                idx = idx + 1;
                if (g_skillGroup[g_skillOrder[idx]] != grp) {
                    for (;;) {
                        if (idx >= nSkills) {
                            // no more skills in this group: go to the next group header
                            if (grp + 1 >= g_groupCount) {
                                goto step_done;
                            }
                            onGroupHeader = 1;
                            idx = grp + 1;
                            goto set_cursor;
                        }
                        idx++;
                        if (g_skillGroup[g_skillOrder[idx]] == grp) {
                            break;
                        }
                    }
                }
                if (idx >= nSkills) {
                    if (grp + 1 >= g_groupCount) {
                        goto step_done;
                    }
                    onGroupHeader = 1;
                    idx = grp + 1;
                }
            set_cursor:
                cursor = idx;
                nSkills = g_skillCount;
            } else {
                // on a group header
                grp = cursor;
                if (groupExpanded[grp] != 0) {
                    // expanded: look for its first skill
                    k = 0;
                    if (g_skillGroup[g_skillOrder[0]] != grp) {
                        for (;;) {
                            if (k >= nSkills) {
                                goto next_group;
                            }
                            k++;
                            if (g_skillGroup[g_skillOrder[k]] == grp) {
                                break;
                            }
                        }
                    }
                    if (k < nSkills) {
                        onGroupHeader = 0;
                        cursor = k;
                        nSkills = g_skillCount;
                        goto step_done;
                    }
                }
            next_group:
                if (grp + 1 < g_groupCount) {
                    cursor = grp + 1;
                    nSkills = g_skillCount;
                }
            }
        step_done:
            left--;
        } while (left != 0);
    }
    Redraw(-1);
}
HOOK_VMETHOD(0x00474210, CSkillListGump, OnScroll);

// 0x00474850  size 0x26c  CSkillGump per-frame update (pending single-click timeout, group-name save, drag auto-scroll)
void __thiscall CSkillListGump::Tick()
{
    unsigned int now;
    unsigned int dct;
    int x_;
    int y_;
    int pt[2];

    now = GetTickCount();
    dct = GetDoubleClickTime();

    if (clickPending == 1) {
        if (static_cast<unsigned int>(now - clickTick) > dct) {
            clickPending = 0;
            if (unk_110 == 0) {
                SkillList_OnClick(this, unk_6c, unk_70);
                goto L_474a4a;
            }
            dragging = 1;
            if (renaming != 0) {
                commit_rename(this);
                save_skill_groups();
            }
            if (renameGroup != -1) {
                renameGroup = -1;
                Redraw(-1);
            }
            if (unk_e0 != 0)
                unk_e0 = 0;
            if (unk_e4 != 0) {
                selIndex = unk_f0;
                selGroup = g_skillGroup[g_skillOrder[unk_f0]];
                Redraw(-1);
            } else {
                selIndex = -1;
                selGroup = -2;
            }
        }
    }

L_474a4a:
    if (dragging != 0 && selIndex >= 0) {
        Mouse_GetPos(g_mouse, &x_, &y_);
        pt[0] = x_;
        pt[1] = y_;
        this->CGump::ScreenToLocal(&pt[0], &pt[1]);
        if (pt[1] < listTop) {
            (this->*PORT_METHOD0(CSkillListGump, void, &UiCb_SkillListUp))();
            return;
        }
        if (pt[1] > listBottom)
            (this->*PORT_METHOD0(CSkillListGump, void, &UiCb_SkillListDown))();
    }
}
HOOK_VMETHOD(0x00474850, CSkillListGump, Tick);

// 0x00474ac0  size 0x1fe  CSkillGump rebuild row buttons (lock/up-down icons)
// The SEH frame around the button `new` is omitted. Byte tables: row -> skill (0x747730), skill -> group (0x747870),
// flag bytes (0x747938, bit 0 = show button).
void __thiscall CSkillListGump::RebuildButtons()
{
    CGump *child;
    CGump *prevChild;
    int yy;
    int32_t *slot;
    int ebx;   // current group number
    int esi;   // current row, -1 = none
    int ecx;   // row count
    int baseY;

    child = lastChild;
    if (child != nullptr) {
        do {
            prevChild = child->prev;
            if (PORT_VSLOT0(child, 0xa8, int)() != 0) {
                if (*reinterpret_cast<int32_t *>(reinterpret_cast<char *>(child) + 0xac) != 0) {
                    if (child != nullptr)
                        b6_delete_gump(child);
                }
            }
            child = prevChild;
        } while (prevChild != nullptr);
    }

    baseY = listTop + 2;
    if (onGroupHeader != 0) {
        ebx = cursor;
        esi = -1;
    } else {
        esi = cursor;
        ebx = g_skillGroup[g_skillOrder[esi]];
    }

    if (ebx >= g_groupCount)
        return;

    ecx = g_skillCount;
    yy = baseY + 1;
    slot = &groupExpanded[ebx];

    for (;;) {
        if (yy + 0xe > listBottom - 2)
            return;

        if (dragging != 0 && selIndex == esi) {
            if (esi == -1)
                goto L_474c46;
            if (selGroup != ebx)
                goto L_474c4a;
        }
        if (esi == -1)
            goto L_474c46;
        if ((g_skillFlags[g_skillOrder[esi]] & 1) == 0)
            goto L_474c46;

        {
            uint8_t *btn;
            btn = (uint8_t *)new CButtonGump(listLeft + 8, yy, (int)(this), 0, 0, (int)(reinterpret_cast<void *>(PORT_ADDR(&CGumpCallbacks::SendIdRequest))), 0x837, 0x837, 0);
            *reinterpret_cast<int32_t *>(btn + 0xa4) = 0x838;   // ? no null check in the original
            *reinterpret_cast<int32_t *>(btn + 0xac) = 1;
            *reinterpret_cast<int32_t *>(btn + 0x9c) = g_skillOrder[esi];
            CGump_AddChild(btn, this, 1);
            ecx = g_skillCount;
        }

    L_474c46:
        yy = yy + 0x11;
    L_474c4a:
        if (ebx < 0 || *slot != 0) {
            esi = esi + 1;
            if (g_skillGroup[g_skillOrder[esi]] != ebx) {
                for (;;) {
                    if (esi == selIndex) {
                        if (selGroup == ebx)
                            break;
                    }
                    if (esi >= ecx)
                        goto L_474c92;
                    esi = esi + 1;
                    if (g_skillGroup[g_skillOrder[esi]] == ebx)
                        break;
                }
            }
        L_474c92:
            if (esi != ecx)
                goto L_474ca1;
        }
        slot = slot + 1;
        ebx = ebx + 1;
        esi = -1;
    L_474ca1:
        if (ebx < g_groupCount)
            continue;
        return;
    }
}
HOOK_METHOD(0x00474ac0, CSkillListGump, RebuildButtons);

// 0x00474cc0  size 0x6f2  CSkillGump::Redraw(int row)  draw list into frame bitmap (row == -1: everything, else only the given skill row)
// 0x00436870 takes (r, g, b); the original also pushes the frame bitmap as a fourth argument that the callee never pops
// (the caller's frame absorbs it), so it is not passed here.
void __thiscall CSkillListGump::Redraw(int row)
{
    char text[80];
    int idx;
    int grp;
    char *grpName;
    int32_t *slot;
    int textW;
    int meas2c;
    int yOff;
    int yOff2;
    int clip[4];
    int yy;
    int bot;
    int pad34;
    int found;
    int i;
    int val;
    unsigned color;

    b6_text_measure(b6_font, 9, "Aq", &textW, &meas2c, &yOff);
    clip[0] = listLeft + 2;
    clip[2] = listRight - 2;
    yy = listTop + 2;
    clip[3] = listBottom - 2;
    clip[1] = yy;

    if (row == -1) {
        B6Bitmap *bmp = b6_frame_bitmap(this, 0);
        int artH = 0;
        int h2d, h2e;
        b6_art_size(0x82d, &idx, &artH);
        bot = artH;
        h2d = bot;
        b6_art_size(0x82e, &idx, &artH);
        h2e = artH;
        SkillList_DrawFrame(unk_50, 2, b6_frame_bitmap(this, 0), bot - 1, artH);
        b6_draw_art(B6_ROWBLIT_COPY, 0x834, 0x76, h2d + 0xa, bmp->px, bmp->w, bmp->h);
        b6_draw_art(B6_ROWBLIT_COPY, 0x836, 0x1e, height - h2e - 0x38, bmp->px, bmp->w, bmp->h);
        b6_draw_art(B6_ROWBLIT_COPY, 0x82b, 0x1e, h2d + 0x24, bmp->px, bmp->w, bmp->h);
        b6_draw_art(B6_ROWBLIT_COPY, 0x82b, 0x1f, height - h2e - 0x46, bmp->px, bmp->w, bmp->h);
        yy = clip[1];
    }

    if (onGroupHeader != 0) {
        grp = cursor;
        idx = -1;
    } else {
        idx = cursor;
        grp = g_skillGroup[g_skillOrder[idx]];
    }

    if (grp >= g_groupCount)
        goto L_end;

    grpName = g_groupNames + grp * 0x11;
    slot = &groupExpanded[grp];
    bot = yy + 0x10;
    pad34 = bot - 0xc;
    if (pad34 + 0xb > clip[3])
        goto L_end;

    for (;;) {
        if (dragging != 0 && selIndex == idx && idx != -1 && selGroup != grp)
            goto L_475319;

        if (row != -1) {
            if (row != idx)
                goto L_475316;
            if (row != -1) {
                color = b6_make_color(b6_pixel_format, 0xa9, 0x98, 0x63);
                b6_fill_rect(b6_font, clip[0], yy, clip[2], bot, static_cast<int>(color));
            }
        }

        if (idx != -1)
            goto L_47512f;

        // ---- group header row ----
        sprintf(text, "%s", grpName);
        b6_text_measure(b6_font, 6, text, &textW, &meas2c, &yOff2);
        if ((textW % 2) == 0)
            textW = textW + 1;

        if ((dragging != 0 && selGroup == grp && *slot == 0) || grp == renameGroup) {
            if (renaming != 0) {
                if (bot >= clip[3])
                    bot = clip[3];
                color = b6_make_color(b6_pixel_format, 0xa9, 0x98, 0x63);
                b6_fill_rect(b6_font, clip[0] + 0xe, yy, clip[2], bot, static_cast<int>(color));
            } else {
                if (bot >= clip[3])
                    bot = clip[3];
                color = b6_make_color(b6_pixel_format, 0xa9, 0x98, 0x63);
                b6_fill_rect(b6_font, clip[0] + 0xe, yy, clip[0] + textW + 0xe, bot, static_cast<int>(color));
            }
        }

        // is there any row in this group?
        clip[0] = 0;
        found = 1;
        if (g_skillCount > 0) {
            i = 0;
            do {
                if (g_skillGroup[g_skillOrder[i]] == grp) {
                    found = 0;
                    break;
                }
                i = i + 1;
            } while (i < g_skillCount);
        }
        if (found == 0) {
            if (*slot != 0) {
                B6Bitmap *b = b6_frame_bitmap(this, 0);
                b6_draw_art_clip(B6_ROWBLIT_COPY, 0x826, clip, listLeft, yy - 1, b->px, b->w, b->h);
            } else {
                B6Bitmap *b = b6_frame_bitmap(this, 0);
                b6_draw_art_clip(B6_ROWBLIT_COPY, 0x827, clip, listLeft, yy - 1, b->px, b->w, b->h);
            }
        }

        clip[0] = listLeft + 2;
        if (renaming == 0 || renameGroup != grp) {
            b6_text_draw2(b6_font, clip[0] + 0xe, yOff + yy, 6, 0, text, b6_frame_bitmap(this, 0)->px, width, clip);
            clip[2] = clip[2] - 0xa;
            SkillList_DrawIcon(clip[0] + textW + 0x12, pad34, 0x835, b6_frame_bitmap(this, 0), clip);
            clip[2] = clip[2] + 0xa;
        }
        goto L_475310;

    L_47512f:
        // ---- skill row ----
        if (dragging != 0 && selIndex == idx && selGroup == grp) {
            clip[2] = width - 0x32;
            if ((clip[2] % 2) == 0)
                clip[2] = clip[2] - 1;
            if (bot >= clip[3])
                bot = clip[3];
            color = b6_make_color(b6_pixel_format, 0xa9, 0x98, 0x63);
            b6_fill_rect(b6_font, clip[0] + 0x14, yy, clip[2], bot, static_cast<int>(color));
        }
        clip[2] = width - 0x5a;
        sprintf(text, "%s", g_skillNames + g_skillOrder[idx] * 0x50);
        b6_text_draw2(b6_font, clip[0] + 0x14, yOff + yy, 9, 0x288, text, b6_frame_bitmap(this, 0)->px, width, clip);
        clip[2] = width;
        if (useOwnValues != 0) {
            val = static_cast<int>(skillValue[g_skillOrder[idx]]);
            sprintf(text, "%d.%d", val / 10, val % 10);
        } else {
            val = static_cast<int>(g_skillValues[g_skillOrder[idx]]);
            sprintf(text, "%d.%d", val / 10, val % 10);
        }
        b6_text_measure(b6_font, 9, text, &textW, &meas2c, &yOff2);
        b6_text_draw(b6_font, clip[2] - textW - 0x36, yOff + yy, 9, 0x288, text, b6_frame_bitmap(this, 0)->px, width, clip);
        clip[2] = listRight - 2;

    L_475310:
    L_475316:
        yy = yy + 0x11;
    L_475319:
        if (reinterpret_cast<uintptr_t>(grpName) < GADDR(g_groupNames) || *slot != 0) {
            idx = idx + 1;
            if (g_skillGroup[g_skillOrder[idx]] != grp) {
                for (;;) {
                    if (idx == selIndex) {
                        if (selGroup == grp)
                            break;
                    }
                    if (idx >= g_skillCount)
                        break;
                    idx = idx + 1;
                    if (g_skillGroup[g_skillOrder[idx]] == grp)
                        break;
                }
            }
            if (idx != g_skillCount)
                goto L_475396;
        }
        // next group
        slot = slot + 1;
        grpName = grpName + 0x11;
        grp = grp + 1;
        idx = -1;

    L_475396:
        if (grp >= g_groupCount)
            break;
        bot = yy + 0x10;
        pad34 = bot - 0xc;
        if (pad34 + 0xb > clip[3])
            break;
    }

L_end:
    RebuildButtons();
}
HOOK_METHOD(0x00474cc0, CSkillListGump, Redraw);

// 0x004755b0  size 0x327  CSkillGump hit-test row at (x, y)   (thiscall, ECX = this, 4 stack args, RET 0x10)
// Walks the visible rows like Redraw, measuring each label; returns 1 with *isGroup / *index set when the point is on a label.
int __thiscall CSkillListGump::HitTestRow(int px, int py, int *isGroup, int *index)
{
    char buf[80];
    int h0;
    int h1;
    int textW;
    int rowTop;
    int rowBottom;
    int32_t *rowPtr;
    char *groupName;
    int item;
    int grp;
    int textX;
    int extra;

    if (px < listLeft + 2) return 0;
    if (px > listRight - 2) return 0;
    rowTop = listTop + 2;
    if (py < rowTop) return 0;
    if (py > listBottom - 2) return 0;

    if (onGroupHeader != 0) {
        grp = cursor;
        item = -1;
    } else {
        item = cursor;
        grp = g_skillGroup[g_skillOrder[item]];
    }
    if (grp >= g_groupCount) return 0;

    rowBottom = rowTop + 0x11;
    rowPtr = &groupExpanded[grp];
    groupName = g_groupNames + grp * 0x11;

    for (;;) {
        if (rowBottom - 2 > listBottom - 2) return 0;

        if (dragging == 0 || selIndex != item || item == -1 || selGroup == grp) {
            if (py >= rowTop && py < rowBottom) {
                if (item == -1) {
                    sprintf(buf, "%s", groupName);
                    textX = listLeft + 2;
                    extra = 0x32;
                    b6_text_measure(b6_font, 6, buf, &textW, &h0, &h1);
                } else {
                    int sk = g_skillOrder[item];
                    unsigned v;
                    if (useOwnValues != 0)
                        v = skillValue[sk];
                    else
                        v = g_skillValues[sk];
                    if (g_skillLockState[sk] <= -1) {
                        sprintf(buf, "%s - %d%%", g_skillNames + sk * 0x50, v);
                        textX = listLeft + 0x16;
                        extra = 0;
                    } else {
                        sprintf(buf, "%s - %d%%", g_skillNames + sk * 0x50, v);
                        textX = listLeft + 0x16;
                        extra = 0x1e;
                    }
                    b6_text_measure(b6_font, 9, buf, &textW, &h0, &h1);
                }
                if (px >= textX && px < extra + textX + textW) {
                    if (item >= 0) {
                        *isGroup = 0;
                        *index = item;
                        return 1;
                    }
                    g_skillRowTop = rowTop;
                    *isGroup = 1;
                    *index = grp;
                    return 1;
                }
            }
            rowTop += 0x11;
            rowBottom += 0x11;
        }

        // advance to the next item / group
        if (reinterpret_cast<uintptr_t>(groupName) < GADDR(g_groupNames) || *rowPtr != 0) {
            item++;
            if (g_skillGroup[g_skillOrder[item]] != grp) {
                for (;;) {
                    if (item == selIndex && selGroup == grp) break;
                    if (item >= g_skillCount) break;
                    item++;
                    if (g_skillGroup[g_skillOrder[item]] == grp) break;
                }
            }
            if (item != g_skillCount) {
                if (grp >= g_groupCount) return 0;
                continue;
            }
        }
        // next group
        rowPtr++;
        grp++;
        groupName += 0x11;
        item = -1;
        if (grp >= g_groupCount) return 0;
    }
}
HOOK_METHOD(0x004755b0, CSkillListGump, HitTestRow);
