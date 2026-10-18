#include "book_gump.h"

#include <cstring>
#include <windows.h>

#include "../runtime/ported.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "font_manager.h"
#include "gump.h"
#include "packet_builders_b25_ports.h"
#include "client_socket.h"
#include "gump_base_ops_ports.h"

#define g_focusedGump GLOBAL_AS(CGump *, g_focusedInput)           // focused input control
#define g_packetBuilder0 GLOBAL_PTR(void, g_packetBuilder_storage)           // this for packet-building calls

namespace {

inline void MeasureSample(int *w, int *h, int *d)
{
    reinterpret_cast<CFontManager *>(BOOK_FONT_OBJ)->MeasureText(4, (char *)(g_refText), w, h, d);
}
inline void FocusGump(CGump *g) { PORT_THISCALL1(&Gump_SetFocus, void, g_gumpManager, CGump *)(g); }
// 0x00433a50: thiscall on the font object, (font, text, x offset); character index under that x. RET 0xc.
inline int CharIndexAt(const char *text, int xOff)
{
    return reinterpret_cast<CFontManager *>(BOOK_FONT_OBJ)->CharsFitting(4, text, xOff);
}
// 0x004641e0 + 0x00478610: build and send the book page request packet (0x66).
inline void SendPageRequest(CObj *book, int pageNo)
{
    char pkt[0x640];
    PORT_THISCALL3(&Pkt_Build_0x66_BookPageRequest, void, g_packetBuilder0, char *, int, int)(pkt, RAWF(int32_t, book, 0x78), pageNo);
    reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(pkt));
}
inline void PlayBookSound() { BOOK_Music(BOOK_MUSIC_OBJ, 0x55, 0, 0, 0, 0, 0); }
inline void Redraw(CBookGump *g) { PORT_VSLOT0(g, 0x114, void)(); }

} // namespace

// 0x00408510  size 0x2ad
// Mouse down: takes focus away from the title / author controls, counts clicks, and (editable
// book) places the caret on the line and column under the cursor of the page that was hit.
void __thiscall CBookGump::MouseDown(int mouseX, int mouseY)
{
    int lineH, ascent, descent;
    int wasFocused = 0;

    CGump *focus = g_focusedGump;
    if (focus == this)
        wasFocused = 1;
    if (focus == titleCtl || focus == authorCtl)
        PORT_THISCALL0(&Gump_CommitFocus, void, g_gumpManager)();   // CGump::CommitFocus
    this->CGump::BringToFront(0);        // bring to front
    int clicks = unk_120;
    unk_11c = 1;
    if (clicks == 1) {
        unk_120 = 2;
    } else {
        unk_120 = 1;
        unk_124 = static_cast<int32_t>(GetTickCount());
    }
    unk_6c = mouseX;
    unk_70 = mouseY;
    if (editable != 0) {
        if (mouseX >= BOOK_LEFT_X && mouseX <= BOOK_LEFT_R && mouseY >= BOOK_LEFT_Y && mouseY <= BOOK_LEFT_B &&
            bufLeft != nullptr) {
            FocusGump(this);
            cursorOnLeft = 1;
            MeasureSample(&ascent, &lineH, &descent);
            curLine = (mouseY - BOOK_LEFT_Y) / lineH;
            char *rec = bufLeft;
            int count = *reinterpret_cast<uint16_t *>(rec);
            if (curLine > count)
                curLine = count;
            if (*reinterpret_cast<uint16_t *>(rec) == 1 && strlen(rec + 2) == 0)
                curLine = 0;
            if ((curLine + 1) * lineH > BOOK_LEFT_B - BOOK_LEFT_Y)
                curLine--;
            curCol = reinterpret_cast<CFontManager *>(BOOK_FONT_OBJ)->CharsFitting(4, rec + 2 + curLine * 0x50, mouseX - BOOK_LEFT_X);
            Redraw(this);
        } else if (mouseX >= BOOK_RIGHT_X && mouseX <= BOOK_RIGHT_R && mouseY >= BOOK_RIGHT_Y &&
                   mouseY <= BOOK_RIGHT_B && bufRight != nullptr) {
            FocusGump(this);
            cursorOnLeft = 0;
            MeasureSample(&ascent, &lineH, &descent);
            curLine = (mouseY - BOOK_RIGHT_Y) / lineH;
            char *rec = bufRight;
            int count = *reinterpret_cast<uint16_t *>(rec);
            if (curLine > count)
                curLine = count;
            if (*reinterpret_cast<uint16_t *>(rec) == 1 && strlen(rec + 2) == 0)
                curLine = 0;
            // the original subtracts the LEFT page's top from the bottom here, not the right page's
            if ((curLine + 1) * lineH > BOOK_LEFT_B - BOOK_LEFT_Y)
                curLine--;
            curCol = reinterpret_cast<CFontManager *>(BOOK_FONT_OBJ)->CharsFitting(4, rec + 2 + curLine * 0x50, mouseX - BOOK_RIGHT_X);
            Redraw(this);
        }
    }
    if (wasFocused != 0 && g_focusedGump == nullptr) {
        FocusGump(this);
        Redraw(this);
    }
}
HOOK_VMETHOD(0x00408510, CBookGump, MouseDown);

// 0x004087c0  size 0x9c
// Mouse up: a click that did not start a drag is reported through slot 0x120 (single) or
// slot 0x124 (second click of a double click).
void __thiscall CBookGump::MouseUp(int mouseX, int mouseY)
{
    unk_11c = 0;
    if (unk_128 != 0) {
        unk_128 = 0;
        unk_120 = 0;
        unk_12c = 0;
        g_dragGump = nullptr;
        return;
    }
    int clicks = unk_120;
    if (clicks == 2) {
        unk_120 = 0;
        PORT_VSLOT2(this, 0x124, void, int, int)(mouseX, mouseY);
        unk_12c = 0;
        g_dragGump = nullptr;
        return;
    }
    if (unk_12c == 0 && clicks == 0)
        PORT_VSLOT2(this, 0x120, void, int, int)(mouseX, mouseY);
    unk_12c = 0;
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x004087c0, CBookGump, MouseUp);

// 0x00408860  size 0x2c
// Mouse move: while the button is held, drag the window by the distance from the press point.
void __thiscall CBookGump::MouseMove(int mouseX, int mouseY)
{
    if (unk_11c != 0)
        Move(mouseX - unk_6c, mouseY - unk_70, 1, nullptr, 1);
}
HOOK_VMETHOD(0x00408860, CBookGump, MouseMove);

// 0x00408890  size 0x85
// Single click: the top-left corner art (id 1 - unk_50) turns back a page (slot 0x11c), the
// top-right corner art (id 2 - unk_50) turns forward (slot 0x118).
void __thiscall CBookGump::OnClickCorner(int mouseX, int mouseY)
{
    int w1, h1, w2, h2;
    BOOK_ArtSize(1 - unk_50, &w1, &h1);
    BOOK_ArtSize(2 - unk_50, &w2, &h2);
    if (mouseX < w1 && mouseY < h1) {
        PORT_VSLOT0(this, 0x11c, void)();
        return;
    }
    int bmpW = *reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(frameData[1]));
    if (mouseX >= bmpW - w2 && mouseY < h2)
        PORT_VSLOT0(this, 0x118, void)();
}
HOOK_VMETHOD(0x00408890, CBookGump, OnClickCorner);

// 0x00408920  size 0x37b
// Double click on a corner: top-left jumps to the cover (page 0), top-right to the last spread.
// Pending edits are committed first; a read-only book asks the server for the pages it now shows.
void __thiscall CBookGump::OnCornerJump(int mouseX, int mouseY)
{
    int w1, h1, w2, h2;
    char pkt[0x640];
    BOOK_ArtSize(1 - unk_50, &w1, &h1);
    BOOK_ArtSize(2 - unk_50, &w2, &h2);

    if (mouseX < w1 && mouseY < h1 && page > 0) {
        this->CBookGump::CommitEdits();   // CommitEdits
        page = 0;
        RAWF(int32_t, titleCtl, 0x68) = 1;
        RAWF(int32_t, authorCtl, 0x68) = 1;
        if (editable == 0) {
            if (page + 1 <= pageCount)
            {
                PORT_THISCALL3(&Pkt_Build_0x66_BookPageRequest, void, g_packetBuilder0, char *, int, int)(pkt, RAWF(int32_t, owner, 0x78), page + 1);
                reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(pkt));
            }
            bufLeft = nullptr;
            if (page + 1 <= pageCount)
                bufRight = pageRecords + 0x648;
            else
                bufRight = nullptr;
            if (bufRight != nullptr)
                *reinterpret_cast<uint16_t *>(bufRight) = 0;
        } else {
            bufLeft = nullptr;
            if (page + 1 <= pageCount) {
                bufRight = pageRecords + page * 0x648;
                Redraw(this);
            } else {
                bufRight = nullptr;
                Redraw(this);
            }
        }
        BOOK_Music(BOOK_MUSIC_OBJ, 0x55, 0, 0, 0, 0, 0);   // 0x004800f0, shared tail in the original
        return;
    }

    int bmpW = *reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(frameData[1]));
    if (mouseX >= bmpW - w2 && mouseY < h2) {
        this->CBookGump::CommitEdits();   // CommitEdits
        page = pageCount;
        if ((static_cast<int>(pageCount) & 1) == 1)   // odd page count: left page is the one before
            page = static_cast<uint16_t>(pageCount - 1);
        if (page > 0) {
            RAWF(int32_t, titleCtl, 0x68) = 0;
            RAWF(int32_t, authorCtl, 0x68) = 0;
        }
        if (editable == 0) {
            PORT_THISCALL3(&Pkt_Build_0x66_BookPageRequest, void, g_packetBuilder0, char *, int, int)(pkt, RAWF(int32_t, owner, 0x78), page);
            reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(pkt));
            if (page + 1 <= pageCount)
            {
                PORT_THISCALL3(&Pkt_Build_0x66_BookPageRequest, void, g_packetBuilder0, char *, int, int)(pkt, RAWF(int32_t, owner, 0x78), page + 1);
                reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket((const uint8_t *)(pkt));
            }
            if (page == 0)
                bufLeft = nullptr;
            else
                bufLeft = pageRecords;
            if (page + 1 <= pageCount)
                bufRight = pageRecords + 0x648;
            else
                bufRight = nullptr;
            if (bufLeft != nullptr)
                *reinterpret_cast<uint16_t *>(bufLeft) = 0;
            if (bufRight != nullptr)
                *reinterpret_cast<uint16_t *>(bufRight) = 0;
        } else {
            if (page > 0)
                bufLeft = pageRecords + page * 0x648 - 0x648;
            else
                bufLeft = nullptr;
            if (page + 1 <= pageCount)
                bufRight = pageRecords + page * 0x648;
            else
                bufRight = nullptr;
            Redraw(this);
        }
        BOOK_Music(BOOK_MUSIC_OBJ, 0x55, 0, 0, 0, 0, 0);   // 0x004800f0, shared tail in the original
    }
}
HOOK_VMETHOD(0x00408920, CBookGump, OnCornerJump);

// 0x0040a280  size 0x11
void __thiscall CBookGump::OnChildChanged(int unused)
{
    (void)unused;
    unk_a4 = 1;
}
HOOK_VMETHOD(0x0040a280, CBookGump, OnChildChanged);

// 0x0040a2a0  size 0x5d
// Tick: a first click that no second click followed within the double-click time becomes a
// single click (slot 0x120) when the button is already up, or starts a drag (unk_128) otherwise.
void __thiscall CBookGump::Tick()
{
    DWORD now = GetTickCount();
    DWORD dbl = GetDoubleClickTime();
    if (unk_120 == 1 && now - static_cast<DWORD>(unk_124) > dbl) {
        unk_120 = 0;
        if (unk_11c == 0) {
            PORT_VSLOT2(this, 0x120, void, int, int)(unk_6c, unk_70);
            return;
        }
        unk_128 = 1;
    }
}
HOOK_VMETHOD(0x0040a2a0, CBookGump, Tick);

// 0x0040a300  size 0x25
void __thiscall CBookGump::RButtonUp(int a, int b)
{
    (void)a;
    (void)b;
    unk_130 = 1;
    g_dragGump = nullptr;
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void *, int)(1);   // deleting destructor
}
HOOK_VMETHOD(0x0040a300, CBookGump, RButtonUp);

// 0x0040a330  size 0x111
// Draw: if the gump manager accepts the clip, blit the visible frame bitmap (frame 1 when
// unk_60 is set, else frame 0) at the screen origin into the back buffer, then draw the children.
void __thiscall CBookGump::Draw(int *clip)
{
    if (visible == 0)
        return;
    uint16_t *bmp;
    if (unk_60 != 0)
        bmp = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(frameData[3]));
    else
        bmp = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(frameData[1]));
    int sy = 0;
    int sx = 0;
    this->CGump::LocalToScreen(&sx, &sy);   // CGump::LocalToScreen
    int rect[4];
    rect[0] = sx;
    rect[1] = sy;
    rect[2] = bmp[0] + sx;
    rect[3] = bmp[1] + sy;
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, const int *, int *)(clip, rect) == 0)
        return;
    int cr[4];
    GumpRectClip(clip, rect, cr);   // RectClip
    int offY = 0;
    int offX = 0;
    if (rect[0] != cr[0])
        offX = cr[0] - rect[0];
    if (rect[1] != cr[1])
        offY = cr[1] - rect[1];
    uint16_t *src = bmp + 2 + (offY * bmp[0] + offX);
    uint16_t *dest = g_backBuffer + ((sy + offY) * 0x280 + offX + sx);
    // 0x0043b760: copy rows. ECX = rows, EDX = columns, stack (dest, src, dest stride, src stride).
    GumpBlitRows(cr[3] - cr[1], cr[2] - cr[0], dest, src,
                                                                                0x280, bmp[0]);
    for (CGump *c = firstChild; c != nullptr; c = c->next)
        PORT_VSLOT1(c, kGumpSlot_Draw, void, const int *)(clip);
}
HOOK_VMETHOD(0x0040a330, CBookGump, Draw);
