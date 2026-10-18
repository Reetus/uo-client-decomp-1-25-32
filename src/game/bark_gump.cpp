#include "bark_gump.h"

#include <cstring>
#include <windows.h>

#include "../runtime/ported.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "text_list.h"
#include "gump.h"
#include "font_manager.h"
#include "vtables.h"

// Chat-line history state shared with the journal (0x006486b0 is the journal object).
#define g_journal GLOBAL_PTR(void, g_msgTarget_storage)

#define g_fontObj GLOBAL_PTR(void, g_fontMgr_storage)

// 0x004012a0  size 0x6ae
// CBarkGump constructor (stack args, RET 0x14). The SEH frame is omitted.
// Appends the text to the journal, picks a position from the speaker, builds a
// 16-bit bitmap of the wrapped text and links the gump into the bark lists.
CBarkGump::CBarkGump(CObj *speaker_, char *text_, uint16_t hue_, uint16_t font_, CGump *param5_)
{
    char buf[1024];
    int youSee;
    int measureOut;
    int rect[4];
    int maxWidth;
    int lines;

    youSee = 0;
    text = static_cast<char *>(::operator new(strlen(text_) + 1));
    strcpy(text, text_);
    unk_c0 = 0;
    hue = hue_;
    font = font_;
    unk_c8 = 0;
    unk_c4 = 0;
    unk_bc = 0;
    unk_d0 = 0;
    unk_cc = 0;

    buf[0] = '\0';
    if (g_barkState == 2) {
        strcpy(buf, g_barkPrefix);
        strcat(buf, g_barkPrefixSep);
    } else if (g_barkState == 3) {
        strcpy(buf, "You see: ");
        youSee = 1;
    }
    strcat(buf, text_);
    reinterpret_cast<CTextList *>(g_journal)->AddLineAndLog(buf, static_cast<int>(hue_), g_barkState == 1);
    g_barkState = 1;
    strcpy(g_barkPrefix, g_emptyString);

    name = const_cast<char *>("bark gump");
    int textLen = static_cast<int>(strlen(text_));
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    owner = nullptr;
    param5 = param5_;

    if (speaker_ == nullptr) {
        maxWidth = 0x140;
        older = g_barkListHead;
        newer = nullptr;
        if (g_barkListHead != nullptr)
            g_barkListHead->newer = this;
        speaker = nullptr;
        g_barkListHead = this;
        x = 0;
        y = 0x1cc;
    } else {
        maxWidth = 200;
        older = RAWF(CBarkGump *, speaker_, 0x68);
        newer = nullptr;
        if (RAWF(CBarkGump *, speaker_, 0x68) != nullptr)
            RAWF(CBarkGump *, speaker_, 0x68)->newer = this;
        speaker = speaker_;
        RAWF(CBarkGump *, speaker_, 0x68) = this;

        if (PORT_VSLOT0(speaker_, 0x2c, int)() != 0 && RAWF(void *, speaker_, 0x7c) != nullptr) {
            CGump *sib = RAWF(CGump *, RAWF(void *, speaker_, 0x7c), 0xac);
            if (sib == nullptr) {
                // No anchor gump for this speaker: delete the half-built bark.
                PORT_VSLOT1(this, 0x00, void, int)(1);
                return;
            }
            unk_a0 = 0;
            unk_9c = 0;
            sib->CGump::LocalToScreen(&unk_9c, &unk_a0);
            unk_9c += sib->unk_6c;
            unk_a0 += sib->unk_70;
            x = unk_9c;
            y = unk_a0;
        } else if (PORT_VSLOT0(speaker_, 0x24, int)() != 0) {
            unk_9c = ((RAWF(int32_t, speaker_, 0x0c) + RAWF(int32_t, speaker_, 0x04)) >> 1) - g_scrollX;
            unk_a0 = (RAWF(int32_t, speaker_, 0x08) - g_scrollY) + 4;
            unk_a0 = RAWF(int32_t, speaker_, 0x1cc) + unk_a0;
            x = unk_9c;
            y = unk_a0;
        } else {
            unk_9c = ((RAWF(int32_t, speaker_, 0x0c) + RAWF(int32_t, speaker_, 0x04)) >> 1) - g_scrollX;
            unk_a0 = (RAWF(int32_t, speaker_, 0x08) - g_scrollY) + 4;
            if (youSee != 0 && PORT_VSLOT0(speaker_, 0x54, int)() == 0) {
                x = g_screenGump->unk_6c;
                y = g_screenGump->unk_70;
            } else {
                x = unk_9c;
                y = unk_a0;
            }
        }
    }

    frameIndex = 0;
    frameCount = 1;
    frameData = FrameData_Alloc(g_frameDataAllocator, 8, "framedata for CBarkGump");
    frameData[0] = 0;
    frameData[1] = 0;

    PORT_THISCALL7(&CFontManager::MeasureTextWrapped, void, g_fontObj, int, const char *, int, int32_t *, int32_t *, int *, int)(static_cast<int>(font_), (textLen > 0) ? text_ : g_titlePart1, maxWidth, &width, &height,
        &measureOut, 0);

    int h = height;
    lines = h / 15;

    if (speaker_ != nullptr)
        x = x + -(width / 2);
    homeX = x;
    if (x + width > 0x27f)
        x = 0x27f - width;
    if (x < 0)
        x = 0;
    y = y - h;
    homeY = y;
    if (y + h > 0x1df)
        y = 0x1df - h;
    if (y < 0)
        y = 0;

    rowStride = width;
    uint8_t *bitmap = static_cast<uint8_t *>(::operator new(static_cast<size_t>(width) * h * 2 + 4));
    frameData[1] = static_cast<int32_t>(reinterpret_cast<uintptr_t>(bitmap));
    *reinterpret_cast<uint16_t *>(bitmap + 0) = static_cast<uint16_t>(width);
    *reinterpret_cast<uint16_t *>(bitmap + 2) = static_cast<uint16_t>(height);
    memset(bitmap + 4, 0, static_cast<size_t>(rowStride) * height * 2);

    rect[0] = 0;
    rect[1] = 0;
    rect[2] = rowStride;
    rect[3] = height;
    reinterpret_cast<CFontManager *>(g_fontObj)->DrawTextWrapped(0, 0, static_cast<int>(font), static_cast<int>(hue_), (char *)(text), rowStride, (uint16_t *)(bitmap + 4), rowStride, (const int *)(rect), 0, 0);

    // Push every older bark of this speaker up by this bark's height.
    for (CBarkGump *s = older; s != nullptr; s = s->older) {
        s->homeY -= height;
        s->y = s->homeY;
        if (s->homeY < 0)
            s->y = 0;
        if (s->height + s->y > 0x1e0)
            s->y = 0x1e0 - s->height;
    }

    this->CBarkGump::StackAndClamp();

    // Union rectangle of this bark and the older ones. It is computed but not used
    // further: the AddChild call below receives (0, 0).
    rect[0] = x;
    rect[1] = y;
    rect[2] = width + x;
    rect[3] = height + y;
    for (CBarkGump *s = older; s != nullptr; s = s->older) {
        if (s->x < rect[0])
            rect[0] = s->x;
        if (s->width + s->x > rect[2])
            rect[2] = s->width + s->x;
        if (s->y < rect[1])
            rect[1] = s->y;
        if (s->height + s->y > rect[3])
            rect[3] = s->height + s->y;
    }

    tickStart = static_cast<int32_t>(GetTickCount());
    if (g_barkMultiLine != 0)
        duration = (lines * g_barkSecondsPerLine * 4000) / 100;
    else
        duration = (g_barkSecondsPerLine * 4000) / 100;
    CGump_AddChild(this, nullptr, 0);
}
HOOK_CTOR(0x004012a0, CBarkGump, (CObj *, char *, uint16_t, uint16_t, CGump *));
