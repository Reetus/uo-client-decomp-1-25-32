// Wide-string register helpers (0x0047c060..0x0047c120), the Unicode text filter, Unicode bark creation and the
// Unicode speech packet 0xAD sender. (Porting pass 2, batch 30.)
// The SEH frames of the bark creators (a try level around operator new) are omitted.
#include "unicode_speech.h"

#include <cstring>

#include "../runtime/ported.h"
#include "crt.h"
#include "object_manager_ops_ports.h"
#include "packet_buffer_ops_ports.h"
#include "client_socket.h"
#include "globals.h"

// 0x00457460: ECX = text, in-place profanity filter.
#define US_Profanity(a1) (Profanity_Apply(a1))
// Outbound packet writers: ECX = packet, EDX = value.
#define US_PktBegin(a1, a2) (PacketBuf_Begin(a1, a2))
#define US_PktU8(a1, a2) (PacketBuf_WriteU8(a1, a2))
#define US_PktU16(a1, a2) (PacketBuf_WriteU16(a1, a2))
#define US_PktU32(a1, a2) (PacketBuf_WriteU32(a1, a2))

// 0x0047c060  size 0x12  ECX = string; returns the length in characters. The original leaves EDX untouched, so
// the hook is a thunk that keeps it.
static int __fastcall WcsLenImpl(const uint16_t *s)
{
    int n = 0;
    while (*s != 0) {
        s++;
        n++;
    }
    return n;
}
__declspec(naked) void WcsLenThunk()
{
    __asm {
        push edx
        call WcsLenImpl
        pop edx
        ret
    }
}
HOOK_FUNC(0x0047c060, WcsLenThunk);

// 0x0047c080  size 0x1f  ECX = destination, EDX = source; copies the wide string with its terminator.
void __fastcall WcsCpy(uint16_t *dst, const uint16_t *src)
{
    uint16_t c = *src;
    while (c != 0) {
        src++;
        *dst = c;
        dst++;
        c = *src;
    }
    *dst = 0;
}
HOOK_FUNC(0x0047c080, WcsCpy);

// 0x0047c0a0  size 0x1e  ECX = wide destination, EDX = narrow source; each byte is sign-extended to 16 bits.
void __fastcall WidenStrCpy(uint16_t *dst, const char *src)
{
    for (char c = *src; c != 0; c = src[1]) {
        *dst = static_cast<uint16_t>(static_cast<int16_t>(c));
        dst++;
        src++;
    }
    *dst = 0;
}
HOOK_FUNC(0x0047c0a0, WidenStrCpy);

// 0x0047c0c0  size 0x2e  ECX = wide destination, EDX = wide source; appends.
void __fastcall WcsCat(uint16_t *dst, const uint16_t *src)
{
    while (*dst != 0)
        dst++;
    uint16_t c = *src;
    while (c != 0) {
        src++;
        *dst = c;
        dst++;
        c = *src;
    }
    *dst = 0;
}
HOOK_FUNC(0x0047c0c0, WcsCat);

// 0x0047c0f0  size 0x2d  ECX = wide destination, EDX = narrow source; appends with sign extension.
void __fastcall WidenStrCat(uint16_t *dst, const char *src)
{
    while (*dst != 0)
        dst++;
    char c = *src;
    while (c != 0) {
        *dst = static_cast<uint16_t>(static_cast<int16_t>(c));
        c = src[1];
        dst++;
        src++;
    }
    *dst = 0;
}
HOOK_FUNC(0x0047c0f0, WidenStrCat);

// 0x0047c120  size 0x23  ECX = narrow destination, EDX = wide source; characters >= 0x80 are dropped.
void __fastcall NarrowStrCpy(char *dst, const uint16_t *src)
{
    uint16_t c = *src;
    while (c != 0) {
        if (c < 0x80) {
            *dst = static_cast<char>(*src);
            dst++;
        }
        c = src[1];
        src++;
    }
    *dst = 0;
}
HOOK_FUNC(0x0047c120, NarrowStrCpy);

// 0x0047c150  size 0x91  ECX = wide string. Copies it narrowed (characters >= 0x80 become spaces) into a new
// buffer, runs the profanity filter over that copy and writes any changed character back.
// ? the copy is not NUL terminated before the filter runs and is never freed (as in the original)
void __fastcall FilterUnicodeText(uint16_t *text)
{
    int len = 0;
    if (text == nullptr)
        return;
    if (text[0] != 0) {
        const uint16_t *p = text;
        do {
            p++;
            len++;
        } while (*p != 0);
    }
    char *buf = static_cast<char *>(::operator new(static_cast<size_t>(len) + 1));
    {
        int i = 0;
        const uint16_t *s = text;
        if (len > 0) {
            do {
                if (*s < 0x80)
                    buf[i] = static_cast<char>(*s);
                else
                    buf[i] = 0x20;
                i++;
                s++;
            } while (i < len);
        }
    }
    US_Profanity(buf);   // 0x00457460
    {
        int i = 0;
        uint16_t *s = text;
        if (len > 0) {
            do {
                if (*s < 0x80) {
                    char c = buf[i];
                    if (static_cast<int>(c) != static_cast<int>(*s & 0xff))
                        *s = static_cast<uint16_t>(static_cast<int16_t>(c));
                }
                i++;
                s++;
            } while (i < len);
        }
    }
}
HOOK_FUNC(0x0047c150, FilterUnicodeText);

// 0x0047c4d0  size 0x66  RET 0x10, ECX not read: (hue, font, text, extra). A system (speakerless) Unicode bark.
CUnicodeBarkGump *__stdcall CreateUnicodeBarkGump(int hue, int font, uint16_t *text, int extra)
{
    CUnicodeBarkGump *g;

    g_barkState = 1;
    g = new CUnicodeBarkGump(nullptr, text, static_cast<uint16_t>(hue), static_cast<uint16_t>(font), extra, nullptr);   // 0x00402850
    return g;
}
HOOK_FUNC(0x0047c4d0, CreateUnicodeBarkGump);

// 0x0047c540  size 0xb2  RET 0x14
CUnicodeBarkGump *__thiscall CBarkOwner::AddUnicodeBarkLimitTwo(uint16_t *text, int hue, int font, int extra, CGump *param5)
{
    CBarkGump *tail = nullptr;
    int count = 0;
    void *mem;

    for (CBarkGump *g = barkList; g != nullptr; g = g->older) {
        tail = g;
        count++;
    }
    if (count >= 3) {
        int n = count - 2;
        CBarkGump *cur = tail;
        do {
            CBarkGump *nextNewer = cur->newer;
            if (cur != nullptr)
                PORT_VSLOT1(cur, 0x00, void, int)(1);
            n--;
            cur = nextNewer;
        } while (n != 0);
    }
    return new CUnicodeBarkGump(reinterpret_cast<CObj *>(this), text, static_cast<uint16_t>(hue), static_cast<uint16_t>(font), extra, param5);   // 0x00402850
}
HOOK_METHOD(0x0047c540, CBarkOwner, AddUnicodeBarkLimitTwo);

// 0x0047c600  size 0x8b  RET 0x14
CUnicodeBarkGump *__thiscall CBarkOwner::AddUnicodeBarkReplaceAll(uint16_t *text, int hue, int font, int extra, CGump *param5)
{
    void *mem;

    while (barkList != nullptr)
        PORT_VSLOT1(barkList, 0x00, void, int)(1);
    return new CUnicodeBarkGump(reinterpret_cast<CObj *>(this), text, static_cast<uint16_t>(hue), static_cast<uint16_t>(font), extra, param5);   // 0x00402850
}
HOOK_METHOD(0x0047c600, CBarkOwner, AddUnicodeBarkReplaceAll);

// 0x0047c730  size 0x1e7  RET 0xc
void __thiscall CSpeechSender::SendUnicodeSpeech(uint16_t *text, int length, int mode)
{
    (void)length;
    uint8_t packet[0x1f4];   // ebp-0x1f8, up to the saved `this` at ebp-4
    uint16_t *end = text;
    uint16_t *p;
    int len;

    while (*end != 0)
        end++;
    for (p = end - 1; reinterpret_cast<uintptr_t>(p) >= reinterpret_cast<uintptr_t>(text); p--) {
        if (*p != 0x20)
            break;
    }
    p[1] = 0;

    if (mode == 2) {
        int n = 0;
        while (text[n] != 0)
            n++;
        uint16_t *tmp = static_cast<uint16_t *>(::operator new(static_cast<size_t>(n) * 2 + 6));
        tmp[0] = 0x2a;
        {
            uint16_t *d = tmp + 1;
            const uint16_t *s = text;
            uint16_t c = *s;
            while (c != 0) {
                *d = c;
                d++;
                s++;
                c = *s;
            }
            *d = 0;
        }
        int cnt = 0;
        while (tmp[cnt] != 0)
            cnt++;
        int last = cnt + 1;
        tmp[cnt] = 0x2a;
        tmp[last] = 0;
        uint32_t lang = US_Language;
        US_PktBegin(packet, 0xad);   // 0x004574e0
        US_PktU8(packet, 2);   // 0x00457520
        US_PktU16(packet, GLOBAL_AS(uint32_t, g_hue1));   // 0x00457570
        US_PktU16(packet, g_colorEmoteB);   // 0x00457570
        US_PktU32(packet, lang);   // 0x004575f0
        if (last >= 0) {
            const uint16_t *s = tmp;
            int k = last + 1;
            do {
                US_PktU16(packet, *s);   // 0x00457570
                s++;
                k--;
            } while (k != 0);
        }
        operator delete(tmp);
    } else {
        len = 0;
        while (text[len] != 0)
            len++;
        uint32_t lang = US_Language;
        US_PktBegin(packet, 0xad);   // 0x004574e0
        US_PktU8(packet, static_cast<uint8_t>(mode));   // 0x00457520
        if (static_cast<uint8_t>(mode) == 2) {
            US_PktU16(packet, GLOBAL_AS(uint32_t, g_hue1));   // 0x00457570
            US_PktU16(packet, g_colorEmoteB);   // 0x00457570
        } else {
            US_PktU16(packet, GLOBAL_AS(uint32_t, g_colorSay));   // 0x00457570
            US_PktU16(packet, g_colorSayFont);   // 0x00457570
        }
        US_PktU32(packet, lang);   // 0x004575f0
        if (len >= 0) {
            const uint16_t *s = text;
            int k = len + 1;
            do {
                US_PktU16(packet, *s);   // 0x00457570
                s++;
                k--;
            } while (k != 0);
        }
    }
    reinterpret_cast<CClientSocket *>(connection)->SendPacket(packet);   // 0x00478610
}
HOOK_METHOD(0x0047c730, CSpeechSender, SendUnicodeSpeech);
