#include "book_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "font_manager.h"
#include "gump.h"
#include "input_control_gump.h"
#include "packet_builders_b25_ports.h"
#include "client_socket.h"
#include "vtables.h"

#define BOOK_PAGE_SIZE 0x648
#define BOOK_LINE_SIZE 0x50
#define g_packetBuilder0 GLOBAL_PTR(void, g_packetBuilder_storage)   // this for packet-building calls

namespace {

// Record of page N (1-based) and its line text.
inline char *Rec(const CBookGump *g, int p) { return g->pageRecords + (p - 1) * BOOK_PAGE_SIZE; }
inline char *LineOf(const CBookGump *g, int p, int line) { return Rec(g, p) + 2 + line * BOOK_LINE_SIZE; }
inline uint16_t &Count(const CBookGump *g, int p) { return *reinterpret_cast<uint16_t *>(Rec(g, p)); }
inline int32_t &Dirty(const CBookGump *g, int p) { return *reinterpret_cast<int32_t *>(Rec(g, p) + 0x644); }

inline void MeasureSample(int *w, int *h, int *d)
{
    reinterpret_cast<CFontManager *>(BOOK_FONT_OBJ)->MeasureText(4, (char *)(g_refText), w, h, d);
}
inline int WrapIndex(const char *text, int width)
{
    return reinterpret_cast<CFontManager *>(BOOK_FONT_OBJ)->FindWrapBreak(4, text, width);
}
inline void FocusGump(CGump *g) { PORT_THISCALL1(&Gump_SetFocus, void, g_gumpManager, CGump *)(g); }

} // namespace

// 0x00406c20  size 0x57b
// CBookGump constructor (stack args, RET 0x20). The SEH frame is omitted.
CBookGump::CBookGump(int posX, int posY, int artId, CObj *ownerObj, char *titleText, char *authorText,
                     uint16_t pageTotal, int isEditable)
{
    char pkt[0x640];
    int textW, textH, textD;

    name = const_cast<char *>("book gump");
    dirty = 0;
    owner = ownerObj;
    if (ownerObj != nullptr)
        RAWF(CBookGump *, ownerObj, 0x88) = this;
    y = posY;
    x = posX;
    unk_50 = -artId;
    unk_130 = 0;
    unk_12c = 0;
    unk_128 = 0;
    unk_124 = 0;
    unk_120 = 0;
    unk_11c = 0;
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    BOOK_ArtSize(artId, &width, &height);   // gump art size -> width, height
    rowStride = width;
    frameIndex = 0;
    frameCount = 1;
    frameData = FrameData_Alloc(g_frameDataAllocator, 8, "framedata for CCourseGump");
    memset(frameData, 0, frameCount * 8);
    frameData[1] = static_cast<int32_t>(
        reinterpret_cast<uintptr_t>(FrameData_Alloc(g_frameDataAllocator, width * height * 2 + 4, "course frame0")));
    uint8_t *bmp = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(frameData[1]));
    memset(bmp + 4, 0, width * height * 2);
    *reinterpret_cast<uint16_t *>(bmp + 0) = static_cast<uint16_t>(width);
    *reinterpret_cast<uint16_t *>(bmp + 2) = static_cast<uint16_t>(height);
    strcpy(title, titleText);
    strcpy(author, authorText);
    page = 0;
    pageCount = pageTotal;
    editable = isEditable;
    unk_a4 = 0;
    bufRight = nullptr;
    bufLeft = nullptr;

    if (isEditable != 0) {
        if (pageTotal > 0)
            pageRecords = static_cast<char *>(::operator new(pageTotal * BOOK_PAGE_SIZE));
        else
            pageRecords = nullptr;
        uint16_t p = 0;
        if (pageCount > 0) {
            do {
                int off = p * BOOK_PAGE_SIZE;
                *reinterpret_cast<uint16_t *>(pageRecords + off) = 0;
                *reinterpret_cast<int32_t *>(pageRecords + off + 0x644) = 0;
                for (int line = 0x14; line != 0; line--) {
                    strcpy(pageRecords + off + 2, g_emptyString);
                    off = off + BOOK_LINE_SIZE;
                }
                p = static_cast<uint16_t>(p + 1);
            } while (p < pageCount);
        }
        if (pageCount >= 1)
            bufRight = pageRecords + page * BOOK_PAGE_SIZE;
    } else {
        pageRecords = static_cast<char *>(::operator new(0xc90));   // two page records
        *reinterpret_cast<uint16_t *>(pageRecords + 0x000) = 0;
        *reinterpret_cast<uint16_t *>(pageRecords + 0x648) = 0;
        if (pageCount >= 1)
            bufRight = pageRecords + 0x648;
    }

    MeasureSample(&textW, &textH, &textD);

    void *ctl = new CInputControlGump(BOOK_LEFT_X, BOOK_LEFT_Y + 0x1e, this, static_cast<uint16_t>(BOOK_LEFT_R - BOOK_LEFT_X + 1), static_cast<uint16_t>(textH * 2 + 8), 0, 0x3b, 4, 0, 1, 0, 0);
    titleCtl = static_cast<CGump *>(ctl);
    strcpy(&RAWF(char, titleCtl, 0xec), title);
    RAWF(int32_t, titleCtl, 0xc8) = 1;
    RAWF(int32_t, titleCtl, 0xcc) = 1;
    reinterpret_cast<CInputControlGump *>(titleCtl)->Redraw();
    if (editable == 0)
        RAWF(int32_t, titleCtl, 0xb4) = 1;
    CGump_AddChild(titleCtl, this, 1);

    ctl = new CInputControlGump(BOOK_LEFT_X, BOOK_LEFT_Y + 0x7c, this, static_cast<uint16_t>(BOOK_LEFT_R - BOOK_LEFT_X + 1), static_cast<uint16_t>(textH + 4), 0, 0x1d, 4, 0, 1, 0, 0);
    authorCtl = static_cast<CGump *>(ctl);
    strcpy(&RAWF(char, authorCtl, 0xec), author);
    RAWF(int32_t, authorCtl, 0xcc) = 1;
    reinterpret_cast<CInputControlGump *>(authorCtl)->Redraw();
    if (editable == 0)
        RAWF(int32_t, authorCtl, 0xb4) = 1;
    CGump_AddChild(authorCtl, this, 1);

    this->CBookGump::RedrawSpread();

    if (editable == 0) {
        if (page != 0 && page <= pageCount) {
            PORT_THISCALL3(&Pkt_Build_0x66_BookPageRequest, void, g_packetBuilder0, char *, int, int)(pkt, RAWF(int32_t, owner, 0x78), page);
            reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(pkt));
        }
        if (static_cast<int>(page) + 1 <= static_cast<int>(pageCount)) {
            PORT_THISCALL3(&Pkt_Build_0x66_BookPageRequest, void, g_packetBuilder0, char *, int, int)(pkt, RAWF(int32_t, owner, 0x78),
                                                              page + 1);
            reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(pkt));
        }
    }
    BOOK_Music(BOOK_MUSIC_OBJ, 0x55, 0, 0, 0, 0, 0);
}
HOOK_CTOR(0x00406c20, CBookGump, (int, int, int, CObj *, char *, char *, uint16_t, int));

// 0x004071a0  size 0xd8
// ~CBookGump. The SEH frame is omitted (state -1 is set only before the base dtor call).
CBookGump::~CBookGump()
{
    this->CBookGump::CommitEdits();   // commit / send pending edits
    BOOK_Music(BOOK_MUSIC_OBJ, 0x55, 0, 0, 0, 0, 0);
    if (owner != nullptr)
        RAWF(int32_t, owner, 0x88) = 0;   // owner->bookGump = NULL
    if (pageRecords != nullptr) {
        operator delete(pageRecords);
        pageRecords = nullptr;
    }
    if (owner != nullptr) {
        if (PORT_VSLOT0(owner, 0x60, int)() != 0)
            RAWF(int32_t, owner, 0xb0) = 0;
    }
    if (frameData != nullptr) {
        int32_t bmp = frameData[1];
        if (bmp != 0) {
            BOOK_FrameFree(g_frameDataAllocator, reinterpret_cast<void *>(static_cast<uintptr_t>(bmp)), "book frame pBMP");
            frameData[1] = 0;
        }
        BOOK_FrameFree(g_frameDataAllocator, frameData, "book framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x004071a0, CBookGump);

// 0x00407ad0  size 0x483
// CBookGump::WrapLine(page, line, &wrapped): word-wrap one line of a page and push the
// overflow to the next line or the next page. Returns 1 if text was moved on.
int __thiscall CBookGump::WrapLine(uint16_t pageNo, uint32_t lineNo, int *outWrapped)
{
    char carry[0x50];
    int lh, w, d;

    char *rec = Rec(this, pageNo);
    *reinterpret_cast<int32_t *>(rec + 0x644) = 1;   // dirty
    *outWrapped = 0;
    MeasureSample(&w, &lh, &d);
    int line = static_cast<int>(lineNo & 0xffff);
    char *base = pageRecords;
    char *text = rec + 2 + line * BOOK_LINE_SIZE;

    int brk = WrapIndex(text, BOOK_LEFT_R - BOOK_LEFT_X);
    int split;
    if (brk == -1) {
        split = static_cast<int>(strlen(text));
    } else {
        split = brk + 1;
        if (text[split] == ' ')
            split = split + 1;
    }
    if (split >= static_cast<int>(strlen(text)))
        return 0;

    uint16_t *cnt = reinterpret_cast<uint16_t *>(rec);
    if (line == static_cast<int>(*cnt) - 1) {   // wrapping the last line of this page
        if ((*cnt + 1) * lh > BOOK_LEFT_B - BOOK_LEFT_Y) {   // no room for another line on this page
            if (pageNo == pageCount) {
                // last page of the book: truncate
                text[split] = '\0';
                if (cursorOnLeft != 0) {
                    if (pageNo != page)
                        return 0;
                } else {
                    if (pageNo != page + 1)
                        return 0;
                }
                if (curLine != line)
                    return 0;
                if (curCol < split)
                    return 0;
                curCol = split - 1;
                return 0;
            }
            // move overflow to the first line of the next page
            char *nextRec = base + pageNo * BOOK_PAGE_SIZE;
            if (*reinterpret_cast<uint16_t *>(nextRec) == 0)
                *reinterpret_cast<uint16_t *>(nextRec) = 1;
            strcpy(carry, text + split);
            strcat(carry, nextRec + 2);
            strcpy(nextRec + 2, carry);
            text[split] = '\0';
            *outWrapped = 1;
            if (cursorOnLeft != 0) {
                if (pageNo != page)
                    return 1;
            } else {
                if (pageNo != page + 1)
                    return 1;
            }
            if (curLine != line)
                return 1;
            if (curCol < split)
                return 1;
            if (cursorOnLeft != 0) {
                cursorOnLeft = 0;
            } else {
                PORT_VSLOT0(this, 0x118, void)();
                cursorOnLeft = 1;
                FocusGump(this);
                PORT_VSLOT0(this, 0x114, void)();
            }
            {
                int col = curCol;
                curLine = 0;
                curCol = col - split;
                if (curCol < 0)
                    curCol = 0;
            }
            return 1;
        }
        // room: add a new empty line at the end of this page
        *cnt = static_cast<uint16_t>(*cnt + 1);
        strcpy(rec + 2 + (*cnt - 1) * BOOK_LINE_SIZE, g_emptyString);
    }

    // push overflow text to the front of the following line on the same page
    char *nextLine = rec + 2 + (line + 1) * BOOK_LINE_SIZE;
    strcpy(carry, text + split);
    strcat(carry, nextLine);
    strcpy(nextLine, carry);
    text[split] = '\0';
    if (cursorOnLeft != 0) {
        if (pageNo != page)
            return 1;
    } else {
        if (pageNo != page + 1)
            return 1;
    }
    if (curLine != line)
        return 1;
    if (curCol < split)
        return 1;
    curCol = curCol - split;
    if (curCol < 0)
        curCol = 0;
    curLine = curLine + 1;
    return 1;
}
HOOK_METHOD(0x00407ad0, CBookGump, WrapLine);

// 0x00408ca0  size 0x15de
// CBookGump key handler (virtual slot 0x3c). Page numbers are 1-based; the left page is `page`,
// the right page is `page + 1`. 0xe021 / 0xe022 are PageUp / PageDown style moves to line 0 /
// the last line (and between the two pages of the spread).
// Text of line `line` of the left (onLeft != 0) or right page buffer: the line records are
// BOOK_LINE_SIZE bytes, a u16 header and then the characters.
static char *LineText(CBookGump *b, int onLeft, int line)
{
    return (onLeft != 0 ? b->bufLeft : b->bufRight) + line * BOOK_LINE_SIZE + 2;
}

// Clamp the caret column to the length of `p`, redraw (vtable slot 0x114); returns the length.
static int ClampColAndRedraw(CBookGump *b, char *p)
{
    int len = static_cast<int>(strlen(p));
    if (b->curCol > len)
        b->curCol = len;
    PORT_VSLOT0(b, 0x114, void)();
    return len;
}

// Tail shared by the Backspace paths: collapse an empty single-line buffer.
static void CollapseEmptyBuffer(char *bufRec)
{
    uint16_t *rec = reinterpret_cast<uint16_t *>(bufRec);
    if (*rec == 1 && strlen(reinterpret_cast<char *>(rec) + 2) == 0)
        *rec = 0;
}

void __thiscall CBookGump::OnKey(int key)
{
    int line_h;
    int unused18, unused14;
    int len;
    int e;
    char *ptr;

    if (key == 0xe02e) {   // Delete: Right, then Backspace, through the vtable
        PORT_VSLOT1(this, 0x3c, void, int)(0xe027);
        PORT_VSLOT1(this, 0x3c, void, int)(8);
        return;
    }

    if (key == 0xe021) {
        if (curLine > 0) {
            curLine = 0;
        } else {
            if (cursorOnLeft == 0) {
                if (page > 0)
                    cursorOnLeft = 1;
            } else if (page > 0) {
                cursorOnLeft = 0;
                PORT_VSLOT0(this, 0x11c, void)();
                FocusGump(this);
            }
        }
        e = (cursorOnLeft != 0) ? page : page + 1;
        ptr = LineOf(this, e, curLine);
        len = ClampColAndRedraw(this, ptr);
        return;
    }

    if (key == 0xe022) {
        unsigned short pg = page;
        e = (cursorOnLeft != 0) ? pg : pg + 1;
        int n = Count(this, e);
        if (curLine < n - 1) {
            curLine = n - 1;
        } else {
            bool flip;
            if (cursorOnLeft != 0)
                flip = pg < pageCount;
            else
                flip = static_cast<int>(pg) + 1 < static_cast<int>(pageCount);
            if (flip) {
                if (cursorOnLeft != 0) {
                    cursorOnLeft = 0;
                } else {
                    cursorOnLeft = 1;
                    PORT_VSLOT0(this, 0x118, void)();
                    FocusGump(this);
                }
            }
        }
        e = (cursorOnLeft != 0) ? page : page + 1;
        int last = Count(this, e) - 1;
        if (last < 0)
            last = 0;
        if (curLine > last)
            curLine = last;
        ptr = LineOf(this, e, curLine);
        len = ClampColAndRedraw(this, ptr);
        return;
    }

    if (key == 0xe024) {   // Home
        if (curCol > 0) {
            curCol = 0;
            PORT_VSLOT0(this, 0x114, void)();
        }
        return;
    }

    if (key == 0xe023) {   // End
        ptr = LineText(this, cursorOnLeft, curLine);
        len = static_cast<int>(strlen(ptr));
        if (curCol >= len)
            return;
        curCol = len;
        PORT_VSLOT0(this, 0x114, void)();
        return;
    }

    if (key == 0xe025) {   // Left
        if (curCol > 0) {
            curCol = curCol - 1;
            PORT_VSLOT0(this, 0x114, void)();
            return;
        }
        if (curLine <= 0) {
            PORT_VSLOT0(this, 0x114, void)();
            return;
        }
        curLine = curLine - 1;
        ptr = LineText(this, cursorOnLeft, curLine);
        len = static_cast<int>(strlen(ptr));
        curCol = len;
        PORT_VSLOT0(this, 0x114, void)();
        return;
    }

    if (key == 0xe027) {   // Right
        if (cursorOnLeft != 0) {
            ptr = LineText(this, 1, curLine);
            len = static_cast<int>(strlen(ptr));
            if (curCol < len) {
                curCol = curCol + 1;
                PORT_VSLOT0(this, 0x114, void)();
                return;
            }
            if (curLine >= *reinterpret_cast<uint16_t *>(bufLeft)) {
                PORT_VSLOT0(this, 0x114, void)();
                return;
            }
            curLine = curLine + 1;
            MeasureSample(&unused18, &line_h, &unused14);
            if ((curLine + 1) * line_h <= BOOK_LEFT_B - BOOK_LEFT_Y) {
                curCol = 0;
                PORT_VSLOT0(this, 0x114, void)();
                return;
            }
            curLine = curLine - 1;
            PORT_VSLOT0(this, 0x114, void)();
            return;
        }
        ptr = LineText(this, 0, curLine);
        len = static_cast<int>(strlen(ptr));
        if (curCol < len) {
            curCol = curCol + 1;
            PORT_VSLOT0(this, 0x114, void)();
            return;
        }
        if (curLine >= *reinterpret_cast<uint16_t *>(bufRight)) {
            PORT_VSLOT0(this, 0x114, void)();
            return;
        }
        curLine = curLine + 1;
        MeasureSample(&unused18, &line_h, &unused14);
        if ((curLine + 1) * line_h <= BOOK_RIGHT_B - BOOK_RIGHT_Y) {
            curCol = 0;
            PORT_VSLOT0(this, 0x114, void)();
            return;
        }
        curLine = curLine - 1;
        PORT_VSLOT0(this, 0x114, void)();
        return;
    }

    if (key == 0xe026) {   // Up
        if (curLine > 0) {
            curLine = curLine - 1;
            ptr = LineText(this, cursorOnLeft, curLine);
            len = ClampColAndRedraw(this, ptr);
            return;
        }
        if (cursorOnLeft != 0)
            return;
        cursorOnLeft = 1;
        curLine = *reinterpret_cast<uint16_t *>(bufLeft);
        MeasureSample(&unused18, &line_h, &unused14);
        if ((curLine + 1) * line_h > BOOK_LEFT_B - BOOK_LEFT_Y)
            curLine = curLine - 1;
        ptr = LineText(this, 1, curLine);
        len = ClampColAndRedraw(this, ptr);
        return;
    }

    if (key == 0xe028) {   // Down
        if (cursorOnLeft != 0) {
            if (curLine < *reinterpret_cast<uint16_t *>(bufLeft)) {
                curLine = curLine + 1;
                MeasureSample(&unused18, &line_h, &unused14);
                if ((curLine + 1) * line_h > BOOK_LEFT_B - BOOK_LEFT_Y) {
                    cursorOnLeft = 0;
                    curLine = 0;
                    ptr = bufRight + 2;
                } else {
                    ptr = bufLeft + curLine * BOOK_LINE_SIZE + 2;
                }
                len = ClampColAndRedraw(this, ptr);
                return;
            }
            if (page < pageCount) {
                cursorOnLeft = 0;
                curLine = 0;
                ptr = bufRight + 2;
                len = ClampColAndRedraw(this, ptr);
                return;
            }
            return;
        }
        if (curLine >= *reinterpret_cast<uint16_t *>(bufRight))
            return;
        curLine = curLine + 1;
        MeasureSample(&unused18, &line_h, &unused14);
        if ((curLine + 1) * line_h > BOOK_LEFT_B - BOOK_LEFT_Y) {
            curLine = curLine - 1;
            return;
        }
        ptr = bufRight + curLine * BOOK_LINE_SIZE + 2;
        len = ClampColAndRedraw(this, ptr);
        return;
    }

    if (key == 8) {   // Backspace
        bool rightTail = false;   // true: finish through the right-page tail (0x4094xx)
        if (curCol > 0) {
            char *s = LineText(this, cursorOnLeft, curLine);
            len = static_cast<int>(strlen(s));
            if (curCol < len) {
                int i = curCol - 1;
                int n = len - 1;
                if (i < n) {
                    do {
                        s[i] = s[i + 1];
                        i++;
                    } while (i < n);
                }
            }
            s[len - 1] = 0;
            curCol = curCol - 1;
            if (cursorOnLeft != 0) {
                CollapseEmptyBuffer(bufLeft);
                Dirty(this, page) = 1;
                PORT_VSLOT0(this, 0x114, void)();
                return;
            }
            rightTail = true;
        } else if (curLine > 0) {
            if (cursorOnLeft != 0) {
                char *cur = bufLeft + curLine * BOOK_LINE_SIZE;
                char *prevLine = cur - 0x4e;
                int prevlen = static_cast<int>(strlen(prevLine));
                strcat(prevLine, cur + 2);
                int r = WrapIndex(prevLine, BOOK_LEFT_R - BOOK_LEFT_X);
                cur = bufLeft + curLine * BOOK_LINE_SIZE;
                if (r == -1) {
                    e = static_cast<int>(strlen(cur - 0x4e));
                } else {
                    e = r + 1;
                    if (cur[e - 0x4e] == 0x20)
                        e++;
                }
                if (e < static_cast<int>(strlen(cur - 0x4e))) {
                    strcpy(cur + 2, cur - 0x4e + e);
                    bufLeft[curLine * BOOK_LINE_SIZE + e - 0x4e] = 0;
                } else {
                    if (this->CBookGump::RemoveLine(page, curLine) != 0) {
                        int p = page + 1;
                        if (this->CBookGump::RemoveLine(p, 0) != 0) {
                            do {
                                p++;
                            } while (this->CBookGump::RemoveLine(p, 0) != 0);
                        }
                    }
                }
                curCol = prevlen;
                curLine = curLine - 1;
                CollapseEmptyBuffer(bufLeft);
                Dirty(this, page) = 1;
                PORT_VSLOT0(this, 0x114, void)();
                return;
            }
            {
                char *cur = bufRight + curLine * BOOK_LINE_SIZE;
                char *prevLine = cur - 0x4e;
                int prevlen = static_cast<int>(strlen(prevLine));
                strcat(prevLine, cur + 2);
                int r = WrapIndex(bufRight + curLine * BOOK_LINE_SIZE - 0x4e, BOOK_LEFT_R - BOOK_LEFT_X);
                cur = bufRight + curLine * BOOK_LINE_SIZE;
                if (r == -1) {
                    e = static_cast<int>(strlen(cur - 0x4e));
                } else {
                    e = r + 1;
                    if (cur[e - 0x4e] == 0x20)
                        e++;
                }
                if (e < static_cast<int>(strlen(cur - 0x4e))) {
                    strcpy(cur + 2, cur - 0x4e + e);
                    bufRight[curLine * BOOK_LINE_SIZE + e - 0x4e] = 0;
                } else {
                    if (this->CBookGump::RemoveLine(page + 1, curLine) != 0) {
                        int p = page + 2;
                        if (this->CBookGump::RemoveLine(p, 0) != 0) {
                            do {
                                p++;
                            } while (this->CBookGump::RemoveLine(p, 0) != 0);
                        }
                    }
                }
                curCol = prevlen;
                curLine = curLine - 1;
            }
            rightTail = true;
        }

        if (rightTail) {
            CollapseEmptyBuffer(bufRight);
            Dirty(this, page + 1) = 1;
            PORT_VSLOT0(this, 0x114, void)();
            return;
        }

        // col <= 0 and line <= 0: merge with the last line of the previous page
        if (page == 0)
            return;
        int p = (cursorOnLeft != 0) ? page : page + 1;
        int count = Count(this, p - 1);
        int prevlen;
        if (count == 0)
            prevlen = 0;
        else
            prevlen = static_cast<int>(strlen(LineOf(this, p - 1, count - 1)));
        if (count == 0) {
            if (strlen(LineOf(this, p, curLine)) != 0)
                Count(this, p - 1) = 1;
        }
        strcat(LineOf(this, p - 1, Count(this, p - 1) - 1), LineOf(this, p, curLine));
        {
            char *joined = LineOf(this, p - 1, Count(this, p - 1) - 1);
            int r = WrapIndex(joined, BOOK_LEFT_R - BOOK_LEFT_X);
            if (r == -1) {
                e = static_cast<int>(strlen(joined));
            } else {
                e = r + 1;
                if (joined[e] == 0x20)
                    e++;
            }
            if (e < static_cast<int>(strlen(joined))) {
                strcpy(LineOf(this, p, curLine), joined + e);
                joined[e] = 0;
            } else {
                int q = p;
                if (this->CBookGump::RemoveLine(q, 0) != 0) {
                    do {
                        q++;
                    } while (this->CBookGump::RemoveLine(q, 0) != 0);
                }
            }
        }
        if (cursorOnLeft == 0) {
            if (page > 0) {
                cursorOnLeft = 1;
                int last = Count(this, page) - 1;
                if (last < 0)
                    last = 0;
                curCol = prevlen;
                curLine = last;
                PORT_VSLOT0(this, 0x114, void)();
                return;
            }
        } else {
            cursorOnLeft = 0;
            int last = Count(this, page - 1) - 1;
            if (last < 0)
                last = 0;
            curLine = last;
            PORT_VSLOT0(this, 0x11c, void)();
            FocusGump(this);
        }
        curCol = prevlen;
        PORT_VSLOT0(this, 0x114, void)();
        return;
    }

    if (key == 0xd) {   // Enter
        if (cursorOnLeft != 0) {
            (*reinterpret_cast<uint16_t *>(bufLeft))++;
            e = *reinterpret_cast<uint16_t *>(bufLeft) - 1;
            if (e >= curLine + 2) {
                int off = e * BOOK_LINE_SIZE;
                do {
                    char *s = bufLeft + off;
                    strcpy(s + 2, s - 0x4e);
                    e--;
                    off -= BOOK_LINE_SIZE;
                } while (e >= curLine + 2);
            }
            strcpy(bufLeft + curLine * BOOK_LINE_SIZE + 0x52, bufLeft + curLine * BOOK_LINE_SIZE + curCol + 2);
            bufLeft[curLine * BOOK_LINE_SIZE + curCol + 2] = 0;
            {
                unsigned int pg = page;
                if (this->CBookGump::OverflowPage(static_cast<int>(pg)) != 0) {
                    do {
                        pg++;
                    } while (this->CBookGump::OverflowPage(static_cast<int>(pg)) != 0);
                }
            }
            Dirty(this, page) = 1;
        } else {
            (*reinterpret_cast<uint16_t *>(bufRight))++;
            e = *reinterpret_cast<uint16_t *>(bufRight) - 1;
            if (e >= curLine + 2) {
                int off = e * BOOK_LINE_SIZE;
                do {
                    char *s = bufRight + off;
                    strcpy(s + 2, s - 0x4e);
                    e--;
                    off -= BOOK_LINE_SIZE;
                } while (e >= curLine + 2);
            }
            strcpy(bufRight + curLine * BOOK_LINE_SIZE + 0x52, bufRight + curLine * BOOK_LINE_SIZE + curCol + 2);
            bufRight[curLine * BOOK_LINE_SIZE + curCol + 2] = 0;
            {
                unsigned int pg = page + 1;
                if (this->CBookGump::OverflowPage(static_cast<int>(pg)) != 0) {
                    do {
                        pg++;
                    } while (this->CBookGump::OverflowPage(static_cast<int>(pg)) != 0);
                }
            }
            Dirty(this, page + 1) = 1;
        }
        curLine = curLine + 1;
        MeasureSample(&unused18, &line_h, &unused14);
        int zero = 0;
        if ((curLine + 1) * line_h > BOOK_LEFT_B - BOOK_LEFT_Y) {
            if (cursorOnLeft != 0) {
                if (page < pageCount) {
                    cursorOnLeft = 0;
                    curLine = 0;
                } else {
                    curLine = curLine - 1;
                }
            } else {
                if (static_cast<int>(page) + 1 < static_cast<int>(pageCount)) {
                    PORT_VSLOT0(this, 0x118, void)();
                    cursorOnLeft = 1;
                    FocusGump(this);
                    curLine = 0;
                } else {
                    curLine = curLine - 1;
                }
            }
        }
        curCol = zero;
        PORT_VSLOT0(this, 0x114, void)();
        return;
    }

    // any other key: typed character
    if (reinterpret_cast<CFontManager *>(BOOK_FONT_OBJ)->HasGlyph(4, key) == 0)
        return;

    if (cursorOnLeft != 0) {
        uint16_t *rec = reinterpret_cast<uint16_t *>(bufLeft);
        if (curLine == *rec)
            *rec = static_cast<uint16_t>(*rec + 1);
    } else {
        uint16_t *rec = reinterpret_cast<uint16_t *>(bufRight);
        if (curLine == *rec)
            *rec = static_cast<uint16_t>(*rec + 1);
    }

    char *buf = (cursorOnLeft != 0) ? bufLeft : bufRight;
    {
        char *s = buf + curLine * BOOK_LINE_SIZE;
        len = static_cast<int>(strlen(s + 2));
        if (curCol == len) {
            s[len + 2] = static_cast<char>(key);
            buf[curLine * BOOK_LINE_SIZE + len + 3] = 0;
        } else {
            char tmp[80];
            char *at = s + curCol + 2;
            strcpy(tmp, at);
            *at = static_cast<char>(key);
            buf[curLine * BOOK_LINE_SIZE + curCol + 3] = 0;
            strcat(buf + curLine * BOOK_LINE_SIZE + 2, tmp);
        }
    }
    {
        unsigned int pg = (cursorOnLeft != 0) ? page : page + 1;
        int ln = curLine;
        int flag;
        curCol = curCol + 1;
        if (WrapLine(static_cast<uint16_t>(pg), static_cast<uint32_t>(ln), &flag) != 0) {
            do {
                if (flag != 0) {
                    pg++;
                    ln = 0;
                } else {
                    ln++;
                }
            } while (WrapLine(static_cast<uint16_t>(pg), static_cast<uint32_t>(ln), &flag) != 0);
        }
    }
    Dirty(this, (cursorOnLeft != 0) ? page : page + 1) = 1;
    PORT_VSLOT0(this, 0x114, void)();
}
HOOK_VMETHOD(0x00408ca0, CBookGump, OnKey);
