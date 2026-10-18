// CTextEntry, second pass: constructor, scalar deleting destructor, key handling, mode setter, and two small
// free helpers of the same source file. 0x00442980 .. 0x00442c60, 0x004430a0.
#include "text_entry.h"

#include <windows.h>

#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "walk.h"
#include "unicode_speech_ops_ports.h"
#include "vtables.h"

#define TE_ScrollChoiceGump GLOBAL_AS(void *, g_scrollChoiceGump)
#define TE_WalkMgr GLOBAL_PTR(void, g_walk_storage)

// 0x00449130: RET 8, (direction, flags) on the walk manager.
#define TE_WalkKey(self, a1, a2) (reinterpret_cast<CWalkManager *>(self)->RequestWalkKey(a1, (uint32_t)(a2)))
// 0x0047c060 wide strlen (ECX = string, result in EAX), 0x0047c080 wide strcpy (ECX = dst, EDX = src).
#define TE_WcsLen PORT_FUNC1(&WcsLenThunk, int, __fastcall, void *)
#define TE_WcsCpy(a1, a2) (WcsCpy((uint16_t *)(a1), (const uint16_t *)(a2)))

// 0x00442980  size 0x2f
CTextEntry::CTextEntry()
{
    echoCommands = 0;
    unk_04 = 0;
    length = 0;
    mode = 0;
    inputGump = nullptr;
    lastLine[0] = 0;
    lastMode = 0;
}
HOOK_CTOR(0x00442980, CTextEntry, ());

// 0x004429b0  size 0x23
// Destructor (empty; the scalar deleting destructor is all the original has).
inline CTextEntry::~CTextEntry()
{
}
HOOK_VTABLE(0x0049f5dc, CTextEntry);
HOOK_SDD(0x004429b0, CTextEntry);   // slot 0

// 0x004429e0  size 0xd
// SetFocus on the main window.
void __cdecl FocusMainWindow()
{
    SetFocus(static_cast<HWND>(g_mainWindow));
}
HOOK_FUNC(0x004429e0, FocusMainWindow);

// 0x004429f0  size 0x20e
// Key down. Alt+F4 and Alt+Tab are left alone (0). When typing is enabled, no scroll choice gump is open and the
// line is empty, the cursor-pad keys (virtual keys 0x21 .. 0x28) walk in direction 1, 3, 5, 7, 6, 0, 2, 4, and
// Ctrl+Q (key 0x51) brings the last entered line back. Other keys in 0x29 .. 0x50 and everything above 0x51 return 0.
int __thiscall CTextEntry::OnKeyDown(int key, int extra)
{
    int ctrl = GetKeyState(0x11) < 0;
    GetKeyState(0x10);
    int alt = GetKeyState(0x12) < 0;
    if (alt != 0) {
        if (key == 0x73 || key == 9)
            return 0;
    }
    if (g_inputActive != 0 && TE_ScrollChoiceGump == nullptr && length == 0) {
        switch (key) {
        case 0x26:
            TE_WalkKey(TE_WalkMgr, 0, extra);   // 0x00449130
            return 1;
        case 0x21:
            TE_WalkKey(TE_WalkMgr, 1, extra);   // 0x00449130
            return 1;
        case 0x27:
            TE_WalkKey(TE_WalkMgr, 2, extra);   // 0x00449130
            return 1;
        case 0x22:
            TE_WalkKey(TE_WalkMgr, 3, extra);   // 0x00449130
            return 1;
        case 0x28:
            TE_WalkKey(TE_WalkMgr, 4, extra);   // 0x00449130
            return 1;
        case 0x23:
            TE_WalkKey(TE_WalkMgr, 5, extra);   // 0x00449130
            return 1;
        case 0x25:
            TE_WalkKey(TE_WalkMgr, 6, extra);   // 0x00449130
            return 1;
        case 0x24:
            TE_WalkKey(TE_WalkMgr, 7, extra);   // 0x00449130
            return 1;
        case 0x51:
            if (ctrl != 0) {
                if (g_useWideWindow != 0) {
                    TE_WcsCpy(buffer, lastLine);                  // 0x0047c080
                    length = TE_WcsLen(buffer);                   // 0x0047c060
                } else {
                    strcpy(reinterpret_cast<char *>(buffer), reinterpret_cast<const char *>(lastLine));
                    length = static_cast<int32_t>(strlen(reinterpret_cast<const char *>(buffer)));
                }
                mode = lastMode;
                RefreshLine();   // 0x00443590
                g_entrySkipNext = 1;
            }
            break;
        default:
            return 0;
        }
    }
    return 1;
}
HOOK_METHOD(0x004429f0, CTextEntry, OnKeyDown);

// 0x00442c60  size 0x49
// ECX = text, EDX = destination: skip leading spaces, copy the first word lowercased (A-Z only) to the destination
// and terminate it. Returns the pointer past the word and the delimiter that ended it (the terminator is not skipped).
const char *__fastcall CopyFirstWordLower(const char *text, char *dest)
{
    const char *p = text;
    if (*p == ' ') {
        char c;
        do {
            c = p[1];
            p++;
        } while (c == ' ');
    }
    char c = *p;
    while (c != ' ' && c != 0) {
        *dest = (c >= 'A' && c <= 'Z') ? c + 0x20 : c;
        c = p[1];
        dest++;
        p++;
    }
    *dest = 0;
    if (*p != 0)
        p++;
    return p;
}
HOOK_FUNC(0x00442c60, CopyFirstWordLower);

// 0x004430a0  size 0x10
void __thiscall CTextEntry::SetMode(int newMode)
{
    mode = newMode;
}
HOOK_METHOD(0x004430a0, CTextEntry, SetMode);
