// Unicode speech helpers of batch 30: the bark list owner (a CObj seen only through its bark list head) and
// the object that sends the Unicode speech packet 0xAD.
#pragma once

#include <cstddef>
#include <cstdint>

#include "bark_gump.h"
#include "unicode_bark_gump.h"

// A CObj as the receiver of the Unicode bark methods; only the bark list head (+0x68) is touched.
class CBarkOwner {
public:
    // 0x0047c540: RET 0x14, (text, hue, font, extra, param5). Deletes all but the newest barks so that at most
    // two remain, then creates a Unicode bark gump for this speaker. Returns the new gump (0 if allocation fails).
    CUnicodeBarkGump *__thiscall AddUnicodeBarkLimitTwo(uint16_t *text, int hue, int font, int extra, CGump *param5);
    // 0x0047c600: RET 0x14, same arguments; deletes every existing bark first.
    CUnicodeBarkGump *__thiscall AddUnicodeBarkReplaceAll(uint16_t *text, int hue, int font, int extra, CGump *param5);

    uint8_t pad_00[0x68];
    CBarkGump *barkList;   // +0x68
};

static_assert(offsetof(CBarkOwner, barkList) == 0x68, "CBarkOwner::barkList");

// The object whose +4 is the connection (CApp, at 0x0069e0e8, when called from the text entry).
class CSpeechSender {
public:
    // 0x0047c730: RET 0xc, (text, length (not read), mode). Trims trailing spaces of `text` in place, then sends
    // packet 0xAD; mode 2 (emote) wraps the text in '*'.
    void __thiscall SendUnicodeSpeech(uint16_t *text, int length, int mode);

    uint8_t pad_00[4];
    void *connection;   // +0x04
};

static_assert(offsetof(CSpeechSender, connection) == 0x04, "CSpeechSender::connection");
