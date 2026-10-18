// uo.cfg / login.cfg handling: LoadConfigFile, WriteConfigSetting, ApplyConfigSetting and the host:port helpers.
// (Porting pass 2, batch 08.) The SEH frame of ApplyConfigSetting and its unused CStr local (constructor 0x004703a0,
// destructor 0x004703c0; the string stays empty, nothing is allocated) are omitted.
//
// The setting table at 0x004a75b0 is an array of {name, id} entries ending with a null name; the id of a name is
// its index (0x00 CdRomDataPath .. 0x38 ReverseSpeakers), 0x39 for an unknown name.
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../runtime/ported.h"
#include "../runtime/test_seams.h"
#include "app.h"
#include "circle_trans.h"
#include "crt.h"
#include <winsock.h>
#include "app_support_ports.h"
#include "globals.h"
#include "cstr.h"


// Winsock imports (thunks in the original image).

// 0x0044ad10: read one line (without the line break) into the shared buffer at 0x0068da30. ECX = FILE *.
#define Orig_ReadLine(o) PORT_THISCALL0(&App_ReadLineFromFile, char *, (o))()
// 0x0044adc0: undo the password obfuscation in place. ECX = string.
#define Orig_Deobfuscate(o) PORT_THISCALL0(&App_DeobfuscateString, void, (o))()

struct CfgEntry {
    const char *name;
    int32_t id;
};
#define g_cfgTable GLOBAL_PTR(CfgEntry, g_cfgTable_storage)

#define g_circleTrans GLOBAL_AS(CCircleTrans, g_circleTrans_storage)

#define g_loginServerHost GLOBAL_PTR(char *, g_loginServerSpec)
#define g_backupServerHost GLOBAL_PTR(char *, g_backupServerSpec)




// 0x00424da0  size 0xd4
// Non-zero when `s` is "a.b.c.d,port": four octets of one to three digits, a comma, then four or five digits and a
// non-digit.
static int __fastcall IsIpPortString(const char *s)
{
    const char *p = s;
    int octet = 0;

    do {
        if (isdigit(*p) == 0)
            return 0;
        p++;
        if (isdigit(*p) != 0)
            p++;
        if (isdigit(*p) != 0)
            p++;
        if (octet < 3) {
            if (*p != '.')
                return 0;
        } else if (*p != ',') {
            return 0;
        }
        p++;
        octet++;
    } while (octet < 4);
    if (isdigit(*p) == 0)
        return 0;
    p++;
    if (isdigit(*p) == 0)
        return 0;
    p++;
    if (isdigit(*p) == 0)
        return 0;
    p++;
    if (isdigit(*p) == 0)
        return 0;
    p++;
    if (isdigit(*p) != 0)
        p++;
    return isdigit(*p) == 0 ? 1 : 0;
}
HOOK_FUNC(0x00424da0, IsIpPortString);

// 0x00424e80  size 0x11c
// "host" or "host,port": the host name is resolved and written as four bytes (a, b, c, d) to the first four
// pointers, the port (0 when there is none) to the last. All zero when winsock or the lookup fails.
void __fastcall ResolveHostPort(const char *spec, int *a, int *b, int *c, int *d, int *port)
{
    char host[0x400];
    uint8_t wsaData[0x190];
    const char *p = spec;
    int n = 0;
    uint8_t *he;
    uint32_t addr;

    char ch = *p;
    while (ch != 0 && ch != ',') {
        host[n] = ch;
        n++;
        p++;
        ch = *p;
    }
    host[n] = 0;
    if (*p == ',') {
        p++;
        sscanf(p, "%d", port);
    } else {
        *port = 0;
    }
    if (WSAStartup(0x101, reinterpret_cast<WSADATA *>(wsaData)) != 0) {
        *port = 0;
        *d = 0;
        *c = 0;
        *b = 0;
        *a = 0;
        return;
    }
    he = reinterpret_cast<uint8_t *>(gethostbyname(host));
    WSACleanup();
    if (he != nullptr) {
        uint8_t **list = *reinterpret_cast<uint8_t ***>(he + 0xc);   // hostent::h_addr_list
        addr = ntohl(*reinterpret_cast<uint32_t *>(list[0]));
        *a = static_cast<int>(addr >> 24);
        *b = static_cast<int>((addr >> 16) & 0xff);
        *c = static_cast<int>((addr >> 8) & 0xff);
        *d = static_cast<int>(addr & 0xff);
        return;
    }
    *port = 0;
    *d = 0;
    *c = 0;
    *b = 0;
    *a = 0;
}
HOOK_FUNC(0x00424e80, ResolveHostPort);

// 0x00424fa0  size 0xda8
// Apply one setting. `which` non-zero = the value comes from uo.cfg, which may not set LoginServer or
// BackupLoginServer (those two come from login.cfg only). Boolean values are "on" or "yes" (case-insensitive).
static void __fastcall ApplyConfigSetting(uint32_t id, char *value, int which)
{
    CStr unused;   // the original constructs and destroys an unused CStr (0x004703a0 / 0x004703c0)
    if (which != 0 && (id == 0x18 || id == 0x19))
        return;
    if (id > 0x38)
        return;
    switch (id) {
    case 0:   // CdRomDataPath
        strncpy(g_0069e520, value, 0x104);
        break;
    case 1:   // CacheSize
        g_cacheMB = atoi(value);
        break;
    case 2:   // HdUsage
        g_installMode = _stricmp(value, "max") == 0 ? 1 : 0;
        break;
    case 3:   // PlayerName
        if (value != nullptr && *value != 0)
            strcpy(g_playerName, value);
        break;
    case 4:   // Password
        if (value != nullptr && *value != 0) {
            Orig_Deobfuscate(value);   // 0x0044adc0
            strcpy(g_password, value);
        }
        break;
    case 5:   // Sound
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_soundEnabledSetting = 1;
        else
            g_soundEnabledSetting = 0;
        break;
    case 6:   // SoundVol
        if (value != nullptr)
            g_soundVolumeSetting = static_cast<uint8_t>(atoi(value));
        break;
    case 7:   // Music
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            mo_defaultEnabled = 1;
        else
            mo_defaultEnabled = 0;
        break;
    case 8:   // MusicVol
        if (value != nullptr)
            g_musicDefaultVolume = static_cast<uint8_t>(atoi(value));
        break;
    case 9:   // ObscenityFilter
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_profanityOn = 1;
        else
            g_profanityOn = 0;
        break;
    case 11:   // FilterPassword
        if (value != nullptr && *value != 0) {
            Orig_Deobfuscate(value);   // 0x0044adc0
            strcpy(g_0069e630, value);
        }
        break;
    case 12:   // PageFlip: on/yes -> flip, gdi -> GDI blit, anything else -> neither
        pb_gdiMode = 0;
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0) {
            g_useFlipChain = 1;
            break;
        }
        g_useFlipChain = 0;
        if (_stricmp(value, "gdi") == 0)
            pb_gdiMode = 1;
        break;
    case 13:   // FrameCounter
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_showFps = 1;
        else
            g_showFps = 0;
        break;
    case 14:   // SavePassword
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_0069e628 = 1;
        else
            g_0069e628 = 0;
        break;
    case 15:   // FullScreen
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_ccVal62c = 1;
        else
            g_ccVal62c = 0;
        break;
    case 17: {   // UserCentral: a.b.c.d,port
        int a;
        int b;
        int c;
        int d;
        int port;
        sscanf(value, "%d.%d.%d.%d,%d", &a, &b, &c, &d, &port);
        g_006f47b8 = ((((a << 8) | b) << 8 | c) << 8) | d;
        g_006f47bc = static_cast<uint16_t>(port);
        break;
    }
    case 18:   // CdKey
        GLOBAL_AS(int32_t, g_authKey) = atoi(value);
        break;
    case 19:   // ShowIntroAnim
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_introMovieFlag = 1;
        else
            g_introMovieFlag = 0;
        break;
    case 20:   // use565: loads the 5-5-5 masks first, then switches to 5-6-5 when the value is on/yes
        g_redShift16 = 10;
        g_greenShift16 = 5;
        g_is565 = 0;
        g_blendMaskB = 0x7bde;
        g_blendMaskA = 0x739c;
        g_shadeAdd = 0x3def;
        g_004a8004 = 0x781f;
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_is565 = 1;
        break;
    case 21:   // autoLogIn
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_loginGumpHook = 1;
        else
            g_loginGumpHook = 0;
        break;
    case 22: {   // OrderCache: comma separated list of idle, startup, shutdown
        char *p = value;
        char *token;
        g_004b7230 = 0;
        g_004b7228 = 0;
        g_004b722c = 0;
        if (*p == 0)
            break;
        do {
            token = p;
            while (*p != 0 && *p != ',')
                p++;
            if (*p != 0) {
                *p = 0;
                p++;
            }
            if (_stricmp(token, "idle") == 0)
                g_004b7230 = 1;
            else if (_stricmp(token, "startup") == 0)
                g_004b7228 = 1;
            else if (_stricmp(token, "shutdown") == 0)
                g_004b722c = 1;
        } while (*p != 0);
        break;
    }
    case 23:   // RememberAcctPW
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_rememberPassword = 1;
        else
            g_rememberPassword = 0;
        break;
    case 24: {   // LoginServer: a host name, or a.b.c.d,port
        int a;
        int b;
        int c;
        int d;
        int port;
        int n;
        if (IsIpPortString(value) == 0) {
            char *copy = static_cast<char *>(::operator new(strlen(value) + 1));
            n = g_loginServerCount;
            g_loginServerHost[n] = copy;
            strcpy(copy, value);
            n = g_loginServerCount;
            g_loginServerIp[n] = 0;
            g_loginServerPort[n] = 0;
        } else {
            sscanf(value, "%d.%d.%d.%d,%d", &a, &b, &c, &d, &port);
            n = g_loginServerCount;
            g_loginServerHost[n] = nullptr;
            g_loginServerIp[n] = static_cast<uint32_t>(((((a << 8) | b) << 8 | c) << 8) | d);
            g_loginServerPort[n] = static_cast<uint16_t>(port);
        }
        n++;
        g_loginServerCount = n;
        if (g_loginServerIndex < 0)
            g_loginServerIndex = 0;
        break;
    }
    case 25: {   // BackupLoginServer: a host name, or a.b.c.d,port
        int a;
        int b;
        int c;
        int d;
        int port;
        int n;
        if (IsIpPortString(value) == 0) {
            char *copy = static_cast<char *>(::operator new(strlen(value) + 1));
            n = g_backupServerCount;
            g_backupServerHost[n] = copy;
            strcpy(copy, value);
            n = g_backupServerCount;
            g_backupServerIp[n] = 0;
            g_backupServerPort[n] = 0;
            n++;
            g_backupServerCount = n;
            break;
        }
        sscanf(value, "%d.%d.%d.%d,%d", &a, &b, &c, &d, &port);
        n = g_backupServerCount;
        g_backupServerHost[n] = nullptr;
        g_backupServerIp[n] = static_cast<uint32_t>(((((a << 8) | b) << 8 | c) << 8) | d);
        g_backupServerPort[n] = static_cast<uint16_t>(port);
        n++;
        g_backupServerCount = n;
        break;
    }
    case 26:   // AcctID
        if (value != nullptr && *value != 0)
            strcpy(g_acctName, value);
        break;
    case 27:   // AcctLastName
        if (value != nullptr && *value != 0)
            strcpy(g_titlePart2, value);
        break;
    case 28:   // AcctPassword
        if (value != nullptr && *value != 0) {
            Orig_Deobfuscate(value);   // 0x0044adc0
            strcpy(g_accountPassword, value);
        }
        break;
    case 29: {   // NextLoginKey
        int key;
        if (value != nullptr && *value != 0) {
            sscanf(value, "%d", &key);
            g_charFlags = static_cast<uint8_t>(key);
        }
        break;
    }
    case 30: {   // DefaultChar
        int slot;
        if (value != nullptr && *value != 0) {
            sscanf(value, "%d", &slot);
            if (slot >= 0 && slot <= 4)
                g_selectedCharSlot = slot;
        }
        break;
    }
    case 31:   // Mouse
        if (_stricmp(value, "slow") == 0)
            g_cursorEnabled2 = 0;
        else if (_stricmp(value, "fast") == 0)
            g_cursorEnabled2 = 1;
        break;
    case 32:   // UseVerData
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_verdataEnabled = 1;
        else
            g_verdataEnabled = 0;
        break;
    case 33:   // ChestOpening
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_introFade = 1;
        else
            g_introFade = 0;
        break;
    case 34: {   // JournalSaveFile
        char *copy = static_cast<char *>(::operator new(strlen(value) + 1));
        g_textLogName = copy;
        strcpy(copy, value);
        break;
    }
    case 35:   // JournalSaveUnicode
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_logWide = 1;
        else
            g_logWide = 0;
        break;
    case 36:   // NotorietyQuery
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_warnOnAttack = 1;
        else
            g_warnOnAttack = 0;
        break;
    case 37:   // UseCircleTrans
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_ccVal220 = 1;
        else
            g_ccVal220 = 0;
        break;
    case 38:   // CircleTransRadius
        g_circleTrans.SetRadius(atoi(value));
        break;
    case 39: {   // CircleTransPos: x,y
        int cx;
        int cy;
        sscanf(value, "%d,%d", &cx, &cy);
        g_circleTrans.SetCenter(cx, cy);
        break;
    }
    case 40:   // FilterPWOn: only ever switched on here
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_ccVal1a0 = 1;
        break;
    case 41:   // Footsteps
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_ccVal2cc = 1;
        else
            g_ccVal2cc = 0;
        break;
    case 42:   // ShowTips
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_tipEnabled = 1;
        else
            g_tipEnabled = 0;
        break;
    case 43:   // LastTip
        GLOBAL_AS(int32_t, g_tipNumber) = atoi(value);
        break;
    case 44:   // CurrentVer
        g_tipLast = atoi(value);
        break;
    case 45:   // BarkTimeModifier: 0 means 100
        g_barkSecondsPerLine = atoi(value);
        if (g_barkSecondsPerLine == 0)
            g_barkSecondsPerLine = 0x64;
        break;
    case 46:   // ScaleBarkTimes
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_barkMultiLine = 1;
        else
            g_barkMultiLine = 0;
        break;
    case 47:   // CombatHueColor
        g_tintColor = atoi(value);
        break;
    case 48:   // FrameSkipping
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_catchUpFlag = 1;
        else
            g_catchUpFlag = 0;
        break;
    case 49:   // AllowPathfind
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            MG_PathEnabled = 1;
        else
            MG_PathEnabled = 0;
        break;
    case 50:   // HoldTabForCombat
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            wp_warModeEnabled = 1;
        else
            wp_warModeEnabled = 0;
        break;
    case 51:   // OffsetGumps
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_004a759c = 1;
        else
            g_004a759c = 0;
        break;
    case 52:   // AutoArrangeMinimized
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_004a75a0 = 1;
        else
            g_004a75a0 = 0;
        break;
    case 53:   // UnicodeSpeech
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_useWideWindow = 1;
        else
            g_useWideWindow = 0;
        break;
    case 54:   // AltKeyModNum
        g_004a75a8 = atoi(value);
        break;
    case 55:   // Sound8Bit
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            g_004b723c = 1;
        else
            g_004b723c = 0;
        break;
    case 56:   // ReverseSpeakers
        if (_stricmp(value, "on") == 0 || _stricmp(value, "yes") == 0)
            so_stereoFlip = 1;
        else
            so_stereoFlip = 0;
        break;
    default:
        break;
    }
}
HOOK_FUNC(0x00424fa0, ApplyConfigSetting);

// 0x004249b0  size 0x105
// Read login.cfg (which == 0) or uo.cfg (which != 0) line by line: "name=value" or "name value"; known names are
// applied through ApplyConfigSetting. The ECX the callers leave is not read.
void __stdcall LoadConfigFile(int which)
{
    FILE *fp;
    char *line;
    char *token;
    char *value;
    int id;
    int i;

    // Test seam, not in the original: UO_DECOMP_LOGIN=a.b.c.d,port (or a host name) replaces login.cfg, which the
    // client takes from the registry install directory, so a test run can be pointed at a local shard.
    if (which == 0) {
        static char injected[128];
        if (::decomp::test_value("UO_DECOMP_LOGIN", injected, sizeof injected)) {
            ApplyConfigSetting(24, injected, 0);
            return;
        }
    }
    if (which != 0)
        fp = fopen(g_app.ResolveDataPath(const_cast<char *>("uo.cfg")), "r");
    else
        fp = fopen(g_app.ResolveDataPath(const_cast<char *>("login.cfg")), "r");
    if (fp == nullptr)
        return;
    if (feof(fp))
        goto done;
    do {
        line = Orig_ReadLine(fp);   // 0x0044ad10
        token = strtok(line, " \t\n\r\f=");
        if (token != nullptr) {
            id = 0x39;
            if (g_cfgTable[0].name != nullptr) {
                for (i = 0; g_cfgTable[i].name != nullptr; i++) {
                    if (_stricmp(token, g_cfgTable[i].name) == 0) {
                        id = g_cfgTable[i].id;
                        break;
                    }
                }
            }
            if (id != 0x39) {
                value = strtok(nullptr, " \t\n\r\f");
                if (value != nullptr && *value != 0)
                    ApplyConfigSetting(static_cast<uint32_t>(id), value, which);
            }
        }
    } while (!feof(fp));
done:
    fclose(fp);
}
HOOK_FUNC(0x004249b0, LoadConfigFile);

// Index of the setting `name` in the table (which is also its id), 0x39 when it is not there.
static inline int CfgIdOfName(const char *name)
{
    if (g_cfgTable[0].name != nullptr) {
        for (int i = 0; g_cfgTable[i].name != nullptr; i++) {
            if (_stricmp(name, g_cfgTable[i].name) == 0)
                return i;
        }
    }
    return 0x39;
}

// 0x00424ac0  size 0x2d6
// Set one "name=value" line in uo.cfg: find the line of that setting, then rewrite the file around it (text before
// the line, the new line, text after it); append "name=value" when there is none.
void __fastcall WriteConfigSetting(uint32_t id, const char *text)
{
    FILE *fp = fopen(g_app.ResolveDataPath(const_cast<char *>("uo.cfg")), "rb");
    FILE *out;
    char *line;
    char *token;
    int lineStart;
    int tailLen;
    int posAfter = 0;
    int found = 0;
    int eofSeen = 0;
    uint8_t *head = nullptr;
    uint8_t *tail = nullptr;

    if (fp == nullptr)
        return;
    for (;;) {
        if (eofSeen != 0)
            break;
        lineStart = ftell(fp);
        line = Orig_ReadLine(fp);   // 0x0044ad10
        if (feof(fp)) {
            eofSeen = 1;
        } else {
            posAfter = ftell(fp);
            token = strtok(line, " \t\n\r\f=");
            if (token != nullptr) {
                if (CfgIdOfName(token) == static_cast<int>(id))
                    found = 1;
            }
        }
        if (found != 0)
            break;
    }
    fclose(fp);

    if (eofSeen != 0) {
        // not there: append
        out = fopen(g_app.ResolveDataPath(const_cast<char *>("uo.cfg")), "at");
        fprintf(out, "%s=%s\n", g_cfgTable[id].name, text);
        fclose(out);
        return;
    }

    out = fopen(g_app.ResolveDataPath(const_cast<char *>("uo.cfg")), "rb");
    if (lineStart > 0) {
        head = static_cast<uint8_t *>(::operator new(static_cast<size_t>(lineStart)));
        fread(head, 1, static_cast<size_t>(lineStart), out);
    }
    fseek(out, 0, 2);
    tailLen = ftell(out) - posAfter;
    if (tailLen > 0) {
        fseek(out, posAfter, 0);
        tail = static_cast<uint8_t *>(::operator new(static_cast<size_t>(tailLen)));
        fread(tail, 1, static_cast<size_t>(tailLen), out);
    }
    fclose(out);

    out = fopen(g_app.ResolveDataPath(const_cast<char *>("uo.cfg")), "wb");
    if (lineStart > 0) {
        fwrite(head, 1, static_cast<size_t>(lineStart), out);
        operator delete(head);
    }
    int len = static_cast<int>(strlen(g_cfgTable[id].name));
    int i;
    for (i = 0; i < len; i++)
        fputc(g_cfgTable[id].name[i], out);
    fputc('=', out);
    len = static_cast<int>(strlen(text));
    for (i = 0; i < len; i++)
        fputc(text[i], out);
    fputc(0xd, out);
    fputc(0xa, out);
    if (tailLen > 0) {
        fwrite(tail, 1, static_cast<size_t>(tailLen), out);
        operator delete(tail);
    }
    fclose(out);
}
HOOK_FUNC(0x00424ac0, WriteConfigSetting);
