// Application support code, second pass (batch 18): line reader and string scrambler, install path lookup,
// out-of-memory handler, cursor flags, the obscene / ignore lists, the periodic game tick, key dispatch,
// simple macro commands and macros.txt saving / parsing.
// The SEH frames of the originals are omitted.
#include "app.h"

#include <windows.h>

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"
#include "gump_manager.h"
#include "macro.h"
#include "obj.h"
#include "crt.h"
#include "raw_field.h"
#include "target_cursor.h"
#include "text_entry.h"
#include "vcall.h"
#include "walk.h"
#include "client_socket.h"
#include "window_helpers_31_ports.h"
#include "mouse_manager.h"
#include "map_gump_ops_ports.h"
#include "music_manager.h"
#include "sound_manager.h"
#include "object_manager.h"
#include "weather.h"
#include "screen_flow_ports.h"
#include "class_0049c690.h"
#include "packet_builders_b25_ports.h"
#include "packet_builders_batch24_ports.h"
#include "bb_msg_gump_ports.h"


#define as_builder GLOBAL_PTR(void, g_packetBuilder_storage)               // packet object: this for the Build_Packet calls
#define as_mouseManager GLOBAL_PTR(void, g_mouse_storage)
#define as_textEntry GLOBAL_AS(CTextEntry *, g_textEntry)
#define as_separator " "                                    // 0x004a60ec

// Send a built packet with the connection object.
static inline void as_send(char *pkt)
{
    reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(pkt));
}

// 0x0044ad10  size 0x72  ECX = FILE *
// Read one line into the line buffer 0x0068da30: stops at '\n' / '\r', swallows the other half of a CR LF pair,
// NUL terminates. The EOF flag is bit 0x10 of the original FILE's flag word.
char *__fastcall App_ReadLineFromFile(FILE *f)
{
    int c = 0;
    char *out = g_lineBuf;
    while (!feof(f)) {
        c = fgetc(f);
        if (c == 0xa || c == 0xd)
            break;
        if (c != -1)
            *out++ = static_cast<char>(c);
    }
    if (!feof(f) && (c == 0xa || c == 0xd)) {
        int c2 = fgetc(f);
        if (c2 == c || (c2 != 0xa && c2 != 0xd))
            ungetc(c2, f);
    }
    *out = 0;
    return g_lineBuf;
}
HOOK_FUNC(0x0044ad10, App_ReadLineFromFile);

// 0x0044ad90  size 0x30  ECX = text
// Scramble in place: every byte + 0x0d, wrapped back by 0x5f when it passes 0x7f.
void __fastcall App_ObfuscateString(char *text)
{
    int len = static_cast<int>(strlen(text));
    for (int i = 0; i < len; i++) {
        uint8_t b = static_cast<uint8_t>(text[i] + 0x0d);
        text[i] = static_cast<char>(b);
        if (b > 0x7f)
            text[i] = static_cast<char>(b - 0x5f);
    }
}
HOOK_FUNC(0x0044ad90, App_ObfuscateString);

// 0x0044adc0  size 0x30  ECX = text
// Inverse of the scrambler: every byte - 0x0d, wrapped up by 0x5f when it falls below 0x20.
void __fastcall App_DeobfuscateString(char *text)
{
    int len = static_cast<int>(strlen(text));
    for (int i = 0; i < len; i++) {
        uint8_t b = static_cast<uint8_t>(text[i] + 0xf3);
        text[i] = static_cast<char>(b);
        if (b < 0x20)
            text[i] = static_cast<char>(b + 0x5f);
    }
}
HOOK_FUNC(0x0044adc0, App_DeobfuscateString);

// 0x0044b130  size 0xcb  ECX = out buffer, EDX = its size
// Read HKLM\SOFTWARE\Origin Worlds Online\Ultima Online\1.0 "InstCDPath" into `out`. The buffer is set to ""
// first in every case. Returns `out`, or null on failure. The key is not closed.
char *__fastcall App_GetInstCDPath(char *out, int outSize)
{
    HKEY key;
    BYTE data[0x400];
    DWORD cb;
    LONG st = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Origin Worlds Online\\Ultima Online\\1.0", 0, 1, &key);
    strcpy(out, g_emptyString);
    if (st != 0)
        return nullptr;
    cb = 0x400;
    if (RegQueryValueExA(key, "InstCDPath", nullptr, nullptr, data, &cb) != 0)
        return nullptr;
    if (outSize < static_cast<int>(cb))
        return nullptr;
    strcpy(out, reinterpret_cast<char *>(data));
    return out;
}
HOOK_FUNC(0x0044b130, App_GetInstCDPath);

// 0x0044b200  size 0x3c
// The new-handler: leave full screen, tell the user and exit(1).
int __cdecl App_OutOfMemoryHandler(size_t)
{
    if (dop_windowedFlag != 0 && g_bFullscreen != 0)
        SwitchToWindowedMode();   // SwitchToWindowedMode
    MessageBoxA(nullptr, g_004a9e80, "Out of memory", 0);
    exit(1);
    return 0;
}
HOOK_FUNC(0x0044b200, App_OutOfMemoryHandler);

// 0x0044b240  size 0x17
// Flag 0x006f4774 = 1 and mouse cursor 0x13.
void __cdecl App_SetCursor13()
{
    g_targetCallbackActive = 1;
    reinterpret_cast<CMouseManager *>(as_mouseManager)->SetMouseCursor(0x13);   // CMouseManager::SetMouseCursor
}
HOOK_FUNC(0x0044b240, App_SetCursor13);

// 0x0044b260  size 0x17
// Flag 0x006f4774 = 0 and mouse cursor 0x11.
void __cdecl App_SetCursor11()
{
    g_targetCallbackActive = 0;
    reinterpret_cast<CMouseManager *>(as_mouseManager)->SetMouseCursor(0x11);   // CMouseManager::SetMouseCursor
}
HOOK_FUNC(0x0044b260, App_SetCursor11);

// 0x0044b620  size 0x21  vtable slot 0 of CApp
void *__thiscall CApp::ScalarDeletingDtor(int flags)
{
    Dtor();
    if ((flags & 1) != 0)
        operator delete(this);
    return this;
}
HOOK_METHOD(0x0044b620, CApp, ScalarDeletingDtor);

// 0x0044b650  size 0xf  ECX = n
// rand() % n with the original CRT's generator.
int __fastcall App_RandomMod(int n)
{
    return rand() % n;
}
HOOK_FUNC(0x0044b650, App_RandomMod);

// 0x0044b6d0  size 0x672
// Load obscene.lst (scrambled words, one per line; written from the default table at 0x004ad238 when missing)
// into the word array at +0x818 (count at +0x56c) and ignore.lst (30-byte name + dword records) into
// +0x570 / +0x7c8 (count at +0x568). The data path is built the way ResolveDataPath does it. The original
// has the line reader inlined (fgetc 0x0048c540 in the read loop, fgetc 0x0048c540 again for the CR LF partner; ungetc 0x0048c480); App_ReadLineFromFile is that loop.
void __thiscall CApp::LoadObsceneAndIgnoreLists()
{
    char word[0x100] = {0};   // original: 0x28 bytes of locals
    FILE *f = fopen(ResolveDataPath(const_cast<char *>("obscene.lst")), "rt");
    if (f == nullptr) {
        FILE *w = fopen(ResolveDataPath(const_cast<char *>("obscene.lst")), "wt");
        char **defaults = g_004ad238;
        for (char **p = defaults; *p != nullptr; p++)
            fprintf(w, "%s\n", *p);
        fclose(w);
        f = fopen(ResolveDataPath(const_cast<char *>("obscene.lst")), "rt");
    }
    RAWF(int, this, 0x56c) = 0;
    while (!feof(f)) {
        const char *line = App_ReadLineFromFile(f);
        if (strlen(line) != 0) {
            int n = RAWF(int, this, 0x56c) + 1;
            RAWF(int, this, 0x56c) = n;
            strcpy(word, line);
            App_DeobfuscateString(word);
            strcpy(&RAWF(char, this, 0x7fa + 30 * n), word);
        }
    }
    fclose(f);

    f = fopen(ResolveDataPath(const_cast<char *>("ignore.lst")), "rb");
    if (f == nullptr) {
        FILE *created = fopen(ResolveDataPath(const_cast<char *>("ignore.lst")), "wb");
        fclose(created);
        f = fopen(ResolveDataPath(const_cast<char *>("ignore.lst")), "rb");
    }
    RAWF(int, this, 0x568) = 0;
    while (fread(word, 0x1e, 1, f) != 0) {
        int n = RAWF(int, this, 0x568) + 1;
        RAWF(int, this, 0x568) = n;
        strcpy(&RAWF(char, this, 0x552 + 30 * n), word);
        fread(&RAWF(char, this, 0x7c4 + 4 * n), 4, 1, f);
    }
    fclose(f);
}
HOOK_METHOD(0x0044b6d0, CApp, LoadObsceneAndIgnoreLists);

// 0x0044bd50  size 0x2ab  RET 4
// which == 0: write obscene.lst (words re-scrambled, one per line); which == 1: write ignore.lst (the 30-byte
// name and the dword of every record). Any other value does nothing.
void __thiscall CApp::SaveObsceneOrIgnoreList(int which)
{
    char word[0x100];   // original: 0x30 bytes
    if (which == 0) {
        FILE *f = fopen(ResolveDataPath(const_cast<char *>("obscene.lst")), "wt");
        if (f == nullptr)
            return;
        for (int i = 0; i < RAWF(int, this, 0x56c); i++) {
            strcpy(word, &RAWF(char, this, 0x818 + 30 * i));
            App_ObfuscateString(word);
            fprintf(f, "%s\n", word);
        }
        fclose(f);
    } else if (which == 1) {
        FILE *f = fopen(ResolveDataPath(const_cast<char *>("ignore.lst")), "wb");
        if (f == nullptr)
            return;
        for (int i = 0; i < RAWF(int, this, 0x568); i++) {
            fwrite(&RAWF(char, this, 0x570 + 30 * i), 0x1e, 1, f);
            fwrite(&RAWF(char, this, 0x7c8 + 4 * i), 4, 1, f);
        }
        fclose(f);
    }
}
HOOK_METHOD(0x0044bd50, CApp, SaveObsceneOrIgnoreList);

// 0x0044c000  size 0x2d  ECX = value
// Is `value` one of the first 0x0069e650 dwords of the array at 0x0069e8b0?
int __fastcall App_IsInList_0069e8b0(int value)
{
    int count = g_ccSel650;
    int *list = g_ccVals8b0;
    for (int i = 0; i < count; i++) {
        if (value == list[i])
            return 1;
    }
    return 0;
}
HOOK_FUNC(0x0044c000, App_IsInList_0069e8b0);

// 0x0044c180  size 0x173  RET 4
// Periodic game tick, at most every 0x4c ms: advances the game clock once a second (a game minute is 20 ticks),
// then updates path following, music, sound, mouse, the walk queue, objects, weather and gumps, and sends a
// ping (0x73) every 0x3c time units while the connection is active. flag != 0 skips the death fade.
void __stdcall App_GameTick(int flag)
{
    uint32_t now = GetTickCount();
    g_gameTick = now;
    g_combatTimeA = static_cast<int>(_time32(nullptr));
    if (now - g_0068dd94 < 0x4c)
        return;
    uint32_t sinceSecond = now - g_fpsSecondTick;
    int secondPassed = 0;
    g_0068dd94 = now;
    if (sinceSecond >= 0x3e8) {
        g_fpsSecondTick = now;
        int sec = g_gameSecond;
        sec++;
        g_gameSecond = sec;
        if (sec >= 0x14) {
            int minute = g_gameMinute;
            g_gameSecond = 0;
            minute++;
            g_gameMinute = minute;
            if (minute >= 0x3c) {
                int hour = g_gameHour;
                g_gameMinute = 0;
                hour++;
                g_gameHour = hour;
                if (hour >= 0x18)
                    g_gameHour = 0;
            }
        }
        secondPassed = 1;
    }
    Pathfind_NextStep();                              // Pathfind_NextStep
    reinterpret_cast<MusicManager *>(GLOBAL_PTR(void, g_musicManager_storage))->Tick();         // music manager update
    reinterpret_cast<CSoundManager *>(GLOBAL_PTR(void, g_soundManager_storage))->Update();         // sound manager update
    reinterpret_cast<CMouseManager *>(as_mouseManager)->UpdateHover();
    g_walk.ProcessStep();                                                // 0x00449490
    reinterpret_cast<CObjectManager *>(GLOBAL_PTR(void, g_objMgr_storage))->UpdateMovement();         // object manager UpdateMovement
    if (secondPassed == 1)
        reinterpret_cast<CObjectManager *>(GLOBAL_PTR(void, g_objMgr_storage))->AgeLabels();
    PORT_THISCALL0(&CWeather::Update, void, GLOBAL_PTR(void, g_weatherObj_storage))();         // weather update
    reinterpret_cast<CGumpManager *>(GLOBAL_PTR(void, g_gumpMgr_storage))->Update();         // gump manager update
    if (flag == 0)
        PORT_THISCALL0(&DeathFadeStep, void, GLOBAL_PTR(void, g_clientState_storage))();     // DeathFadeStep
    reinterpret_cast<CWorldObj *>(GLOBAL_PTR(void, g_clientState_storage))->CWorldObj::ToVoid();
    if (g_flag6f479c == 0 && g_useHuffman == 0) {
        g_006f47a4 = g_combatTimeA;
        return;
    }
    int t = g_combatTimeA;
    if (t - g_006f47a4 > 0x3c) {
        char *pkt = reinterpret_cast<char *>(&flag);   // the original builds the ping in the argument slot
        g_006f47a4 = t;
        PORT_THISCALL2(&Pkt_Build_0x73_Ping, void, as_builder, char *, int)(pkt, 0);   // Pkt_Build_0x73_Ping
        as_send(pkt);
    }
}
HOOK_FUNC(0x0044c180, App_GameTick);

// 0x0044c300  size 0x39
// Send packet 0x12 with subtype 0x43 and an empty string.
void __cdecl App_SendAction43()
{
    char pkt[0xf8];
    PORT_THISCALL4(&Build_Packet_0x12, void, as_builder, char *, int, const char *, int)(pkt, 0x43, g_emptyString, 0);   // Build_Packet_0x12
    as_send(pkt);
}
HOOK_FUNC(0x0044c300, App_SendAction43);

// 0x0044c4a0  size 0x142  RET 4
// Key dispatch. Keys -8..-1 are the eight walk directions (RequestWalkKey with flags 0); 9 (Tab) and, when
// nothing else takes it, 27 (Esc) go to the focused input control; everything else is typed text.
void __stdcall App_HandleKey(int key)
{
    switch (key) {
    case -8: g_walk.RequestWalkKey(3, 0); return;
    case -7: g_walk.RequestWalkKey(4, 0); return;
    case -6: g_walk.RequestWalkKey(5, 0); return;
    case -5: g_walk.RequestWalkKey(2, 0); return;
    case -4: g_walk.RequestWalkKey(6, 0); return;
    case -3: g_walk.RequestWalkKey(1, 0); return;
    case -2: g_walk.RequestWalkKey(0, 0); return;
    case -1: g_walk.RequestWalkKey(7, 0); return;
    case 9:
        if (g_focusedInput != nullptr)
            decomp::vcall_void(g_focusedInput, 0x3c, key);
        return;
    case 27:
        if (g_promptReply != 0) {
            as_textEntry->SendLine(0, reinterpret_cast<int>(g_emptyString));
            return;
        }
        if (g_targetCallbackActive != 0) {
            TargetCursor_SendCancel();
            return;
        }
        if (g_focusedInput != nullptr) {
            decomp::vcall_void(g_focusedInput, 0x3c, key);
            return;
        }
        g_gumpMgr.DeleteGumpsSlot80();
        return;
    default:
        if (g_focusedInput != nullptr) {
            decomp::vcall_void(g_focusedInput, 0x3c, key);
            return;
        }
        as_textEntry->HandleChar(key);
        return;
    }
}
HOOK_FUNC(0x0044c4a0, App_HandleKey);

// 0x0044c640  size 0xf0  ECX = kind (0..8)
// Walk the gump list from its tail (previous at +0x3c) and return the first gump that answers the virtual
// question picked by `kind` (slots 0xc0, 0x78, 0xac, 0x94, 0xb0, 0xd0, 0xe8, 0x90, 0x10c). Kinds 1, 2 also need
// the gump's owner to be the player, kind 7 the owner to be the player's item in slot +0x118. Null if none.
void *__fastcall Gump_FindByKind(int kind)
{
    void *g = GLOBAL_AS(void *, g_gumpListTail);
    while (g != nullptr) {
        if (static_cast<unsigned>(kind) <= 8) {
            switch (kind) {
            case 0:
                if (decomp::vcall(PORT_RET(int), g, 0xc0) != 0)
                    return g;
                break;
            case 1:
                if (decomp::vcall(PORT_RET(int), g, 0x78) != 0 && RAWF(void *, g, 0x34) == g_player)
                    return g;
                break;
            case 2:
                if (decomp::vcall(PORT_RET(int), g, 0xac) != 0 && RAWF(void *, g, 0x34) == g_player)
                    return g;
                break;
            case 3:
                if (decomp::vcall(PORT_RET(int), g, 0x94) != 0)
                    return g;
                break;
            case 4:
                if (decomp::vcall(PORT_RET(int), g, 0xb0) != 0)
                    return g;
                break;
            case 5:
                if (decomp::vcall(PORT_RET(int), g, 0xd0) != 0)
                    return g;
                break;
            case 6:
                if (decomp::vcall(PORT_RET(int), g, 0xe8) != 0)
                    return g;
                break;
            case 7:
                if (decomp::vcall(PORT_RET(int), g, 0x90) != 0 && RAWF(void *, g, 0x34) == RAWF(void *, g_player, 0x118))
                    return g;
                break;
            case 8:
                if (decomp::vcall(PORT_RET(int), g, 0x10c) != 0)
                    return g;
                break;
            }
        }
        g = RAWF(void *, g, 0x3c);
    }
    return nullptr;
}
HOOK_FUNC(0x0044c640, Gump_FindByKind);

// 0x0044c760  size 0x152  ECX = command (0..8)
// The built-in macro commands: open gumps (0x1388, status 8, 7, 0x1392), double-click the player or the item
// in the player's slot +0x118, request status (packet 0x34 subtypes 4 / 5) and packet 0x12 subtype 0x43.
void __fastcall App_RunSimpleMacro(int command)
{
    char dblPkt[8];
    char statPkt[0x10];
    char pkt[0xf8];
    uint32_t serial;
    switch (command) {
    case 0:
        g_gumpMgr.CreateGump(nullptr, 0x1388, nullptr, 0);
        return;
    case 1:
        serial = RAWF(uint32_t, g_player, 0x78) | 0x80000000;
        PORT_THISCALL2(&Build_Packet_0x06, void, as_builder, char *, uint32_t)(dblPkt, serial);   // Build_Packet_0x06_DoubleClick
        as_send(dblPkt);
        return;
    case 2:
        g_gumpMgr.CreateGump(g_player, 8, nullptr, 0);
        serial = RAWF(uint32_t, g_player, 0x78);
        PORT_THISCALL3(&Build_Packet_0x34, void, as_builder, char *, int, uint32_t)(statPkt, 4, serial);   // Build_Packet_0x34
        as_send(statPkt);
        return;
    case 3:
        g_gumpMgr.CreateGump(nullptr, 7, nullptr, 0);
        return;
    case 4:
        serial = RAWF(uint32_t, g_player, 0x78);
        PORT_THISCALL3(&Build_Packet_0x34, void, as_builder, char *, int, uint32_t)(statPkt, 5, serial);   // Build_Packet_0x34
        as_send(statPkt);
        g_desktopChanged = 1;
        return;
    case 5:
        PORT_THISCALL4(&Build_Packet_0x12, void, as_builder, char *, int, const char *, int)(pkt, 0x43, g_emptyString, 0);   // Build_Packet_0x12
        as_send(pkt);
        return;
    case 7: {
        void *item = RAWF(void *, g_player, 0x118);
        if (item == nullptr)
            return;
        serial = RAWF(uint32_t, item, 0x78);
        PORT_THISCALL2(&Build_Packet_0x06, void, as_builder, char *, uint32_t)(dblPkt, serial);   // Build_Packet_0x06_DoubleClick
        as_send(dblPkt);
        return;
    }
    case 8:
        g_gumpMgr.CreateGump(nullptr, 0x1392, nullptr, 0);
        return;
    default:
        return;
    }
}
HOOK_FUNC(0x0044c760, App_RunSimpleMacro);

// 0x0044ce90  size 0xda  ECX = out, the 20-byte action by value on the stack, RET 0x14
// Format a macro action as text for macros.txt: "<kind name> <argument>"; the argument is the stored text
// (types 1-4), a name from one of the tables (types 5, 8-11), a skill name (13) or a spell name (15).
static void __fastcall Macro_FormatAction(char *out, MacroAction action)
{
    const char *arg = nullptr;
    strcpy(out, g_macroCmdNames[action.type]);
    strcat(out, as_separator);
    switch (action.type - 1) {
    case 0:
    case 1:
    case 2:
    case 3:
        arg = action.text;
        break;
    case 4:
        arg = g_004a6ac8[action.arg];
        break;
    case 7:
    case 8:
    case 9:
    case 10:
        arg = g_macroArgNames8[action.arg];
        break;
    case 12:
        arg = g_skillNames + action.arg * 0x50;   // g_skillNames, 0x50 bytes each
        break;
    case 14:
        arg = StripParenSuffix(
            reinterpret_cast<const char *>(g_spellNames[action.arg * 2]));   // StripParenSuffix
        break;
    default:
        return;
    }
    strcat(out, arg);
}
HOOK_FUNC(0x0044ce90, Macro_FormatAction);

// 0x0044cfa0  size 0x194
// Write macros.txt: for every macro "<key name> <shift> <alt> <ctrl>", one line per action, then "########".
void __thiscall CApp::SaveMacros()
{
    char keyName[0x20];
    char line[0x100];   // original: 0x80 bytes
    MacroRecord *m = RAWF(MacroRecord *, this, 0x1774);
    FILE *f = fopen(ResolveDataPath(const_cast<char *>("macros.txt")), "wt");
    while (m != nullptr) {
        GetKeyNameTextA(m->scan & 0xbfffffff, keyName, 0x14);
        fprintf(f, "%s %d %d %d\n", keyName, m->shift, m->alt, m->ctrl);
        for (int i = 0; i < m->count; i++) {
            Macro_FormatAction(line, m->actions[i]);
            fprintf(f, "%s\n", line);
        }
        fprintf(f, "########\n");
        m = m->next;
    }
    fclose(f);
}
HOOK_METHOD(0x0044cfa0, CApp, SaveMacros);

// 0x0044d140  size 0x523  ECX = line, EDX = out action
// Parse one macros.txt action line: the first word is looked up in the kind table (1-based index into
// 0x004af80c, count at 0x004af804) and gives out->type; the rest of the line is the argument: free text
// (types 1-4, copied to new memory), or a name matched against a table (types 5, 8-11, 13, 15), whose last
// matching index becomes out->arg.
void __fastcall Macro_ParseAction(char *line, MacroAction *out)
{
    char text[0x80];
    out->type = 0;
    out->arg = 0;
    out->text = nullptr;
    out->unk_0c = 0;
    out->unk_10 = 0;
    char *tok = strtok(line, as_separator);
    int kinds = g_macroCmdCount;
    for (int i = 1; i <= kinds; i++) {
        if (strcmp(tok, g_004af80c[i - 1]) == 0) {
            out->type = i;
            break;
        }
    }
    if (out->type == 0)
        return;
    switch (out->type - 1) {
    case 0:
    case 1:
    case 2:
    case 3:
        tok = strtok(nullptr, as_separator);
        strcpy(text, g_emptyString);
        while (tok != nullptr) {
            if (strlen(text) != 0)
                strcat(text, as_separator);
            strcat(text, tok);
            tok = strtok(nullptr, as_separator);
        }
        out->text = static_cast<char *>(::operator new(strlen(text) + 1));
        strcpy(out->text, text);
        return;
    case 4: {
        tok = strtok(nullptr, as_separator);
        int idx = 0;
        for (const char *const *p = g_004a6ac8; p < g_004a6ae8; p++, idx++) {
            if (strcmp(tok, *p) == 0)
                out->arg = idx;
        }
        return;
    }
    case 7:
    case 8:
    case 9:
    case 10: {
        tok = strtok(nullptr, as_separator);
        const char *const *table = g_macroArgNames8;
        if (table[0] == nullptr)
            return;
        for (int idx = 0; table[idx] != nullptr; idx++) {
            if (strcmp(tok, table[idx]) == 0)
                out->arg = idx;
        }
        return;
    }
    case 12:
        tok = strtok(nullptr, as_separator);
        strcpy(text, g_emptyString);
        while (tok != nullptr) {
            if (strlen(text) != 0)
                strcat(text, as_separator);
            strcat(text, tok);
            tok = strtok(nullptr, as_separator);
        }
        if (g_skillCount <= 0)   // g_SkillCount
            return;
        for (int idx = 0; idx < g_skillCount; idx++) {
            if (strcmp(text, g_skillNames + idx * 0x50) == 0)
                out->arg = idx;
        }
        return;
    case 14: {
        tok = strtok(nullptr, as_separator);
        strcpy(text, g_emptyString);
        while (tok != nullptr) {
            if (strlen(text) != 0)
                strcat(text, as_separator);
            strcat(text, tok);
            tok = strtok(nullptr, as_separator);
        }
        const char **spells = g_spellNames;   // 8-byte records, the first dword is the name
        if (strlen(spells[0]) == 0)
            return;
        int idx = 0;
        do {
            const char *name = StripParenSuffix(spells[idx * 2]);   // StripParenSuffix
            if (strcmp(text, name) == 0)
                out->arg = idx;
            idx++;
        } while (strlen(spells[idx * 2]) != 0);
        return;
    }
    default:
        return;
    }
}
HOOK_FUNC(0x0044d140, Macro_ParseAction);
