// CTextList (text history lines) and the text log file writers (0x00478e80-0x00479400).
// (Porting pass 2, batch 29.)
#include "text_list.h"

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "font_manager.h"
#include "crt.h"
#include "raw_field.h"
#include "scroll_thumb_gump.h"
#include "unicode_font_ports.h"
#include "unicode_speech_ops_ports.h"
#include "text_gump.h"
#include "vtables.h"
#include "globals.h"

#define g_textListFrozen GLOBAL_AS(int32_t, g_labelFlag)     // non-zero: AddLine*AndLog does nothing
#define TL_GumpIsText 0x94                                 // CGump vtable slot: non-zero for a CTextGump


// 0x0047cc80: ECX = text, EDX = &width; stack (&height, &yOffset), RET 8.
#define TL_MeasureUnicode(a1, a2, a3, a4) (MeasureUnicodeString(a1, a2, a3, a4))
// Register-convention string helpers (ECX = destination or source, EDX = source).
#define TL_wcslen PORT_FUNC1(&WcsLenThunk, int, __fastcall, const uint16_t *)               // wcslen_reg
#define TL_wcscpy(a1, a2) (WcsCpy(a1, a2))  // wcscpy_reg
#define TL_widen(a1, a2) (WidenStrCpy(a1, a2))       // widen_strcpy_reg
#define TL_narrow(a1, a2) (NarrowStrCpy(a1, a2))      // narrow_strcpy_reg

// 0x00478e80  size 0x1b
CTextList::CTextList()
{
    tail = nullptr;
    head = nullptr;
    count = 0;
    flag10 = 1;
    flag11 = 0;
}
HOOK_CTOR(0x00478e80, CTextList, ());

// 0x00478ea0  size 0x4e  scalar deleting destructor: free every line (text, then node), delete the list if flags & 1
inline CTextList::~CTextList()
{
    CTextLine *line = head;
    while (line != nullptr) {
        void *text = line->text;
        CTextLine *nextLine = line->next;
        if (text != nullptr)
            operator delete(text);
        operator delete(line);
        line = nextLine;
    }
}
HOOK_VTABLE(0x004a0838, CTextList);
HOOK_SDD(0x00478ea0, CTextList);   // slot 0

// 0x00478ef0  size 0x33
// The destructor as the atexit thunk 0x0044ab20 jumps to it (ECX = object): the inline destructor, out of line.
void __fastcall TextList_Dtor_00478ef0(void *self, void *)
{
    reinterpret_cast<CTextList *>(self)->CTextList::~CTextList();
}
HOOK_FUNC(0x00478ef0, TextList_Dtor_00478ef0);

// 0x00478f30  size 0x13c  append a line: when 100 lines are held the oldest is dropped (and every text gump that had it
// as its first shown line moves on to the next); the new line records the measured height and baseline of font 9.
void __thiscall CTextList::AddLine(char *text, int hue, int arg)
{
    int outW;
    int outH;
    int outBase;

    if (count == 100) {
        CGump *g = g_gumpListTail;
        while (g != nullptr) {
            if (PORT_VSLOT0(g, TL_GumpIsText, int)() != 0) {
                CTextGump *tg = static_cast<CTextGump *>(g);
                if (tg->cur == head)
                    tg->cur = tg->cur->next;
            }
            g = g->prev;
        }
        CTextLine *old = head;
        head = old->next;
        head->prev = nullptr;
        if (old->text != nullptr)
            operator delete(old->text);
        operator delete(old);
        count--;
    }

    g_fontMgr.MeasureText(9, text, &outW, &outH, &outBase);   // 0x004327b0
    CTextLine *line = static_cast<CTextLine *>(::operator new(0x20));
    char *copy = static_cast<char *>(::operator new(strlen(text) + 1));
    line->text = copy;
    line->isUnicode = 0;
    strcpy(copy, text);
    line->hue = hue;
    RAWF(int32_t, line, 0x0c) = outBase;
    RAWF(int32_t, line, 0x08) = outH;
    RAWF(int32_t, line, 0x14) = arg;
    line->next = nullptr;
    line->prev = tail;
    if (head != nullptr) {
        tail->next = line;
        count = count + 1;
        tail = line;
        return;
    }
    count = count + 1;
    tail = line;
    head = line;
}
HOOK_METHOD(0x00478f30, CTextList, AddLine);

// 0x00479070  size 0x117  as AddLine for a Unicode line (measured by 0x0047cc80, copied with 0x0047c060 / 0x0047c080)
void __thiscall CTextList::AddLineUnicode(uint16_t *text, int hue, int arg)
{
    int outW;
    int outH;
    int outBase;

    if (count == 100) {
        CGump *g = g_gumpListTail;
        while (g != nullptr) {
            if (PORT_VSLOT0(g, TL_GumpIsText, int)() != 0) {
                CTextGump *tg = static_cast<CTextGump *>(g);
                if (tg->cur == head)
                    tg->cur = tg->cur->next;
            }
            g = g->prev;
        }
        CTextLine *old = head;
        head = old->next;
        head->prev = nullptr;
        if (old->text != nullptr)
            operator delete(old->text);
        operator delete(old);
        count--;
    }

    TL_MeasureUnicode(text, &outW, &outH, &outBase);   // 0x0047cc80
    CTextLine *line = static_cast<CTextLine *>(::operator new(0x20));
    int n = TL_wcslen(text);   // 0x0047c060
    uint16_t *copy = static_cast<uint16_t *>(::operator new(static_cast<size_t>(n) * 2 + 2));
    line->text = copy;
    line->isUnicode = 1;
    TL_wcscpy(copy, text);   // 0x0047c080
    line->hue = hue;
    RAWF(int32_t, line, 0x0c) = outBase;
    RAWF(int32_t, line, 0x08) = outH;
    RAWF(int32_t, line, 0x14) = arg;
    line->next = nullptr;
    line->prev = tail;
    if (head != nullptr) {
        tail->next = line;
        count = count + 1;
        tail = line;
        return;
    }
    count = count + 1;
    tail = line;
    head = line;
}
HOOK_METHOD(0x00479070, CTextList, AddLineUnicode);

// 0x00479190  size 0xfc  ECX = text: append it to the log file (opened "a+" on first use) as UTF-16 lines when the
// log is wide (g_logWide, or g_unicodeText when that is -1), else as "%s\n". The UTF-16 line end is 0x000a, 0x000d.
void __fastcall LogTextToFile(const char *text)
{
    if (GLOBAL_AS(const char *, g_textLogName) == nullptr)
        return;
    FILE *file = g_textLogFile;
    if (file == nullptr) {
        file = fopen(GLOBAL_AS(const char *, g_textLogName), "a+");   // 0x0048ad40
        g_textLogFile = file;
        if (file == nullptr)
            return;
    }
    int32_t wide = g_logWide;
    if (wide == -1)
        wide = g_useWideWindow;
    if (wide != 0) {
        if (text[0] != 0) {
            uint16_t *w = static_cast<uint16_t *>(::operator new(strlen(text) * 2 + 2));
            TL_widen(w, text);   // 0x0047c0a0
            fwrite(w, 2, strlen(text), g_textLogFile);   // 0x0048ba70
            operator delete(w);
            file = g_textLogFile;
        }
        uint32_t ch = 0x0a;
        fwrite(&ch, 2, 1, file);
        ch = 0x0d;
        fwrite(&ch, 2, 1, g_textLogFile);
        return;
    }
    fprintf(file, "%s\n", text);   // 0x0048bce0
}
HOOK_FUNC(0x00479190, LogTextToFile);

// 0x00479290  size 0xe9  ECX = Unicode text: as LogTextToFile; a narrow log gets a copy narrowed by 0x0047c120
void __fastcall LogUnicodeTextToFile(const uint16_t *text)
{
    if (GLOBAL_AS(const char *, g_textLogName) == nullptr)
        return;
    if (g_textLogFile == nullptr) {
        g_textLogFile = fopen(GLOBAL_AS(const char *, g_textLogName), "a+");   // 0x0048ad40
        if (g_textLogFile == nullptr)
            return;
    }
    int32_t wide = g_logWide;
    if (wide == -1)
        wide = g_useWideWindow;
    if (wide != 0) {
        if (text[0] != 0) {
            fwrite(text, 2, static_cast<size_t>(TL_wcslen(text)), g_textLogFile);   // 0x0047c060, 0x0048ba70
        }
        uint32_t ch = 0x0a;
        fwrite(&ch, 2, 1, g_textLogFile);
        ch = 0x0d;
        fwrite(&ch, 2, 1, g_textLogFile);
        return;
    }
    int n = TL_wcslen(text);   // 0x0047c060
    char *narrow = static_cast<char *>(::operator new(static_cast<size_t>(n) + 1));
    TL_narrow(narrow, text);   // 0x0047c120
    fprintf(g_textLogFile, "%s\n", narrow);   // 0x0048bce0
    operator delete(narrow);
}
HOOK_FUNC(0x00479290, LogUnicodeTextToFile);

// 0x00479380  size 0x71  add a Unicode line (arg 0), log it, then bring every text gump up to date: scroll to the end
// when its child at +0xc4 has frame 0, re-place the thumb and repaint. Does nothing while g_textListFrozen is set.
void __thiscall CTextList::AddLineUnicodeAndLog(uint16_t *text, int hue, int unused)
{
    (void)unused;
    if (g_textListFrozen != 0)
        return;
    AddLineUnicode(text, hue, 0);   // 0x00479070
    LogUnicodeTextToFile(text);     // 0x00479290
    CGump *g = g_gumpListTail;
    while (g != nullptr) {
        if (PORT_VSLOT0(g, TL_GumpIsText, int)() != 0) {
            CTextGump *tg = static_cast<CTextGump *>(g);
            if (tg->childC4->frameIndex == 0)
                tg->CTextGump::ScrollToEnd();   // ScrollToEnd
            static_cast<CScrollThumbGump *>(tg->childC0)->PlaceFromTextList();   // 0x00413b20
            tg->CTextGump::RedrawText(1, 1);   // RedrawText
        }
        g = g->prev;
    }
}
HOOK_METHOD(0x00479380, CTextList, AddLineUnicodeAndLog);

// 0x00479400  size 0x71  as AddLineUnicodeAndLog for a narrow line
void __thiscall CTextList::AddLineAndLog(char *text, int hue, int unused)
{
    (void)unused;
    if (g_textListFrozen != 0)
        return;
    AddLine(text, hue, 0);   // 0x00478f30
    LogTextToFile(text);     // 0x00479190
    CGump *g = g_gumpListTail;
    while (g != nullptr) {
        if (PORT_VSLOT0(g, TL_GumpIsText, int)() != 0) {
            CTextGump *tg = static_cast<CTextGump *>(g);
            if (tg->childC4->frameIndex == 0)
                tg->CTextGump::ScrollToEnd();   // ScrollToEnd
            static_cast<CScrollThumbGump *>(tg->childC0)->PlaceFromTextList();   // 0x00413b20
            tg->CTextGump::RedrawText(1, 1);   // RedrawText
        }
        g = g->prev;
    }
}
HOOK_METHOD(0x00479400, CTextList, AddLineAndLog);
