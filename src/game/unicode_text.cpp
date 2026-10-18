// Unicode speech packet handler and word-wrapped unifont text (measure / draw).
#include "unicode_text.h"

#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"
#include "crt.h"
#include "packet_buffer_ops_ports.h"
#include "object_manager.h"
#include "app_support_ports.h"
#include "gump_lookup.h"
#include "unicode_speech.h"
#include "unicode_speech_ops_ports.h"
#include "unicode_font_ports.h"
#include "packet_builders_b25_ports.h"
#include "client_socket.h"
#include "text_entry.h"

// --- globals -------------------------------------------------------------------------------
#define g_speechSystemObj GLOBAL_PTR(void, g_msgTarget_storage)
#define g_objectTable GLOBAL_PTR(void, g_objMgr_storage)
#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)

// --- original helpers ----------------------------------------------------------------------
// packet reader helpers: ECX = packet, EDX = &cursor, destination (and max length) on the stack
#define Packet_ReadDword(a1, a2, a3) (PacketBuf_ReadU32((const uint8_t *)(a1), (uint32_t *)(a2), (uint32_t *)(a3)))
#define Packet_ReadWord(a1, a2, a3) (PacketBuf_ReadU16((const uint8_t *)(a1), (uint32_t *)(a2), (uint16_t *)(a3)))
#define Packet_ReadByte(a1, a2, a3) (PacketBuf_ReadU8((const uint8_t *)(a1), (uint32_t *)(a2), (uint8_t *)(a3)))
#define Packet_ReadString(a1, a2, a3, a4) (PacketBuf_ReadPtr((const uint8_t *)(a1), (uint32_t *)(a2), (const uint8_t **)(a3), a4))

#define ObjectTable_Find(self, a1) ((void *)reinterpret_cast<CObjectManager *>(self)->FindBySerial((uint32_t)(a1)))       // find object by serial
#define Serial_IsIgnored(a1) (App_IsInList_0069e8b0(a1))   // ECX = serial
#define Obj_GetBarkColorFlag PORT_FUNC1(&CGump_00436f70::FindOpenGumpForTarget, int, __fastcall, void *)   // ECX = object
#define Obj_AddBark(self, a1, a2, a3, a4, a5) (reinterpret_cast<CBarkOwner *>(self)->AddUnicodeBarkLimitTwo(a1, a2, a3, a4, (CGump *)(a5)))      // keeps at most 2 barks
#define Obj_AddBarkReplace(self, a1, a2, a3, a4, a5) (reinterpret_cast<CBarkOwner *>(self)->AddUnicodeBarkReplaceAll(a1, a2, a3, a4, (CGump *)(a5)))   // deletes all barks first
#define System_AddBark(o, a1, a2, a3, a4) PORT_THISCALL4(&CreateUnicodeBarkGump, void, (o), int, int, uint16_t *, int)(a1, a2, a3, a4)
#define TextFilter_Apply(o) PORT_THISCALL0(&FilterUnicodeText, void, (o))()               // in-place filter of a wide string, ECX = string

// glyph lookup: ECX = code point, EDX = &xoffset, stack &yoffset, &width, &height; returns glyph pointer or 0
#define Unifont_GetGlyph(a1, a2, a3, a4, a5) ((int)UnifontGetGlyph((uint16_t)(a1), a2, a3, a4, a5))
#define Unifont_Load() (LoadUnifont())
// draw one line: ECX = dest, EDX = pitch, stack: str, x, y, color, clip
#define Unifont_DrawLine(a1, a2, a3, a4, a5, a6, a7) (DrawUnicodeString(a1, a2, (const uint16_t *)(a3), a4, a5, a6, (const int *)(a7)))

// 0x0047c1f0  size 0x2b3
// Packet 0xAE handler (Unicode speech, server -> client). One stack arg = the packet.
void __stdcall HandleUnicodeSpeechPacket(void *packet)
{
    int cursor = 0;
    int serial = 0;
    int lang = 0;
    uint32_t font = 0;
    uint32_t hue = 0;
    char *name = nullptr;     // allocated by the string reader
    uint32_t type = 0;
    uint16_t *text = nullptr;   // allocated by the string reader (max 0x800 wide chars)
    uint32_t model = 0;
    int i;
    void *obj;
    int r;

    Packet_ReadDword(packet, &cursor, &serial);
    Packet_ReadWord(packet, &cursor, &model);   // model: written into the argument slot and ignored
    Packet_ReadByte(packet, &cursor, &type);
    Packet_ReadWord(packet, &cursor, &hue);
    Packet_ReadWord(packet, &cursor, &font);
    Packet_ReadDword(packet, &cursor, &lang);
    Packet_ReadString(packet, &cursor, &name, 0x1e);
    Packet_ReadString(packet, &cursor, &text, 0x800);

    // ntohs over the text
    i = 0;
    while (*(uint16_t *)((char *)text + i) != 0) {
        uint16_t w = *(uint16_t *)((char *)text + i);
        w = (uint16_t)((w << 8) | (w >> 8));
        i += 2;
        *(uint16_t *)((char *)text + i - 2) = w;
        if (i >= 0x1000)
            break;
    }

    if (name == nullptr)
        return;
    if (text == nullptr)
        return;
    if (g_suppressSpeech != 0)
        return;
    if (g_profanityOn != 0)
        TextFilter_Apply(text);   // ECX = text (still in ECX from the null test)

    // ? hue and font keep whatever bytes were in the stack slots above the word the reader wrote;
    // ? here they are zero-extended. The 0x0047c4d0 calls mask them with 0xffff, the 0x0047c540/0x0047c600 calls do not.
    switch (type & 0xff) {
    case 0:
    case 2:
    case 3:
    case 4:
    case 8:
    case 9:
        obj = ObjectTable_Find(g_objectTable, serial);
        if (obj != nullptr) {
            if (Serial_IsIgnored(serial) != 0)
                return;
            strcpy(g_barkPrefix, name);
            g_barkState = 2;
            Obj_AddBark(obj, text, (int)hue, (int)font, lang, 0);
            return;
        }
        System_AddBark(g_speechSystemObj, (int)(hue & 0xffff), (int)(font & 0xffff), text, lang);
        return;

    case 6:
        obj = nullptr;
        if (serial != -1)
            obj = ObjectTable_Find(g_objectTable, serial);
        if (obj != nullptr) {
            if (Serial_IsIgnored(serial) != 0)
                return;
            strcpy(g_barkPrefix, name);
            g_barkState = 3;
            r = Obj_GetBarkColorFlag(obj);
            Obj_AddBark(obj, text, (int)hue, (int)font, lang, r);
            return;
        }
        System_AddBark(g_speechSystemObj, (int)(hue & 0xffff), (int)(font & 0xffff), text, lang);
        return;

    case 7:
        obj = ObjectTable_Find(g_objectTable, serial);
        if (obj != nullptr) {
            r = Serial_IsIgnored(serial);
            if (r != 0)
                return;
            strcpy(g_barkPrefix, name);
            g_barkState = 2;
            Obj_AddBarkReplace(obj, text, (int)hue, (int)font, lang, r);   // r is 0 here
            return;
        }
        System_AddBark(g_speechSystemObj, (int)(hue & 0xffff), (int)(font & 0xffff), text, lang);
        return;

    default:   // 1, 5, > 9
        return;
    }
}
HOOK_FUNC(0x0047c1f0, HandleUnicodeSpeechPacket);

// 0x0047c690  size 0x9e
// Send a unicode string as an 8-bit speech packet. 2 stack args (RET 8).
void __thiscall CSpeechInput::SendAsSpeechPacket(int speechMode, uint16_t *text)
{
    char narrow[0x800];
    char packet[0x10000];
    char *out;

    out = narrow;
    while (*text != 0) {
        if (*text < 0x80) {
            *out = (char)*text;
            out = out + 1;
        }
        text = text + 1;
    }
    *out = 0;

    // thiscall on the packet builder: (packet, DAT_005285d0, DAT_005285d4, speechMode, text)
    PORT_THISCALL5(&Pkt_Build_0x9A_PromptReply, void, g_packetBuilder, char *, int, int, int, char *)(packet, g_promptSerialA, g_promptSerialB, speechMode, narrow);
    // thiscall on *(0x0069e0ec): (packet)
    reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(packet));
    g_promptReply = 0;
    unk_08 = 0;
    reinterpret_cast<CTextEntry *>(this)->SetMode(0);
    reinterpret_cast<CTextEntry *>(this)->RefreshLine();
}
HOOK_METHOD(0x0047c690, CSpeechInput, SendAsSpeechPacket);

// 0x0047cd70  size 0x33c
// Measure a word-wrapped unicode string: ECX = string, EDX = max width, stack out-pointers
// width, height, zero (RET 0xc). Lines advance by 0x12.
void __fastcall MeasureWrappedUnicode(uint16_t *str, int maxWidth, int *outWidth, int *outHeight, int *outZero)
{
    int widest = 0;
    int height = 0;
    int zero = 0;
    int accum;
    int lastSpace;
    int breakPos;
    int spaceFlag;
    int len;
    int i;
    uint16_t *buf;
    uint16_t *cur;
    uint16_t line[0xe8];
    int xoff, yoff, width, glyphH;
    int adv;
    uintptr_t tbl;
    uint16_t ch;
    int rest;

    if (str == nullptr)
        return;

    len = 0;
    while (str[len] != 0)
        len++;
    buf = (uint16_t *)::operator new(len * 2 + 2);
    {
        uint16_t *d = buf;
        uint16_t *s = str;
        while (*s != 0) {
            *d = *s;
            d++;
            s++;
        }
        *d = 0;
    }

    for (;;) {
        accum = 0;
        lastSpace = -1;
        len = 0;
        while (buf[len] != 0)
            len++;
        breakPos = -1;
        cur = buf;
        for (i = 0; i < len; i++) {
            if (*cur == 0x20) {
                adv = 8;
            } else {
                Unifont_GetGlyph(*cur, &xoff, &yoff, &width, &glyphH);
                adv = width + xoff + 1;
            }
            accum += adv;
            if (*cur == 0x20 && i > 1)
                lastSpace = i - 1;
            if (accum > maxWidth) {
                if (lastSpace != -1)
                    breakPos = lastSpace;
                else
                    breakPos = i - 1;
                break;
            }
            cur++;
        }

        if (breakPos != -1) {
            int n = breakPos + 1;
            spaceFlag = (buf[n] == 0x20) ? 1 : 0;
            memcpy(line, buf, n * 2);
            line[n] = 0;

            zero = 0;
            uint16_t *p = line;
            ch = line[0];
            adv = 0;
            if (ch != 0) {
                do {
                    int have;
                    if (ch == 0x20)
                        have = 1;
                    else
                        have = (Unifont_GetGlyph(ch, &xoff, &yoff, &width, &glyphH) != 0);
                    if (have) {
                        if (ch == 0x20) {
                            adv = 8;
                        } else {
                            Unifont_GetGlyph(ch, &xoff, &yoff, &width, &glyphH);
                            adv = width + xoff + 1;
                        }
                        zero += adv;   // [ebp-8] accumulates the line width
                    }
                    p++;
                    ch = *p;
                } while (ch != 0);
                adv = zero;
            } else {
                adv = 0;
            }
            if (adv > widest)
                widest = adv;
            height += 0x12;
            zero = 0;

            len = 0;
            while (buf[len] != 0)
                len++;
            rest = len - spaceFlag - n;
            i = 0;
            if (rest > 0) {
                uint16_t *src = buf + (n + spaceFlag);
                uint16_t *dst = buf;
                int cnt = rest;
                i = rest;
                do {
                    *dst = *src;
                    src++;
                    dst++;
                    cnt--;
                } while (cnt != 0);
            }
            buf[i] = 0;
        }

        if (breakPos == -1)
            break;
    }

    // last line
    cur = buf;
    accum = 0;
    if (*cur != 0) {
        do {
            ch = *cur;
            tbl = GLOBAL_AS(uintptr_t, g_unifontTable);
            int have;
            if (ch == 0x20) {
                have = 1;
            } else {
                if (g_unifontLoaded == 0) {
                    Unifont_Load();
                    tbl = GLOBAL_AS(uintptr_t, g_unifontTable);
                }
                int ofs = 0;
                if (tbl != 0)
                    ofs = *(int *)(tbl + ch * 4);
                uintptr_t g;
                if (tbl == 0 || ofs == 0)
                    g = 0;
                else
                    g = ofs + tbl + 4;
                have = (g != 0);
            }
            if (have) {
                ch = *cur;
                if (ch == 0x20) {
                    adv = 8;
                } else {
                    if (g_unifontLoaded == 0) {
                        Unifont_Load();
                        tbl = GLOBAL_AS(uintptr_t, g_unifontTable);
                    }
                    int ofs2 = 0;
                    if (tbl != 0)
                        ofs2 = *(int *)(tbl + ch * 4);
                    if (tbl == 0 || ofs2 == 0) {
                        xoff = 0;
                        width = 0;
                    } else {
                        xoff = *(char *)(ofs2 + tbl);
                        width = *(char *)(ofs2 + tbl + 2);
                    }
                    adv = width + 1 + xoff;
                }
                accum += adv;
            }
            cur++;
        } while (*cur != 0);
    }
    if (accum > widest)
        widest = accum;
    if (zero == 0)
        zero = 0;
    *outWidth = widest;
    *outHeight = height + 0x12;
    *outZero = zero;
    operator delete(buf);
}
HOOK_FUNC(0x0047cd70, MeasureWrappedUnicode);

// 0x0047d0b0  size 0x375
// Draw a word-wrapped unicode string, one Unifont_DrawLine call per line. ECX = x, EDX = y,
// stack: colorIndex, str, maxWidth, dest, pitch, clip (RET 0x18). Lines advance by 0x12.
void __fastcall DrawWrappedUnicode(int x, int y, int colorIndex, uint16_t *str, int maxWidth,
                                          uint16_t *dest, int pitch, int *clip)
{
    int y0 = y;
    int xx = x;
    int linesH = 0;
    int curY;
    int accum;
    int lastSpace;
    int breakPos;
    int spaceFlag;
    int len;
    int i;
    int n;
    uint16_t *buf;
    uint16_t *cur;
    uint16_t line[0xe8];
    int xoff, yoff, width, glyphH;
    int adv;
    uintptr_t tbl;
    uint16_t ch;
    int rest;
    int sum;   // computed by the original but never used

    if (str == nullptr)
        return;

    len = 0;
    while (str[len] != 0)
        len++;
    buf = (uint16_t *)::operator new(len * 2 + 2);
    {
        uint16_t *d = buf;
        uint16_t *s = str;
        while (*s != 0) {
            *d = *s;
            d++;
            s++;
        }
        *d = 0;
    }
    if (g_profanityOn != 0 && g_inputRedrawing == 0)
        TextFilter_Apply(buf);
    curY = y;

    for (;;) {
        lastSpace = -1;
        accum = 0;
        len = 0;
        while (buf[len] != 0)
            len++;
        breakPos = -1;
        cur = buf;
        for (i = 0; i < len; i++) {
            if (*cur == 0x20) {
                adv = 8;
            } else {
                Unifont_GetGlyph(*cur, &xoff, &yoff, &width, &glyphH);
                adv = width + xoff + 1;
            }
            accum += adv;
            if (*cur == 0x20 && i > 1)
                lastSpace = i - 1;
            if (accum > maxWidth) {
                if (lastSpace != -1)
                    breakPos = lastSpace;
                else
                    breakPos = i - 1;
                break;
            }
            cur++;
        }

        if (breakPos != -1) {
            n = breakPos + 1;
            spaceFlag = (buf[n] == 0x20) ? 1 : 0;
            memcpy(line, buf, n * 2);
            line[n] = 0;

            sum = 0;
            cur = line;
            if (line[0] != 0) {
                do {
                    int have;
                    if (*cur == 0x20)
                        have = 1;
                    else
                        have = (Unifont_GetGlyph(*cur, &xoff, &yoff, &width, &glyphH) != 0);
                    if (have) {
                        if (*cur == 0x20) {
                            adv = 8;
                        } else {
                            Unifont_GetGlyph(*cur, &xoff, &yoff, &width, &glyphH);
                            adv = width + xoff + 1;
                        }
                        sum += adv;
                    }
                    cur++;
                } while (*cur != 0);
            }

            Unifont_DrawLine(dest, pitch, line, xx, curY, *(uint16_t *)(g_fontHueTable + colorIndex * 0x58), clip);
            linesH += 0x12;
            curY += 0x12;

            len = 0;
            while (buf[len] != 0)
                len++;
            rest = len - n - spaceFlag;
            i = 0;
            if (rest > 0) {
                uint16_t *src = buf + (n + spaceFlag);
                uint16_t *dst = buf;
                int cnt = rest;
                i = rest;
                do {
                    *dst = *src;
                    src++;
                    dst++;
                    cnt--;
                } while (cnt != 0);
            }
            buf[i] = 0;
        }

        if (breakPos == -1)
            break;
    }

    // last line
    cur = buf;
    sum = 0;
    if (*cur != 0) {
        do {
            ch = *cur;
            tbl = GLOBAL_AS(uintptr_t, g_unifontTable);
            int have;
            if (ch == 0x20) {
                have = 1;
            } else {
                if (g_unifontLoaded == 0) {
                    Unifont_Load();
                    tbl = GLOBAL_AS(uintptr_t, g_unifontTable);
                }
                int ofs = 0;
                if (tbl != 0)
                    ofs = *(int *)(tbl + ch * 4);
                uintptr_t g;
                if (tbl == 0 || ofs == 0)
                    g = 0;
                else
                    g = ofs + tbl + 4;
                have = (g != 0);
            }
            if (have) {
                ch = *cur;
                if (ch == 0x20) {
                    adv = 8;
                } else {
                    if (g_unifontLoaded == 0) {
                        Unifont_Load();
                        tbl = GLOBAL_AS(uintptr_t, g_unifontTable);
                    }
                    int ofs2 = 0;
                    if (tbl != 0)
                        ofs2 = *(int *)(tbl + ch * 4);
                    if (tbl == 0 || ofs2 == 0) {
                        xoff = 0;
                        width = 0;
                    } else {
                        xoff = *(char *)(ofs2 + tbl);
                        width = *(char *)(ofs2 + tbl + 2);
                    }
                    adv = width + 1 + xoff;
                }
                sum += adv;
            }
            cur++;
        } while (*cur != 0);
    }
    (void)sum;
    Unifont_DrawLine(dest, pitch, buf, xx, linesH + y0, *(uint16_t *)(g_fontHueTable + colorIndex * 0x58), clip);
    operator delete(buf);
}
HOOK_FUNC(0x0047d0b0, DrawWrappedUnicode);
