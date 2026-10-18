// Small helpers of the packet/text area (0x00478cb0-0x00478d90): "(test)" check, the fullscreen toggles,
// space collapsing and the speech sender. (Porting pass 2, batch 29.)
#include <windows.h>

#include <cstdint>
#include <cstring>

#include "../runtime/ported.h"
#include "client_socket.h"
#include "crt.h"
#include "sound_ops_ports.h"
#include "packet_builders_batch24_ports.h"
#include "globals.h"

// 0x00480e20: SendMessageA(main window, WM_COMMAND, 0x9c99, 0), no arguments, returns the SendMessageA result.
#define Misc_PostFullscreenCommand() ((int)PostMenuCommand9c99())

// 0x00478cb0  size 0x15  ECX = text: non-zero when it contains "(test)" (strstr at 0x0048c9f0)
int __fastcall StrContainsTest(const char *text)
{
    return strstr(text, "(test)") != nullptr ? 1 : 0;
}
HOOK_FUNC(0x00478cb0, StrContainsTest);

// 0x00478cd0  size 0x3  returns 0 (option test stub). ? the callers' argument conventions are not annotated; it takes none.
int __cdecl Misc_Return0()
{
    return 0;
}
HOOK_FUNC(0x00478cd0, Misc_Return0);

// 0x00478ce0  size 0xf  when the fullscreen flag is set, post the 0x9c99 menu command (a tail jump in the original)
int __cdecl Misc_ToggleIfFullscreen()
{
    if (g_bFullscreen != 0)
        return Misc_PostFullscreenCommand();   // 0x00480e20
    return 0;
}
HOOK_FUNC(0x00478ce0, Misc_ToggleIfFullscreen);

// 0x00478cf0  size 0xe  ECX = wanted fullscreen state: post the 0x9c99 menu command when it differs from the current one
// (a tail jump in the original; EAX is left as it was when nothing is posted)
void __fastcall Misc_SetFullscreen(int wanted)
{
    if (wanted == g_bFullscreen)
        return;
    Misc_PostFullscreenCommand();   // 0x00480e20
}
HOOK_FUNC(0x00478cf0, Misc_SetFullscreen);

// 0x00478d00  size 0x8e  ECX = string, modified in place: every run of spaces becomes a single space
void __fastcall CollapseSpaces(char *text)
{
    int len = static_cast<int>(strlen(text));
    if (len <= 1)
        return;
    char *buf = static_cast<char *>(::operator new(static_cast<size_t>(len) + 1));
    int prevSpace = 0;
    int out = 0;
    for (int i = 0; i < len; i++) {
        char c = text[i];
        if (c == ' ') {
            if (prevSpace == 0) {
                buf[out] = c;
                out++;
            }
            prevSpace = 1;
        } else {
            buf[out] = c;
            out++;
            prevSpace = 0;
        }
    }
    buf[out] = 0;
    strcpy(text, buf);
    operator delete(buf);
}
HOOK_FUNC(0x00478d00, CollapseSpaces);

// The object whose second dword is the connection that speech packets go to.
// ? the class is not identified; only +4 is read.
struct CSpeechSenderRaw {
    void *unk_00;
    CClientSocket *socket;   // +0x04
};

// Build_Packet_0x03 (0x00462db0), a __stdcall (buf, text, len, type) in the port; the original passes
// ECX = 0x0069e0e0 (the packet builder object) which the port ignores.
#define Misc_BuildSpeech(o, a1, a2, a3, a4) PORT_THISCALL4(&Build_Packet_0x03, void, (o), uint8_t *, const char *, uint32_t, uint32_t)(a1, a2, a3, a4)

// 0x00478d90  size 0xef  ECX = sender, stack (text, unused, type), RET 0xc. Collapses the spaces of the text, for type 2
// wraps it in "*" ... "*" (the wrapped copy is never freed), builds the speech packet (0x03) and sends it.
void __fastcall SendSpeechText(CSpeechSenderRaw *self, int /*edx*/, char *text, int /*unused*/, int type)
{
    uint8_t packet[0xfc];
    const char *send;

    CollapseSpaces(text);   // 0x00478d00
    if (type == 2) {
        char *wrapped = static_cast<char *>(::operator new(strlen(text) + 3));
        strcpy(wrapped, g_004a754c);   // "*"
        strcat(wrapped, text);
        strcat(wrapped, g_004a754c);
        send = wrapped;
    } else {
        send = text;
    }
    uint32_t len = static_cast<uint32_t>(strlen(send));
    Misc_BuildSpeech(reinterpret_cast<void *>(GADDR(g_packetBuilder_storage)), packet, send, len, static_cast<uint32_t>(type));   // 0x00462db0
    self->socket->SendPacket(packet);   // 0x00478610
}
HOOK_FUNC(0x00478d90, SendSpeechText);
