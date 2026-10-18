#include <cstring>

#include "../runtime/ported.h"
#include "bb_gump.h"
#include "bb_line_gump.h"
#include "bb_msg_gump.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "packet_builders_b25_ports.h"
#include "client_socket.h"
#include "scroll_thumb_gump.h"
#include "font_manager.h"

#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)         // this for packet-building calls
#define g_fontObj GLOBAL_PTR(void, g_fontMgr_storage)

// 0x00403920  size 0xef
// CBBGump::OnPostNew. The SEH frame is omitted. Collects the player's equipped
// (graphic, hue) pairs and opens a CBBMsgGump (ctor 0x00404610, 0x2b4 bytes) to compose a post.
void __thiscall CBBGump::OnPostNew(uint32_t parentSerial, char *subject)
{
    struct Equip {
        uint16_t id;
        uint16_t hue;
    };
    // The original had room for 20 entries (and overflowed for more); 25 layers exist.
    Equip eq[25];
    uint8_t n = 0;
    void *player = g_player;

    for (int layerIdx = 1; layerIdx < 0x1a; layerIdx++) {
        uint8_t l = g_equipLayerOrder[layerIdx];
        void *item = RAWF(void *, player, 0xc4 + l * 4);
        if (item != nullptr) {
            eq[n].id = RAWF(uint16_t, item, 0x38);
            eq[n].hue = RAWF(uint16_t, item, 0x3e);
            n = static_cast<uint8_t>(n + 1);
        }
    }

    // Argument order from the push sequence: board (this->owner) first.
    CBBMsgGump *gump = new CBBMsgGump(owner, parentSerial, g_playerName, subject, (char *)(g_bbPostText), RAWF(uint16_t, g_player, 0x38), RAWF(uint16_t, g_player, 0x3e), n, (uint16_t *)(eq), 1);
    CGump_AddChild(gump, nullptr, 1);
}
HOOK_METHOD(0x00403920, CBBGump, OnPostNew);

// 0x00403fc0  size 0xed
// CBBLineGump::MouseUp. Clears the press state; on a pending second click
// sends a type-3 packet once (guarded by 0x004b6b48), otherwise a click at x < 0x16 toggles the replies.
void __thiscall CBBLineGump::MouseUp(int mouseX, int mouseY)
{
    char packetBuf[0x1904];

    unk_a4 = 0;
    g_dragGump = nullptr;
    if (unk_9c != 0) {
        unk_9c = 0;
        clickState = 0;
        unk_a8 = 0;
        return;
    }
    if (clickState == 2) {
        clickState = 0;
        if (g_bbRequestPending == 0) {
            g_bbRequestPending = 1;
            if (post != nullptr)
                g_bbRequestSerial = RAWF(int32_t, post, 0x78);
            else
                g_bbRequestSerial = 0;
            PORT_THISCALL4(&Pkt_Build_0x71_BBoardSimple, void, g_packetBuilder, char *, int, int, int)(packetBuf, 3, RAWF(int32_t, parent->owner, 0x78), RAWF(int32_t, owner, 0x78));
            reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(packetBuf));
            unk_a8 = 0;
            return;
        }
    } else if (mouseX < 0x16) {
        clickState = 0;
        this->CBBLineGump::ToggleReplies(mouseX, mouseY);   // toggle replies
    }
    unk_a8 = 0;
}
HOOK_VMETHOD(0x00403fc0, CBBLineGump, MouseUp);

// 0x004045c0  size 0x4b
// CBBLineGump::PrevVisibleLine. Walks the +0x40 chain.
// ? pass2 calls +0x40 the previous sibling; gump.h names it `next`.
CBBLineGump *__thiscall CBBLineGump::PrevVisibleLine()
{
    CBBLineGump *l;
    for (l = static_cast<CBBLineGump *>(next); l != nullptr; l = static_cast<CBBLineGump *>(l->next)) {
        if (PORT_VSLOT0(l, 0xe0, int)() != 0) {
            int isVisible;
            if (l->post == nullptr)
                isVisible = 1;
            else
                isVisible = (RAWF(int32_t, RAWF(void *, l->post, 0x88), 0x98) == 1);
            if (isVisible != 0)
                return l;
        }
    }
    return l;
}
HOOK_METHOD(0x004045c0, CBBLineGump, PrevVisibleLine);

// 0x004054e0  size 0x8f6
// CBBMsgGump::OnKey. Text editing for the post composer. lines[] hold malloc(0x51)
// buffers; `lastLine` is the index of the virtual last line (lines[lastLine] exists).
// FUN_00406270 redraws the body; the scrollbar update is thiscall 0x00413c00 on the child at +0x98.
// Clamp the caret to the end of the current line, then redraw (0x405d25).
static void ClampCaretAndRedraw(CBBMsgGump *gump)
{
    char *line = gump->lines[gump->cur];
    if (gump->caret > static_cast<int>(strlen(line)))
        gump->caret = static_cast<int>(strlen(line));
    gump->CBBMsgGump::RedrawText();
}

void __thiscall CBBMsgGump::OnKey(int key)
{
    char tmp[0x50];

    char *l;
    int n, i, split, prevLen;

    if (key == 0xe021) {
        if (cur > 0)
            cur = 0;
        ClampCaretAndRedraw(this);
        return;
    }

    if (key == 0xe022) {
        int last = lastLine - 1;
        if (cur < last)
            cur = last;
        if (cur <= (last < 0 ? 0 : last)) {
            ClampCaretAndRedraw(this);
            return;
        }
        cur = (last < 0) ? 0 : last;
        ClampCaretAndRedraw(this);
        return;
    }

    if (key == 0xe024) {   // Home
        if (caret > 0) {
            caret = 0;
            this->CBBMsgGump::RedrawText();
        }
        return;
    }

    if (key == 0xe023) {   // End
        char *line = lines[cur];
        if (caret >= static_cast<int>(strlen(line)))
            return;
        caret = static_cast<int>(strlen(line));
        this->CBBMsgGump::RedrawText();
        return;
    }

    if (key == 0xe025) {   // Left
        if (caret > 0) {
            caret = caret - 1;
            this->CBBMsgGump::RedrawText();
            return;
        }
        if (cur <= 0) {
            this->CBBMsgGump::RedrawText();
            return;
        }
        cur = cur - 1;
        caret = static_cast<int>(strlen(lines[cur]));
        this->CBBMsgGump::RedrawText();
        return;
    }

    if (key == 0xe027) {   // Right
        int c = cur;
        int cr = caret;
        if (cr < static_cast<int>(strlen(lines[c]))) {
            caret = cr + 1;
            this->CBBMsgGump::RedrawText();
            return;
        }
        if (c >= lastLine) {
            this->CBBMsgGump::RedrawText();
            return;
        }
        cur = c + 1;
        if (lineHeight * (c + 2) <= textBottom - textTop) {
            caret = 0;
            this->CBBMsgGump::RedrawText();
            return;
        }
        cur = c;
        this->CBBMsgGump::RedrawText();
        return;
    }

    if (key == 0xe026) {   // Up
        if (cur <= 0)
            return;
        cur = cur - 1;
        if (cur < top) {
            top = top - 1;
            reinterpret_cast<CScrollThumbGump *>(scrollbar)->SetPosition(top, lastLine);
        }
        ClampCaretAndRedraw(this);
        return;
    }

    if (key == 0xe028) {   // Down
        if (cur >= lastLine)
            return;
        if (cur >= 0xf9)
            return;
        cur = cur + 1;
        if ((cur - top + 1) * lineHeight + textOrigin > textBottom) {
            top = top + 1;
            reinterpret_cast<CScrollThumbGump *>(scrollbar)->SetPosition(top, lastLine);
        }
        if (cur >= lastLine) {
            caret = 0;
            this->CBBMsgGump::RedrawText();
            return;
        }
        ClampCaretAndRedraw(this);
        return;
    }

    if (key == 8) {   // Backspace
        if (caret > 0) {
            l = lines[cur];
            n = static_cast<int>(strlen(l));
            if (caret < n) {
                for (i = caret - 1; i < n - 1; i++)
                    l[i] = l[i + 1];
            }
            l[n - 1] = '\0';
            caret = caret - 1;
            if (lastLine != 1) {
                this->CBBMsgGump::RedrawText();
                return;
            }
            if (strlen(lines[0]) != 0) {
                this->CBBMsgGump::RedrawText();
                return;
            }
            lastLine = 0;
            this->CBBMsgGump::RedrawText();
            return;
        }

        if (cur <= 0)
            return;
        prevLen = static_cast<int>(strlen(lines[cur - 1]));
        strcat(lines[cur - 1], lines[cur]);
        split = reinterpret_cast<CFontManager *>(g_fontObj)->FindWrapBreak(9, lines[cur - 1], textRight - textLeft);
        if (split == -1) {
            split = static_cast<int>(strlen(lines[cur - 1]));
        } else {
            split = split + 1;
            if (lines[cur - 1][split] == ' ')
                split = split + 1;
        }
        if (split < static_cast<int>(strlen(lines[cur - 1]))) {
            strcpy(lines[cur], lines[cur - 1] + split);
            lines[cur - 1][split] = '\0';
        } else if (cur < lastLine) {
            i = cur;
            if (i < lastLine - 1) {
                do {
                    strcpy(lines[i], lines[i + 1]);
                    i = i + 1;
                } while (i < lastLine - 1);
            }
            lastLine = lastLine - 1;
            operator delete(lines[lastLine + 1]);
            lines[lastLine + 1] = nullptr;
        }
        caret = prevLen;
        cur = cur - 1;
        if (lastLine != 1) {
            this->CBBMsgGump::RedrawText();
            return;
        }
        if (strlen(lines[0]) != 0) {
            this->CBBMsgGump::RedrawText();
            return;
        }
        lastLine = 0;
        operator delete(lines[1]);
        this->CBBMsgGump::RedrawText();
        return;
    }

    if (key == 0xd) {   // Enter
        if (lastLine + 1 >= 0x50)
            return;
        lastLine = lastLine + 1;
        lines[lastLine] = static_cast<char *>(::operator new(0x51));
        strcpy(lines[lastLine], g_emptyString);
        i = lastLine - 1;
        if (i >= cur + 2) {
            do {
                strcpy(lines[i], lines[i - 1]);
                i = i - 1;
            } while (i >= cur + 2);
        }
        strcpy(lines[cur + 1], lines[cur] + caret);
        lines[cur][caret] = '\0';
        cur = cur + 1;
        if ((cur - top + 1) * lineHeight + textOrigin > textBottom)
            top = top + 1;
        caret = 0;
        this->CBBMsgGump::RedrawText();
        reinterpret_cast<CScrollThumbGump *>(scrollbar)->SetPosition(top, lastLine);
        return;
    }

    // printable character
    if (reinterpret_cast<CFontManager *>(g_fontObj)->HasGlyph(9, key) == 0)
        return;
    if (cur == lastLine) {
        if (lastLine + 1 >= 0x50)
            return;
        lastLine = lastLine + 1;
        lines[lastLine] = static_cast<char *>(::operator new(0x51));
        strcpy(lines[lastLine], g_emptyString);
    }
    l = lines[cur];
    if (caret == static_cast<int>(strlen(l))) {
        n = static_cast<int>(strlen(l));
        l[n] = static_cast<char>(key);
        lines[cur][n + 1] = '\0';
    } else {
        l = l + caret;
        strcpy(tmp, l);
        *l = static_cast<char>(key);
        lines[cur][caret + 1] = '\0';
        strcat(lines[cur], tmp);
    }
    caret = caret + 1;
    g_bbWrapFull = 0;
    i = cur;
    if (this->CBBMsgGump::WrapLine(i) != 0) {   // word-wrap line i; true if it pushed text to the next line
        do {
            i = i + 1;
        } while (this->CBBMsgGump::WrapLine(i) != 0);
    }
    if (i != cur) {
        this->CBBMsgGump::RedrawText();
        return;
    }
    if (g_bbWrapFull == 0) {
        this->CBBMsgGump::RedrawText();
        return;
    }
    caret = caret - 1;
    this->CBBMsgGump::RedrawText();
}
HOOK_VMETHOD(0x004054e0, CBBMsgGump, OnKey);
