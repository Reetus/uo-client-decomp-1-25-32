#include "char_ui_gumps.h"

#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "gump_manager.h"
#include "login_gumps.h"
#include "sound_manager.h"
#include "music_manager.h"
#include "client_misc_ops_ports.h"
#include "dirty_rects_ports.h"
#include "gump.h"
#include "hue_gump.h"
#include "batch02_helpers.h"
#include "packet_builders_ports.h"
#include "client_socket.h"
#include "packet_builders_b25_ports.h"
#include "art_row_blits_ports.h"

#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)    // this for packet-building calls
#define g_artPlain PTR_AT(void, PORT_ADDR(&RowBlitCopyNonZero))         // art drawer object (ECX of 0x00433ed0)
#define g_focusGump GLOBAL_AS(int32_t, g_focusedInput)

// 0x0040f180  size 0xc2
// Character select: "Change password" button. The SEH frame is omitted.
void __thiscall CCharSelectGump::OnChangePassword()
{
    int slot = selectedSlot;
    if (strlen(&RAWF(char, (&slots)[slot], 0xec)) == 0) {
        visible = 0;
        PORT_THISCALL4(&CreateGump2ac, void, g_gumpManager, const char *, CGump *, int, int)(
                                                                         "No character to change password on.", this, 1, 0);
        return;
    }
    visible = 0;
    CGump *dlg = new CChgPWGump(&RAWF(char, (&slots)[slot], 0xec), (char *)(g_emptyString));
    CGump_AddChild(dlg, nullptr, 1);
}
HOOK_METHOD(0x0040f180, CCharSelectGump, OnChangePassword);

namespace {

// Control selection globals written by CollectSelections.

inline int32_t CtlValue(CGump *ctl) { return RAWF(int32_t, ctl, 0xb0); }
inline CtlListNode *CtlItems(CGump *ctl) { return RAWF(CtlListNode *, ctl, 0xb8); }
inline char *ItemName(CtlListNode *node) { return &RAWF(char, node->ctl, 0xec); }
inline int32_t ItemValue(CtlListNode *node) { return RAWF(int32_t, node->ctl, 0xe8); }

} // namespace

// 0x0040fc30  size 0x487
// Character creation: copy the selections from the gump's controls into the globals.
// ? the meaning of the globals and of the callees 0x00478cf0 / 0x0044fdf0 is not established.
void __thiscall CCharCreateGump::CollectSelections()
{
    g_colorSay = hue_c0;
    g_hue1 = hue_c2;

    int v = CtlValue(ctl_c8);
    if (g_ccMusicVal != v)
        reinterpret_cast<CSoundManager *>(GLOBAL_PTR(void, g_soundManager_storage))->Enable(v);
    v = CtlValue(ctl_cc);
    if (g_ccSoundVal != v)
        reinterpret_cast<MusicManager *>(GLOBAL_PTR(void, g_musicManager_storage))->SetEnabled(v);
    g_ccVal62c = CtlValue(ctl_d4);
    // ECX carries the value just stored (no stack argument).
    Misc_SetFullscreen(g_ccVal62c);
    g_warnOnAttack = CtlValue(ctl_e0);
    g_ccVal2cc = CtlValue(ctl_d8);
    g_ccVal220 = CtlValue(ctl_dc);
    // ECX is left holding the value of ctl_dc; the callee takes no stack argument.
    PORT_FUNC1(&InvalidateAll, void, __fastcall, int)(g_ccVal220);

    // free the existing global list (nodes of 0xe4 bytes, entry strings at node+0x20+i*0x14)
    uint8_t *node = g_macroListHead;
    if (g_ccListCount > 0) {
        int n = 0;
        do {
            uint8_t *nextNode = RAWF(uint8_t *, node, 0xe0);
            int i = 0;
            if (RAWF(int32_t, node, 0x14) > 0) {
                uint8_t *p = node + 0x20;
                do {
                    if (RAWF(void *, p, 0) != nullptr)
                        operator delete(RAWF(void *, p, 0));
                    i++;
                    p += 0x14;
                } while (i < RAWF(int32_t, node, 0x14));
            }
            operator delete(node);
            n++;
            node = nextNode;
        } while (n < g_ccListCount);
    }
    g_ccListCount = 0;
    g_macroListHead = nullptr;

    // deep copy the list at this+0x164 into the global list
    uint8_t *src = profList;
    uint8_t *prevNode = nullptr;
    if (src != nullptr) {
        do {
            uint8_t *dst = static_cast<uint8_t *>(::operator new(0xe4));
            RAWF(int32_t, dst, 0xe0) = 0;
            RAWF(int32_t, dst, 0x00) = RAWF(int32_t, src, 0x00);
            RAWF(int32_t, dst, 0x04) = RAWF(int32_t, src, 0x04);
            RAWF(int32_t, dst, 0x08) = RAWF(int32_t, src, 0x08);
            RAWF(int32_t, dst, 0x0c) = RAWF(int32_t, src, 0x0c);
            RAWF(int32_t, dst, 0x10) = RAWF(int32_t, src, 0x10);
            RAWF(int32_t, dst, 0x14) = RAWF(int32_t, src, 0x14);
            if (RAWF(int32_t, dst, 0x14) > 0) {
                int i = 0;
                uint8_t *s = src + 0x20;
                uint8_t *d = dst + 0x18;
                do {
                    RAWF(int32_t, d, 0x00) = *reinterpret_cast<int32_t *>(s - 0x8);   // src[0x18 + i*0x14]
                    RAWF(int32_t, d, 0x04) = *reinterpret_cast<int32_t *>(s - 0x4);   // src[0x1c + i*0x14]
                    char *str = RAWF(char *, s, 0);                    // src[0x20 + i*0x14]
                    if (str != nullptr) {
                        char *copy = static_cast<char *>(::operator new(strlen(str) + 1));
                        RAWF(char *, d, 0x08) = copy;
                        memcpy(copy, RAWF(char *, s, 0), strlen(RAWF(char *, s, 0)) + 1);
                    } else {
                        RAWF(int32_t, d, 0x08) = 0;
                    }
                    i++;
                    s += 0x14;
                    d += 0x14;
                } while (i < RAWF(int32_t, dst, 0x14));
            }
            if (prevNode == nullptr)
                g_macroListHead = dst;
            else
                RAWF(uint8_t *, prevNode, 0xe0) = dst;
            g_ccListCount = g_ccListCount + 1;
            prevNode = dst;
            src = RAWF(uint8_t *, src, 0xe0);
        } while (src != nullptr);
    }

    if (page_f4->layer == 3) {
        g_ccVal1a0 = CtlValue(ctl_f8);
        g_ccSel650 = RAWF(int32_t, list_10c, 0xcc) - 1;
        CtlListNode *it = CtlItems(list_10c);
        if (it != nullptr) {
            int32_t *val = g_ccVals8b0;
            char *nm = g_ccNames658;
            while (it->next != nullptr) {
                strcpy(nm, ItemName(it));
                nm += 0x1e;
                val++;
                val[-1] = ItemValue(it);
                it = it->next;
            }
        }
        g_profanityOn = CtlValue(page_f4);
        g_ccSel654 = RAWF(int32_t, list_110, 0xcc) - 1;
        it = CtlItems(list_110);
        if (it != nullptr) {
            char *nm = g_ccNames900;
            while (it->next != nullptr) {
                strcpy(nm, ItemName(it));
                nm += 0x1e;
                it = it->next;
            }
        }
    }

    g_ccVal268 = CtlValue(ctl_134);
    if (g_ccVal268 != 0)
        g_ccSel264 = 0;
    else
        g_ccSel264 = RAWF(int32_t, list_130, 0xcc) - 1;
    {
        CtlListNode *it = CtlItems(list_130);
        if (it != nullptr) {
            int32_t *val = g_ccVals76c;
            char *nm = g_ccNames4c4;
            while (it->next != nullptr) {
                strcpy(nm, ItemName(it));
                nm += 0x1e;
                val++;
                val[-1] = ItemValue(it);
                it = it->next;
            }
        }
    }

    g_ccSel260 = RAWF(int32_t, list_12c, 0xcc) - 1;
    {
        CtlListNode *it = CtlItems(list_12c);
        if (it != nullptr) {
            int32_t *clr = g_ccVals7bc;
            char *nm = g_ccNames26c;
            int k = 0;
            while (it->next != nullptr) {
                strcpy(nm, ItemName(it));
                clr++;
                nm += 0x1e;
                g_ccVals71c[k] = ItemValue(it);   // EBX-0xa4 after EBX += 4
                clr[-1] = 0;                      // DAT_0069f7bc[k]
                k++;
                it = it->next;
            }
        }
    }
}
HOOK_METHOD(0x0040fc30, CCharCreateGump, CollectSelections);

// 0x004118b0  size 0xd1
// Toggle the "Emote Sample" gump. The SEH frame is omitted.
void __thiscall CCharCreateGump::ToggleEmoteSample()
{
    CGump *found = nullptr;
    for (CGump *g = g_gumpListTail; g != nullptr; g = g->prev) {
        if (PORT_VSLOT0(g, 0xfc, int)() != 0 && RAWF(int32_t, g, 0xb8) != 0)
            found = g;   // last match wins, the loop is not exited
    }
    if (found != nullptr) {
        found->CGump::BringToFront(0);   // bring to front
        return;
    }
    CGump *obj = new CHueGump(100, 100, 0, 1, "Emote Sample", hue_c2);
    RAWF(int32_t, obj, 0xb8) = 1;   // written even when the ctor result is NULL, as the asm does
    CGump_AddChild(obj, nullptr, 1);
}
HOOK_METHOD(0x004118b0, CCharCreateGump, ToggleEmoteSample);

// 0x00410110  size 0x50
void __thiscall CGump_00410110::SetStateTwo()
{
    if (ref_f0 == g_focusGump)
        PORT_THISCALL0(&Gump_CommitFocus, void, g_gumpManager)();
    if (layer != 2) {
        layer = 2;
        // ECX = 0x004764c0, EDX = 0x961 (art id), stack (0, 0, bitmap data, width, height)
        uint8_t *bmp = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(frameData[1]));
        PORT_FUNC7(&DrawGumpArt, void, __fastcall, void *, int, int, int, void *, int, int)(g_artPlain, 0x961, 0, 0,
                                                                                         bmp + 4, width, height);
    }
}
HOOK_METHOD(0x00410110, CGump_00410110, SetStateTwo);

// 0x00410820  size 0x47
// Build a type-5 packet naming this gump, send it, then delete this gump.
void __thiscall CGump_00410820::SendAndDelete()
{
    char packet[0x1904];
    PORT_THISCALL3(&Packet_Build0x71, void, g_packetBuilder, char *, int, void *)(packet, 5, this);
    reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(packet));
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);   // scalar deleting dtor
}
HOOK_METHOD(0x00410820, CGump_00410820, SendAndDelete);

// 0x00410910  size 0x44
void __thiscall CGump_00410910::SendOwnerPacket()
{
    char packet[0x1904];
    int a = RAWF(int32_t, RAWF(void *, owner, 0x7c), 0x78);
    int b = RAWF(int32_t, owner, 0x78);
    PORT_THISCALL4(&Pkt_Build_0x71_BBoardSimple, void, g_packetBuilder, char *, int, int, int)(packet, 6, a, b);
    reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(packet));
}
HOOK_METHOD(0x00410910, CGump_00410910, SendOwnerPacket);
