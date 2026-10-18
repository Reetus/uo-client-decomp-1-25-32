// Button callbacks and small handlers from 0x0040cf90 to 0x0040df70. The callbacks are passed by
// address to gump constructors and are called with ECX = the gump that owns the button, so each is a
// __fastcall taking that gump as its only argument (EDX unused, nothing popped).
#include <cstring>

#include "../runtime/ported.h"
#include "bill_gump.h"
#include "fighting_gump.h"
#include "globals.h"
#include "object_manager.h"
#include "crt.h"
#include "raw_field.h"
#include "shop_gump.h"
#include "skill_list_gump.h"
#include "status_gump.h"
#include "client_socket.h"
#include "scroll_thumb_gump.h"
#include "packet_builders_batch24_ports.h"
#include "gump_manager.h"
#include "client_misc_ops_ports.h"
#include "gump_cascade_ports.h"
#include "yes_no_gump.h"
#include "font_manager.h"
#include "text_gump.h"
#include "packet_builders_b25_ports.h"
#include "char_flow_gumps_ports.h"
#include "small_methods_u1_ports.h"
#include "class_0049c690.h"

#define g_packetBuilder0 GLOBAL_PTR(void, g_packetBuilder_storage)    // this for packet-building calls
#define g_fontObj GLOBAL_PTR(void, g_fontMgr_storage)

namespace {

inline void SendPacket(char *pkt) { reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(pkt)); }
inline void ThumbSet(void *thumb, int value, int range) { reinterpret_cast<CScrollThumbGump *>(thumb)->SetPosition(value, range); }

} // namespace

// 0x0040cf90  size 0x4b
// Toggle the frame index of the child at +0x10c (flag at its +0x10) and send packet 0x56 with sub-command 6
// for the owner object.
void __fastcall UiCb_ToggleChildSend56(CGump *self)
{
    char pkt[0x10];
    CGump *child = RAWF(CGump *, self, 0x10c);
    int old = child->frameIndex;
    child->frameIndex = (old == 0);
    PORT_THISCALL6(&Build_Packet_0x56, void, g_packetBuilder0, char *, int, int, int, int, int)(pkt,
                                                                    RAWF(int32_t, self->owner, 0x78), 6, 0, 0, 0);
    SendPacket(pkt);
}
HOOK_FUNC(0x0040cf90, UiCb_ToggleChildSend56);

// 0x0040cfe0  size 0x1ce
// Open (or un-iconify and raise) the status gump of the owner object, then request its status (packet 0x34,
// sub-command 4). A mobile other than the player that the 0x00478cd0 test does not allow gets a pinned gump.
// The inlined cascade-slot release of the original is the call to 0x0040d300. SEH omitted.
void __fastcall UiCb_OpenStatusGump(CGump *self)
{
    char pkt[0x10];
    CObj *obj = self->owner;
    CGump *g = reinterpret_cast<CGumpManager *>(g_gumpManager)->FindGumpForObject(obj, 0, 0, g_gumpListTail);
    if (g == nullptr) {
        if (obj != g_player && Misc_Return0() == 0) {
            CStatusGump *sg = new CStatusGump(obj, 1);
            CGump_AddChild(sg, nullptr, 1);
        } else {
            CStatusGump *sg = new CStatusGump(obj, 0);
            CGump_AddChild(sg, nullptr, 1);
        }
    } else {
        if (g->unk_60 != 0) {
            if (obj == g_player || Misc_Return0() != 0) {
                int32_t curX = g->x;
                int32_t curY = g->y;
                g->x = g->unk_7c;
                g->unk_84 = curX;
                int32_t keyX = g->unk_8c;
                g->unk_60 = 0;
                g->unk_88 = curY;
                g->y = g->unk_80;
                if (keyX != 0)
                    GumpCascade_Free(keyX, g->unk_90);   // release cascade slot
            }
        }
        CGump_BringToFront(g, 0);
    }
    PORT_THISCALL3(&Build_Packet_0x34, void, g_packetBuilder0, char *, int, int)(pkt, 4, RAWF(int32_t, self->owner, 0x78));
    SendPacket(pkt);
}
HOOK_FUNC(0x0040cfe0, UiCb_OpenStatusGump);

// 0x0040d4f0  size 0x3c
// Ask for the player's own status (packet 0x34, sub-command 5) and set the flag at 0x004b7188.
void __cdecl UiCb_RequestPlayerStatus()
{
    char pkt[0x10];
    PORT_THISCALL3(&Build_Packet_0x34, void, g_packetBuilder0, char *, int, int)(pkt, 5, RAWF(int32_t, g_player, 0x78));
    SendPacket(pkt);
    g_desktopChanged = 1;
}
HOOK_FUNC(0x0040d4f0, UiCb_RequestPlayerStatus);

// 0x0040d530  size 0x142
// Toggle the fighting gump (singleton at 0x00648908): create it at (100, 100), or if it exists restore it
// from its iconified state (releasing the cascade slot) and raise it. SEH omitted.
void __cdecl UiCb_ToggleFightingGump()
{
    CGump *g = g_fightingGump;
    if (g == nullptr) {
        CFightingGump *fg = new CFightingGump(100, 100);
        g_fightingGump = fg;
        CGump_AddChild(fg, nullptr, 1);
        return;
    }
    if (g->unk_60 != 0) {
        int32_t curX = g->x;
        int32_t curY = g->y;
        g->unk_84 = curX;
        g->x = g->unk_7c;
        int32_t keyX = g->unk_8c;
        g->unk_88 = curY;
        g->unk_60 = 0;
        g->y = g->unk_80;
        if (keyX != 0)
            GumpCascade_Free(keyX, g->unk_90);   // release cascade slot
        g = g_fightingGump;
    }
    CGump_BringToFront(g, 0);
}
HOOK_FUNC(0x0040d530, UiCb_ToggleFightingGump);

// 0x0040d6a0  size 0x70
// Open the "Quit Ultima Online?" yes / no dialog (callbacks 0x00410dc0 and 0x0040cf60). SEH omitted.
void __cdecl OpenQuitDialog()
{
    CGump *dlg = new CYesNoGump(g_quitText, (void *)(PORT_ADDR(&CF_ResetToMainMenu)), (void *)(PORT_ADDR(&DeleteGumpCallback)), 0);
    CGump_AddChild(dlg, nullptr, 1);
}
HOOK_FUNC(0x0040d6a0, OpenQuitDialog);

// 0x0040d720  size 0xad
// Text gump "scroll down" button: advance the top line by 6 pixels, moving to the next text node when the
// current one is used up; then reposition the scroll thumb and redraw. Fields: +0xb0 / +0xb8 text left /
// right, +0xc0 thumb, +0xc4 frame holder, +0xc8 current text node, +0xcc pixel offset into it.
void __fastcall UiCb_TextScrollDown(CGump *self)
{
    int outBase, outH, outW;
    RAWF(CGump *, self, 0xc4)->frameIndex = 1;
    int wrapW = RAWF(int32_t, self, 0xb8) - RAWF(int32_t, self, 0xb0) + 1;
    if (RAWF(char *, self, 0xc8) != nullptr) {
        RAWF(int32_t, self, 0xcc) = RAWF(int32_t, self, 0xcc) + 6;
        reinterpret_cast<CFontManager *>(g_fontObj)->MeasureTextWrapped(0, (char *)(*reinterpret_cast<const char **>(RAWF(char *, self, 0xc8))), wrapW, &outW, &outH, &outBase, 2);
        int off = RAWF(int32_t, self, 0xcc);
        if (off >= outH) {
            if (RAWF(char *, RAWF(char *, self, 0xc8), 0x18) != nullptr) {
                RAWF(int32_t, self, 0xcc) = off - outH;
                RAWF(char *, self, 0xc8) = RAWF(char *, RAWF(char *, self, 0xc8), 0x18);
            } else {
                RAWF(int32_t, self, 0xcc) = outH - 1;
            }
        }
    }
    reinterpret_cast<CScrollThumbGump *>(RAWF(void *, self, 0xc0))->PlaceFromTextList();   // CScrollThumbGump::PlaceFromTextList
    reinterpret_cast<CTextGump *>(self)->RedrawText(0, 1);       // CTextGump::RedrawText
}
HOOK_FUNC(0x0040d720, UiCb_TextScrollDown);

// 0x0040d7d0  size 0x9f
// Text gump "scroll up" button: move the top line up by 6 pixels, into the previous text node when it goes
// below zero.
void __fastcall UiCb_TextScrollUp(CGump *self)
{
    int outBase, outH, outW;
    RAWF(CGump *, self, 0xc4)->frameIndex = 1;
    int wrapW = RAWF(int32_t, self, 0xb8) - RAWF(int32_t, self, 0xb0) + 1;
    RAWF(int32_t, self, 0xcc) -= 6;
    if (RAWF(int32_t, self, 0xcc) < 0) {
        char *cur = RAWF(char *, self, 0xc8);
        char *prev = RAWF(char *, cur, 0x1c);
        if (prev != nullptr) {
            RAWF(char *, self, 0xc8) = prev;
            reinterpret_cast<CFontManager *>(g_fontObj)->MeasureTextWrapped(0, (char *)(*reinterpret_cast<const char **>(prev)), wrapW, &outW, &outH, &outBase, 2);
            RAWF(int32_t, self, 0xcc) = RAWF(int32_t, self, 0xcc) + outH;
        } else {
            RAWF(int32_t, self, 0xcc) = 0;
        }
    }
    reinterpret_cast<CScrollThumbGump *>(RAWF(void *, self, 0xc0))->PlaceFromTextList();   // CScrollThumbGump::PlaceFromTextList
    reinterpret_cast<CTextGump *>(self)->RedrawText(1, 0);       // CTextGump::RedrawText
}
HOOK_FUNC(0x0040d7d0, UiCb_TextScrollUp);

// 0x0040d870  size 0x40
// Tip window "next tip": request the tip (packet 0xa7, flag 1) and close the window.
void __fastcall UiCb_TipNext(CGump *self)
{
    char pkt[8];
    PORT_THISCALL3(&Pkt_Build_0xA7_RequestTip, void, g_packetBuilder0, char *, uint16_t, int)(pkt, GLOBAL_AS(uint16_t, g_tipNumber), 1);
    SendPacket(pkt);
    if (self != nullptr)
        PORT_VSLOT1(self, 0x00, void *, int)(1);   // deleting destructor
}
HOOK_FUNC(0x0040d870, UiCb_TipNext);

// 0x0040d8b0  size 0x40
// Tip window "previous tip" / close: request with flag 0 and close the window.
void __fastcall UiCb_TipPrev(CGump *self)
{
    char pkt[8];
    PORT_THISCALL3(&Pkt_Build_0xA7_RequestTip, void, g_packetBuilder0, char *, uint16_t, int)(pkt, GLOBAL_AS(uint16_t, g_tipNumber), 0);
    SendPacket(pkt);
    if (self != nullptr)
        PORT_VSLOT1(self, 0x00, void *, int)(1);   // deleting destructor
}
HOOK_FUNC(0x0040d8b0, UiCb_TipPrev);

// 0x0040d8f0  size 0x2f
// Text gump: when the frame holder (+0xc4) has not been touched (+0x10 == 0), scroll to the end, place the
// thumb and redraw.
void __fastcall UiCb_TextScrollToEnd(CGump *self)
{
    if (RAWF(CGump *, self, 0xc4)->frameIndex == 0) {
        reinterpret_cast<CTextGump *>(self)->ScrollToEnd();                       // CTextGump::ScrollToEnd
        reinterpret_cast<CScrollThumbGump *>(RAWF(void *, self, 0xc0))->PlaceFromTextList();   // CScrollThumbGump::PlaceFromTextList
        reinterpret_cast<CTextGump *>(self)->RedrawText(1, 1);       // CTextGump::RedrawText
    }
}
HOOK_FUNC(0x0040d8f0, UiCb_TextScrollToEnd);

// 0x0040d920  size 0x3f
// Pick up the item the gump shows: close the gump through its slot 0x1c, then start dragging the owner
// (when it is an item, slot 0x2c) with the count held at +0x9c.
void __fastcall UiCb_PickUpOwner(CGump *self)
{
    CObj *item = nullptr;
    int count = RAWF(int32_t, self, 0x9c);
    if (PORT_VSLOT0(self->owner, 0x2c, int)() != 0)
        item = self->owner;
    PORT_VSLOT2(self, 0x1c, void, int, int)(0, 0);
    if (item != nullptr && count != 0)
        (&g_objMgr)->CObjectManager::BeginPickUp((CDynamicObj *)(item), (uint16_t)(count));   // CObjectManager::BeginPickUp
}
HOOK_FUNC(0x0040d920, UiCb_PickUpOwner);

// 0x0040d960  size 0x12b
// Shop gump "next": sell mode steps the first visible item (+0xc2) up; buy mode steps the vendor list cursor
// (+0xa4, next at +0x40), falling back to the head of the vendor's second container, then redraws and puts
// the scroll thumb at the cursor's index among the two containers' items.
void __fastcall UiCb_ShopNext(CShopGump *self)
{
    int index = 0;
    int found = 0;
    if (RAWF(int32_t, self->billGump, 0xac) != 0)
        return;
    if (self->sellMode != 0) {
        uint16_t cur = self->unk_c2;
        if (static_cast<int>(cur) >= static_cast<int>(self->itemCount) - 1)
            return;
        self->unk_c2 = static_cast<uint16_t>(cur + 1);
        self->CShopGump::RedrawFrame();   // RedrawFrame
        int range = self->itemCount;
        int value = self->unk_c2;
        ThumbSet(self->scrollThumb, value, range);
        return;
    }
    uint8_t *cursor = RAWF(uint8_t *, self, 0xa4);
    if (cursor == nullptr)
        return;
    uint8_t *next = RAWF(uint8_t *, cursor, 0x40);
    if (next != nullptr) {
        RAWF(uint8_t *, self, 0xa4) = next;
    } else if (self->unk_b4 == 0) {
        uint8_t *vendor2 = RAWF(uint8_t *, self->owner, 0x130);
        uint8_t *first = RAWF(uint8_t *, vendor2, 0xa8);
        if (first != nullptr) {
            RAWF(uint8_t *, self, 0xa4) = first;
            self->unk_b4 = 1;
        }
    }
    self->CShopGump::RedrawFrame();   // RedrawFrame
    CObj *vendor = self->owner;
    uint8_t *cont = RAWF(uint8_t *, vendor, 0x12c);
    if (cont != nullptr) {
        uint8_t *it = RAWF(uint8_t *, cont, 0xa8);
        if (it != nullptr) {
            uint8_t *sel = RAWF(uint8_t *, self, 0xa4);
            do {
                if (sel == it)
                    found = index;
                it = RAWF(uint8_t *, it, 0x40);
                index++;
            } while (it != nullptr);
        }
    }
    cont = RAWF(uint8_t *, vendor, 0x130);
    if (cont != nullptr) {
        uint8_t *it = RAWF(uint8_t *, cont, 0xa8);
        if (it != nullptr) {
            uint8_t *sel = RAWF(uint8_t *, self, 0xa4);
            do {
                if (sel == it)
                    found = index;
                it = RAWF(uint8_t *, it, 0x40);
                index++;
            } while (it != nullptr);
        }
    }
    ThumbSet(self->scrollThumb, found, index);
}
HOOK_FUNC(0x0040d960, UiCb_ShopNext);

// 0x0040da90  size 0x12b
// Shop gump "previous": the mirror of 0x0040d960 (previous link at +0x44; from the vendor's second container
// back to the tail of the first).
void __fastcall UiCb_ShopPrev(CShopGump *self)
{
    int index = 0;
    int found = 0;
    if (RAWF(int32_t, self->billGump, 0xac) != 0)
        return;
    if (self->sellMode != 0) {
        uint16_t cur = self->unk_c2;
        if (cur <= 0)
            return;
        self->unk_c2 = static_cast<uint16_t>(cur - 1);
        self->CShopGump::RedrawFrame();   // RedrawFrame
        int range = self->itemCount;
        int value = self->unk_c2;
        ThumbSet(self->scrollThumb, value, range);
        return;
    }
    uint8_t *cursor = RAWF(uint8_t *, self, 0xa4);
    if (cursor == nullptr)
        return;
    uint8_t *prev = RAWF(uint8_t *, cursor, 0x44);
    if (prev != nullptr) {
        RAWF(uint8_t *, self, 0xa4) = prev;
    } else if (self->unk_b4 != 0) {
        uint8_t *cont = RAWF(uint8_t *, self->owner, 0x12c);
        uint8_t *first = RAWF(uint8_t *, cont, 0xa8);
        if (first != nullptr) {
            RAWF(uint8_t *, self, 0xa4) = first;
            if (RAWF(uint8_t *, first, 0x40) != nullptr) {
                uint8_t *it;
                do {
                    it = RAWF(uint8_t *, RAWF(uint8_t *, self, 0xa4), 0x40);
                    RAWF(uint8_t *, self, 0xa4) = it;
                } while (RAWF(uint8_t *, it, 0x40) != nullptr);
            }
            self->unk_b4 = 0;
        }
    }
    self->CShopGump::RedrawFrame();   // RedrawFrame
    CObj *vendor = self->owner;
    uint8_t *cont = RAWF(uint8_t *, vendor, 0x12c);
    if (cont != nullptr) {
        uint8_t *it = RAWF(uint8_t *, cont, 0xa8);
        if (it != nullptr) {
            uint8_t *sel = RAWF(uint8_t *, self, 0xa4);
            do {
                if (sel == it)
                    found = index;
                it = RAWF(uint8_t *, it, 0x40);
                index++;
            } while (it != nullptr);
        }
    }
    cont = RAWF(uint8_t *, vendor, 0x130);
    if (cont != nullptr) {
        uint8_t *it = RAWF(uint8_t *, cont, 0xa8);
        if (it != nullptr) {
            uint8_t *sel = RAWF(uint8_t *, self, 0xa4);
            do {
                if (sel == it)
                    found = index;
                it = RAWF(uint8_t *, it, 0x40);
                index++;
            } while (it != nullptr);
        }
    }
    ThumbSet(self->scrollThumb, found, index);
}
HOOK_FUNC(0x0040da90, UiCb_ShopPrev);

// 0x0040dbc0  size 0x62
// Bill gump "next": step the first visible bill line (+0xb4) to the next one and redraw; the thumb (+0xc0)
// goes to that line's position in the shop's cart list.
void __fastcall UiCb_BillNext(CBillGump *self)
{
    if (RAWF(int32_t, self, 0xac) != 0)
        return;
    BillEntry *cur = self->entries;
    if (cur == nullptr)
        return;
    BillEntry *next = cur->next;
    if (next == nullptr)
        return;
    self->entries = next;
    self->CBillGump::RedrawFrame();   // RedrawFrame
    BillEntry *node = RAWF(BillEntry *, self->shop, 0xcc);
    int index = 0;
    int found = 0;
    if (node != nullptr) {
        BillEntry *sel = self->entries;
        do {
            if (node == sel)
                found = index;
            node = node->next;
            index++;
        } while (node != nullptr);
    }
    ThumbSet(RAWF(void *, self, 0xc0), found, index);
}
HOOK_FUNC(0x0040dbc0, UiCb_BillNext);

// 0x0040dc30  size 0x82
// Bill gump "previous": find the node before the first visible line in the shop's cart list, make it the
// first visible line, redraw and move the thumb.
void __fastcall UiCb_BillPrev(CBillGump *self)
{
    if (RAWF(int32_t, self, 0xac) != 0)
        return;
    BillEntry *sel = self->entries;
    BillEntry *node = RAWF(BillEntry *, self->shop, 0xcc);
    if (sel == nullptr || node == sel || node == nullptr)
        return;
    for (;;) {
        BillEntry *nx = node->next;
        if (nx == sel)
            break;
        node = nx;
        if (node == nullptr)
            break;
    }
    if (node == nullptr)
        return;
    self->entries = node;
    self->CBillGump::RedrawFrame();   // RedrawFrame
    BillEntry *it = RAWF(BillEntry *, self->shop, 0xcc);
    int index = 0;
    int found = 0;
    if (it != nullptr) {
        BillEntry *cur = self->entries;
        do {
            if (it == cur)
                found = index;
            it = it->next;
            index++;
        } while (it != nullptr);
    }
    ThumbSet(RAWF(void *, self, 0xc0), found, index);
}
HOOK_FUNC(0x0040dc30, UiCb_BillPrev);


// 0x0040dcc0  size 0x15d
// (The redraw / scroll tail, 0x00474cc0 and 0x00413c00, appears twice in the original and once here.)
// Skills window "down" arrow: move the cursor to the next entry. On a group header: into the group's first
// skill row if the group is expanded, else to the next group header. On a skill row: to the next row of the
// same group, else to the next group's header. The scroll position follows by one.
void __fastcall UiCb_SkillListDown(CSkillListGump *self)
{
    int next;
    self->CSkillListGump::OnOk();
    if (self->onGroupHeader != 0) {
        int cur = self->cursor;
        if (self->groupExpanded[cur] != 0) {
            int row = 0;
            int rows;
            if (g_skillGroup[g_skillOrder[0]] != cur) {
                rows = g_skillCount;
                for (;;) {
                    if (row >= rows)
                        goto groupNotFound;
                    row++;
                    if (g_skillGroup[g_skillOrder[row]] == cur)
                        break;
                }
            } else {
                rows = g_skillCount;
            }
            if (row >= rows)
                goto groupNotFound;
            self->onGroupHeader = 0;
            next = row;
            goto moveTo;
        groupNotFound:
            next = cur + 1;
            if (next >= g_groupCount)
                return;
            goto moveTo;
        }
        next = cur + 1;
        if (next >= g_groupCount)
            return;
        goto moveTo;
    } else {
        int row = self->cursor;
        int group = g_skillGroup[g_skillOrder[row]];
        row++;
        int rows;
        if (g_skillGroup[g_skillOrder[row]] != group) {
            rows = g_skillCount;
            for (;;) {
                if (row >= rows)
                    goto rowsDone;
                row++;
                if (g_skillGroup[g_skillOrder[row]] == group)
                    break;
            }
        } else {
            rows = g_skillCount;
        }
    rowsDone:
        if (row < rows) {
            next = row;
            goto moveTo;
        }
        next = group + 1;
        if (next >= g_groupCount)
            return;
        self->onGroupHeader = 1;
    }
moveTo:
    self->cursor = next;
    self->CSkillListGump::Redraw(-1);   // Redraw(-1)
    {
        int pos = self->scrollPos + 1;
        int range = self->scrollRange;
        self->scrollPos = pos;
        ThumbSet(reinterpret_cast<void *>(static_cast<uintptr_t>(self->unk_94)), pos, range);
    }
}
HOOK_FUNC(0x0040dcc0, UiCb_SkillListDown);

// 0x0040de20  size 0x152
// (The redraw / scroll tail, 0x00474cc0 and 0x00413c00, appears three times in the original (0x0040df03, 0x0040df46, 0x0040de57: SetPosition 0x00413c00 each time) and once here.)
// Skills window "up" arrow: the mirror of 0x0040dcc0. On a group header: to the last row of the previous
// group when it is expanded (or of the ungrouped rows for group -1), else its header. On a skill row: to the
// previous row of the same group, else to the group's header. The scroll position follows by minus one.
void __fastcall UiCb_SkillListUp(CSkillListGump *self)
{
    int next;
    self->CSkillListGump::OnOk();
    int cur = self->cursor;
    if (self->onGroupHeader != 0) {
        int prevGroup = cur - 1;
        if (prevGroup != -1) {
            if (self->groupExpanded[prevGroup] == 0) {
                if (prevGroup <= -1)
                    return;
                next = prevGroup;
                goto moveTo;
            }
        }
        {
            int r = g_skillCount - 1;
            if (r >= 0) {
                do {
                    if (g_skillGroup[g_skillOrder[r]] == prevGroup)
                        break;
                    r--;
                } while (r >= 0);
            }
            if (r >= 0) {
                self->onGroupHeader = 0;
                next = r;
                goto moveTo;
            }
        }
        if (prevGroup <= -1)
            return;
        next = prevGroup;
        goto moveTo;
    } else {
        int group = g_skillGroup[g_skillOrder[cur]];
        int r = cur - 1;
        if (r >= 0) {
            do {
                if (g_skillGroup[g_skillOrder[r]] == group)
                    break;
                r--;
            } while (r >= 0);
        }
        if (r >= 0) {
            next = r;
            goto moveTo;
        }
        if (group <= -1)
            return;
        self->onGroupHeader = 1;
        next = group;
    }
moveTo:
    self->cursor = next;
    self->CSkillListGump::Redraw(-1);   // Redraw(-1)
    {
        int pos = self->scrollPos - 1;
        int range = self->scrollRange;
        self->scrollPos = pos;
        ThumbSet(reinterpret_cast<void *>(static_cast<uintptr_t>(self->unk_94)), pos, range);
    }
}
HOOK_FUNC(0x0040de20, UiCb_SkillListUp);
