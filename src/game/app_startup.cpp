// CApp: constructor, destructor, player-mobile allocation, macros and keynames.
// decomp/pass2/0x0044b280.cpp, 0x0044b660.cpp, 0x0044c030.cpp, 0x0044c8e0.cpp, 0x0044d780.cpp.
//
// CApp is larger than the fields named in app.h; the other fields are reached by byte offset (APPF).
#include "app.h"

#include <windows.h>

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "../runtime/test_seams.h"
#include "globals.h"
#include "macro.h"
#include "obj.h"
#include "player.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "cstr.h"
#include "config_file_ports.h"
#include "packet_builders_batch24_ports.h"
#include "client_socket.h"
#include "client_misc_ops_ports.h"
#include "walk.h"
#include "app_session_ops_ports.h"
#include "window_helpers_31_ports.h"
#include "app_support_ports.h"
#include "book_gump.h"
#include "object_manager.h"
#include "object_manager_ops_ports.h"
#include "yes_no_gump.h"
#include "char_creation_events_ports.h"
#include "small_methods_u1_ports.h"
#include "vtables.h"
#include "class_0049c690.h"

#define APPF(type, off) RAWF(type, this, (off))

// Path and line buffers the original code shares.
#define g_critSect GLOBAL_PTR(void, g_soundLock_storage)
#define g_gumpBusy GLOBAL_AS(int, g_scrollChoiceGump)
#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)
#undef g_objMgr   // the class header's typed form; this file addresses the object by offset
#define g_objMgr GLOBAL_PTR(void, g_objMgr_storage)
#define g_walkMgr GLOBAL_PTR(void, g_walk_storage)
#define g_keynameCount 0x59

// Append the data directory (+ "\\" if needed) and `name` to g_pathBuf.
static void BuildDataPath(const CApp &app, const char *name)
{
    strcpy(g_resolvedPath, app.dataPath);
    if (g_resolvedPath[0] != 0) {
        if (g_resolvedPath[strlen(g_resolvedPath) - 1] != '\\')
            strcat(g_resolvedPath, g_pathSeparator);
    }
    strcat(g_resolvedPath, name);
}

// 0x0044b280  size 0x39b
// Client-app constructor. Refuses to start when another copy's window exists, reads ExePath from the
// registry (HKLM\SOFTWARE\Origin Worlds Online\Ultima Online\1.0), loads login.cfg / uo.cfg
// (0x004249b0), chardata.mul and the macros. The SEH frame is omitted.
CApp *__thiscall CApp::Ctor()
{
    HKEY key;
    DWORD cb;
    char *p;

    APPF(uint32_t, 0x408) = 0;
    APPF(uint32_t, 0x410) = 0;
    APPF(uint32_t, 0x40c) = 0;
    reinterpret_cast<CStr *>(&APPF(uint8_t, 0x414))->Ctor();   // member constructor
    APPF(uint32_t, 0x000) = reinterpret_cast<uintptr_t>(vtbl_CApp);                                          // vftable
    _set_new_handler(App_OutOfMemoryHandler);                // out-of-memory handler
    InitializeCriticalSection(static_cast<LPCRITICAL_SECTION>(g_critSect));
    // Test seam: UO_DECOMP_MULTI lets a test run start beside another client.
    if (FindWindowA(g_classNamePtr, nullptr) != nullptr && !::decomp::test_switch("UO_DECOMP_MULTI")) {
        MessageBoxA(nullptr, g_004aa048 /* "Another copy of UO is already running!" */, nullptr, 0x40);
        exit(0);
    }
    APPF(uint32_t, 0x177c) = 0;
    APPF(uint32_t, 0x434) = 0;
    APPF(uint8_t, 0x438) = 0;
    APPF(uint8_t, 0x18c) = 0;
    sprintf(&APPF(char, 0x1aa), g_004aa038, g_004aa03c);   // "%s", "UO Player"
    APPF(uint8_t, 0x548) = 0;
    APPF(uint8_t, 0x1940) = 0;
    APPF(uint8_t, 0x1904) = 0;
    APPF(uint8_t, 0x195e) = 0;
    APPF(uint32_t, 0x430) = 1;
    APPF(uint32_t, 0x408) = 0;
    APPF(uint32_t, 0x40c) = 0;
    APPF(uint32_t, 0x410) = 0;
    APPF(uint16_t, 0x3e8) = 0x3b2;
    APPF(uint16_t, 0x3ec) = 0x3b2;
    APPF(uint32_t, 0x424) = 0;
    APPF(uint32_t, 0x008) = 0;
    APPF(uint32_t, 0x1e4) = 0x32;
    APPF(uint16_t, 0x3ea) = 3;
    APPF(uint16_t, 0x3ee) = 3;
    APPF(uint32_t, 0x540) = 0;
    APPF(uint32_t, 0x544) = 1;
    APPF(uint32_t, 0x404) = 0;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, g_004a9e50, 0, 1, &key) != 0)
        goto fail;
    p = dataPath;   // +0x1e8: ExePath goes here
    cb = 0x104;
    if (RegQueryValueExA(key, g_004aa030 /* "ExePath" */, nullptr, nullptr, reinterpret_cast<LPBYTE>(p), &cb) != 0)
        goto fail;

    if (strchr(p, '\\') != nullptr) {
        do {
            p = strchr(p, '\\');
            p = p + 1;
        } while (strchr(p, '\\') != nullptr);
    }
    *p = 0;
    APPF(uint32_t, 0x1778) = 0;
    APPF(uint32_t, 0x1774) = 0;
    APPF(uint8_t, 0x17b8) = 0;
    APPF(uint32_t, 0x1900) = 0;
    (this->*PORT_METHOD1(CApp, void, &LoadConfigFile, int))(1);   // load uo.cfg
    (this->*PORT_METHOD1(CApp, void, &LoadConfigFile, int))(0);   // load login.cfg
    APPF(uint32_t, 0x3f0) = 0x02b202b2;
    APPF(uint32_t, 0x3f4) = 0x02b202b2;
    APPF(uint16_t, 0x3f8) = 0x02b2;
    APPF(uint32_t, 0x3fa) = 0x02250225;
    APPF(uint32_t, 0x3fe) = 0x02250225;
    APPF(uint16_t, 0x402) = 0x0225;

    BuildDataPath(g_app, "chardata.mul");
    {
        FILE *f = fopen(g_resolvedPath, g_fopenReadMode);
        if (f != nullptr) {
            fread(&APPF(uint8_t, 0x3f0), 2, 5, f);
            fread(&APPF(uint8_t, 0x3fa), 2, 5, f);
            fclose(f);
        }
    }
    LoadKeynamesAndMacros();
    APPF(uint32_t, 0x1788) = 0;
    APPF(uint32_t, 0x1784) = 0;
    APPF(uint32_t, 0x178c) = 0;
    APPF(uint32_t, 0x1790) = 0;
    APPF(uint32_t, 0x1794) = 0;
    APPF(uint32_t, 0x17a0) = 0;
    APPF(uint32_t, 0x179c) = 0;
    APPF(uint32_t, 0x1798) = 0;
    APPF(uint32_t, 0x17a4) = 0;
    APPF(uint32_t, 0x17a8) = 0;
    APPF(uint32_t, 0x17ac) = 0;
    APPF(uint32_t, 0x17b0) = 0;
    APPF(uint32_t, 0x17b4) = 0;
    APPF(uint32_t, 0x1780) = 4;
    APPF(uint8_t, 0x1e0) = 0;
    APPF(uint8_t, 0x1e1) = 0;
    return this;

fail:
    MessageBoxA(nullptr, g_004a9f8c, s_errorCaption /* "ERROR" */, 0x10);
    return this;
}
HOOK_METHOD(0x0044b280, CApp, Ctor);

// 0x0044b660  size 0x70
// Allocate the player mobile: operator new(0x20c), then the player constructor 0x00454c30 (ECX = block,
// stack (this+8, 0, 0)). The SEH frame is omitted.
void __thiscall CApp::AllocPlayerMobile()
{
    CPlayer *p = new CPlayerNew(APPF(uint32_t, 0x008), 0, 0);
    g_player = p;
}
HOOK_METHOD(0x0044b660, CApp, AllocPlayerMobile);

// 0x0044c030  size 0x14e
// CApp destructor body. The arrays at +0x1790 (count +0x178c) and +0x17b0 (count +0x17ac) are arrays of
// malloc'd pointers: each element is freed, then the array. The seven blocks at +0x1790..+0x17a8 are
// freed only when the count at +0x178c is non-zero. The SEH frame is omitted.
void __thiscall CApp::Dtor()
{
    int i;
    int n;

    APPF(uint32_t, 0x000) = reinterpret_cast<uintptr_t>(vtbl_CApp);   // vftable
    DeleteCriticalSection(static_cast<LPCRITICAL_SECTION>(g_critSect));
    operator delete(APPF(void *, 0x1774));

    n = APPF(int, 0x178c);
    i = 0;
    if (n > 0) {
        do {
            operator delete(APPF(void **, 0x1790)[i]);
            n = APPF(int, 0x178c);
            i++;
        } while (i < n);
    }
    if (n != 0) {
        operator delete(APPF(void *, 0x1790));
        operator delete(APPF(void *, 0x1794));
        operator delete(APPF(void *, 0x1798));
        operator delete(APPF(void *, 0x179c));
        operator delete(APPF(void *, 0x17a0));
        operator delete(APPF(void *, 0x17a4));
        operator delete(APPF(void *, 0x17a8));
    }

    n = APPF(int, 0x17ac);
    i = 0;
    if (n > 0) {
        do {
            operator delete(APPF(void **, 0x17b0)[i]);
            n = APPF(int, 0x17ac);
            i++;
        } while (i < n);
    }
    if (n != 0)
        operator delete(APPF(void *, 0x17b0));
    if (APPF(void *, 0x17b4) != nullptr)
        operator delete(APPF(void *, 0x17b4));
    reinterpret_cast<CStr *>(&APPF(uint8_t, 0x414))->Dtor();   // member destructor
}
HOOK_METHOD(0x0044c030, CApp, Dtor);

// Send a text command packet: build with 0x00463170 (ECX = packet builder; stack: buffer, id, text, length)
// and send with 0x00478610 (ECX = *(0x69e0ec)).
static void SendTextPacket(char *pkt, int id, const char *text)
{
    PORT_THISCALL4(&Build_Packet_0x12, void, g_packetBuilder, char *, int, const char *, size_t)(pkt, id, text, strlen(text));
    reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(pkt));
}

// 0x0044c8e0  size 0x54e
// Run the macro with index `macroIndex`: walk the list (next at +0xe0) and execute each action (type 1..21;
// switch table at 0x44ce30). Most actions are gated on DAT_005285c8 (7 is not). The sprintf results in
// cases 12/15/16 go to dead locals but the calls remain. Cases 18 and 19 send packet 0xc7 with the
// strings at 0x4aa098 ("bow") and 0x4aa090 ("salute"). The SEH frame is omitted.
void __thiscall CApp::RunMacro(int macroIndex)
{
    MacroRecord *macro = APPF(MacroRecord *, 0x1774);
    int i;
    MacroAction *act;
    char dead[0x400];   // unused sprintf destinations
    char pkt[0x400];    // packet buffer filled by 0x00463170
    char t[0x20];       // formatted number text
    CGump *gump;

    if (macroIndex > 0) {
        do {
            macro = macro->next;
        } while (--macroIndex != 0);
    }
    i = 0;
    if (macro->count <= 0)
        return;
    act = &macro->actions[0];
    do {
        if (static_cast<unsigned>(act->type - 1) <= 0x14) {
            switch (act->type) {
            case 1:
                if (g_inputActive != 0)
                    PORT_THISCALL3(&SendSpeechText, void, &g_app, char *, int, int)(act->text, static_cast<int>(strlen(act->text)) + 1, 0);
                break;
            case 2:
                if (g_inputActive != 0)
                    PORT_THISCALL3(&SendSpeechText, void, &g_app, char *, int, int)(act->text, static_cast<int>(strlen(act->text)) + 1, 2);
                break;
            case 3:
                if (g_inputActive != 0)
                    PORT_THISCALL3(&SendSpeechText, void, &g_app, char *, int, int)(act->text, static_cast<int>(strlen(act->text)) + 1, 8);
                break;
            case 4:
                if (g_inputActive != 0)
                    PORT_THISCALL3(&SendSpeechText, void, &g_app, char *, int, int)(act->text, static_cast<int>(strlen(act->text)) + 1, 9);
                break;
            case 5:
                if (g_inputActive != 0)
                    PORT_THISCALL2(&CWalkManager::RequestWalk, void, g_walkMgr, int, int)(act->arg, -1);
                break;
            case 6:
                if (g_inputActive != 0)
                    SetWarMode(-1);
                break;
            case 7:
                PORT_FUNC0(&PasteClipboardText, void, __cdecl)();   // not gated by DAT_005285c8
                break;
            case 8:
                if (g_inputActive != 0)
                    App_RunSimpleMacro(act->arg);
                break;
            case 9:
                if (g_inputActive != 0) {
                    gump = (CGump *)Gump_FindByKind(act->arg);
                    if (gump != nullptr)
                        decomp::vdelete(gump);
                }
                break;
            case 10:
                if (g_inputActive != 0) {
                    gump = (CGump *)Gump_FindByKind(act->arg);
                    if (gump != nullptr)
                        PORT_FUNC1(&CBookGump::Iconify, void, __fastcall, CGump *)(gump);
                }
                break;
            case 11:
                if (g_inputActive != 0) {
                    gump = (CGump *)Gump_FindByKind(act->arg);
                    if (gump != nullptr)
                        PORT_FUNC1(&CBookGump::Uniconify, void, __fastcall, CGump *)(gump);
                }
                break;
            case 12:
                if (g_inputActive != 0) {
                    sprintf(dead, g_004aa038, g_emptyString);
                    SendTextPacket(pkt, 0x58, g_emptyString);
                }
                break;
            case 13:
                if (g_inputActive != 0) {
                    sprintf(t, g_004a6520, act->arg, 0);
                    SendTextPacket(pkt, 0x24, t);
                }
                break;
            case 14:
                if (g_inputActive != 0 && g_macroLastItem != -1) {
                    sprintf(t, g_004a6520, g_macroLastItem, 0);
                    SendTextPacket(pkt, 0x24, t);
                }
                break;
            case 15:
                if (g_inputActive != 0) {
                    sprintf(t, g_fmtNumber, act->arg + 1);
                    sprintf(dead, g_004aa038, t);
                    SendTextPacket(pkt, 0x56, t);
                }
                break;
            case 16:
                if (g_inputActive != 0 && g_lastSpell != -1) {
                    sprintf(t, g_fmtNumber, g_lastSpell + 1);
                    sprintf(dead, g_004aa038, t);
                    SendTextPacket(pkt, 0x56, t);
                }
                break;
            case 17:
                if (g_inputActive != 0 && g_macroTarget != 0) {
                    int o = (int)reinterpret_cast<CObjectManager *>(g_objMgr)->FindBySerial((uint32_t)(g_macroTarget));   // find object by serial
                    if (o != 0)
                        PORT_THISCALL1(&Object_Use, void, g_objMgr, int)(o);
                }
                break;
            case 18:
                if (g_inputActive != 0)
                    SendTextPacket(pkt, 0xc7, g_004aa098);   // "bow"
                break;
            case 19:
                if (g_inputActive != 0)
                    SendTextPacket(pkt, 0xc7, g_004aa090);   // "salute"
                break;
            case 20:
                if (g_inputActive != 0 && g_gumpBusy == 0) {
                    CGump *obj = new CYesNoGump(g_quitText /* "Quit         Ultima Online?" */, reinterpret_cast<void *>(PORT_ADDR(&U1_ExitButton)), reinterpret_cast<void *>(PORT_ADDR(&DeleteGumpCallback)), 0);
                    CGump_AddChild(obj, nullptr, 1);
                }
                break;
            case 21:
                if (g_inputActive != 0)
                    PORT_FUNC0(&SingleClickVisible, void, __cdecl)();
                break;
            }
        }
        i++;
        act++;
    } while (i < macro->count);
}
HOOK_METHOD(0x0044c8e0, CApp, RunMacro);

// Read one line of the file into the line buffer: stops at '\n' or '\r', swallows a CRLF partner by
// ungetc'ing a character that is not the other half of the pair, NUL terminates. This is the fgetc loop
// the original has inlined at every call site; EOF is the _IOEOF bit of the original FILE's flag word.
static void ReadLine(FILE *f)
{
    int ch = 0;
    char *p = g_lineBuf;
    if (!feof(f)) {
        do {
            ch = fgetc(f);
            if (ch == 0xa || ch == 0xd)
                break;
            if (ch != -1) {
                *p = static_cast<char>(ch);
                p++;
            }
        } while (!feof(f));
        if (!feof(f)) {
            if (ch == 0xa || ch == 0xd) {
                int ch2 = fgetc(f);
                if (ch2 == ch || (ch2 != 0xa && ch2 != 0xd))
                    ungetc(ch2, f);
            }
        }
    }
    *p = 0;
}

// 0x0044d780  size 0x5e1
// Load keynames.txt (0x59 key names, one per scan code, into the slot table at 0x6f47d8) and macros.txt
// (header line "<keyname> <shift> <alt> <ctrl>" followed by action lines up to a line starting with '#').
// The three trailing flags are located by scanning back for '0'/'1' three times and terminating the key
// name at the separator before the first flag. Unknown key names skip their lines. A missing macros.txt
// returns immediately (no fclose).
void __thiscall CApp::LoadKeynamesAndMacros()
{
    MacroRecord *prev = nullptr;
    MacroRecord *cur;
    FILE *f;
    char **slot;
    char *p;
    int scan;
    int found;
    int shift;
    int alt;
    int ctrl;

    APPF(uint32_t, 0x1778) = 0;
    if (g_skillCount != 0)
        PORT_FUNC0(&LoadSkillNameTable, void, __cdecl)();

    BuildDataPath(*this, "keynames.txt");
    f = fopen(g_resolvedPath, g_004a8aa0);   // text read mode
    if (f != nullptr) {
        slot = g_keynameSlots;
        do {
            ReadLine(f);
            char *s = static_cast<char *>(::operator new(strlen(g_lineBuf) + 1));
            *slot = s;
            strcpy(s, g_lineBuf);
            slot++;
        } while (slot < g_keynameSlots + g_keynameCount);
        fclose(f);
    } else {
        memset(g_keynameSlots, 0, g_keynameCount * 4);
    }

    BuildDataPath(*this, "macros.txt");
    f = fopen(g_resolvedPath, g_004a8aa0);
    if (f == nullptr)
        return;

    while (!feof(f)) {
        // read header line: "<keyname> <shift> <alt> <ctrl>"
        ReadLine(f);
        if (strlen(g_lineBuf) == 0)
            continue;

        // locate the three trailing 0/1 flags, scanning back from the last character
        p = g_lineBuf + strlen(g_lineBuf) - 1;
        for (int n = 3; n != 0; n--) {
            while (*p != '0' && *p != '1')
                p--;
            p--;
        }
        *p = 0;   // terminate the key name
        p++;      // -> first flag digit

        // look the key name up in the keynames table
        scan = 0;
        found = 0;
        slot = g_keynameSlots;
        do {
            if (*slot != nullptr && strcmp(*slot, g_lineBuf) == 0) {
                cur = static_cast<MacroRecord *>(::operator new(sizeof(MacroRecord)));
                cur->vkey = static_cast<int32_t>(MapVirtualKeyA(scan, 1));
                cur->scan = scan << 16;
                cur->next = nullptr;
                if (prev == nullptr)
                    GLOBAL_AS(MacroRecord *, g_macroListHead) = cur;   // head of macro list
                else
                    prev->next = cur;
                prev = cur;
                APPF(int, 0x1778) = APPF(int, 0x1778) + 1;
                found = 1;
                break;
            }
            slot++;
            scan++;
        } while (slot < g_keynameSlots + g_keynameCount);
        cur = prev;

        if (found != 0) {
            // sscanf(strtok(p, " "), "%d", &shift) and the same for alt and ctrl (original CRT: strtok keeps its state)
            sscanf(strtok(p, g_titlePart1), g_fmtNumber, &shift);
            sscanf(strtok(nullptr, g_titlePart1), g_fmtNumber, &alt);
            sscanf(strtok(nullptr, g_titlePart1), g_fmtNumber, &ctrl);
            cur->shift = (shift != 0);
            cur->alt = (alt != 0);
            cur->count = 0;
            cur->ctrl = (ctrl != 0);
            if (!feof(f)) {
                while (g_lineBuf[0] != '#') {
                    ReadLine(f);
                    if (!feof(f) && g_lineBuf[0] != '#') {
                        cur = prev;
                        // ECX = line buffer, EDX = &cur->actions[cur->count]: parse one macro action line
                        Macro_ParseAction(g_lineBuf, &cur->actions[cur->count]);
                        cur->count = cur->count + 1;
                    }
                    if (feof(f))
                        break;
                }
            }
        } else {
            if (!feof(f)) {
                while (g_lineBuf[0] != '#') {
                    ReadLine(f);
                    if (feof(f))
                        break;
                }
            }
        }
    }
    fclose(f);
}
HOOK_METHOD(0x0044d780, CApp, LoadKeynamesAndMacros);
