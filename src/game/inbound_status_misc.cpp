// Inbound packet handlers, second pass (batch 22), part 1: status, follow, skills, shop, login errors, sound,
// weather, map plot, target cursor.
//
//   0x0045aa70 0x30  swing target           0x0045aac0 0x31  pause client (clears the swing target)
//   0x0045aae0 0x11  mobile status          0x0045af10 0x15  follow
//   0x0045b010 0x3A  skills                 0x0045b390 0x3B  buy items (shop done / close)
//   0x0045b4c0 0x38  pathfinding move       0x0045c1d0 0x4E  personal light level
//   0x0045c240 0x5B  time                   0x0045c2d0 0x53  login error
//   0x0045c550 0x55  login complete         0x0045c5b0 0x56  map plot
//   0x0045c790 0x54  play sound effect      0x0045c970 0x65  set weather
//   0x0045d610       GetPacketWriteOffset   0x0045d900 0x6C  target cursor
//
// All handlers are __stdcall (RET 4) with the packet pointer. The first field of an inbound packet is at
// "base" = 3 for a variable-length opcode and 1 otherwise (packet_cursor.h); the original expands that test
// inline at every field read (or calls 0x00458ca0, which returns the same value), the port reads at
// base + offset. The inline copies of strlen/strcpy are host calls. The SEH frames the original wraps around
// operator new + constructor calls are omitted.
#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "pos3.h"
#include "app.h"
#include "bill_gump.h"
#include "book_gump.h"
#include "course_gump.h"
#include "edit_host_gump.h"
#include "effect_sequence.h"
#include "globals.h"
#include "gump.h"
#include "gump_manager.h"
#include "login_gumps.h"
#include "obj.h"
#include "object_manager.h"
#include "crt.h"
#include "packet_cursor.h"
#include "raw_field.h"
#include "skill_list_gump.h"
#include "sound_manager.h"
#include "status_gump.h"
#include "text_entry.h"
#include "vcall.h"
#include "weather.h"
#include "text_gump_ctor_ports.h"
#include "shop_gump.h"
#include "walk_queue_ports.h"
#include "gump_art_ops_ports.h"
#include "window_helpers_31_ports.h"
#include "sound_ops_ports.h"
#include "app_support_ports.h"
#include "small_methods_u1_ports.h"
#include "target_cursor.h"

// ---------------------------------------------------------------------------
// Globals (single-file use).
// ---------------------------------------------------------------------------
#define g_msgTarget GLOBAL_PTR(void, g_msgTarget_storage)               // on-screen message / journal object
#define g_skillCountW GLOBAL_AS(uint16_t, g_skillCount)
#define g_weatherObj GLOBAL_AS(CWeather, g_weatherObj_storage)

// ---------------------------------------------------------------------------
// Original code not ported yet.
// ---------------------------------------------------------------------------
static CObj *FindObject(uint32_t serial) { return static_cast<CObj *>((&g_objMgr)->CObjectManager::FindBySerial(serial)); }
static void Journal_Say(const char *text) { PORT_THISCALL3(&CreateBarkGump, void, g_msgTarget, int, int, const char *)(0, 3, text); }
static void SkillGump_SetOwnValues(void *gump, int v) { reinterpret_cast<CSkillListGump *>(gump)->SetUseOwnValues(v); }
static void SkillGump_SetValue(void *gump, int index, int value) { reinterpret_cast<CSkillListGump *>(gump)->SetSkillValue(index, (uint16_t)(value)); }
static void Shop_Redraw(void *shop) { reinterpret_cast<CShopGump *>(shop)->RedrawFrame(); }
#define PathfindTo(a1, a2, a3) (Walk_PathfindTo((uint16_t)(a1), (uint16_t)(a2), a3))
#define GumpArtSize(a1, a2, a3) (GumpArtSizeImpl(a1, a2, a3))

static uint32_t rd32(const uint8_t *p, uint32_t off)
{
    uint32_t v;
    memcpy(&v, p + off, sizeof(v));
    return v;
}
static uint16_t rd16(const uint8_t *p, uint32_t off)
{
    uint16_t v;
    memcpy(&v, p + off, sizeof(v));
    return v;
}

// ---------------------------------------------------------------------------
// 0x0045aa70  size 0x46
// Inbound packet 0x30: remember the object with the given serial as the player's swing target (player+0x1f4).
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x30(const uint8_t *packet)
{
    uint32_t serial = ntohl(rd32(packet, pkt::Base(packet)));
    RAWF(CObj *, g_player, 0x1f4) = FindObject(serial);
}
HOOK_FUNC(0x0045aa70, Packet_Handle0x30);

// ---------------------------------------------------------------------------
// 0x0045aac0  size 0x16
// Inbound packet 0x31: clear the swing target. The packet is not read.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x31(const uint8_t *packet)
{
    (void)packet;
    RAWF(CObj *, g_player, 0x1f4) = nullptr;
}
HOOK_FUNC(0x0045aac0, Packet_Handle0x31);

// ---------------------------------------------------------------------------
// 0x0045aae0  size 0x42e
// Inbound packet 0x11: mobile status. Fills the status block at 0x0064cd38 (name at +8) and pushes it into
// every status gump showing that mobile (and the bill/shop value when it is the player).
// Uses 0x00458ca0 (base offset) and 0x004576a0 (ReadU16) inline in the original; the base-offset call
// 0x00458ca0 occurs 8 times (0x00458ca0 0x00458ca0 0x00458ca0 0x00458ca0 0x00458ca0 0x00458ca0), all folded into d.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x11(const uint8_t *packet)
{
    const uint8_t *d = packet + pkt::Base(packet);
    uint32_t serial = ntohl(rd32(d, 0));
    strcpy(g_0064cd40, reinterpret_cast<const char *>(d + 4));    // name, 30 bytes in the packet
    g_0064cd66 = ntohs(rd16(d, 0x22));
    g_0064cd68 = ntohs(rd16(d, 0x24));
    uint8_t renameFlag = d[0x26];
    uint8_t haveStats = d[0x27];
    if (haveStats != 0) {
        g_0064cd5e = d[0x28];                                  // sex
        g_0064cd60 = ntohs(rd16(d, 0x29));
        g_0064cd62 = ntohs(rd16(d, 0x2b));
        g_0064cd64 = ntohs(rd16(d, 0x2d));
        g_0064cd6a = ntohs(rd16(d, 0x2f));
        g_0064cd6c = ntohs(rd16(d, 0x31));
        g_0064cd6e = ntohs(rd16(d, 0x33));
        g_0064cd70 = ntohs(rd16(d, 0x35));
        g_0064cd74 = ntohl(rd32(d, 0x37));
        g_0064cd78 = ntohs(rd16(d, 0x3b));
        g_0064cd7a = ntohs(rd16(d, 0x3d));               // 0x004576a0
    } else {
        g_0064cd5e = 0;
        g_0064cd60 = 0;
        g_0064cd62 = 0;
        g_0064cd64 = 0;
        g_0064cd6a = 0;
        g_0064cd6c = 0;
        g_0064cd6e = 0;
        g_0064cd70 = 0;
        g_0064cd74 = 0;
        g_0064cd78 = 0;
        g_0064cd7a = 0;
    }

    CObj *obj = FindObject(serial);
    for (CGump *g = g_gumpListTail; g != nullptr; g = g->prev) {
        if (decomp::vcall(PORT_RET(int), g, 0xac) != 0 && g->owner == obj) {
            memcpy(reinterpret_cast<uint8_t *>(g) + 0x94, GLOBAL_PTR(uint8_t, g_statusBlock), 0x44);   // 0x11 dwords
            if (renameFlag != 0)
                static_cast<CEditHostGump *>(g)->CreateEditControl();              // 0x004694d0
            static_cast<CStatusGump *>(g)->RedrawFrame(1, 1, 1, 1);                // 0x00468e00
        } else if (decomp::vcall(PORT_RET(int), g, 0xa0) != 0 && obj == g_player) {
            RAWF(uint16_t, g, 0xd0) = RAWF(uint16_t, GLOBAL_PTR(uint8_t, g_statusBlock), 0x3c);         // low word of the gold dword at 0x0064cd74
            RAWF(CBillGump *, g, 0xd8)->RedrawFrame();                             // 0x0046e5e0
        }
    }
}
HOOK_FUNC(0x0045aae0, Packet_Handle0x11);

// ---------------------------------------------------------------------------
// 0x0045af10  size 0xfc
// Inbound packet 0x15: follow. Either serial being 0 means "no longer following" (bark on the player);
// otherwise "Now following." barks on the followed object.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x15(const uint8_t *packet)
{
    const uint8_t *d = packet + pkt::Base(packet);
    uint32_t first = ntohl(rd32(d, 0));
    uint32_t second = ntohl(rd32(d, 4));
    if (first == 0 || second == 0) {
        strcpy(g_barkPrefix, "YOU");
        g_player->CreateBarkKeepTwo("No longer following.", 0, 3, nullptr);        // 0x00453320
        g_followingFlag = 0;
    }
    CObj *obj = FindObject(second);
    if (obj != nullptr) {
        strcpy(g_barkPrefix, "???");
        obj->CreateBarkKeepTwo("Now following.", 0, 3, nullptr);                   // 0x00453320
        g_followingFlag = 1;
    }
}
HOOK_FUNC(0x0045af10, Packet_Handle0x15);

// ---------------------------------------------------------------------------
// 0x0045b010  size 0x372
// Inbound packet 0x3A: skills. Type 0xFE: skill name table; 0xFF: single value update; 0 / 1 / other: full
// list (0 = the player's values, 1 = a separate skill gump for another mobile).
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x3A(const uint8_t *packet)
{
    const uint8_t *d = packet + pkt::Base(packet);
    uint8_t type = d[0];
    uint32_t off = 1;

    if (type == 0xfe) {
        g_skillCountW = ntohs(rd16(d, 1));
        uint32_t cur = 3;
        char *name = g_skillNames;
        for (int i = 0; i < g_skillCount; i++) {
            g_skillFlags[i] = d[cur++];
            uint32_t len = d[cur++];
            sprintf(name, "%s", d + cur);                                      // 0x0048b1d0
            cur += len;
            name += 0x50;
        }
        return;
    }
    if (type == 0xff) {
        uint32_t index = ntohs(rd16(d, 1));
        uint16_t value = ntohs(rd16(d, 3));
        g_skillValues[index & 0xffff] = value;
        if (g_pSkillWindow != nullptr)
            static_cast<CSkillListGump *>(g_pSkillWindow)->Redraw(-1);                // 0x00474cc0
        return;
    }

    void *newGump = nullptr;
    if (type == 1) {
        newGump = new CSkillListGump(0x64, 0x64);                                  // 0x00472da0
        CGump_AddChild(static_cast<CGump *>(newGump), nullptr, 1);                 // 0x0043bcc0
        SkillGump_SetOwnValues(newGump, 1);                                        // 0x004760f0
    } else if (type == 0) {
        memset(g_skillValues, 0, 0x19 * 4);
    }

    int counter = 0;
    do {
        uint32_t id = ntohs(rd16(d, off));
        off += 2;
        if (static_cast<uint16_t>(id) == 0)
            break;
        uint16_t value = ntohs(rd16(d, off));
        off += 2;
        if (type == 0)
            (g_skillValues - 1)[id & 0xffff] = value;                               // values[id - 1]
        else if (type == 1)
            SkillGump_SetValue(newGump, static_cast<int>(id & 0xffff) - 1, value);  // 0x00476100
        counter++;
    } while (counter < 0x32);

    if (type == 0) {
        CGump *sg = g_pSkillWindow;
        if (sg != nullptr) {
            if (g_desktopChanged != 0) {
                sg->BringToFront(0);                                                // 0x0043ba70
                if (sg->unk_60 != 0)
                    reinterpret_cast<CBookGump *>(g_pSkillWindow)->Uniconify();       // 0x0040d410
            }
            static_cast<CSkillListGump *>(g_pSkillWindow)->Redraw(-1);                // 0x00474cc0
        } else if (g_desktopChanged != 0) {
            void *gump = new CSkillListGump(0x64, 0x64);                            // 0x00472da0
            g_pSkillWindow = static_cast<CGump *>(gump);
            CGump_AddChild(static_cast<CGump *>(gump), nullptr, 1);                // 0x0043bcc0
            g_desktopChanged = 0;
        }
    } else if (type == 1) {
        static_cast<CSkillListGump *>(newGump)->Redraw(-1);                         // 0x00474cc0
    }
}
HOOK_FUNC(0x0045b010, Packet_Handle0x3A);

// ---------------------------------------------------------------------------
// 0x0045b390  size 0x124
// Inbound packet 0x3B: buy items. flag 1: reset and redraw the shop gump(s) and bill of the vendor;
// flag 0: delete the shop gump(s) (and with it the bill) of the vendor.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x3B(const uint8_t *packet)
{
    const uint8_t *d = packet + pkt::Base(packet);
    uint32_t serial = ntohl(rd32(d, 0));
    uint8_t flag = d[4];
    CObj *obj = FindObject(serial);
    if (obj == nullptr)
        return;
    if (flag == 1) {
        CGump *g = g_gumpListTail;
        if (g == nullptr)
            return;
        do {
            CGump *prev = g->prev;
            if (decomp::vcall(PORT_RET(int), g, 0xa0) != 0 && g->owner == obj) {
                RAWF(int32_t, g, 0xbc) = 0;
                Shop_Redraw(g);                                                     // 0x0046d280
                RAWF(CBillGump *, g, 0xd8)->RedrawFrame();                          // 0x0046e5e0
            }
            g = prev;
        } while (g != nullptr);
        return;
    }
    if (flag != 0)
        return;
    CGump *g = g_gumpListTail;
    if (g == nullptr)
        return;
    do {
        CGump *prev = g->prev;
        if (decomp::vcall(PORT_RET(int), g, 0xa0) != 0 && g->owner == obj) {
            if (prev == RAWF(CGump *, g, 0xd8))
                prev = prev->prev;
            int32_t saved = g_suppressFlag;
            g_suppressFlag = 1;
            if (g != nullptr)
                decomp::vdelete(g);
            g_suppressFlag = saved;
        }
        g = prev;
    } while (g != nullptr);
}
HOOK_FUNC(0x0045b390, Packet_Handle0x3B);

// ---------------------------------------------------------------------------
// 0x0045b4c0  size 0x92
// Inbound packet 0x38: pathfind to (x, y, z).
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x38(const uint8_t *packet)
{
    const uint8_t *d = packet + pkt::Base(packet);
    uint32_t x = ntohs(rd16(d, 0));
    uint32_t y = ntohs(rd16(d, 2));
    uint32_t z = ntohs(rd16(d, 4));
    PathfindTo(x, y, z);                                                            // 0x00449b10
}
HOOK_FUNC(0x0045b4c0, Packet_Handle0x38);

// ---------------------------------------------------------------------------
// 0x0045c1d0  size 0x6f
// Inbound packet 0x4E: personal light level of an object (signed byte) via 0x004538a0.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x4E(const uint8_t *packet)
{
    const uint8_t *d = packet + pkt::Base(packet);
    uint32_t serial = ntohl(rd32(d, 0));
    int8_t level = static_cast<int8_t>(d[4]);
    CObj *obj = FindObject(serial);
    if (obj != nullptr)
        reinterpret_cast<CDynamicObj *>(obj)->SetLight(level);
}
HOOK_FUNC(0x0045c1d0, Packet_Handle0x4E);

// ---------------------------------------------------------------------------
// 0x0045c240  size 0x81
// Inbound packet 0x5B: game time of day (hour, minute, second bytes).
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x5B(const uint8_t *packet)
{
    const uint8_t *d = packet + pkt::Base(packet);
    GLOBAL_AS(uint32_t, g_gameHour) = d[0];
    GLOBAL_AS(uint32_t, g_gameMinute) = d[1];
    GLOBAL_AS(uint32_t, g_gameSecond) = d[2];
}
HOOK_FUNC(0x0045c240, Packet_Handle0x5B);

// ---------------------------------------------------------------------------
// 0x0045c2d0  size 0x21c
// Inbound packet 0x53: login error. Shows a message gump for the reason code (jump table of 8 entries,
// codes 3 and 4 and above 7 share the default text).
// ---------------------------------------------------------------------------
static void LoginError_Show(const char *text, CGump *gump, int p3, int p4)
{
    CreateGump2ac(static_cast<int>(reinterpret_cast<uintptr_t>(text)), static_cast<int>(reinterpret_cast<uintptr_t>(gump)), p3, p4);   // 0x00439f60
}

// The original calls 0x00439f60 seven times (0x00439f60 0x00439f60 0x00439f60 0x00439f60 0x00439f60), once per
// branch; each goes through LoginError_Show.
void __stdcall Packet_Handle0x53(const uint8_t *packet)
{
    char buf[0x100];
    uint8_t code = packet[pkt::Base(packet)];

    if (g_loginObj648914 != nullptr) {
        decomp::vdelete(g_loginObj648914);
        g_loginObj648914 = nullptr;
    }
    if (g_waitingGump != nullptr) {
        decomp::vdelete(g_waitingGump);
        g_waitingGump = nullptr;
    }

    switch (code) {
    case 7:
        LoginError_Show("You have been idle for too long.  If you do not do anything in the next minute, you will be logged out.",
                        g_prevGump, 1, 0);
        return;
    case 2:
        sprintf(buf, "This character already exists.\nPlaying...");           // 0x0048b1d0
        LoginError_Show(buf, g_prevGump, 1, 0);
        return;
    case 0:
        if (g_prevGump != nullptr) {
            sprintf(buf, "Incorrect password.");
            LoginError_Show(buf, g_prevGump, 1, 0);
            return;
        }
        sprintf(buf, "You entered an incorrect password.  Closing...");
        LoginError_Show(buf, g_prevGump, 1, 1);
        return;
    case 1:
        if (g_prevGump != nullptr) {
            sprintf(buf, "This character does not exist anymore.  You will have to recreate it.");
            static_cast<CLoginGump *>(g_prevGump)->RemoveEntry(g_selectedCharSlot);   // 0x00421540
            g_selectedCharSlot = g_selectedCharSlot - 1;
            if (g_selectedCharSlot < 0)
                g_selectedCharSlot = 0;
            LoginError_Show(buf, g_prevGump, 1, 0);
            return;
        }
        sprintf(buf, "Character does not exist.\n Closing...");
        ShowMessageBoxAndExit(buf, "Access Denied!", 0);   // fatal box
        exit(0);                                                               // 0x0048b610
        return;
    case 5:
        LoginError_Show("Another character from this account is currently online in this world.  You must either log in as that "
                        "character or wait for it to time out.",
                        g_prevGump, 1, 0);
        return;
    case 6:
        LoginError_Show("An error has occurred in the synchronization between the login servers and this world.  Please close "
                        "your client and try again.",
                        g_prevGump, 1, 0);
        return;
    default:                                                                         // 3, 4 and above 7
        LoginError_Show("The client could not attach to the game server. It must have been taken down, please wait a few "
                        "minutes and try again.",
                        g_prevGump != nullptr ? g_prevGump : g_dlgParent, 1, 0);
        return;
    }
}
HOOK_FUNC(0x0045c2d0, Packet_Handle0x53);

// ---------------------------------------------------------------------------
// 0x0045c550  size 0x5d
// Inbound packet 0x55: login complete. The packet is not read.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x55(const uint8_t *packet)
{
    (void)packet;
    CGump *pending = g_loginObj648914;
    if (pending == nullptr) {
        GLOBAL_AS(CApp, g_app_storage).LoadKeynamesAndMacros();                         // 0x0044d780
        g_screenGump->visible = 1;
        g_inputActive = 1;
        GLOBAL_AS(CTextEntry *, g_textEntry)->RefreshLine();                          // 0x00443590
        g_gumpMgr.DeleteTransientGumps();                                           // 0x0043c670
        g_gumpMgr.LoadDesktop();                                                    // 0x0043c6c0
        return;
    }
    RAWF(uint32_t, RAWF(void *, pending, 0x44), 0xc4) = PORT_ADDR(&U1_LoginCompleteCallback);
}
HOOK_FUNC(0x0045c550, Packet_Handle0x55);

// ---------------------------------------------------------------------------
// 0x0045c5b0  size 0x1be
// Inbound packet 0x56: map plot command for the map item with the given serial (its gump is at item+0xb0).
// Commands 1..5 add / insert / change a pin, 6 is ignored, 7 toggles the plotting mode, 8 is "edit denied".
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x56(const uint8_t *packet)
{
    const uint8_t *d = packet + pkt::Base(packet);
    uint32_t serial = ntohl(rd32(d, 0));
    uint8_t cmd = d[4];
    uint8_t flag = d[5];
    uint32_t x = ntohs(rd16(d, 6));
    uint32_t y = ntohs(rd16(d, 8));
    x += 0x18;
    y += 0x1f;

    CObj *obj = FindObject(serial);
    if (obj == nullptr)
        return;
    if (decomp::vcall(PORT_RET(int), obj, 0x60) == 0)
        return;
    CGump *gump = RAWF(CGump *, obj, 0xb0);
    if (gump == nullptr)
        return;

    switch (cmd) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        reinterpret_cast<CObjVt49f960 *>(obj)->ListOp(cmd, flag, x, y);
        static_cast<CCourseGump *>(gump)->Redraw();                                 // 0x00428a90
        return;
    case 7: {
        int toggle = (flag == 1) ? 1 : 0;
        RAWF(int32_t, gump, 0xcc) = toggle;
        void *child = RAWF(void *, gump, 0x10c);
        RAWF(int32_t, child, 0x10) = toggle;
        int w = 0;
        int h = 0;
        void *child2 = RAWF(void *, gump, 0x10c);
        int art = (flag != 0) ? RAWF(int32_t, child2, 0xa0) : RAWF(int32_t, child2, 0x50);
        GumpArtSize(art, &w, &h);                                                   // 0x00433e70
        int halfGump = RAWF(int32_t, gump, 0x1c) / 2;
        int halfArt = w / 2;
        RAWF(int32_t, RAWF(void *, gump, 0x10c), 0x2c) = halfGump - halfArt;
        RAWF(int32_t, RAWF(void *, gump, 0x108), 0x68) = toggle;
        return;
    }
    case 8:
        Journal_Say("Request to edit denied.");                                     // 0x00479480
        return;
    default:                                                                         // 6 and out of range
        return;
    }
}
HOOK_FUNC(0x0045c5b0, Packet_Handle0x56);

// ---------------------------------------------------------------------------
// 0x0045c790  size 0x1db
// Inbound packet 0x54: play sound effect. mode 0: set the ambient sound; otherwise play sound `id` at (x, y, z),
// either through the effect sequence (when one is being built) or directly at full volume.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x54(const uint8_t *packet)
{
    Pos3 pos;
    uint8_t mode = packet[pkt::Base(packet)];
    uint16_t id = ntohs(*reinterpret_cast<const uint16_t *>(packet + pkt::Base(packet) + 1));
    ntohs(*reinterpret_cast<const uint16_t *>(packet + pkt::Base(packet) + 3));                                // result unused in the original
    pos.x = *reinterpret_cast<const uint16_t *>(packet + pkt::Base(packet) + 5);
    pos.x = ntohs(pos.x);
    pos.y = *reinterpret_cast<const uint16_t *>(packet + pkt::Base(packet) + 7);
    pos.y = ntohs(pos.y);
    pos.z = *reinterpret_cast<const uint16_t *>(packet + pkt::Base(packet) + 9);
    pos.z = ntohs(pos.z);

    if (mode == 0) {
        (&g_soundManager)->CSoundManager::SetAmbientSound(id);             // SetAmbientSound
        return;
    }
    if (id > 0x24b)
        return;
    if (g_sequencing != 0) {
        uint8_t *rec = static_cast<uint8_t *>(::operator new(8));
        *reinterpret_cast<uint16_t *>(rec) = id;
        *reinterpret_cast<Pos3 *>(rec + 2) = pos;
        g_effectSequence->Append(8, rec, 8);                                        // 0x0043e020
        operator delete(rec);
        return;
    }
    // PlaySound3D(id, &pos, 1.0f, 0, 0, 0)
    PORT_THISCALL6(&PlaySound3D, void, &g_soundManager, uint32_t, uint16_t *, uint32_t, int, int, int)(id, reinterpret_cast<uint16_t *>(&pos), 0x3f800000u, 0, 0, 0);
}
HOOK_FUNC(0x0045c790, Packet_Handle0x54);

// ---------------------------------------------------------------------------
// 0x0045c970  size 0x7f
// Inbound packet 0x65: weather (type, count, signed temperature).
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x65(const uint8_t *packet)
{
    const uint8_t *d = packet + pkt::Base(packet);
    uint32_t type = d[0];
    uint32_t count = d[1];
    int temperature = static_cast<int8_t>(d[2]);
    g_weatherObj.HandlePacket(static_cast<int>(type), static_cast<int>(count), temperature);   // 0x0047d610
}
HOOK_FUNC(0x0045c970, Packet_Handle0x65);

// ---------------------------------------------------------------------------
// 0x0045d610  size 0x1a
// Current write offset of an outbound packet buffer (ECX): the word at +1 for a variable-length opcode,
// else the global fixed-length cursor. The result is zero-extended in EAX.
// ---------------------------------------------------------------------------
uint32_t __fastcall Packet_GetWriteOffset(const uint8_t *buf)
{
    return pkt::Cursor(buf);
}
HOOK_FUNC(0x0045d610, Packet_GetWriteOffset);

// ---------------------------------------------------------------------------
// 0x0045d900  size 0x9c
// Inbound packet 0x6C: target cursor request (type byte, id, flags). Installs the cursor and the callback
// 0x0045d630 that answers it.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x6C(const uint8_t *packet)
{
    const uint8_t *d = packet + pkt::Base(packet);
    uint8_t type = d[0];
    uint32_t id = ntohl(rd32(d, 1));
    uint8_t flags = d[5];
    App_SetCursor13();                                         // set the target mouse cursor
    g_targetCursorId = id;
    g_targetFlags = flags;
    GLOBAL_AS(uint32_t, g_targetCursorType) = type;
    g_targetCallback = PORT_ADDR(&CTargetObj::SendTargetResponse);                                            // CTargetObj::SendTargetResponse
}
HOOK_FUNC(0x0045d900, Packet_Handle0x6C);
