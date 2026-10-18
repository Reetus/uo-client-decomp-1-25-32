#include "char_creation_pages.h"

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "batch01_helpers.h"
#include "char_creation_gump.h"
#include "char_ui_gumps.h"
#include "city_choice_gump.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "scroll_choice_gump.h"
#include "packet_builders_batch24_ports.h"
#include "packet_builders_b25_ports.h"
#include "client_socket.h"
#include "gump_manager.h"
#include "config_file_ports.h"
#include "app_support_ports.h"
#include "app_session_ops_ports.h"
#include "tutorial_gump.h"
#include "client_misc_ops_ports.h"
#include "rotated_blit.h"
#include "resize_gump.h"
#include "net_ping_ports.h"
#include "gump.h"
#include "hue_gump.h"

// The SEH frames of the originals (try levels around operator new) are omitted.

#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)    // this for packet-building calls

// 0x0041c930: scroll choice gump constructor (operator new(0x22cc)), RET 0xc, (owner, anchor field, art).
#define SC_New(a1, a2, a3) ((CGump *)new CScrollChoiceGump(a1, (CInputControlGump *)(a2), a3))   // 0x0041c930, size 0x22cc
// 0x0041d7b0: add an entry, RET 0xc, (value, name, 0).
#define SC_AddEntry(self, a1, a2, a3) (reinterpret_cast<CScrollChoiceGump *>(self)->AddEntry(a1, a2, (uint16_t)(a3)))
// 0x0041d830: finish / redraw the list, plain RET.
#define SC_Redraw(self) (reinterpret_cast<CScrollChoiceGump *>(self)->Redraw())

// 0x00463a30: packet 0x2C builder on the object at 0x0069e0e0, (buf, value), RET 8.
#define Pkt_Build2c(o, a1, a2) PORT_THISCALL2(&Build_Packet_0x2C, void, (o), void *, int)(a1, a2)
// 0x00463010: packet 0x05 builder, (buf, value), RET 8.
#define Pkt_Build05(o, a1, a2) PORT_THISCALL2(&Build_Packet_0x05, void, (o), void *, int)(a1, a2)
// 0x00465190: packet 0x7d (menu response) builder, RET 0x18, (buf, serial, model, index, choice, 0).
#define Pkt_Build7d(o, a1, a2, a3, a4, a5, a6) PORT_THISCALL6(&Pkt_Build_0x7D_MenuResponse, void, (o), void *, unsigned int, uint16_t, int, unsigned int, int)(a1, a2, a3, a4, a5, a6)
// 0x00465eb0: packet 0xAC (text entry reply) builder, RET 0x14, (buf, serial, id, flag, text).
#define Pkt_BuildAc(o, a1, a2, a3, a4, a5) PORT_THISCALL5(&Pkt_Build_0xAC_GumpTextEntryReply, void, (o), void *, unsigned int, uint16_t, int, const char *)(a1, a2, a3, a4, a5)
// 0x00478610: send a built packet, RET 4.
#define Pkt_Send(self, a1) (reinterpret_cast<CClientSocket *>(self)->SendPacket((const uint8_t *)(a1)))
// 0x00439f60: alert gump on the gump manager, RET 0x10, (text, 0, 0, 0).
#define Gump_Alert(o, a1, a2, a3, a4) PORT_THISCALL4(&CreateGump2ac, void, (o), const char *, int, int, int)(a1, a2, a3, a4)

static void Config_SetString(uint32_t id, const char *text)
{
    WriteConfigSetting(id, text);
}

// 0x004113a0  size 0xc2
void __thiscall CGumpHandlers04::OpenSkillPicker()
{
    RAWF(CGump *, this, 0x98)->visible = 0;
    CGump *list = SC_New(this, RAWF(void *, this, 0xa4), 0x4cc);
    for (int i = 0; i < g_skillCount; i++) {
        unsigned id = g_skillOrder[i];
        SC_AddEntry(list, static_cast<int>(id), &g_skillNames[id * 0x50], 0);
    }
    SC_Redraw(list);
    gfx::AddChild(list, nullptr);
    GLOBAL_AS(void *, g_scrollChoiceGump) = list;
}
HOOK_METHOD(0x004113a0, CGumpHandlers04, OpenSkillPicker);

// 0x004114c0  size 0x303
// City page finish button: copies the character creation pages into the globals that SendCreateCharacter reads,
// then shows the tutorial or creates the character.
void __thiscall CCityChoiceGump::Finish()
{
    CCharCreationGump *cc = g_charCreationGump;
    g_0069e2b1 = static_cast<uint8_t>(cc->genderBox->value);
    g_0069e2bc = static_cast<uint16_t>(static_cast<uint16_t>(cc->skinHueIndex) + 1);
    g_0069e2b8 = static_cast<uint16_t>(cc->hairBox->value);
    g_0069e2ba = static_cast<uint16_t>(static_cast<uint16_t>(cc->hairHue) + 1);
    g_0069e2c0 = static_cast<uint16_t>(cc->beardBox->value);
    g_0069e2be = static_cast<uint16_t>(static_cast<uint16_t>(cc->beardHue) + 1);
    Config_SetString(3, g_playerName);   // 0x00424ac0
    if (g_0069e628 != 0) {
        char pw[0x20];
        strcpy(pw, g_password);
        PORT_THISCALL0(&App_ObfuscateString, void, pw)();   // obfuscate the copy in place
        Config_SetString(4, pw);
    }

    uint8_t *stat = GLOBAL_AS(uint8_t *, g_pCharStatGump);
    g_0069e2b4 = static_cast<uint8_t>(RAWF(CInputControlGump *, stat, 0xa0)->value);
    g_0069e2b5 = static_cast<uint8_t>(RAWF(CInputControlGump *, stat, 0xa4)->value);
    g_0069e2b6 = static_cast<uint8_t>(RAWF(CInputControlGump *, stat, 0xa8)->value);
    uint8_t *skills = g_pInitSkillGump;
    g_0069e2c2 = static_cast<uint8_t>(RAWF(CInputControlGump *, skills, 0x9c)->value);
    g_0069e2c3 = static_cast<uint8_t>(RAWF(CInputControlGump *, skills, 0xa8)->value);
    g_0069e2c4 = static_cast<uint8_t>(RAWF(CInputControlGump *, skills, 0xa0)->value);
    g_0069e2c5 = static_cast<uint8_t>(RAWF(CInputControlGump *, skills, 0xac)->value);
    g_0069e2c6 = static_cast<uint8_t>(RAWF(CInputControlGump *, skills, 0xa4)->value);
    g_0069e2c7 = static_cast<uint8_t>(RAWF(CInputControlGump *, skills, 0xb0)->value);

    // The gump's name pointer is pointed at the city field's text (+0xec) and used as the key below.
    char *cityText = &RAWF(char, cityField, 0xec);
    name = cityText;
    char **startNames = g_locNames;
    int idx = 0;
    for (;;) {
        char *s = startNames[idx];
        if (s != nullptr && strcmp(cityText, s) == 0)
            break;
        if (idx >= g_locCount)
            break;
        idx++;
    }
    // ? the loop reads startNames[count] when nothing matches, as the original does.
    g_0069e2c9 = GLOBAL_AS(uint8_t *, g_locIds)[idx * 4];

    // First free slot of the character select page: the lowest clear bit of the slot ids in use.
    uint8_t *select = GLOBAL_AS(uint8_t *, g_prevGump);
    int mask = 0;
    for (int k = 0; k < 5; k++) {
        int v = RAWF(int32_t, select, 0x9c + k * 4);
        if (v >= 0)
            mask |= 1 << (v & 0x1f);
    }
    int slot = 0;
    while ((mask & 1) != 0) {
        mask >>= 1;
        slot++;
    }
    g_selectedCharSlot = slot;
    g_colorSay = 0x5a;
    g_hue1 = 0x225;

    // 0x004a6eb0 is the tutorial page file table (see tutorial_gump.cpp); its first entry is the first page.
    char *firstPage = g_tutorialTable[0];
    char *resolved = g_app.ResolveDataPath2(firstPage);
    char path[0x50];
    strcpy(path, resolved);
    FILE *fp = fopen(path, g_fopenReadMode);
    if (fp == nullptr) {
        SendCreateCharacter();   // SendCreateCharacter
    } else {
        fclose(fp);
        CGump *tutorial = new CTutorialGump(0);
        gfx::AddChild(tutorial, nullptr);
    }
    deco1->visible = 0;
    deco2->visible = 0;
    visible = 0;
}
HOOK_METHOD(0x004114c0, CCityChoiceGump, Finish);

// 0x00411990  size 0x37
void __thiscall CGumpHandlers04::SendPacket2cAndDelete()
{
    uint8_t pkt[2];   // packet 0x2c is two bytes
    Pkt_Build2c(g_packetBuilder, pkt, 2);
    Pkt_Send(g_pktSender, pkt);
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);   // scalar deleting destructor
}
HOOK_METHOD(0x00411990, CGumpHandlers04, SendPacket2cAndDelete);

// 0x004119d0  size 0x27
void __thiscall CGumpHandlers04::SendPacket2cOne()
{
    uint8_t pkt[2];   // packet 0x2c is two bytes
    Pkt_Build2c(g_packetBuilder, pkt, 1);
    Pkt_Send(g_pktSender, pkt);
}
HOOK_METHOD(0x004119d0, CGumpHandlers04, SendPacket2cOne);

// 0x00411a00  size 0x16c
void __thiscall CGumpHandlers04::OpenProfessionPicker()
{
    RAWF(CGump *, this, 0xc8)->visible = 0;
    CGump *list = SC_New(this, RAWF(void *, this, 0xc4), 0x4ca);
    SC_AddEntry(list, 1, "Alchemist", 0);
    SC_AddEntry(list, 2, "Animal Handler", 0);
    SC_AddEntry(list, 3, "Assassin", 0);
    SC_AddEntry(list, 4, "Bard", 0);
    SC_AddEntry(list, 5, "Blacksmith", 0);
    SC_AddEntry(list, 6, "Bowyer", 0);
    SC_AddEntry(list, 7, "Healer", 0);
    SC_AddEntry(list, 8, "Ranger", 0);
    SC_AddEntry(list, 9, "Shepherd", 0);
    SC_AddEntry(list, 0xa, "Thief", 0);
    SC_AddEntry(list, 0xb, "Tinker", 0);
    SC_AddEntry(list, 0xc, "Trader", 0);
    SC_AddEntry(list, 0xd, "Warrior", 0);
    SC_AddEntry(list, 0xe, "Wizard", 0);
    SC_Redraw(list);
    gfx::AddChild(list, nullptr);
    GLOBAL_AS(void *, g_scrollChoiceGump) = list;
}
HOOK_METHOD(0x00411a00, CGumpHandlers04, OpenProfessionPicker);

// 0x00411b70  size 0x34
void __thiscall CGumpHandlers04::ScrollChoicePrev()
{
    if (RAWF(int32_t, this, 0xbc) > 0) {
        RAWF(int32_t, this, 0xbc) = RAWF(int32_t, this, 0xbc) - 1;
        SC_Redraw(this);
        RAWF(CScrollThumbGump *, this, 0x22c0)->SetPosition(RAWF(int32_t, this, 0xbc), RAWF(int32_t, this, 0xc0));
    }
}
HOOK_METHOD(0x00411b70, CGumpHandlers04, ScrollChoicePrev);

// 0x00411bb0  size 0x3d
void __thiscall CGumpHandlers04::ScrollChoiceNext()
{
    if (RAWF(int32_t, this, 0xbc) < RAWF(int32_t, this, 0xc0) - 1) {
        RAWF(int32_t, this, 0xbc) = RAWF(int32_t, this, 0xbc) + 1;
        SC_Redraw(this);
        RAWF(CScrollThumbGump *, this, 0x22c0)->SetPosition(RAWF(int32_t, this, 0xbc), RAWF(int32_t, this, 0xc0));
    }
}
HOOK_METHOD(0x00411bb0, CGumpHandlers04, ScrollChoiceNext);

// 0x00411c20  size 0x16b
void __thiscall CCharCreationGump::OpenHairPicker()
{
    art_c4->visible = 0;
    CGump *list = SC_New(this, hairBox, 0x4ca);
    SC_AddEntry(list, 0, "NONE", 0);
    SC_AddEntry(list, 0x203b, "Short", 0);
    SC_AddEntry(list, 0x203c, "Long", 0);
    SC_AddEntry(list, 0x203d, "Pony tail", 0);
    SC_AddEntry(list, 0x2044, "Mohawk", 0);
    SC_AddEntry(list, 0x2045, "Pageboy", 0);
    if (genderBox->value == 1)
        SC_AddEntry(list, 0x2046, "Buns", 0);
    SC_AddEntry(list, 0x2047, "Curly", 0);
    SC_AddEntry(list, 0x2048, "Receding", 0);
    SC_AddEntry(list, 0x2049, "2 Tails", 0);
    SC_AddEntry(list, 0x204a, "Topknot", 0);
    SC_Redraw(list);
    gfx::AddChild(list, nullptr);
    GLOBAL_AS(void *, g_scrollChoiceGump) = list;
}
HOOK_METHOD(0x00411c20, CCharCreationGump, OpenHairPicker);

// 0x00411d90  size 0x121
void __thiscall CCharCreationGump::OpenBeardPicker()
{
    art_c8->visible = 0;
    CGump *list = SC_New(this, beardBox, 0x4cc);
    SC_AddEntry(list, 0, "NONE", 0);
    SC_AddEntry(list, 0x2040, "Goatee", 0);
    SC_AddEntry(list, 0x203e, "Long beard", 0);
    SC_AddEntry(list, 0x203f, "Short beard", 0);
    SC_AddEntry(list, 0x2041, "Moustache", 0);
    SC_AddEntry(list, 0x204b, "Short beard/moustache", 0);
    SC_AddEntry(list, 0x204c, "Long beard/moustache", 0);
    SC_AddEntry(list, 0x204d, "Vandyke", 0);
    SC_Redraw(list);
    gfx::AddChild(list, nullptr);
    GLOBAL_AS(void *, g_scrollChoiceGump) = list;
}
HOOK_METHOD(0x00411d90, CCharCreationGump, OpenBeardPicker);

// 0x00411ec0  size 0xcb
void __thiscall CCityChoiceGump::OnServerListButton()
{
    deco1->visible = 0;
    CGump *list = SC_New(this, serverField, 0x4cc);
    for (int i = 0; i < g_serverCount; i++) {
        char *serverName = g_serverNames[i];
        // 0x00478cb0: ECX = text, non-zero when the text contains "(test)" (plain RET)
        if (serverName != nullptr && PORT_THISCALL0(&StrContainsTest, int, serverName)() == 0)
            SC_AddEntry(list, i, g_serverNames[i], 0);
    }
    SC_Redraw(list);
    gfx::AddChild(list, nullptr);
    GLOBAL_AS(void *, g_scrollChoiceGump) = list;
}
HOOK_METHOD(0x00411ec0, CCityChoiceGump, OnServerListButton);

// 0x004120f0  size 0x83
void __thiscall CGumpHandlers04::HuePickerOk()
{
    CGump *frame = RAWF(CGump *, this, 0xb8);
    int frameSel = frame->frameIndex;
    g_fightMode = frameSel;
    CScrollThumbGump *thumbA = RAWF(CScrollThumbGump *, this, 0xac);
    int lo = thumbA->minX;
    int pctA = (thumbA->x - lo) * 100 / (thumbA->maxX - lo);
    g_fightValueA = pctA;
    CScrollThumbGump *thumbB = RAWF(CScrollThumbGump *, this, 0xb0);
    int pctB = (thumbB->x - thumbB->minX) * 100 / (thumbB->maxX - thumbB->minX);
    g_fightValueB = pctB;
    // 0x00431480: ECX = frame index, EDX = first percentage, stack = second percentage, RET 4
    SendWarModePacket(frameSel, pctA, pctB);
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);   // scalar deleting destructor
}
HOOK_METHOD(0x004120f0, CGumpHandlers04, HuePickerOk);

// 0x00412280  size 0xa3
void __thiscall CGumpHandlers04::AddBackdropChildDc()
{
    // RET 0x24, (x, y, w, h, w, 0xdc, w, 0x1cc - y, parent); the original dereferences g below even when new failed
    CGump *g = new CResizeGump(x, y, width, height, width, 0xdc, width, 0x1cc - y, this);
    g->unk_58 = static_cast<int32_t>(reinterpret_cast<uintptr_t>(gfx::AllocTagged(g->height * g->width * 2, "gump background")));
    gfx::AddChild(g, nullptr);
}
HOOK_METHOD(0x00412280, CGumpHandlers04, AddBackdropChildDc);

// 0x004123e0  size 0xa3
void __thiscall CGumpHandlers04::AddBackdropChild104()
{
    // RET 0x24, (x, y, w, h, w, 0x104, w, 0x1cc - y, parent); the original dereferences g below even when new failed
    CGump *g = new CResizeGump(x, y, width, height, width, 0x104, width, 0x1cc - y, this);
    g->unk_58 = static_cast<int32_t>(reinterpret_cast<uintptr_t>(gfx::AllocTagged(g->width * g->height * 2, "gump background")));
    gfx::AddChild(g, nullptr);
}
HOOK_METHOD(0x004123e0, CGumpHandlers04, AddBackdropChild104);

// 0x00412740  size 0x4c
void __thiscall CGumpHandlers04::MenuCancel()
{
    uint8_t pkt[0x10];
    Pkt_Build7d(g_packetBuilder, pkt, RAWF(uint32_t, this, 0x2ac), RAWF(uint16_t, this, 0x2b0), 0, 0, 0);
    Pkt_Send(g_pktSender, pkt);
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);   // scalar deleting destructor
}
HOOK_METHOD(0x00412740, CGumpHandlers04, MenuCancel);

// 0x00412790  size 0xb2
// Radio choice dialog OK: the answer is the id stored at +0xaa + index * 2 of the first checked radio child.
void __thiscall CGumpHandlers04::MenuOk()
{
    unsigned choice = 0xffffffffu;
    int index = 1;
    for (CGump *c = firstChild; c != nullptr; c = c->next) {
        if (PORT_VSLOT0(c, 0xc4, int)() != 0) {   // is a radio button
            if (RAWF(int32_t, c, 0xb0) != 0) {
                choice = RAWF(uint16_t, this, 0xaa + index * 2);
                break;
            }
            index++;
        }
    }
    if (choice != 0xffffffffu) {
        uint8_t pkt[0x10];
        Pkt_Build7d(g_packetBuilder, pkt, RAWF(uint32_t, this, 0x2ac), RAWF(uint16_t, this, 0x2b0), index, choice, 0);
        Pkt_Send(g_pktSender, pkt);
        if (this != nullptr)
            PORT_VSLOT1(this, 0x00, void, int)(1);   // scalar deleting destructor
    } else {
        Gump_Alert(g_gumpManager, "Must choose one to continue.", 0, 0, 0);
    }
}
HOOK_METHOD(0x00412790, CGumpHandlers04, MenuOk);

// 0x00412850  size 0x3d
void __thiscall CGumpHandlers04::SendPacket05AndDelete()
{
    uint8_t pkt[8];
    Pkt_Build05(g_packetBuilder, pkt, MG_NotorietyOwner);
    Pkt_Send(g_pktSender, pkt);
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);   // scalar deleting destructor
}
HOOK_METHOD(0x00412850, CGumpHandlers04, SendPacket05AndDelete);

// The three server list sorts below differ only in the key array and in the field the finished count is
// copied to. Selection sort of the server indexes in idx[]; entries whose flag byte is 0x32 are skipped.

// 0x00412b30  size 0x117
void __thiscall CGumpHandlers04::SortServersByKeyA()
{
    uint8_t *idx = static_cast<uint8_t *>(::operator new(g_serverCount + 1));
    for (int i = 0; i < g_serverCount; i++)
        idx[i] = static_cast<uint8_t>(i);
    for (int i = 0; i < g_serverCount - 1; i++) {
        int best = -1;
        for (int j = i; j < g_serverCount; j++) {
            unsigned k = idx[j];
            if (g_serverFlags[k] != 0x32) {
                if (best == -1 || g_serverSortKeys[k] < g_serverSortKeys[idx[best]])
                    best = j;
            }
        }
        if (best != i && best != -1) {
            unsigned t = idx[i];
            idx[i] = idx[best];
            idx[best] = static_cast<uint8_t>(t);
        }
    }
    RAWF(int32_t, this, 0xc0) = 0;
    for (int i = 0; i < g_serverCount; i++) {
        unsigned k = idx[i];
        SC_AddEntry(this, g_serverValues[k], g_serverNames[k], 0);
    }
    RAWF(int32_t, this, 0xc8) = RAWF(int32_t, this, 0xd4);
    SC_Redraw(this);
    operator delete(idx);
}
HOOK_METHOD(0x00412b30, CGumpHandlers04, SortServersByKeyA);

// 0x00412c50  size 0x117
void __thiscall CGumpHandlers04::SortServersByKeyB()
{
    uint8_t *idx = static_cast<uint8_t *>(::operator new(g_serverCount + 1));
    for (int i = 0; i < g_serverCount; i++)
        idx[i] = static_cast<uint8_t>(i);
    for (int i = 0; i < g_serverCount - 1; i++) {
        int best = -1;
        for (int j = i; j < g_serverCount; j++) {
            unsigned k = idx[j];
            if (g_serverFlags[k] != 0x32) {
                if (best == -1 || g_serverKeyA[k] < g_serverKeyA[idx[best]])
                    best = j;
            }
        }
        if (best != i && best != -1) {
            unsigned t = idx[i];
            idx[i] = idx[best];
            idx[best] = static_cast<uint8_t>(t);
        }
    }
    RAWF(int32_t, this, 0xc0) = 0;
    for (int i = 0; i < g_serverCount; i++) {
        unsigned k = idx[i];
        SC_AddEntry(this, g_serverValues[k], g_serverNames[k], 0);
    }
    RAWF(int32_t, this, 0xcc) = RAWF(int32_t, this, 0xd4);
    SC_Redraw(this);
    operator delete(idx);
}
HOOK_METHOD(0x00412c50, CGumpHandlers04, SortServersByKeyB);

// 0x00412d70  size 0x11e
void __thiscall CGumpHandlers04::SortServersByPing()
{
    Net_PingServerList();   // Net_PingServerList
    uint8_t *idx = static_cast<uint8_t *>(::operator new(g_serverCount + 1));
    for (int i = 0; i < g_serverCount; i++)
        idx[i] = static_cast<uint8_t>(i);
    for (int i = 0; i < g_serverCount - 1; i++) {
        int best = -1;
        for (int j = i; j < g_serverCount; j++) {
            unsigned k = idx[j];
            if (g_serverFlags[k] != 0x32) {
                if (best == -1 || g_serverPing[k] < g_serverPing[idx[best]])
                    best = j;
            }
        }
        if (best != i && best != -1) {
            unsigned t = idx[i];
            idx[i] = idx[best];
            idx[best] = static_cast<uint8_t>(t);
        }
    }
    RAWF(int32_t, this, 0xc0) = 0;
    for (int i = 0; i < g_serverCount; i++) {
        unsigned k = idx[i];
        SC_AddEntry(this, g_serverValues[k], g_serverNames[k], 0);
    }
    RAWF(int32_t, this, 0xd0) = RAWF(int32_t, this, 0xd4);
    SC_Redraw(this);
    operator delete(idx);
}
HOOK_METHOD(0x00412d70, CGumpHandlers04, SortServersByPing);

// 0x00412e90  size 0x87
void __thiscall CGumpHandlers04::TextEntryOk()
{
    uint8_t pkt[0x110];
    CInputControlGump *edit = RAWF(CInputControlGump *, this, 0xa0);
    if (edit->mode == 1)
        sprintf(edit->text, "%d", edit->value);
    Pkt_BuildAc(g_packetBuilder, pkt, RAWF(uint32_t, this, 0x98), RAWF(uint16_t, this, 0x9c), 1, RAWF(CInputControlGump *, this, 0xa0)->text);
    Pkt_Send(g_pktSender, pkt);
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);   // scalar deleting destructor
}
HOOK_METHOD(0x00412e90, CGumpHandlers04, TextEntryOk);

// 0x00412f20  size 0x87
void __thiscall CGumpHandlers04::TextEntryCancel()
{
    uint8_t pkt[0x110];
    CInputControlGump *edit = RAWF(CInputControlGump *, this, 0xa0);
    if (edit->mode == 1)
        sprintf(edit->text, "%d", edit->value);
    Pkt_BuildAc(g_packetBuilder, pkt, RAWF(uint32_t, this, 0x98), RAWF(uint16_t, this, 0x9c), 0, RAWF(CInputControlGump *, this, 0xa0)->text);
    Pkt_Send(g_pktSender, pkt);
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);   // scalar deleting destructor
}
HOOK_METHOD(0x00412f20, CGumpHandlers04, TextEntryCancel);

// 0x004117d0  size 0xd1
// Toggle the "Speech Sample" gump. The SEH frame is omitted.
void __thiscall CCharCreateGump::ToggleSpeechSample()
{
    CGump *found = nullptr;
    for (CGump *g = g_gumpListTail; g != nullptr; g = g->prev) {
        if (PORT_VSLOT0(g, 0xfc, int)() != 0 && RAWF(int32_t, g, 0xb4) != 0)
            found = g;   // last match wins, the loop is not exited
    }
    if (found != nullptr) {
        found->CGump::BringToFront(0);   // bring to front
        return;
    }
    CGump *obj = new CHueGump(100, 100, 0, 1, "Speech Sample", hue_c0);
    RAWF(int32_t, obj, 0xb4) = 1;   // written even when the ctor result is NULL, as the asm does
    CGump_AddChild(obj, nullptr, 1);
}
HOOK_METHOD(0x004117d0, CCharCreateGump, ToggleSpeechSample);
