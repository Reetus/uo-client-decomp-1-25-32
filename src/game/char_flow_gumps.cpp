// Second pass, batch 03 (0x0040df80 - 0x004112d0): skill group list, shop cart handlers, login / character select /
// character creation buttons, client options buttons, bulletin board and list scrolling handlers.
// The SEH frames (try levels around `new`) of the original are omitted throughout.
#include <windows.h>

#include <cstring>

#include "../runtime/ported.h"
#include "bb_gump.h"
#include "bb_line_gump.h"
#include "bb_msg_gump.h"
#include "bill_gump.h"
#include "char_creation_gump.h"
#include "char_flow_gumps.h"
#include "char_stat_gump.h"
#include "char_ui_gumps.h"
#include "client_cfg_gump.h"
#include "course_gump.h"
#include "globals.h"
#include "init_skill_gump.h"
#include "login_gumps.h"
#include "obj_picker_gump.h"
#include "crt.h"
#include "raw_field.h"
#include "scroll_choice_gump.h"
#include "shop_gump.h"
#include "skill_gump.h"
#include "skill_list_gump.h"
#include "client_socket.h"
#include "gump.h"
#include "gump_manager.h"
#include "config_file_ports.h"
#include "skill_groups_ports.h"
#include "scroll_thumb_gump.h"
#include "gump_art_ops_ports.h"
#include "batch02_helpers.h"
#include "app_support_ports.h"
#include "app_session_ops_ports.h"
#include "main_menu_gump.h"
#include "waiting_gump.h"
#include "main_window_ports.h"
#include "packet_builders_batch24_ports.h"
#include "obj.h"
#include "city_choice_gump.h"
#include "name_filter.h"
#include "packet_builders_b25_ports.h"
#include "app.h"
#include "packet_builders_ports.h"
#include "class_0049c690.h"
#include "edit_list_gump.h"
#include "art_record_ops_ports.h"
#include "hue_gump.h"
#include "input_control_gump.h"
#include "intro_movie_ports.h"
#include "art_row_blits_ports.h"

#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)    // this for packet-building calls
#define g_focusGump GLOBAL_AS(int32_t, g_focusedInput)
#undef g_app   // the class header's typed form; this file addresses the object by offset
#define g_app GLOBAL_PTR(void, g_app_storage)
#define g_artPlain PTR_AT(void, PORT_ADDR(&RowBlitCopyNonZero))         // art drawer object (ECX of 0x00433ed0)

#define g_pCharCreation GLOBAL_AS(CGump *, g_charCreationGump)
#define g_pCharStat GLOBAL_AS(CGump *, g_pCharStatGump)
#define g_pInitSkill GLOBAL_AS(CGump *, g_pInitSkillGump)
#define g_pCityChoice GLOBAL_AS(CGump *, g_cityChoiceGump)


#define g_acctLastName g_titlePart2[0]

// ---------------------------------------------------------------------------
// Original code not ported yet, and small helpers.
// ---------------------------------------------------------------------------
static inline void CF_Send(char *packet) { reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(packet)); }
static inline void CF_VDelete(void *gump) { PORT_VSLOT1(gump, 0x00, void, int)(1); }   // scalar deleting dtor
static void CF_AddChild(void *gump) { reinterpret_cast<CGump *>(gump)->LinkInto(nullptr, 1); }   // link into the top-level list
// Message gump through 0x00439f60 (ECX = gump manager).
static inline void *CF_Message(const char *text, void *parent, int a, int b)
{
    return PORT_THISCALL4(&CreateGump2ac, void *, g_gumpManager, const char *, void *, int, int)(text, parent, a, b);
}
static inline void CF_Config(uint32_t id, const char *text) { WriteConfigSetting(id, text); }   // WriteConfigSetting
static inline void CF_SaveSkillGroups() { SaveSkillGroups(); }
static inline void CF_CommitFocus() { PORT_THISCALL0(&Gump_CommitFocus, void, g_gumpManager)(); }
static inline void CF_ThumbSet(void *thumb, int value, int range) { reinterpret_cast<CScrollThumbGump *>(thumb)->SetPosition(value, range); }
static void CF_ArtSize(int artId, int *w, int *h) { GumpArtSizeImpl(artId, w, h); }
static inline void CF_DrawArt(int artId, const CGump *g)
{
    uint8_t *bmp = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(g->frameData[1]));
    PORT_FUNC7(&DrawGumpArt, void, __fastcall, void *, int, int, int, void *, int, int)(g_artPlain, artId, 0, 0, bmp + 4, g->width,
                                                                                     g->height);
}
static inline void CF_Obfuscate(char *text) { PORT_THISCALL0(&App_ObfuscateString, void, text)(); }
static inline void CF_EnterBritannia(char *name, char *account, int slot)   // 0x0044e360 ShowEnteringBritannia
{
    ShowEnteringBritannia((int)(name), (int)(account), slot);
}
// The 0x2ac-byte waiting gump (ctor 0x0043d740) is created, its last child hidden and linked in, by hand in each caller
// (the original inlines it).
static inline char *CF_Text(const void *control) { return &RAWF(char, const_cast<void *>(control), 0xec); }
static void CF_SendDisconnectAndClose()   // tail shared by the "back to the main menu" handlers
{
    g_flag6f493c = 0;
    g_inGame7211ac = 0;
    reinterpret_cast<CSocketBase *>(g_pktSender)->Close();   // CSocketBase::Close
    PORT_THISCALL0(&SetMainWindowTitle, void, const_cast<char *>(g_emptyString))();    // SetMainWindowTitle("")
}
// 0x0046cfd0 / 0x0046ce00 / 0x0046d280 on the shop gump (ECX = shop gump).
#define Shop_SellAdd(shop, id, delta) (shop->*PORT_METHOD2(CShopGump, void, &CShopGump::SellAddToCart, uint32_t, int))(id, delta)
#define Shop_Add(shop, slot, key, delta) (shop->*PORT_METHOD3(CShopGump, void, &CShopGump::AddToCart, uint32_t, int, int))(slot, key, delta)
#define Shop_SellAddN(shop, id, delta) (shop->*PORT_METHOD2(CShopGump, void, &CShopGump::SellAddToCart, uint16_t, int))(id, delta)
#define Shop_AddN(shop, slot, key, delta) (shop->*PORT_METHOD3(CShopGump, void, &CShopGump::AddToCart, uint8_t, int, int))(slot, key, delta)
#define Shop_Redraw(shop) (shop->*PORT_METHOD0(CShopGump, void, &CShopGump::RedrawFrame))()

static int CF_RepeatStep(int32_t &counter)
{
    if (counter < 100)
        counter++;
    if (counter < 10)
        return 1;
    if (counter < 0x14)
        return 2;
    if (counter < 0x32)
        return 5;
    return 10;
}

// 0x0040df80  size 0xd0
// Skill window: add a group named "New Group" (at most 12), select it, redraw and save skillgrp.mul.
void __thiscall CSkillListGump::AddGroup()
{
    this->CSkillListGump::OnOk();   // commit a pending group rename
    if (g_groupCount < 12) {
        char groupName[0x50];
        sprintf(groupName, "New Group");   // 0x0048b1d0
        int32_t n = g_groupCount;
        strcpy(g_groupNames + n * 0x11, groupName);
        n++;
        g_groupCount = n;
        cursor = n - 1;
        onGroupHeader = 1;
        scrollRange++;
        scrollPos = scrollRange - 1;
        CF_ThumbSet(reinterpret_cast<void *>(static_cast<uintptr_t>(unk_94)), scrollRange - 1, scrollRange);   // 0x00413c00
        Redraw(-1);                                                                                // 0x00474cc0
        CF_SaveSkillGroups();
        return;
    }
    CF_Message("Too many skill groups.", nullptr, 0, 0);
}
HOOK_METHOD(0x0040df80, CSkillListGump, AddGroup);

// 0x0040e050  size 0x13c
// Shop gump: auto-repeat of the "+" button; adds 1 / 2 / 5 / 10 of the selected item to the cart.
void __thiscall CShopGump::RepeatIncrease()
{
    // The original repeats the add call and the redraw in every step branch: 0x0046ce00 0x0046cfd0 0x0046cfd0
    // 0x0046d280 0x0046d280 0x0046d280 0x0046d280
    if (RAWF(int32_t, billGump, 0xac) != 0)
        return;
    if (sellMode != 0) {
        if (itemCount == 0)
            return;
        int step = CF_RepeatStep(unk_0c);
        Shop_SellAdd(this, unk_c4, step);   // 0x0046cfd0
        Shop_Redraw(this);                  // 0x0046d280
        return;
    }
    if (unk_a8 == 0)
        return;
    int step = CF_RepeatStep(unk_0c);
    Shop_Add(this, unk_ac, RAWF(int32_t, reinterpret_cast<void *>(static_cast<uintptr_t>(unk_a8)), 0x78), step);   // 0x0046ce00
    Shop_Redraw(this);
}
HOOK_METHOD(0x0040e050, CShopGump, RepeatIncrease);

// 0x0040e190  size 0xe2
// Shop row gump: increase handler (steps 1/2/5/10). The callees are thiscalls on the shop gump (+0xb0).
void __thiscall CSkillGump::OnIncrease()
{
    // The original repeats the add call and the redraw in every step branch: 0x0046cfd0 0x0046d280 0x0046d280
    if (disabled != 0)
        return;
    CShopGump *shop = static_cast<CShopGump *>(parentList);
    BillEntry *node = reinterpret_cast<BillEntry *>(shop->nodeList);
    if (node == nullptr)
        return;
    while (node->key != unk_08) {
        node = node->next;
        if (node == nullptr)
            break;
    }
    if (node == nullptr)
        return;
    int step = CF_RepeatStep(unk_0c);
    if (lockFlag != 0)
        Shop_SellAdd(shop, node->index, step);   // 0x0046cfd0
    else
        Shop_Add(shop, node->slot, node->key, step);   // 0x0046ce00
    Shop_Redraw(shop);                           // 0x0046d280
}
HOOK_METHOD(0x0040e190, CSkillGump, OnIncrease);

// 0x0040e390  size 0x89
// Shop gump: send the buy (0x3b) or sell (0x9f) order for the cart.
void __thiscall CShopGump::SendOrder()
{
    unk_bc = 1;
    if (sellMode != 0) {
        char packet[0x7dc];
        PORT_THISCALL3(&Build_Packet_0x9F, void, g_packetBuilder, char *, int, void *)(packet, RAWF(int32_t, owner, 0x78), nodeList);   // Build_Packet_0x9F_SellItems
        CF_Send(packet);
    } else {
        char packet[0x70c];
        PORT_THISCALL4(&Build_Packet_0x3B, void, g_packetBuilder, char *, int, int, void *)(packet, RAWF(int32_t, owner, 0x78), 2, nodeList);   // Build_Packet_0x3B_BuyItems
        CF_Send(packet);
    }
}
HOOK_METHOD(0x0040e390, CShopGump, SendOrder);

// 0x0040e420  size 0x6d
// Bill gump: remove every line of the cart from the shop, redraw it and reset the scroll thumb at +0xc0.
void __thiscall CBillGump::ClearCart()
{
    BillEntry *node = reinterpret_cast<BillEntry *>(shop->nodeList);
    if (node == nullptr)
        return;
    do {
        if (sellMode != 0)
            Shop_SellAddN(shop, node->index, -static_cast<int32_t>(node->quantity));   // 0x0046cfd0
        else
            Shop_AddN(shop, node->slot, node->key, -static_cast<int32_t>(node->quantity));   // 0x0046ce00
        node = reinterpret_cast<BillEntry *>(shop->nodeList);
    } while (node != nullptr);
    Shop_Redraw(shop);   // 0x0046d280
    CF_ThumbSet(RAWF(void *, this, 0xc0), 0, 1);
}
HOOK_METHOD(0x0040e420, CBillGump, ClearCart);

// 0x0040e490  size 0x51
// Map gump: send the clear-pins packet (0x56, action 5), clear the owner's pins, redraw.
void __thiscall CCourseGump::ClearPins()
{
    char packet[0x10];
    PORT_THISCALL6(&Build_Packet_0x56, void, g_packetBuilder, char *, int, int, int, int, int)(packet, RAWF(int32_t, owner, 0x78), 5, 0, 0, 0);   // Pkt_Build_0x56_MapPin
    CF_Send(packet);
    reinterpret_cast<CObjVt49f960 *>(owner)->ListOp(5, 0, 0, 0);
    Redraw();   // 0x00428a90
}
HOOK_METHOD(0x0040e490, CCourseGump, ClearPins);

// 0x0040e4f0  size 0xce
// ? Character creation page: show the city choice gump (creating it when missing) and hide this page.
void __thiscall CCharCreationGump::ShowCityChoice()
{
    if (g_pCityChoice != nullptr) {
        g_pCityChoice->visible = 1;
        RAWF(CGump *, g_pCityChoice, 0x9c)->visible = 1;
        RAWF(CGump *, g_pCityChoice, 0xa0)->visible = 1;
    } else {
        CGump *obj = new CCityChoiceGump();
        g_pCityChoice = obj;
        obj->LinkInto(nullptr, 1);
        CGump_BringToFront(RAWF(CGump *, g_pCityChoice, 0x9c), 0);
        CGump_BringToFront(RAWF(CGump *, g_pCityChoice, 0xa0), 0);
    }
    art_c4->visible = 0;
    art_c8->visible = 0;
    visible = 0;
}
HOOK_METHOD(0x0040e4f0, CCharCreationGump, ShowCityChoice);

// 0x0040e5c0  size 0x9d
// ? Character creation page: show the skill page (creating it when missing) and hide this page.
void __thiscall CCharCreationGump::ShowInitSkill()
{
    if (g_pInitSkill != nullptr) {
        g_pInitSkill->visible = 1;
        RAWF(CGump *, g_pInitSkill, 0x98)->visible = 1;
    } else {
        CGump *obj = new CInitSkillGump();
        g_pInitSkill = obj;
        CF_AddChild(obj);
    }
    art_c4->visible = 0;
    art_c8->visible = 0;
    visible = 0;
}
HOOK_METHOD(0x0040e5c0, CCharCreationGump, ShowInitSkill);

// 0x0040e660  size 0x168
// ? Name page: validate the name, copy name and password into the globals, show the skill page.
void __thiscall CCharStatGump::NextFromName()
{
    char *charName = CF_Text(RAWF(void *, this, 0x98));
    if (strlen(charName) < 2) {
        visible = 0;
        CF_Message("Your character name is too short.", this, 1, 0);
        return;
    }
    if (NameIsRejected(charName) != 0) {   // IsNameRejected
        visible = 0;
        CF_Message("Unacceptable name.", this, 1, 0);
        return;
    }
    strcpy(g_playerName, CF_Text(RAWF(void *, this, 0x98)));
    strcpy(g_password, CF_Text(RAWF(void *, this, 0x9c)));
    if (g_pInitSkill != nullptr) {
        g_pInitSkill->visible = 1;
        RAWF(CGump *, g_pInitSkill, 0x98)->visible = 1;
    } else {
        CGump *obj = new CInitSkillGump();
        g_pInitSkill = obj;
        CF_AddChild(obj);
    }
    RAWF(CGump *, this, 0xc8)->visible = 0;
    visible = 0;
}
HOOK_METHOD(0x0040e660, CCharStatGump, NextFromName);

// 0x0040e7d0  size 0x151
// ? Cancel character creation: back to the character select gump, delete every creation page.
void __thiscall CCharStatGump::CancelCreation()
{
    if (g_prevGump != nullptr) {
        g_prevGump->visible = 1;
    } else {
        CGump *obj = new CLoginGump();
        g_prevGump = obj;
        CF_AddChild(obj);
    }
    CGump *child = RAWF(CGump *, this, 0xc8);
    if (child != nullptr)
        CF_VDelete(child);
    if (this != nullptr)
        CF_VDelete(this);

    CGump *g = g_pCityChoice;
    if (g != nullptr) {
        child = RAWF(CGump *, g, 0x9c);
        if (child != nullptr) {
            CF_VDelete(child);
            g = g_pCityChoice;
        }
        child = RAWF(CGump *, g, 0xa0);
        if (child != nullptr) {
            CF_VDelete(child);
            g = g_pCityChoice;
        }
        if (g != nullptr)
            CF_VDelete(g);
        g_pCityChoice = nullptr;
    }

    g = g_pCharCreation;
    if (g != nullptr) {
        child = RAWF(CGump *, g, 0xc4);
        if (child != nullptr) {
            CF_VDelete(child);
            g = g_pCharCreation;
        }
        child = RAWF(CGump *, g, 0xc8);
        if (child != nullptr) {
            CF_VDelete(child);
            g = g_pCharCreation;
        }
        if (g != nullptr)
            CF_VDelete(g);
        g_pCharCreation = nullptr;
    }

    g = g_pInitSkill;
    if (g != nullptr) {
        child = RAWF(CGump *, g, 0x98);
        if (child != nullptr) {
            CF_VDelete(child);
            g = g_pInitSkill;
        }
        if (g != nullptr)
            CF_VDelete(g);
        g_pInitSkill = nullptr;
    }
}
HOOK_METHOD(0x0040e7d0, CCharStatGump, CancelCreation);

// 0x0040e930  size 0x139
// ? Skill page: refuse duplicate skill choices, otherwise show the appearance page and hide this one.
void __thiscall CInitSkillGump::NextFromSkills()
{
    int32_t a = name_9c->value;
    int32_t b = name_a0->value;
    if (a != b && a != name_a4->value && b != name_a4->value) {
        if (g_pCharCreation != nullptr) {
            g_pCharCreation->visible = 1;
            RAWF(CGump *, g_pCharCreation, 0xc4)->visible = 1;
            RAWF(CGump *, g_pCharCreation, 0xc8)->visible = 1;
        } else {
            CGump *obj = new CCharCreationGump();
            g_pCharCreation = obj;
            CF_AddChild(obj);
            CGump_BringToFront(RAWF(CGump *, g_pCharCreation, 0xc4), 0);
            CGump_BringToFront(RAWF(CGump *, g_pCharCreation, 0xc8), 0);
        }
        visible = 0;
        art_98->visible = 0;
        return;
    }
    CF_Message("You have two of the same skill.", this, 1, 0);
    visible = 0;
}
HOOK_METHOD(0x0040e930, CInitSkillGump, NextFromSkills);

// 0x0040eab0  size 0x12d
// Password prompt: log in with the selected character and the typed password, then delete this gump.
// ? The original clears this->visible after deleting this gump (use after free); kept.
void __thiscall CPlayCharGump::Ok()
{
    CGump *select = g_prevGump;
    int32_t idx = RAWF(int32_t, select, 0x98);
    char *slotName = CF_Text(RAWF(void *, select, 0xb0 + idx * 4));
    if (strlen(slotName) == 0) {
        visible = 0;
        CF_Message("No character to login with.", this, 1, 0);
        return;
    }
    strcpy(g_playerName, slotName);
    strcpy(g_password, CF_Text(password));
    CF_Config(3, g_playerName);   // 0x00424ac0
    char number[2];
    sprintf(number, "%d", idx);   // 0x0048b1d0
    CF_Config(0x1e, number);
    int32_t slot = RAWF(int32_t, select, 0x9c + idx * 4);
    g_selectedCharSlot = slot;
    g_colorSay = g_slotHue0[slot];
    g_hue1 = g_slotHue1[slot];
    if (this != nullptr)
        CF_VDelete(this);
    visible = 0;
    CF_EnterBritannia(g_playerName, g_password, g_selectedCharSlot);   // 0x0044e360
}
HOOK_METHOD(0x0040eab0, CPlayCharGump, Ok);

// 0x0040ebe0  size 0x196
// Character select: Play button. Characters with a password get the CPlayCharGump prompt.
void __thiscall CCharSelectGump::Play()
{
    int32_t idx = selectedSlot;
    char *slotName = CF_Text(RAWF(void *, this, 0xb0 + idx * 4));
    if (strlen(slotName) == 0) {
        visible = 0;
        CF_Message("No character to login with.", this, 1, 0);
        return;
    }
    int32_t slot = RAWF(int32_t, this, 0x9c + idx * 4);
    if (strlen(g_slotNeedsPw + slot * 30) != 0) {
        CGump *obj = new CPlayCharGump(CF_Text(RAWF(void *, this, 0xb0 + idx * 4)), (int)(g_emptyString));
        obj->LinkInto(nullptr, 1);
        visible = 0;
        return;
    }
    strcpy(g_playerName, slotName);
    CF_Config(3, g_playerName);
    char number[16];
    sprintf(number, "%d", idx);   // 0x0048b1d0
    CF_Config(0x1e, number);
    g_selectedCharSlot = slot;
    g_colorSay = g_slotHue0[slot];
    g_hue1 = g_slotHue1[slot];
    CF_EnterBritannia(CF_Text(RAWF(void *, this, 0xb0 + idx * 4)), const_cast<char *>(g_emptyString), slot);   // 0x0044e360
    visible = 0;
}
HOOK_METHOD(0x0040ebe0, CCharSelectGump, Play);

// 0x0040ed80  size 0xc5
// Character select: Back button. Deletes this gump, sends the disconnect packet and returns to the main menu.
void __thiscall CCharSelectGump::Disconnect()
{
    // via CF_SendDisconnectAndClose: 0x00476ad0 0x00480d60
    if (this != nullptr)
        CF_VDelete(this);
    char packet[0x20];
    PORT_THISCALL1(&Build_Packet_0x01, void, g_packetBuilder, char *)(packet);   // Build_Packet_0x01
    CF_Send(packet);
    CGump *menu = g_pMainMenu;
    g_prevGump = nullptr;
    if (menu != nullptr)
        menu->visible = 1;
    else {
        CGump *fresh = new CMainMenuGump();
        g_pMainMenu = fresh;
        fresh->LinkInto(nullptr, 1);
    }
    CF_SendDisconnectAndClose();
}
HOOK_METHOD(0x0040ed80, CCharSelectGump, Disconnect);

// 0x0040ee50  size 0xfa
// Character select: New Character button (at most 5 characters): open the skill and stat pages.
void __thiscall CCharSelectGump::NewCharacter()
{
    if (RAWF(int32_t, this, 0x94) == 5) {
        CF_Message("You already have 5 characters.", nullptr, 0, 0);
        return;
    }
    if (g_prevGump != nullptr)
        g_prevGump->visible = 0;
    CGump *obj = new CInitSkillGump();
    g_pInitSkill = obj;
    obj->visible = 0;   // no null check in the original either
    RAWF(CGump *, g_pInitSkill, 0x98)->visible = 0;
    g_pInitSkill->LinkInto(nullptr, 1);
    obj = new CCharStatGump();
    g_pCharStat = obj;
    obj->LinkInto(nullptr, 1);
}
HOOK_METHOD(0x0040ee50, CCharSelectGump, NewCharacter);

// 0x0040ef50  size 0xc2
// Character select: Delete button. Opens the CDeleteCharGump for the selected character.
void __thiscall CCharSelectGump::OpenDeleteDialog()
{
    int32_t idx = selectedSlot;
    char *slotName = CF_Text(RAWF(void *, this, 0xb0 + idx * 4));
    if (strlen(slotName) == 0) {
        visible = 0;
        CF_Message("No character to delete.", this, 1, 0);
        return;
    }
    visible = 0;
    CGump *obj = new CDeleteCharGump(CF_Text(RAWF(void *, this, 0xb0 + idx * 4)), (char *)(g_emptyString));
    obj->LinkInto(nullptr, 1);
}
HOOK_METHOD(0x0040ef50, CCharSelectGump, OpenDeleteDialog);

// 0x0040f020  size 0x15b
// Delete character dialog: OK. Sends the delete packet (0x83) with the typed password and shows the waiting gump.
void __thiscall CDeleteCharGump::DelChar()
{
    // via CF_NewWaiting: operator new 0x0048ad70, link 0x0043bcc0
    CGump *select = g_prevGump;
    int32_t idx = RAWF(int32_t, select, 0x98);
    if (strlen(CF_Text(RAWF(void *, select, 0xb0 + idx * 4))) == 0) {
        CF_Message("No character to delete.", nullptr, 1, 0);
        if (this != nullptr)
            CF_VDelete(this);
        return;
    }
    if (g_waitingGump != nullptr)
        CF_VDelete(g_waitingGump);
    char pw[0x20];
    strcpy(pw, CF_Text(password));
    if (this != nullptr)
        CF_VDelete(this);
    g_waitingGump = new CGump_0043de30(-1, -1, "Deleting character... ", (int)(select), 1, 1, 0);
    g_waitingGump->lastChild->visible = 0;   // no null check in the original either
    g_waitingGump->LinkInto(nullptr, 1);
    char packet[0x28];
    PORT_THISCALL3(&Pkt_Build_0x83_DeleteCharacter, void, g_packetBuilder, char *, int, char *)(packet, RAWF(int32_t, select, 0x9c + idx * 4),
                                                         pw);   // Pkt_Build_0x83_DeleteCharacter
    CF_Send(packet);
    select->visible = 0;
}
HOOK_METHOD(0x0040f020, CDeleteCharGump, DelChar);

// 0x0040f250  size 0x15d
// Change password dialog: OK. Checks the two new passwords match, sends the 0x84 packet, shows the waiting gump.
void __thiscall CChgPWGump::Confirm()
{
    // via CF_NewWaiting: operator new 0x0048ad70, link 0x0043bcc0
    if (strcmp(CF_Text(newPassword), CF_Text(verifyPassword)) != 0) {
        visible = 0;
        CF_Message("Passwords do not match!", this, 0, 0);
        return;
    }
    CGump *select = g_prevGump;
    int32_t slot = RAWF(int32_t, select, 0x9c + RAWF(int32_t, select, 0x98) * 4);
    if (g_waitingGump != nullptr)
        CF_VDelete(g_waitingGump);
    char packet[0x48];
    PORT_THISCALL4(&Pkt_Build_0x84, void, g_packetBuilder, char *, int, char *, char *)(packet, slot, CF_Text(oldPassword),
                                                                 CF_Text(newPassword));   // Pkt_Build_0x84
    CF_Send(packet);
    if (this != nullptr)
        CF_VDelete(this);
    select->visible = 0;
    g_waitingGump = new CGump_0043de30(-1, -1, "Changing password... ", (int)(select), 1, 1, 0);
    g_waitingGump->lastChild->visible = 0;   // no null check in the original either
    g_waitingGump->LinkInto(nullptr, 1);
}
HOOK_METHOD(0x0040f250, CChgPWGump, Confirm);

// 0x0040f3b0  size 0x24c
// Account login: Connect. Copies the fields, connects when the socket is down, sends the account login and
// saves the account settings to uo.cfg.
// ? The upper bytes of the stack argument of 0x0044e570 (the byte at 0x0069f8a0) are leftover register contents in
//   the original; it is passed zero-extended.
void __thiscall CAcctLoginGump::Connect()
{
    // via CF_NewWaiting: operator new 0x0048ad70, link 0x0043bcc0
    g_acctLastName = 0;
    strcpy(g_acctName, CF_Text(nameInput));
    strcpy(g_accountPassword, CF_Text(passwordInput));
    g_rememberPassword = RAWF(int32_t, checkBox, 0xb0);
    if (RAWF(int32_t, g_pktSender, 4) == -1) {
        visible = 0;
        g_waitingGump = new CGump_0043de30(-1, -1, "Connecting... ", (int)(g_pMainMenu), 1, 1, 0);
        g_waitingGump->lastChild->visible = 0;   // no null check in the original either
        g_waitingGump->LinkInto(nullptr, 1);
        int32_t rc[4] = {0, 0, 0x280, 0x1e0};
        reinterpret_cast<CGumpManager *>(g_gumpManager)->DrawGumps(rc, 1);   // CGumpManager::DrawRect
        int ok = ConnectLoginServer();
        if (ok == 0) {
            if (g_waitingGump != nullptr)
                CF_VDelete(g_waitingGump);
            g_waitingGump = nullptr;
            visible = 0;
            if (g_waitingGump != nullptr)
                return;
            g_waitingGump = static_cast<CGump *>(
                CF_Message("Couldn't connect to Ultima Online.  Please try again in a few moments.", this, 1, 0));
            return;
        }
        if (g_waitingGump != nullptr)
            CF_VDelete(g_waitingGump);
        g_waitingGump = nullptr;
    }
    SendAccountLogin((int)(g_acctName), (int)(g_accountPassword), g_charFlags);   // SendAccountLogin
    CF_Config(0x1a, g_acctName);
    if (g_rememberPassword != 0) {
        CF_Config(0x17, "yes");
        char tmp[0x40];
        strcpy(tmp, g_accountPassword);
        CF_Obfuscate(tmp);   // 0x0044ad90
        CF_Config(0x1c, tmp);
    } else {
        CF_Config(0x17, "no");
        CF_Config(0x1c, g_emptyString);
    }
    g_rememberPassword = RAWF(int32_t, checkBox, 0xb0);
    visible = 0;
}
HOOK_METHOD(0x0040f3b0, CAcctLoginGump, Connect);

// 0x0040f600  size 0xa6
// Account login: Cancel. Deletes this gump, shows the main menu and closes the connection.
void __thiscall CAcctLoginGump::Cancel()
{
    // via CF_SendDisconnectAndClose: 0x00476ad0 0x00480d60
    if (this != nullptr)
        CF_VDelete(this);
    CGump *menu = g_pMainMenu;
    g_dlgParent = nullptr;
    if (menu != nullptr)
        menu->visible = 1;
    else {
        CGump *fresh = new CMainMenuGump();
        g_pMainMenu = fresh;
        fresh->LinkInto(nullptr, 1);
    }
    CF_SendDisconnectAndClose();
}
HOOK_METHOD(0x0040f600, CAcctLoginGump, Cancel);

// 0x0040f6b0  size 0x83
// ? Rename group dialog: OK. Stores the typed name, redraws the skills window, saves, deletes this dialog.
void __thiscall CSkillGroupDlg::RenameGroupOK()
{
    char *text = CF_Text(nameEdit);
    if (strlen(text) != 0) {
        strcpy(g_groupNames + group * 0x11, text);
        if (GLOBAL_AS(CSkillListGump *, g_pSkillWindow) != nullptr)
            GLOBAL_AS(CSkillListGump *, g_pSkillWindow)->Redraw(-1);   // 0x00474cc0
        CF_SaveSkillGroups();
    }
    if (this != nullptr)
        CF_VDelete(this);
}
HOOK_METHOD(0x0040f6b0, CSkillGroupDlg, RenameGroupOK);

// 0x0040f740  size 0xdf
// ? Delete group dialog: OK. Skills of the group become ungrouped, later groups shift down by one.
void __thiscall CSkillGroupDlg::DeleteGroup()
{
    for (int32_t i = 0; i < g_skillCount; i++) {
        if (g_skillGroup[i] == group)
            g_skillGroup[i] = 0;
    }
    int32_t g = group + 1;
    if (g < g_groupCount) {
        char *names = g_groupNames + g * 0x11;
        do {
            for (int32_t i = 0; i < g_skillCount; i++) {
                if (g_skillGroup[i] == g)
                    g_skillGroup[i]--;
            }
            strcpy(names - 0x11, names);
            names += 0x11;
            g++;
        } while (g < g_groupCount);
    }
    g_groupCount = g_groupCount - 1;
    if (GLOBAL_AS(CSkillListGump *, g_pSkillWindow) != nullptr)
        GLOBAL_AS(CSkillListGump *, g_pSkillWindow)->Redraw(-1);   // 0x00474cc0
    if (this != nullptr)
        CF_VDelete(this);
    CF_SaveSkillGroups();
}
HOOK_METHOD(0x0040f740, CSkillGroupDlg, DeleteGroup);

// 0x0040f830  size 0x2a4
// Client options gump: OK. Applies the selections (0x0040fc30), writes them to uo.cfg and the saved files,
// sends the two 0x69 packets and deletes the gump. Does nothing when 0x004b718c is set.
void __thiscall CClientCfgGump::Apply()
{
    reinterpret_cast<CCharCreateGump *>(this)->CollectSelections();   // CCharCreateGump::CollectSelections
    if (g_004b718c != 0)
        return;
    char text[0x50];
    uint8_t v;
    CF_Config(5, toggle_c8->checked != 0 ? "on" : "off");
    v = g_byte_68db4c;
    sprintf(text, "%d", v);   // 0x0048b1d0
    CF_Config(6, text);
    CF_Config(7, toggle_cc->checked != 0 ? "on" : "off");
    v = g_byte_69faac;
    sprintf(text, "%d", v);
    CF_Config(8, text);
    g_slotHue0[g_selectedCharSlot] = g_colorSay;
    g_slotHue1[g_selectedCharSlot] = g_hue1;
    reinterpret_cast<CApp *>(g_app)->SaveCharData();   // SaveCharData
    CF_Config(0xf, toggle_d4->checked != 0 ? "on" : "off");
    CF_Config(0x29, toggle_d8->checked != 0 ? "on" : "off");
    CF_Config(0x25, toggle_dc->checked != 0 ? "yes" : "no");
    CF_Config(0x24, toggle_e0->checked != 0 ? "on" : "off");
    reinterpret_cast<CApp *>(g_app)->SaveMacros();   // CMacroManager::SaveMacros
    char obfuscated[0x24];
    strcpy(obfuscated, g_0069e630);
    CF_Obfuscate(obfuscated);
    CF_Config(0xb, obfuscated);
    CF_Config(0x28, g_ccVal1a0 != 0 ? "yes" : "no");
    CF_Config(9, toggle_f4->checked != 0 ? "on" : "off");
    reinterpret_cast<CApp *>(g_app)->SaveObsceneOrIgnoreList(0);   // SaveObsceneOrIgnoreList
    reinterpret_cast<CApp *>(g_app)->SaveObsceneOrIgnoreList(1);
    unk_a4 = 0;
    char packet[0x300];
    if (g_ccVal268 != 0)
        PORT_THISCALL2(&Packet_Build0x69, void, g_packetBuilder, char *, int)(packet, 0);   // Pkt_Build_0x69
    else
        PORT_THISCALL2(&Packet_Build0x69, void, g_packetBuilder, char *, int)(packet, 1);
    CF_Send(packet);
    PORT_THISCALL2(&Packet_Build0x69, void, g_packetBuilder, char *, int)(packet, 2);
    CF_Send(packet);
    if (this != nullptr)
        CF_VDelete(this);
}
HOOK_METHOD(0x0040f830, CClientCfgGump, Apply);

// 0x0040fae0  size 0x146
// Client options gump: Defaults button for the current tab (layer byte).
void __thiscall CClientCfgGump::SetDefaults()
{
    uint8_t tab = layer;
    if (tab == 1) {
        word_c0 = static_cast<uint16_t>(App_RandomMod(1000) + 1);   // RandomMod
        word_c2 = static_cast<uint16_t>(App_RandomMod(1000) + 1);
        toggle_c8->checked = 1;
        toggle_c8->Redraw();   // 0x004163f0
        toggle_cc->checked = 1;
        toggle_cc->Redraw();
        toggle_d4->checked = 0;
        toggle_d4->Redraw();
        reinterpret_cast<CWorldObj *>(this)->CWorldObj::ToVoid();
        return;
    }
    if (tab == 2) {
        reinterpret_cast<CWorldObj *>(this)->CWorldObj::ToVoid();
        return;
    }
    if (tab == 3 && toggle_f4->layer == tab) {
        toggle_f4->checked = 0;
        toggle_f4->Redraw();
        void *list = list_110;
        void *item = RAWF(void *, list, 0xb8);
        while (item != nullptr) {
            reinterpret_cast<CEditListGump *>(list)->RemoveItem((CEditListNode *)(item));   // CEditListGump::RemoveItem
            list = list_110;
            item = RAWF(void *, list, 0xb8);
        }
        const char *text = g_004ad1e0[0];
        if (text != nullptr) {
            const char **entry = g_004ad1e0;
            do {
                list_110->CEditListGump::AddItem(*entry++, 0);   // CEditListGump::AddItem
                text = *entry;
            } while (text != nullptr);
        }
        list_110->CEditListGump::AddItem(g_emptyString, 0);
        return;
    }
    if (tab == 4) {
        toggle_134->checked = 0;
        toggle_134->Redraw();
    }
}
HOOK_METHOD(0x0040fae0, CClientCfgGump, SetDefaults);

// 0x004100c0  size 0x50
// Client options gump: switch to tab 1 and redraw the background (art 0x960).
void __thiscall CClientCfgGump::ShowTab1()
{
    if (RAWF(int32_t, this, 0xf0) == g_focusGump)
        CF_CommitFocus();   // 0x0043a720
    if (layer != 1) {
        layer = 1;
        CF_DrawArt(0x960, this);   // 0x00433ed0
    }
}
HOOK_METHOD(0x004100c0, CClientCfgGump, ShowTab1);

// 0x00410160  size 0x5f
// Client options gump: switch to the tab in word_c6 and redraw the background (art word_c4).
void __thiscall CClientCfgGump::ShowTabC6()
{
    if (RAWF(int32_t, this, 0xf0) == g_focusGump)
        CF_CommitFocus();   // 0x0043a720
    if (static_cast<uint16_t>(layer) != word_c6) {
        layer = static_cast<uint8_t>(word_c6);
        CF_DrawArt(word_c4, this);   // 0x00433ed0
    }
}
HOOK_METHOD(0x00410160, CClientCfgGump, ShowTabC6);

// 0x004102c0  size 0xd5
// Client options gump: append a blank key binding node (0xe4 bytes) to the list copy and make it current.
void __thiscall CClientCfgGump::AddKeyBinding()
{
    if (RAWF(int32_t, this, 0xf0) == g_focusGump)
        CF_CommitFocus();   // 0x0043a720
    uint8_t *cur = RAWF(uint8_t *, this, 0x168);
    if (cur == nullptr) {
        void *node = ::operator new(0xe4);
        RAWF(void *, this, 0x164) = node;
        RAWF(void *, this, 0x168) = node;
    } else {
        if (RAWF(void *, cur, 0xe0) != nullptr) {
            do {
                uint8_t *tail = RAWF(uint8_t *, this, 0x168);
                uint8_t *nextNode = RAWF(uint8_t *, tail, 0xe0);
                RAWF(uint8_t *, this, 0x168) = nextNode;
            } while (RAWF(void *, RAWF(uint8_t *, this, 0x168), 0xe0) != nullptr);
        }
        void *node = ::operator new(0xe4);
        RAWF(void *, RAWF(uint8_t *, this, 0x168), 0xe0) = node;
        RAWF(void *, this, 0x168) = RAWF(void *, RAWF(uint8_t *, this, 0x168), 0xe0);
    }
    RAWF(int32_t, RAWF(uint8_t *, this, 0x168), 0xe0) = 0;
    RAWF(int32_t, RAWF(uint8_t *, this, 0x168), 0x14) = 0;
    RAWF(int32_t, RAWF(uint8_t *, this, 0x168), 0x04) = 0;
    RAWF(int32_t, RAWF(uint8_t *, this, 0x168), 0x08) = 0;
    RAWF(int32_t, RAWF(uint8_t *, this, 0x168), 0x0c) = 0;
    RAWF(int32_t, RAWF(uint8_t *, this, 0x168), 0x10) = 0;
    this->CClientCfgGump::UpdateKeyBindingDisplay();   // UpdateKeyBindingDisplay
}
HOOK_METHOD(0x004102c0, CClientCfgGump, AddKeyBinding);

// 0x004103a0  size 0xca
// Client options gump: delete the current key binding node and unlink it from the list copy.
// ? When the current node is the head and has a successor the original leaves the head pointer on the freed
//   node (the predecessor search never matches the head); kept.
void __thiscall CClientCfgGump::DeleteKeyBinding()
{
    if (RAWF(int32_t, this, 0xf0) == g_focusGump)
        CF_CommitFocus();   // 0x0043a720
    uint8_t *node = RAWF(uint8_t *, this, 0x168);
    if (node == nullptr)
        return;
    int32_t i = 0;
    if (RAWF(int32_t, node, 0x14) > 0) {
        uint8_t *p = node + 0x20;
        do {
            if (RAWF(void *, p, 0) != nullptr)
                operator delete(RAWF(void *, p, 0));   // 0x0048ad60
            i++;
            p += 0x14;
        } while (i < RAWF(int32_t, node, 0x14));
    }
    uint8_t *pred = RAWF(uint8_t *, this, 0x164);
    if (pred != nullptr) {
        while (pred != nullptr && RAWF(uint8_t *, pred, 0xe0) != node)
            pred = RAWF(uint8_t *, pred, 0xe0);
        if (pred != nullptr)
            RAWF(void *, pred, 0xe0) = RAWF(void *, node, 0xe0);
    }
    uint8_t *after = RAWF(uint8_t *, node, 0xe0);
    if (after != nullptr) {
        RAWF(void *, this, 0x168) = after;
    } else if (pred != nullptr) {
        RAWF(void *, this, 0x168) = pred;
    } else {
        RAWF(void *, this, 0x164) = nullptr;
        RAWF(void *, this, 0x168) = nullptr;
    }
    this->CClientCfgGump::UpdateKeyBindingDisplay();   // UpdateKeyBindingDisplay
    operator delete(node);                         // 0x0048ad60
}
HOOK_METHOD(0x004103a0, CClientCfgGump, DeleteKeyBinding);

// 0x00410470  size 0x54
// Client options gump: select the previous key binding node.
void __thiscall CClientCfgGump::PrevKeyBinding()
{
    if (RAWF(int32_t, this, 0xf0) == g_focusGump)
        CF_CommitFocus();   // 0x0043a720
    uint8_t *cur = RAWF(uint8_t *, this, 0x168);
    if (cur == nullptr)
        return;
    uint8_t *node = RAWF(uint8_t *, this, 0x164);
    if (node == nullptr)
        return;
    while (node != nullptr && RAWF(uint8_t *, node, 0xe0) != cur)
        node = RAWF(uint8_t *, node, 0xe0);
    if (node != nullptr) {
        RAWF(void *, this, 0x168) = node;
        this->CClientCfgGump::UpdateKeyBindingDisplay();   // UpdateKeyBindingDisplay
    }
}
HOOK_METHOD(0x00410470, CClientCfgGump, PrevKeyBinding);

// 0x004104d0  size 0x40
// Client options gump: select the next key binding node.
void __thiscall CClientCfgGump::NextKeyBinding()
{
    if (RAWF(int32_t, this, 0xf0) == g_focusGump)
        CF_CommitFocus();   // 0x0043a720
    uint8_t *cur = RAWF(uint8_t *, this, 0x168);
    if (cur != nullptr) {
        uint8_t *nextNode = RAWF(uint8_t *, cur, 0xe0);
        if (nextNode != nullptr) {
            RAWF(void *, this, 0x168) = nextNode;
            this->CClientCfgGump::UpdateKeyBindingDisplay();   // UpdateKeyBindingDisplay
        }
    }
}
HOOK_METHOD(0x004104d0, CClientCfgGump, NextKeyBinding);

namespace {
// Shared by the two list scrollers: y of a line gump moves by `step` and it is shown only inside the frame.
void SetRowVisibility(CGump *row, int step, int32_t frameHeight)
{
    int32_t rowY = row->y;
    row->visible = (rowY + step <= frameHeight && rowY >= 3) ? 1 : 0;
}
} // namespace

// 0x00410510  size 0xb8
// ? Scrolling list gump: scroll back by one entry (entries are nodes {gump, next, prev} at +0xb8, current at +0xc0).
void __thiscall CGump_00410510::ScrollBack()
{
    int w;
    int h;
    CF_ArtSize(0x13a7, &w, &h);   // 0x00433e70
    h++;
    if (RAWF(int32_t, this, 0xd4) <= 0)
        return;
    int32_t index = RAWF(int32_t, this, 0xd4) - 1;
    RAWF(int32_t, this, 0xd4) = index;
    uint8_t *cur = RAWF(uint8_t *, this, 0xc0);
    RAWF(void *, this, 0xc0) = RAWF(void *, cur, 8);
    for (uint8_t *node = RAWF(uint8_t *, this, 0xb8); node != nullptr; node = RAWF(uint8_t *, node, 4)) {
        CGump *row = RAWF(CGump *, node, 0);
        row->y = row->y + h;
        SetRowVisibility(row, h, height);
    }
    if (RAWF(int32_t, this, 0xd4) == 0)
        RAWF(CGump *, this, 0x94)->visible = 0;
    CGump *down = RAWF(CGump *, this, 0x98);
    if (down->visible == 0)
        down->visible = 1;
    reinterpret_cast<CEditListGump *>(this)->RedrawBackground();
}
HOOK_METHOD(0x00410510, CGump_00410510, ScrollBack);

// 0x004105d0  size 0xc7
// ? Scrolling list gump: scroll forward by one entry.
void __thiscall CGump_00410510::ScrollForward()
{
    int w;
    int h;
    CF_ArtSize(0x13a7, &w, &h);   // 0x00433e70
    h++;
    int32_t index = RAWF(int32_t, this, 0xd4) + 1;
    if (index >= RAWF(int32_t, this, 0xcc))
        return;
    uint8_t *cur = RAWF(uint8_t *, this, 0xc0);
    RAWF(int32_t, this, 0xd4) = index;
    RAWF(void *, this, 0xc0) = RAWF(void *, cur, 4);
    for (uint8_t *node = RAWF(uint8_t *, this, 0xb8); node != nullptr; node = RAWF(uint8_t *, node, 4)) {
        CGump *row = RAWF(CGump *, node, 0);
        row->y = row->y - h;
        SetRowVisibility(row, h, height);
    }
    if (RAWF(int32_t, this, 0xd4) >= RAWF(int32_t, this, 0xcc) - 1)
        RAWF(CGump *, this, 0x98)->visible = 0;
    CGump *up = RAWF(CGump *, this, 0x94);
    if (up->visible == 0)
        up->visible = 1;
    reinterpret_cast<CEditListGump *>(this)->RedrawBackground();
}
HOOK_METHOD(0x004105d0, CGump_00410510, ScrollForward);

// 0x004106a0  size 0xa9
// ? Row gump: when some row is above the top, move all rows down by 0x1a.
void __thiscall CGump_004106a0::ScrollRowsDown()
{
    int count = 0;
    for (CGump *row = lastChild; row != nullptr; row = row->prev) {
        if (PORT_VSLOT0(row, 0xd8, int)() != 0 && row->y < 0)
            count++;
    }
    if (count == 0)
        return;
    for (CGump *row = lastChild; row != nullptr; row = row->prev) {
        if (PORT_VSLOT0(row, 0xd8, int)() != 0) {
            row->y = row->y + 0x1a;
            int32_t rowY = row->y;
            if (rowY + 0x1a < height && rowY > 0)
                row->visible = 1;
            else
                row->visible = 0;
        }
    }
    if (count == 1)
        RAWF(CGump *, this, 0x94)->visible = 0;
    RAWF(CGump *, this, 0x98)->visible = 1;
}
HOOK_METHOD(0x004106a0, CGump_004106a0, ScrollRowsDown);

// 0x00410750  size 0xaa
// ? Row gump: when at least two rows are below the top, move all rows up by 0x1a.
void __thiscall CGump_004106a0::ScrollRowsUp()
{
    int count = 0;
    for (CGump *row = lastChild; row != nullptr; row = row->prev) {
        if (PORT_VSLOT0(row, 0xd8, int)() != 0 && row->y > 0)
            count++;
    }
    if (count < 2)
        return;
    for (CGump *row = lastChild; row != nullptr; row = row->prev) {
        if (PORT_VSLOT0(row, 0xd8, int)() != 0) {
            row->y = row->y - 0x1a;
            int32_t rowY = row->y;
            if (rowY + 0x1a < height && rowY > 0)
                row->visible = 1;
            else
                row->visible = 0;
        }
    }
    if (count == 2)
        RAWF(CGump *, this, 0x98)->visible = 0;
    RAWF(CGump *, this, 0x94)->visible = 1;
}
HOOK_METHOD(0x00410750, CGump_004106a0, ScrollRowsUp);

// 0x00410880  size 0x8d
// ? Bulletin board message gump: Reply. Opens a composer on the board with the subject "RE: <subject at +0xde>".
void __thiscall CBBMsgGump::OnReply()
{
    if (owner == nullptr)
        return;
    void *board = RAWF(void *, owner, 0x7c);
    if (board == nullptr)
        return;
    if (PORT_VSLOT0(board, 0x30, int)() == 0)
        return;
    if (RAWF(void *, RAWF(void *, owner, 0x7c), 0xac) == nullptr)
        return;
    char replySubject[0x54];
    sprintf(replySubject, "RE: %s", subject);   // 0x0048b1d0
    int32_t parentSerial = postSerial;
    if (parentSerial != 0) {
        reinterpret_cast<CBBGump *>(RAWF(void *, RAWF(void *, owner, 0x7c), 0xac))->OnPostNew(parentSerial, replySubject);   // CBBGump::OnPostNew
        return;
    }
    reinterpret_cast<CBBGump *>(RAWF(void *, RAWF(void *, owner, 0x7c), 0xac))->OnPostNew(RAWF(int32_t, owner, 0x78), replySubject);
}
HOOK_METHOD(0x00410880, CBBMsgGump, OnReply);

namespace {
// Common tail of the bulletin board list scrollers: how many visible lines precede the current one.
void BBUpdateThumb(CBBGump *board)
{
    int selected = 0;
    int count = 0;
    for (CGump *g = board->lastChild; g != nullptr; g = g->prev) {
        if (PORT_VSLOT0(g, 0xe0, int)() != 0 && reinterpret_cast<CBBLineGump *>(g)->IsVisibleByParent() != 0) {   // visible line?
            if (RAWF(CGump *, board, 0xac) == g)
                selected = count;
            count++;
        }
    }
    CF_ThumbSet(RAWF(void *, board, 0xb8), selected, count - 1);
}
} // namespace

// 0x00410960  size 0xe6
// Bulletin board gump: scroll the list by one line towards the previous line.
void __thiscall CBBGump::ScrollUp()
{
    // via BBUpdateThumb: 0x00404540 (the second visibility test) and the thumb update 0x00413c00
    CBBLineGump *cur = RAWF(CBBLineGump *, this, 0xac);
    if (cur == nullptr)
        return;
    CBBLineGump *prevLine = cur->PrevVisibleLine();   // 0x004045c0
    if (prevLine == nullptr)
        return;
    RAWF(CBBLineGump *, this, 0xac) = prevLine;
    for (CGump *g = lastChild; g != nullptr; g = g->prev) {
        if (PORT_VSLOT0(g, 0xe0, int)() != 0) {
            g->y = g->y + g_004a619c;
            int32_t rowY = g->y;
            if (rowY + g_004a619c <= RAWF(int32_t, this, 0xa0) && rowY >= RAWF(int32_t, this, 0x98)) {
                if (reinterpret_cast<CBBLineGump *>(g)->IsVisibleByParent() != 0)
                    g->visible = 1;
            } else {
                g->visible = 0;
            }
        }
    }
    RAWF(CBBLineGump *, this, 0xac)->PrevVisibleLine();   // 0x004045c0, result unused
    BBUpdateThumb(this);
}
HOOK_METHOD(0x00410960, CBBGump, ScrollUp);

// 0x00410a50  size 0xe6
// Bulletin board gump: scroll the list by one line towards the next line.
void __thiscall CBBGump::ScrollDown()
{
    // via BBUpdateThumb: 0x00404540 (the second visibility test) and the thumb update 0x00413c00
    void *cur = RAWF(void *, this, 0xac);
    if (cur == nullptr)
        return;
    void *nextLine = reinterpret_cast<CBBLineGump *>(cur)->NextVisibleLine();
    if (nextLine == nullptr)
        return;
    RAWF(void *, this, 0xac) = nextLine;
    for (CGump *g = lastChild; g != nullptr; g = g->prev) {
        if (PORT_VSLOT0(g, 0xe0, int)() != 0) {
            g->y = g->y - g_004a619c;
            int32_t rowY = g->y;
            if (rowY + g_004a619c <= RAWF(int32_t, this, 0xa0) && rowY >= RAWF(int32_t, this, 0x98)) {
                if (reinterpret_cast<CBBLineGump *>(g)->IsVisibleByParent() != 0)
                    g->visible = 1;
            } else {
                g->visible = 0;
            }
        }
    }
    reinterpret_cast<CBBLineGump *>(RAWF(void *, this, 0xac))->NextVisibleLine();   // result unused
    BBUpdateThumb(this);
}
HOOK_METHOD(0x00410a50, CBBGump, ScrollDown);

// 0x00410b40  size 0x34
// Bulletin board message gump: scroll the text up by one line.
void __thiscall CBBMsgGump::ScrollUp()
{
    if (top > 0) {
        top = top - 1;
        this->CBBMsgGump::RedrawText();   // RedrawText
        CF_ThumbSet(scrollbar, top, lastLine);
    }
}
HOOK_METHOD(0x00410b40, CBBMsgGump, ScrollUp);

// 0x00410b80  size 0x3c
// Bulletin board message gump: scroll the text down by one line.
void __thiscall CBBMsgGump::ScrollDown()
{
    int32_t nextTop = top + 1;
    if (nextTop < lastLine) {
        top = nextTop;
        this->CBBMsgGump::RedrawText();   // RedrawText
        CF_ThumbSet(scrollbar, top, lastLine);
    }
}
HOOK_METHOD(0x00410b80, CBBMsgGump, ScrollDown);

// 0x00410bc0  size 0xb3
// Object picker gump: step back one entry and show its picture size.
void __thiscall CObjPickerGump::PrevPage()
{
    if (unk_ac != 0 || unk_b8 == 0)
        return;
    unk_ac = 1;
    unk_b8 = unk_b8 - 1;
    uint8_t *art = PORT_THISCALL1(&ArtRecord_Get, uint8_t *, GLOBAL_PTR(void, g_artCache_storage), uint32_t)(ids[unk_b8] + 0x4000u);
    if (art == nullptr)
        art = PORT_THISCALL1(&ArtRecord_Get, uint8_t *, GLOBAL_PTR(void, g_artCache_storage), uint32_t)(0x4000);
    if (art != nullptr) {
        RAWF(int32_t, this, 0xbc) = RAWF(uint16_t, art, 4) - 1;
        RAWF(int32_t, this, 0xc0) = -((RAWF(uint16_t, art, 4) - 6) / 4);
        this->CObjPickerGump::RedrawList();   // RedrawList
        return;
    }
    RAWF(int32_t, this, 0xbc) = 0x2b;
    RAWF(int32_t, this, 0xc0) = -9;
    this->CObjPickerGump::RedrawList();   // RedrawList
}
HOOK_METHOD(0x00410bc0, CObjPickerGump, PrevPage);

// 0x00410c80  size 0xa2
// Object picker gump: step forward one entry.
// ? The original does not store the new index here (unlike PrevPage); kept.
void __thiscall CObjPickerGump::NextPage()
{
    if (unk_ac != 0 || unk_b8 + 1 >= count)
        return;
    unk_ac = 1;
    uint8_t *art = PORT_THISCALL1(&ArtRecord_Get, uint8_t *, GLOBAL_PTR(void, g_artCache_storage), uint32_t)(ids[unk_b8] + 0x4000u);
    if (art == nullptr)
        art = PORT_THISCALL1(&ArtRecord_Get, uint8_t *, GLOBAL_PTR(void, g_artCache_storage), uint32_t)(0x4000);
    RAWF(int32_t, this, 0xbc) = 1;
    if (art != nullptr) {
        int32_t artWidth = RAWF(uint16_t, art, 4);
        RAWF(int32_t, this, 0xc0) = (artWidth - 6) / 4;
        this->CObjPickerGump::RedrawList();   // RedrawList
        return;
    }
    RAWF(int32_t, this, 0xc0) = 9;
    this->CObjPickerGump::RedrawList();   // RedrawList
}
HOOK_METHOD(0x00410c80, CObjPickerGump, NextPage);

// 0x00410d30  size 0x3c
// ? Hue picker gump: step the position down.
void __thiscall CHueGump_00410d30::StepDown()
{
    uint16_t pos = RAWF(uint16_t, this, 0xda);
    if (pos > 0) {
        RAWF(uint16_t, this, 0xda) = static_cast<uint16_t>(pos - 1);
        RAWF(uint16_t, this, 0xd8) = static_cast<uint16_t>(RAWF(uint16_t, this, 0xd8) - 1);
        reinterpret_cast<CHueGump *>(this)->Redraw();   // CHueGump::Redraw
        CF_ThumbSet(RAWF(void *, this, 0x94), RAWF(uint16_t, this, 0xda), 4);
    }
}
HOOK_METHOD(0x00410d30, CHueGump_00410d30, StepDown);

// 0x00410d70  size 0x3d
// ? Hue picker gump: step the position up (at most 4).
void __thiscall CHueGump_00410d30::StepUp()
{
    uint16_t pos = RAWF(uint16_t, this, 0xda);
    if (pos < 4) {
        RAWF(uint16_t, this, 0xda) = static_cast<uint16_t>(pos + 1);
        RAWF(uint16_t, this, 0xd8) = static_cast<uint16_t>(RAWF(uint16_t, this, 0xd8) + 1);
        reinterpret_cast<CHueGump *>(this)->Redraw();   // CHueGump::Redraw
        CF_ThumbSet(RAWF(void *, this, 0x94), RAWF(uint16_t, this, 0xda), 4);
    }
}
HOOK_METHOD(0x00410d70, CHueGump_00410d30, StepUp);

// 0x00410dc0  size 0x130
// ? Drop the connection (sending the disconnect packet when there is one), reset the world and return to a fresh
// main menu. Takes no arguments (ECX is not read).
void __cdecl CF_ResetToMainMenu()
{
    // main menu: new CMainMenuGump, link 0x0043bcc0
    if (g_pktSender != nullptr) {
        char packet[0x20];
        PORT_THISCALL1(&Build_Packet_0x01, void, g_packetBuilder, char *)(packet);   // Build_Packet_0x01
        CF_Send(packet);
        if (g_pktSender != nullptr)
            CF_VDelete(g_pktSender);
        g_pktSender = nullptr;
    }
    g_inputActive = 0;
    g_flag6f493c = 0;
    GLOBAL_AS(int32_t, g_loginFlag648928) = 1;
    g_inGame7211ac = 0;
    g_pktSender = new CClientSocket();
    ResetWorldState();                   // reset world state
    reinterpret_cast<CGumpManager *>(g_gumpManager)->SaveDesktop();           // CGumpManager::SaveDesktop
    reinterpret_cast<CGumpManager *>(g_gumpManager)->DeleteTransientGumps();           // delete the deletable top-level gumps
    g_screenGump->visible = 0;
    PORT_THISCALL0(&SetMainWindowTitle, void, const_cast<char *>(g_emptyString))();           // SetMainWindowTitle("")
    memset(g_backBuffer, 0, 0x25800 * 4);
    CGump *fresh = new CMainMenuGump();
    g_pMainMenu = fresh;
    fresh->LinkInto(nullptr, 1);
}
HOOK_FUNC(0x00410dc0, CF_ResetToMainMenu);

// 0x00410ef0  size 0x117
// ? Show the account login gump with the remembered account name (and password when remembered).
void __cdecl CF_ShowAcctLogin()
{
    g_pMainMenu->visible = 0;
    CGump *obj = new CAcctLoginGump();
    g_dlgParent = obj;
    if (strlen(g_acctName) != 0) {
        strcpy(CF_Text(RAWF(void *, obj, 0x94)), g_acctName);
        reinterpret_cast<CInputControlGump *>(RAWF(void *, g_dlgParent, 0x94))->Redraw();   // CInputControlGump::Redraw
    }
    if (g_rememberPassword != 0) {
        strcpy(CF_Text(RAWF(void *, g_dlgParent, 0x98)), g_accountPassword);
        reinterpret_cast<CInputControlGump *>(RAWF(void *, g_dlgParent, 0x98))->Redraw();
    }
    g_dlgParent->LinkInto(nullptr, 1);
}
HOOK_FUNC(0x00410ef0, CF_ShowAcctLogin);

// 0x00411010  size 0x7d
// ? Deletes itself, plays the intro movie, then shows a fresh main menu gump.
void __thiscall CGump_00411010::Run()
{
    // main menu: new CMainMenuGump, link 0x0043bcc0
    if (this != nullptr)
        CF_VDelete(this);
    g_pMainMenu = nullptr;
    PlayIntroMovie_Wve();   // PlayIntroMovie_Wve
    CGump *fresh = new CMainMenuGump();
    g_pMainMenu = fresh;
    fresh->LinkInto(nullptr, 1);
}
HOOK_METHOD(0x00411010, CGump_00411010, Run);

// 0x004110e0  size 0xd2  (LaunchWelcomeSplash)
// Run "<install CD path>welcome.exe splash"; report a missing CD path or setup program in a message gump.
void __cdecl CF_LaunchWelcomeSplash()
{
    char path[0x100];
    App_GetInstCDPath(path, 0xc8);   // GetInstCDPath
    if (strlen(path) == 0) {
        g_pMainMenu->visible = 0;
        PORT_THISCALL4(&CreateGump2ac, void *, g_gumpManager, const char *, void *, int, int)("CD path not found!", g_pMainMenu, 1, 0);
        return;
    }
    strcat(path, "welcome.exe splash");
    UINT rc = WinExec(path, 10);
    if (rc == 2 || rc == 3) {
        g_pMainMenu->visible = 0;
        PORT_THISCALL4(&CreateGump2ac, void *, g_gumpManager, const char *, void *, int, int)("Setup program not found.  Make sure you have your UO cd in the drive.", g_pMainMenu, 1, 0);
    }
}
HOOK_FUNC(0x004110e0, CF_LaunchWelcomeSplash);

// 0x004111c0  size 0x3d
// ? Handler of the 0x2ac-byte waiting gump: re-show its +0x2a0 child, forget it as the waiting gump and either
// reset to the main menu (flag at +0x9c) or delete it. The tail jump to 0x00410dc0 becomes a call.
void __thiscall CGump_004111c0::Close()
{
    CGump *child = RAWF(CGump *, this, 0x2a0);
    if (child != nullptr)
        child->visible = 1;
    if (g_waitingGump == this)
        g_waitingGump = nullptr;
    if (RAWF(int32_t, this, 0x9c) != 0) {
        CF_ResetToMainMenu();   // 0x00410dc0
        return;
    }
    if (this != nullptr)
        CF_VDelete(this);
}
HOOK_METHOD(0x004111c0, CGump_004111c0, Close);

// 0x00411200  size 0xc2
// Skill page: open the skill choice list for the name box at +0x9c.
void __thiscall CInitSkillGump::OpenSkillChoice0()
{
    art_98->visible = 0;
    CScrollChoiceGump *list = new CScrollChoiceGump(this, name_9c, 0x4cc);   // 0x0041c930
    for (int32_t i = 0; i < g_skillCount; i++) {
        uint32_t id = g_skillOrder[i];
        list->AddEntry(static_cast<int>(id), g_skillNames + id * 0x50, 0);   // 0x0041d7b0
    }
    list->Redraw();   // 0x0041d830
    list->LinkInto(nullptr, 1);
    g_scrollChoiceGump = list;
}
HOOK_METHOD(0x00411200, CInitSkillGump, OpenSkillChoice0);

// 0x004112d0  size 0xc2
// Skill page: open the skill choice list for the name box at +0xa0.
void __thiscall CInitSkillGump::OpenSkillChoice1()
{
    art_98->visible = 0;
    CScrollChoiceGump *list = new CScrollChoiceGump(this, name_a0, 0x4cc);   // 0x0041c930
    for (int32_t i = 0; i < g_skillCount; i++) {
        uint32_t id = g_skillOrder[i];
        list->AddEntry(static_cast<int>(id), g_skillNames + id * 0x50, 0);   // 0x0041d7b0
    }
    list->Redraw();   // 0x0041d830
    list->LinkInto(nullptr, 1);
    g_scrollChoiceGump = list;
}
HOOK_METHOD(0x004112d0, CInitSkillGump, OpenSkillChoice1);
