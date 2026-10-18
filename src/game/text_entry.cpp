// CTextEntry methods. 0x00442910, 0x00442cb0, 0x004430b0, 0x00443590.
//
// Shared by the narrow and wide (unicode) paths: the buffer at +0x0c holds
// chars, or 16-bit chars when the global unicode flag is set.
#include "text_entry.h"

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "client_socket.h"
#include "unicode_speech_ops_ports.h"
#include "input_gump.h"
#include "packet_builders_b25_ports.h"
#include "bark_gump.h"
#include "packet_builders_batch24_ports.h"
#include "client_helpers_31_ports.h"
#include "unicode_text.h"
#include "client_misc_ops_ports.h"
#include "unicode_font_ports.h"
#include "font_manager.h"
#include "unicode_speech.h"

#define g_hideCursor GLOBAL_AS(int, g_focusedInput)        // nonzero: no trailing underscore
#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)
#define g_fontObject GLOBAL_PTR(void, g_fontMgr_storage)
#define g_playerBytes GLOBAL_AS(uint8_t *, g_player)
// Input line colours.

static void send_net(const void *pkt) { reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(pkt)); }

// ---------------------------------------------------------------------------
// 0x00443590  size 0xa4b
// Refresh the on-screen input line from the typed buffer, with a mode prefix.
// The original has one copy of this block per mode; they differ only in the
// prefix string and colours, so the blocks are one helper here. SEH frame omitted.
// ---------------------------------------------------------------------------
static void show_line(CTextEntry *t, const char *prefix, uint32_t c1, uint32_t c2)
{
    if (g_useWideWindow != 0) {
        // wide buffer = prefix + typed text
        void *src = t->buffer;
        int wlen = PORT_FUNC1(&WcsLenThunk, int, __fastcall, void *)(src);   // wide strlen
        void *wtext = ::operator new((size_t)(wlen + (int)strlen(prefix)) * 2 + 2);
        WidenStrCpy((uint16_t *)(wtext), prefix);   // narrow -> wide copy
        WcsCat((uint16_t *)(wtext), (const uint16_t *)(src));            // wide append
        if (t->inputGump != nullptr) {
            PORT_THISCALL3(&CInputGump::RedrawUnicode, void, t->inputGump, void *, uint32_t, uint32_t)(wtext, c1, c2);
            operator delete(wtext);
        } else {
            void *gump = new CInputGump(wtext, c1, c2, 1);
            t->inputGump = gump;
            operator delete(wtext);
        }
    } else {
        // new char[strlen(prefix) + strlen(buffer) + 1]; strcpy; strcat
        char *text = static_cast<char *>(::operator new(strlen(prefix) + strlen(reinterpret_cast<char *>(t->buffer)) + 1));
        strcpy(text, prefix);
        strcat(text, reinterpret_cast<char *>(t->buffer));
        if (t->inputGump != nullptr) {
            PORT_THISCALL3(&CInputGump::Redraw, void, t->inputGump, char *, uint32_t, uint32_t)(text, c1, c2);
            operator delete(text);
        } else {
            void *gump = new CInputGump(text, c1, c2, 0);
            t->inputGump = gump;
            operator delete(text);
        }
    }
}

void __thiscall CTextEntry::RefreshLine()
{
    uint16_t *wide = reinterpret_cast<uint16_t *>(buffer);
    char *narrow = reinterpret_cast<char *>(buffer);

    // cursor: an underscore appended unless the hide flag is set
    if (g_hideCursor != 0) {
        if (g_useWideWindow != 0)
            wide[length] = 0;
        else
            narrow[length] = 0;
    } else {
        if (g_useWideWindow != 0) {
            wide[length] = '_';
            wide[length + 1] = 0;
        } else {
            narrow[length] = '_';
            narrow[length + 1] = 0;
        }
    }

    int m = mode;
    if (m == 2) {
        show_line(this, "Emote: ", g_hue1, g_colorEmoteB);
    } else if (m == 8) {
        show_line(this, "Whisper: ", g_colorSay, g_colorSayFont);
    } else if (m == 9) {
        show_line(this, "Yell: ", g_colorSay, g_colorSayFont);
    } else if (m == 5) {
        show_line(this, "Command: ", 0x3b2, g_colorSayFont);
    } else if (m == 1) {
        show_line(this, "Broadcast: ", 0x3b2, g_colorSayFont);
    } else {
        // no prefix: show the buffer directly
        if (g_useWideWindow != 0) {
            if (inputGump != nullptr) {
                reinterpret_cast<CInputGump *>(inputGump)->RedrawUnicode((uint16_t *)(buffer), (uint16_t)(g_colorSay), (uint16_t)(g_colorSayFont));
            } else {
                void *gump = new CInputGump(buffer, g_colorSay, g_colorSayFont, 1);
                inputGump = gump;
            }
        } else {
            if (inputGump != nullptr) {
                reinterpret_cast<CInputGump *>(inputGump)->Redraw((char *)(buffer), (uint16_t)(g_colorSay), (uint16_t)(g_colorSayFont));
            } else {
                void *gump = new CInputGump(buffer, g_colorSay, g_colorSayFont, 0);
                inputGump = gump;
            }
        }
    }

    // drop the cursor again
    if (g_useWideWindow != 0)
        wide[length] = 0;
    else
        narrow[length] = 0;
}
HOOK_METHOD(0x00443590, CTextEntry, RefreshLine);

// ---------------------------------------------------------------------------
// 0x00442910  size 0x66
// Format a text line into a 64K stack buffer, send it, reset the text entry.
// ---------------------------------------------------------------------------
void __thiscall CTextEntry::SendLine(int arg1, int arg2)
{
    char buf[0x10000];

    PORT_THISCALL5(&Pkt_Build_0x9A_PromptReply, void, g_packetBuilder, char *, int, int, int, int)(buf, g_promptSerialA, g_promptSerialB, arg1, arg2);
    reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(buf));

    g_promptReply = 0;
    length = 0;
    mode = 0;
    RefreshLine();
}
HOOK_METHOD(0x00442910, CTextEntry, SendLine);

// ---------------------------------------------------------------------------
// 0x00442cb0  size 0x30d
// Try the typed line as a dot-command; returns 1 if consumed. The command
// table at 0x004a8c20 holds {id, name} pairs: 0x0a "who", 0x0b "where",
// 0x14 "query", 0xc9 "fullscreen", 0xc7 "action", 0xca "helpme",
// 0xcb "version", 0xcc "a"; the end id is -1. SEH frame omitted.
// ---------------------------------------------------------------------------
static void echo_bark(const char *msg)
{
    g_barkState = 1;   // a byte (mov byte ptr [0x6486c0], bl); the name the bark gump shows starts right after it
    new CBarkGump(nullptr, (char *)(msg), 0, 0, nullptr);   // 0x004012a0
}

int __thiscall CTextEntry::TryDotCommand()
{
    char text[0x100];
    char word[0x100];   // first word, lowercased
    char msg[0x200];
    char pkt1[0x100];
    char pkt2[0x100];   // ? 4 bytes in the pass2 frame; the packet size of 0x00462840 is not known
    const char *rest;
    int idx;
    int *entry;

    if (g_useWideWindow == 0) {
        buffer[length] = 0;
        strcpy(text, reinterpret_cast<char *>(buffer));
    } else {
        reinterpret_cast<uint16_t *>(buffer)[length] = 0;
        NarrowStrCpy((char *)(text), (const uint16_t *)(buffer));   // wide -> narrow
    }

    rest = text;
    char *w = word;
    if (*rest == ' ') {
        do {
            rest = rest + 1;
        } while (*rest == ' ');
    }
    while (*rest != ' ') {
        if (*rest == '\0')
            break;
        char c = *rest;
        if (c >= 'A' && c <= 'Z')
            c = (char)(c + 0x20);
        *w = c;
        w = w + 1;
        rest = rest + 1;
    }
    *w = 0;
    if (*rest != 0)
        rest = rest + 1;

    idx = 0;
    if (g_dotCommandTable[0] == -1)
        return 0;

    entry = g_dotCommandTable;
    for (;;) {
        if (strcmp(word, reinterpret_cast<const char *>(entry[1])) == 0) {
            if (echoCommands != 0) {
                sprintf(msg, ".%s %s", reinterpret_cast<const char *>(g_dotCommandTable[idx * 2 + 1]), rest);
                echo_bark(msg);
            }

            switch (g_dotCommandTable[idx * 2]) {
            case 0xb:   // "where"
                sprintf(msg, "You are at: %d %d %d in %s.",
                        (int)RAWF(int16_t, g_playerBytes, 0x20),
                        (int)RAWF(int16_t, g_playerBytes, 0x22),
                        (int)RAWF(int16_t, g_playerBytes, 0x24),
                        g_worldName);
                echo_bark(msg);
                return 1;

            case 0xc7: {   // "action"
                int len = (int)strlen(rest);
                // ? the second argument is (len & 0xffffff00) | id: pass2 notes the upper bits are leftover len bits
                PORT_THISCALL4(&Build_Packet_0x12, void, g_packetBuilder, char *, int, const char *, int)(pkt1, (len & 0xffffff00) | (g_dotCommandTable[idx * 2] & 0xff), rest, len);
                send_net(pkt1);
                return 1;
            }

            case 0xca:   // "helpme"
                if (g_pktSender != nullptr) {
                    PORT_THISCALL3(&Build_Packet_0x9B, void, g_packetBuilder, char *, int, const char *)(pkt2, 0, rest);
                    send_net(pkt2);
                    return 1;
                }
                return 1;

            case 0xcb:   // "version"
                PrintVersion();
                return 1;

            default:
                return 1;
            }
        }
        idx = idx + 1;
        entry = entry + 2;
        if (*entry == -1)
            return 0;
    }
}
HOOK_METHOD(0x00442cb0, CTextEntry, TryDotCommand);

// ---------------------------------------------------------------------------
// 0x004430b0  size 0x466
// Handle one typed character. Jump table 0x00443518 (index c - 8, c <= 0x2e):
// 8 backspace; 0xa, 0xd Enter; 0x20 space; 0x2b, 0x2d plus/minus; 0x2e dot;
// others default. Jump table 0x00443560 (first char - 0x21, <= 0x3b): '!' mode 0,
// apostrophe mode 2, ':' mode 8, ';' mode 9, others store a space at [1].
// ---------------------------------------------------------------------------
void __thiscall CTextEntry::HandleChar(int ch)
{
    char sendbuf[0x10000];
    int w, h1, h2;
    int unicode;
    int first;
    uint16_t *wide = reinterpret_cast<uint16_t *>(buffer);
    char *narrow = reinterpret_cast<char *>(buffer);

    if (g_inputActive == 0)
        return;

    if (g_entrySkipNext != 0) {
        g_entrySkipNext = 0;
        return;
    }

    if ((unsigned int)(ch - 8) > 0x26)
        goto default_char;

    switch ((unsigned int)(ch - 8)) {
    case 0x00:   // 8: backspace
        if (length <= 0)
            return;
        length = length - 1;
        if (length != 0) {
            RefreshLine();
            return;
        }
        mode = 0;
        RefreshLine();
        return;

    case 0x02:   // 0xa
    case 0x05:   // 0xd: Enter
        if (length <= 0) {
            mode = 0;
            RefreshLine();
            return;
        }
        if (g_useWideWindow != 0) {
            wide[length] = 0;
            WcsCpy((uint16_t *)(lastLine), (const uint16_t *)(buffer));   // remember last line (wide)
        } else {
            narrow[length] = 0;
            strcpy(reinterpret_cast<char *>(lastLine), narrow);
        }
        lastMode = mode;

        if (g_promptReply == 1) {
            if (g_useWideWindow != 0) {
                reinterpret_cast<CSpeechInput *>(this)->SendAsSpeechPacket(1, (uint16_t *)(buffer));
                length = 0;
                mode = 0;
                RefreshLine();
                return;
            }
            PORT_THISCALL5(&Pkt_Build_0x9A_PromptReply, void, g_packetBuilder, char *, int, int, int, void *)(sendbuf, g_promptSerialA, g_promptSerialB, 1, buffer);
            send_net(sendbuf);
            g_promptReply = 0;
            length = 0;
            mode = 0;
            RefreshLine();
            length = 0;
            mode = 0;
            RefreshLine();
            return;
        }

        if (mode == 5) {
            TryDotCommand();
            length = 0;
            mode = 0;
            RefreshLine();
            return;
        }

        if (g_useWideWindow != 0) {
            reinterpret_cast<CSpeechSender *>(&g_app)->SendUnicodeSpeech((uint16_t *)(buffer), length, mode);
            length = 0;
            mode = 0;
            RefreshLine();
            return;
        }
        PORT_THISCALL3(&SendSpeechText, void, &g_app, void *, int, int)(buffer, length, mode);
        length = 0;
        mode = 0;
        RefreshLine();
        return;

    case 0x18:   // 0x20: space
        if (length == 1) {
            unicode = g_useWideWindow;
            if (unicode != 0)
                first = wide[0];
            else
                first = (short)(signed char)narrow[0];
            first = (first & 0xffff) - 0x21;
            if ((unsigned int)first > 0x1a)
                goto space_store_second;

            switch (first) {
            case 0x00:   // '!'
                mode = 0;
                length = 0;
                RefreshLine();
                return;
            case 0x06:   // apostrophe
                mode = 2;
                length = 0;
                RefreshLine();
                return;
            case 0x19:   // ':'
                mode = 8;
                length = 0;
                RefreshLine();
                return;
            case 0x1a:   // ';'
                mode = 9;
                length = 0;
                RefreshLine();
                return;
            default:
            space_store_second:
                if (unicode != 0)
                    wide[1] = 0x20;
                else
                    narrow[1] = 0x20;
                goto advance;
            }
        }
        if (length >= 0x80)
            return;
        if (g_useWideWindow != 0)
            MeasureUnicodeString((const uint16_t *)(buffer), &w, &h1, &h2);
        else
            reinterpret_cast<CFontManager *>(g_fontObject)->MeasureText(3, narrow, &w, &h1, &h2);
        if (w >= 0x258)
            return;
        if (g_useWideWindow != 0)
            wide[length] = 0x20;
        else
            narrow[length] = 0x20;
        goto advance;

    case 0x23:   // 0x2b plus
    case 0x25:   // 0x2d minus
        if (unk_04 != 0) {
            unk_04 = unk_04 - 1;
            return;
        }
        goto default_char;

    case 0x26:   // 0x2e dot
        if (length >= 0x80)
            return;
        if (g_useWideWindow != 0)
            MeasureUnicodeString((const uint16_t *)(buffer), &w, &h1, &h2);
        else
            reinterpret_cast<CFontManager *>(g_fontObject)->MeasureText(3, narrow, &w, &h1, &h2);
        if (w >= 0x258)
            return;
        if (g_useWideWindow != 0)
            wide[length] = 0x2e;
        else
            narrow[length] = 0x2e;
        goto advance;

    default:
        goto default_char;
    }

default_char:
    if (length >= 0x80)
        return;
    if (g_useWideWindow != 0) {
        if (UnifontHasGlyph((uint16_t)(ch)) == 0)
            return;
        MeasureUnicodeString((const uint16_t *)(buffer), &w, &h1, &h2);
        if (w >= 0x258)
            return;
        wide[length] = (uint16_t)ch;
    } else {
        if (reinterpret_cast<CFontManager *>(g_fontObject)->HasGlyph(3, ch) == 0)
            return;
        reinterpret_cast<CFontManager *>(g_fontObject)->MeasureText(3, narrow, &w, &h1, &h2);
        if (w >= 0x258)
            return;
        narrow[length] = (char)ch;
    }

advance:
    length = length + 1;
    RefreshLine();
}
HOOK_METHOD(0x004430b0, CTextEntry, HandleChar);
