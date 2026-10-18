// CBookGump: default constructor, page turning, edit commit, page overflow / line removal and the
// spread redraw (0x00406ae0-0x00407f60). The remaining CBookGump methods are in book_gump.cpp and
// book_gump_ui.cpp.
#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "book_gump.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "packet_builders_b25_ports.h"
#include "font_manager.h"
#include "client_socket.h"
#include "packet_builders_batch24_ports.h"
#include "vtables.h"

#define BP_PAGE_SIZE 0x648
#define BP_LINE_SIZE 0x50
#define g_packetBuilderBP GLOBAL_PTR(void, g_packetBuilder_storage)   // this for packet-building calls
// 0x004641e0 (build "request page" packet 0x66) and 0x00478610 (send), both thiscall with a stack buffer.
#define BP_RequestPage(pkt, pg) \
    PORT_THISCALL3(&Pkt_Build_0x66_BookPageRequest, void, g_packetBuilderBP, char *, int, uint16_t)(pkt, RAWF(int32_t, owner, 0x78), pg)
// 0x004327b0 on the font object: measure the sample text "Aq" in font 4 -> (width, line height, descent).
#define MeasureSampleBP(w, h, d) \
    PORT_THISCALL5(&CFontManager::MeasureText, void, gfx::textObj(), int, const char *, int *, int *, int *)(4, "Aq", w, h, d)
#define BP_Send(pkt) PORT_THISCALL1(&CClientSocket::SendPacket, void, g_pktSender, char *)(pkt)

namespace {

inline uint16_t *Bmp(int32_t slot) { return reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(slot))); }
} // namespace

// 0x00406ae0  size 0x101
// CBookGump default constructor: base ctor, vtable, every field cleared, empty title and author.
CBookGump::CBookGump()
{
    name = const_cast<char *>("book gump");
    dirty = 0;
    owner = nullptr;
    x = 0;
    y = 0;
    unk_130 = 0;
    unk_12c = 0;
    unk_128 = 0;
    unk_124 = 0;
    unk_120 = 0;
    unk_11c = 0;
    unk_60 = 0;
    unk_50 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    frameIndex = 0;
    frameCount = 0;
    page = 0;
    pageCount = 0;
    pageRecords = nullptr;
    bufRight = nullptr;
    bufLeft = nullptr;
    unk_a4 = 0;
    editable = 0;
    strcpy(title, g_emptyString);
    strcpy(author, g_emptyString);
    authorCtl = nullptr;
    titleCtl = nullptr;
}
HOOK_CTOR(0x00406ae0, CBookGump, ());

HOOK_VTABLE(0x0049c728, CBookGump);
HOOK_SDD(0x00406bf0, CBookGump);

// 0x00407280  size 0x18c
// CBookGump::NextPage (virtual slot 0x118). Moves to the next spread (page += 2) when the right page is
// not the last one. A read-only book asks the server for the two new pages and blanks its buffers; an
// editable one points the buffers at the page records and redraws. Plays sound 0x55.
void __thiscall CBookGump::NextPage()
{
    char pkt[0x650];

    CGump_BringToFront(this, 0);
    if (static_cast<int>(page) + 1 < static_cast<int>(pageCount)) {
        CommitEdits();
        page = static_cast<uint16_t>(page + 2);
        titleCtl->visible = 0;
        authorCtl->visible = 0;
        if (editable == 0) {
            BP_RequestPage(pkt, page);
            BP_Send(pkt);
            if (static_cast<int>(page) + 1 <= static_cast<int>(pageCount)) {
                BP_RequestPage(pkt, page + 1);
                BP_Send(pkt);
            }
            bufLeft = pageRecords;
            *reinterpret_cast<uint16_t *>(bufLeft) = 0;
            if (static_cast<int>(page) + 1 <= static_cast<int>(pageCount))
                bufRight = pageRecords + BP_PAGE_SIZE;
            else
                bufRight = nullptr;
            if (bufRight != nullptr)
                *reinterpret_cast<uint16_t *>(bufRight) = 0;
        } else {
            char *rec = pageRecords + page * BP_PAGE_SIZE;
            bufLeft = rec - BP_PAGE_SIZE;
            if (static_cast<int>(page) + 1 <= static_cast<int>(pageCount))
                bufRight = rec;
            else
                bufRight = nullptr;
            PORT_VSLOT0(this, 0x114, void)();   // Redraw
        }
        BOOK_Music(BOOK_MUSIC_OBJ, 0x55, 0, 0, 0, 0, 0);
    }
}
HOOK_VMETHOD(0x00407280, CBookGump, NextPage);

// 0x00407410  size 0x1de
// CBookGump::PrevPage. Mirror of NextPage (page -= 2); landing on the cover shows the title and author
// controls again.
void __thiscall CBookGump::PrevPage()
{
    char pkt[0x650];

    CGump_BringToFront(this, 0);
    if (!(page > 0))
        return;
    CommitEdits();
    page = static_cast<uint16_t>(page - 2);
    if (page == 0) {
        titleCtl->visible = 1;
        authorCtl->visible = 1;
    }
    if (editable == 0) {
        if (page > 0 && page <= pageCount) {
            BP_RequestPage(pkt, page);
            BP_Send(pkt);
        }
        if (static_cast<int>(page) + 1 <= static_cast<int>(pageCount)) {
            BP_RequestPage(pkt, page + 1);
            BP_Send(pkt);
        }
        if (page == 0)
            bufLeft = nullptr;
        else
            bufLeft = pageRecords;
        if (static_cast<int>(page) + 1 <= static_cast<int>(pageCount))
            bufRight = pageRecords + BP_PAGE_SIZE;
        else
            bufRight = nullptr;
        if (bufLeft != nullptr)
            *reinterpret_cast<uint16_t *>(bufLeft) = 0;
        if (bufRight != nullptr)
            *reinterpret_cast<uint16_t *>(bufRight) = 0;
    } else {
        if (page > 0)
            bufLeft = pageRecords + page * BP_PAGE_SIZE - BP_PAGE_SIZE;
        else
            bufLeft = nullptr;
        if (static_cast<int>(page) + 1 <= static_cast<int>(pageCount))
            bufRight = pageRecords + page * BP_PAGE_SIZE;
        else
            bufRight = nullptr;
        PORT_VSLOT0(this, 0x114, void)();   // Redraw
    }
    BOOK_Music(BOOK_MUSIC_OBJ, 0x55, 0, 0, 0, 0, 0);
}
HOOK_VMETHOD(0x00407410, CBookGump, PrevPage);

// 0x004075f0  size 0x13a
// CBookGump::CommitEdits. For an editable book that has an owner: when the header changed, copy the title and
// author out of the edit controls and send packet 0x93; then send packet 0x66 for every page record whose
// dirty flag (+0x644) is set and clear the flag.
void __thiscall CBookGump::CommitEdits()
{
    char hdrPkt[0x68];
    char pagePkt[0x650];

    if (editable == 0)
        return;
    if (owner == nullptr)
        return;
    if (unk_a4 != 0) {
        strcpy(title, &RAWF(char, titleCtl, 0xec));
        strcpy(author, &RAWF(char, authorCtl, 0xec));
        // 0x00464010: build the book header packet 0x93 (owner serial, title, author).
        PORT_THISCALL4(&Build_Packet_0x93, void, g_packetBuilderBP, char *, int, char *, char *)(hdrPkt,
                                                                   RAWF(int32_t, owner, 0x78), title, author);
        BP_Send(hdrPkt);
        unk_a4 = 0;
    }
    uint32_t p = 1;
    if (pageCount >= 1) {
        do {
            if (*reinterpret_cast<int32_t *>(&pageRecords[(p & 0xffff) * BP_PAGE_SIZE - 4]) != 0) {
                // 0x00464360: build the page-write packet 0x66 (this, page number).
                PORT_THISCALL3(&Pkt_Build_0x66_BookPageSend, void, g_packetBuilderBP, char *, CBookGump *, int)(pagePkt, this,
                                                                         static_cast<int>(p));
                BP_Send(pagePkt);
                *reinterpret_cast<int32_t *>(&pageRecords[(p & 0xffff) * BP_PAGE_SIZE - 4]) = 0;
            }
            p++;
        } while (static_cast<uint16_t>(p) <= pageCount);
    }
}
HOOK_METHOD(0x004075f0, CBookGump, CommitEdits);

// 0x00407730  size 0x19b
// CBookGump::OverflowPage. Marks page `pageNo` dirty. When its lines no longer fit in the text box (line
// height * count > bottom - top), the last line is dropped from it and, if a next page exists, pushed in as
// that page's first line (the next page's lines shift down); returns 1. On the last page the dropped line
// is blanked and 0 is returned.
int __thiscall CBookGump::OverflowPage(int pageArg)
{
    int w, lh, d;
    int off = (pageArg & 0xffff) * BP_PAGE_SIZE;
    char *rec = pageRecords + off - BP_PAGE_SIZE;

    *reinterpret_cast<int32_t *>(pageRecords + off - 4) = 1;
    MeasureSampleBP(&w, &lh, &d);
    uint16_t cnt = *reinterpret_cast<uint16_t *>(rec);
    if (lh * cnt <= BOOK_LEFT_B - BOOK_LEFT_Y)
        return 0;
    *reinterpret_cast<uint16_t *>(rec) = static_cast<uint16_t>(cnt - 1);
    if (static_cast<uint16_t>(pageArg) < pageCount) {
        char *nextRec = pageRecords + off;
        *reinterpret_cast<uint16_t *>(nextRec) = static_cast<uint16_t>(*reinterpret_cast<uint16_t *>(nextRec) + 1);
        int i = *reinterpret_cast<uint16_t *>(nextRec) - 1;
        if (i >= 1) {
            do {
                strcpy(nextRec + 2 + i * BP_LINE_SIZE, nextRec + 2 + (i - 1) * BP_LINE_SIZE);
                i--;
            } while (i != 0);
        }
        strcpy(nextRec + 2, rec + 2 + *reinterpret_cast<uint16_t *>(rec) * BP_LINE_SIZE);
        return 1;
    }
    strcpy(rec + 2 + *reinterpret_cast<uint16_t *>(rec) * BP_LINE_SIZE, g_emptyString);
    return 0;
}
HOOK_METHOD(0x00407730, CBookGump, OverflowPage);

// 0x004078d0  size 0x1f7
// CBookGump::RemoveLine. Marks the page dirty and shifts the lines after `lineNo` up by one. `needPull` is
// whether the page would have been over-full with one more line; when the removed line was inside the page
// and a next page has lines, the next page's first line is moved into the freed last slot (and returned as
// 1); otherwise the last slot is blanked and the page's line count drops by one.
int __thiscall CBookGump::RemoveLine(int pageArg, int lineArg)
{
    int w, lh, d;
    int off = (pageArg & 0xffff) * BP_PAGE_SIZE;
    char *rec = pageRecords + off - BP_PAGE_SIZE;

    *reinterpret_cast<int32_t *>(pageRecords + off - 4) = 1;
    MeasureSampleBP(&w, &lh, &d);
    int cnt = *reinterpret_cast<uint16_t *>(rec);
    int needPull = ((cnt + 1) * lh > BOOK_LEFT_B - BOOK_LEFT_Y) ? 1 : 0;
    int ln = lineArg & 0xffff;
    if (ln < cnt - 1) {
        do {
            strcpy(rec + 2 + ln * BP_LINE_SIZE, rec + 2 + (ln + 1) * BP_LINE_SIZE);
            ln++;
        } while (ln < *reinterpret_cast<uint16_t *>(rec) - 1);
    }
    uint16_t now = *reinterpret_cast<uint16_t *>(rec);
    if (static_cast<uint16_t>(lineArg) >= now)
        return needPull;
    char *nextRec = pageRecords + off;
    if (needPull != 0 && static_cast<uint16_t>(pageArg) < pageCount && *reinterpret_cast<uint16_t *>(nextRec) > 0) {
        strcpy(rec + 2 + (now - 1) * BP_LINE_SIZE, nextRec + 2);
        strcpy(nextRec + 2, g_emptyString);
        return needPull;
    }
    strcpy(rec + 2 + (now - 1) * BP_LINE_SIZE, g_emptyString);
    *reinterpret_cast<uint16_t *>(rec) = static_cast<uint16_t>(*reinterpret_cast<uint16_t *>(rec) - 1);
    return 0;
}
HOOK_METHOD(0x004078d0, CBookGump, RemoveLine);

// 0x00407f60  size 0x5ae
// CBookGump::RedrawSpread (virtual slot 0x114). Repaints the frame: left page art (-unk_50), the second art
// when not on the cover, the right page art when a right page exists, then on the cover the "TITLE" and
// "by" labels, the text lines of both pages (font 4, hue 0x12) while they fit above the text box
// bottom, a "_" caret (hue 8) when this gump has the focus, and the two page numbers (font 9, hue 0x386).
void __thiscall CBookGump::RedrawSpread()
{
    int v04;
    int i;
    int v0c, v10;
    int rect[4];
    char num[16];

    rect[0] = 0;
    rect[1] = 0;
    rect[2] = width;
    rect[3] = height;

    uint16_t *bmp = Bmp(frameData[1]);
    gfx::DrawArt(gfx::artPlain(), -unk_50, 0, 0, bmp + 2, bmp[0], bmp[1]);
    if (page > 0) {
        bmp = Bmp(frameData[1]);
        gfx::DrawArt(gfx::artPlain(), 1 - unk_50, 0, 0, bmp + 2, bmp[0], bmp[1]);
    }
    if (static_cast<int>(page) + 1 < static_cast<int>(pageCount)) {
        gfx::ArtSize(2 - unk_50, &v10, &v0c);
        bmp = Bmp(frameData[1]);
        gfx::DrawArt(gfx::artPlain(), 2 - unk_50, bmp[0] - v10, 0, bmp + 2, bmp[0], bmp[1]);
    }
    gfx::TextMeasure(4, "Aq", &v10, &v0c, &v04);
    if (bufLeft == nullptr && page == 0) {
        gfx::TextDraw(BOOK_LEFT_X + 0x28, v04 + BOOK_LEFT_Y, 4, 0x386, "TITLE", Bmp(frameData[1]) + 2, width,
                      &BOOK_LEFT_X);
        gfx::TextDraw(BOOK_LEFT_X + 0x32, v04 + BOOK_LEFT_Y + 0x64, 4, 0x386, "by", Bmp(frameData[1]) + 2, width,
                      &BOOK_LEFT_X);
    }

    // Left page.
    if (bufLeft != nullptr) {
        int rowY = BOOK_LEFT_Y;
        int off = 0;
        i = 0;
        if (*reinterpret_cast<uint16_t *>(bufLeft) > 0) {
            do {
                if (v0c + rowY > BOOK_LEFT_B)
                    break;
                gfx::TextDraw(BOOK_LEFT_X, v04 + rowY, 4, 0x12, bufLeft + off + 2, Bmp(frameData[1]) + 2, width,
                              &BOOK_LEFT_X);
                if (g_focusedInput == this && cursorOnLeft != 0 && curLine == i) {
                    int cw = gfx::TextWidth(4, curCol, bufLeft + off + 2);
                    gfx::TextDraw(cw + BOOK_LEFT_X, v04 + rowY + 4, 4, 8, "_", Bmp(frameData[1]) + 2, width, rect);
                }
                rowY += v0c;
                i++;
                off += BP_LINE_SIZE;
            } while (i < *reinterpret_cast<uint16_t *>(bufLeft));
        }
        if (g_focusedInput == this && cursorOnLeft != 0) {
            if (curLine == *reinterpret_cast<uint16_t *>(bufLeft))
                gfx::TextDraw(BOOK_LEFT_X, v0c * curLine + v04 + BOOK_LEFT_Y + 4, 4, 8, "_", Bmp(frameData[1]) + 2,
                              width, rect);
        }
    }

    // Right page.
    if (bufRight != nullptr) {
        int rowY = BOOK_RIGHT_Y;
        int off = 0;
        i = 0;
        if (*reinterpret_cast<uint16_t *>(bufRight) > 0) {
            do {
                if (v0c + rowY > BOOK_RIGHT_B)
                    break;
                gfx::TextDraw(BOOK_RIGHT_X, v04 + rowY, 4, 0x12, bufRight + off + 2, Bmp(frameData[1]) + 2, width,
                              &BOOK_RIGHT_X);
                if (g_focusedInput == this && cursorOnLeft == 0 && curLine == i) {
                    int cw = gfx::TextWidth(4, curCol, bufRight + off + 2);
                    gfx::TextDraw(cw + BOOK_RIGHT_X, v04 + rowY + 4, 4, 8, "_", Bmp(frameData[1]) + 2, width, rect);
                }
                rowY += v0c;
                i++;
                off += BP_LINE_SIZE;
            } while (i < *reinterpret_cast<uint16_t *>(bufRight));
        }
        if (g_focusedInput == this && cursorOnLeft == 0) {
            if (curLine == *reinterpret_cast<uint16_t *>(bufRight))
                gfx::TextDraw(BOOK_RIGHT_X, v0c * curLine + v04 + BOOK_RIGHT_Y + 4, 4, 8, "_", Bmp(frameData[1]) + 2,
                              width, rect);
        }
    }

    // Page numbers.
    if (page != 0 && page <= pageCount) {
        sprintf(num, "%d", static_cast<int>(page));   // 0x0048b1d0
        gfx::TextMeasure(9, num, &v10, &v0c, &v04);
        gfx::TextDraw((BOOK_LEFT_X + BOOK_LEFT_R) / 2 - v10 / 2, v04 + BOOK_LEFT_B - 8, 9, 0x386, num,
                      Bmp(frameData[1]) + 2, width, rect);
    }
    int rightNo = static_cast<int>(page) + 1;
    if (rightNo <= static_cast<int>(pageCount)) {
        sprintf(num, "%d", rightNo);   // 0x0048b1d0
        gfx::TextMeasure(9, num, &v10, &v0c, &v04);
        gfx::TextDraw((BOOK_RIGHT_X + BOOK_RIGHT_R) / 2 - v10 / 2, v04 + BOOK_RIGHT_B - 8, 9, 0x386, num,
                      Bmp(frameData[1]) + 2, width, rect);
    }
}
HOOK_VMETHOD(0x00407f60, CBookGump, RedrawSpread);
