// Inbound packet handlers, second pass (batch 22), part 3: book header / pages and the client-config name lists.
//
//   0x0045c9f0 0x93  book header (title, author, page count)    0x0045cbe0 0x66  book pages (text lines)
//   0x0045cec0 0x69  name lists of the client config gump       0x0045d2c0 0x6A  online status of a listed name
//
// All are __stdcall (RET 4) with the packet pointer. The first field is at base = 3 for a variable-length
// opcode and 1 otherwise (packet_cursor.h); the original expands that test inline at every field read, the port
// reads at base + offset. The SEH frames the original wraps around operator new + constructor calls are omitted.
// The inline strlen / strcpy sequences are host calls.
#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "book_gump.h"
#include "edit_list_gump.h"
#include "globals.h"
#include "gump.h"
#include "input_control_gump.h"
#include "obj.h"
#include "object_manager.h"
#include "crt.h"
#include "packet_cursor.h"
#include "raw_field.h"
#include "vcall.h"
#include "text_gump_ctor_ports.h"

#define g_msgTarget GLOBAL_PTR(void, g_msgTarget_storage)               // on-screen message / journal object
#define g_nameListDone GLOBAL_AS(uint32_t, g_ccVal268)
#define g_values1 GLOBAL_PTR(uint32_t, g_ccVals76c)
#define g_values2 GLOBAL_PTR(uint32_t, g_ccVals71c)
#define g_online2 GLOBAL_PTR(uint32_t, g_ccVals7bc)             // parallel to g_values2 (g_values2 + 0xa0 bytes)
#define g_flags3 GLOBAL_PTR(uint32_t, g_flags_69f80c)

static CObj *FindObject(uint32_t serial) { return static_cast<CObj *>((&g_objMgr)->CObjectManager::FindBySerial(serial)); }
static void Journal_Say(const char *text) { PORT_THISCALL3(&CreateBarkGump, void, g_msgTarget, int, int, const char *)(0, 3, text); }

static uint32_t rd32(const uint8_t *p, uint32_t off)
{
    uint32_t v;
    memcpy(&v, p + off, sizeof(v));
    return v;
}
static uint16_t rd16(const uint8_t *p, uint32_t off)
{
    uint16_t v;
    memcpy(&v, p + off, sizeof(v));
    return v;
}

// The book gump of `obj` (first gump on the list, walked from the tail, whose vtable slot 0xcc answers true and
// whose owner is `obj`), or null.
static CBookGump *FindBookGump(CObj *obj)
{
    for (CGump *g = g_gumpListTail; g != nullptr; g = g->prev) {
        if (decomp::vcall(PORT_RET(int), g, 0xcc) != 0 && g->owner == obj)
            return static_cast<CBookGump *>(g);
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// 0x0045c9f0  size 0x1e2
// Inbound packet 0x93: book header. serial, editable flag byte, page count word, title (0x3c bytes), author.
// Updates the title / author of an open book gump and redraws it (vtable slot 0x114), else opens a new one.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x93(const uint8_t *packet)
{
    const uint8_t *d = packet + pkt::Base(packet);
    uint32_t serial = ntohl(rd32(d, 0));
    uint8_t editable = d[4];
    uint16_t pages = ntohs(rd16(d, 5));
    char *title = const_cast<char *>(reinterpret_cast<const char *>(d + 7));
    char *author = const_cast<char *>(reinterpret_cast<const char *>(d + 0x43));

    CObj *obj = FindObject(serial);
    if (obj == nullptr)
        return;
    CBookGump *book = FindBookGump(obj);
    if (book != nullptr) {
        strcpy(book->title, title);
        strcpy(book->author, author);
        decomp::vcall_void(book, 0x114);                                            // CBookGump::RedrawSpread
        return;
    }
    CBookGump *gump = new CBookGump(0, 0, 0x1fe, obj, title, author, pages, editable);   // 0x00406c20
    CGump_AddChild(static_cast<CGump *>(gump), nullptr, 1);                         // 0x0043bcc0
}
HOOK_FUNC(0x0045c9f0, Packet_Handle0x93);

// ---------------------------------------------------------------------------
// 0x0045cbe0  size 0x2da
// Inbound packet 0x66: book pages. serial, page count, then per page {page number, line count, that many
// NUL-terminated lines}. Lines are copied into the page records (0x648 bytes each, 0x50 bytes per line) of the
// open book gump of that serial; unused lines are cleared and the spread is redrawn when it shows the page.
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x66(const uint8_t *packet)
{
    const uint8_t *d = packet + pkt::Base(packet);
    uint32_t serial = ntohl(rd32(d, 0));
    uint32_t pageTotal = ntohs(rd16(d, 4));
    uint32_t off = 6;

    CObj *obj = FindObject(serial);
    if (obj == nullptr)
        return;
    CBookGump *book = FindBookGump(obj);
    if (static_cast<uint16_t>(pageTotal) == 0)
        return;

    uint32_t remaining = pageTotal & 0xffff;
    do {
        uint32_t pageNo = ntohs(rd16(d, off));
        off += 2;
        uint32_t lines = ntohs(rd16(d, off));
        off += 2;
        if (book != nullptr) {
            uint32_t index;
            if (book->editable != 0)
                index = pageNo - 1;
            else
                index = static_cast<uint16_t>(static_cast<uint16_t>(pageNo) - book->page);
            uint32_t pageOff = (index & 0xffff) * 0x648;
            if (static_cast<uint16_t>(lines) != 0) {
                uint32_t lineOff = 0;
                for (uint32_t n = lines & 0xffff; n != 0; n--) {
                    const char *text = reinterpret_cast<const char *>(d + off);
                    size_t len = strlen(text);
                    sprintf(book->pageRecords + lineOff + pageOff + 2, "%s", text);   // 0x0048b1d0
                    off += static_cast<uint32_t>(len) + 1;
                    lineOff += 0x50;
                }
            }
            if (static_cast<uint16_t>(lines) < 0x14) {
                uint32_t lineOff = (lines & 0xffff) * 0x50;
                for (uint32_t n = 0x14 - (lines & 0xffff); n != 0; n--) {
                    strcpy(book->pageRecords + lineOff + pageOff + 2, g_emptyString);     // empty line
                    lineOff += 0x50;
                }
            }
            uint16_t lineCount = static_cast<uint16_t>(lines);
            memcpy(book->pageRecords + (index & 0xffff) * 0x648, &lineCount, 2);
            bool redraw = true;
            if (book->editable != 0) {
                uint16_t left = book->page;
                if (static_cast<uint16_t>(pageNo) < left)
                    redraw = false;
                else if (static_cast<int32_t>(pageNo & 0xffff) > static_cast<int32_t>(left) + 1)
                    redraw = false;
            }
            if (redraw)
                decomp::vcall_void(book, 0x114);                                    // CBookGump::RedrawSpread
        }
        remaining--;
    } while (remaining != 0);
}
HOOK_FUNC(0x0045cbe0, Packet_Handle0x66);

// ---------------------------------------------------------------------------
// 0x0045cec0  size 0x3ea
// Inbound packet 0x69: name lists of the client config gump (tab 4). Byte 0 selects the action, byte 1 is a
// count:
//   0  mark the lists as received (0x0069f268 = 1)
//   1  count {name, dword} entries into the table at 0x0069f4c4 / 0x0069f76c and refill the list at gump+0x130
//   2  the same into 0x0069f26c / 0x0069f71c (clearing the online flags) and the list at gump+0x12c
//   3  count flag bytes into 0x0069f80c and recolour the rows of the list at gump+0x12c
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x69(const uint8_t *packet)
{
    const uint8_t *d = packet + pkt::Base(packet);
    uint32_t off = 2;
    uint8_t type = d[0];
    uint8_t count = d[1];

    switch (type) {
    case 0:
        g_nameListDone = 1;
        return;
    case 1: {
        g_nameListDone = 0;
        int32_t n = count;
        g_ccSel264 = n;
        if (n > 0) {
            char *name = g_ccNames4c4;
            uint32_t *value = g_values1;
            for (int32_t left = n; left != 0; left--) {
                const char *text = reinterpret_cast<const char *>(d + off);
                size_t len = strlen(text);
                strcpy(name, text);
                off += static_cast<uint32_t>(len) + 1;
                *value = ntohl(rd32(d, off));
                off += 4;
                name += 0x1e;
                value += 1;
            }
        }
        void *cfg = GLOBAL_AS(void *, g_clientCfgGump);
        if (cfg == nullptr)
            return;
        CEditListGump *list = RAWF(CEditListGump *, cfg, 0x130);
        while (list->head != nullptr)
            list->RemoveItem(list->head);                                           // 0x00417100
        if (g_ccSel264 > 0) {
            int32_t i = 0;
            do {
                list->AddItem(g_ccNames4c4 + i * 0x1e, static_cast<int>(g_values1[i]));  // 0x00416d40
                i++;
            } while (i < g_ccSel264);
        }
        list->AddItem(g_newRowText, 0);                                             // 0x00416d40
        return;
    }
    case 2: {
        int32_t n = count;
        g_ccSel260 = n;
        if (n > 0) {
            char *name = g_ccNames26c;
            uint32_t *value = g_values2;
            for (int32_t left = n; left != 0; left--) {
                const char *text = reinterpret_cast<const char *>(d + off);
                size_t len = strlen(text);
                strcpy(name, text);
                off += static_cast<uint32_t>(len) + 1;
                *value = ntohl(rd32(d, off));
                off += 4;
                value[0xa0 / 4] = 0;                                                // online flag (g_online2)
                name += 0x1e;
                value += 1;
            }
        }
        void *cfg = GLOBAL_AS(void *, g_clientCfgGump);
        if (cfg == nullptr)
            return;
        CEditListGump *list = RAWF(CEditListGump *, cfg, 0x12c);
        while (list->head != nullptr)
            list->RemoveItem(list->head);                                           // 0x00417100
        if (g_ccSel260 > 0) {
            int32_t i = 0;
            do {
                list->AddItem(g_ccNames26c + i * 0x1e, static_cast<int>(g_values2[i]));  // 0x00416d40
                i++;
            } while (i < g_ccSel260);
        }
        list->AddItem(g_newRowText, 0);                                             // 0x00416d40
        return;
    }
    case 3: {
        int32_t n = count;
        for (int32_t i = 0; i < n; i++)
            g_flags3[i] = (d[i + 2] != 0) ? 1 : 0;
        void *cfg = GLOBAL_AS(void *, g_clientCfgGump);
        if (cfg == nullptr)
            return;
        CEditListNode *node = RAWF(CEditListGump *, cfg, 0x12c)->head;
        if (node == nullptr)
            return;
        const uint32_t *flag = g_flags3;
        while (node->next != nullptr) {
            uint16_t hue;
            if (*flag != 0) {
                if (flag[-0x50 / 4] == 0)                                           // g_online2 entry
                    hue = 0x386;
                else
                    hue = 0x170;
            } else {
                hue = 0x21;
            }
            node->row->hue = hue;
            node->row->Redraw();                                                    // 0x0041e0f0
            node = node->next;
            flag++;
        }
        return;
    }
    default:
        return;
    }
}
HOOK_FUNC(0x0045cec0, Packet_Handle0x69);

// ---------------------------------------------------------------------------
// 0x0045d2c0  size 0x10e
// Inbound packet 0x6A: a listed name (index) went online / offline. Records the state, recolours the matching
// row (case-insensitive name compare) in the list at gump+0x12c and prints "<name> is now online/offline.".
// ---------------------------------------------------------------------------
void __stdcall Packet_Handle0x6A(const uint8_t *packet)
{
    const uint8_t *d = packet + pkt::Base(packet);
    uint32_t index = d[0];
    uint8_t online = d[1];
    void *cfg = GLOBAL_AS(void *, g_clientCfgGump);
    g_online2[index] = (online != 0) ? 1 : 0;
    const char *name = g_ccNames26c + index * 0x1e;
    if (cfg != nullptr) {
        CEditListNode *node = RAWF(CEditListGump *, cfg, 0x12c)->head;
        while (node != nullptr) {
            CInputControlGump *row = node->row;
            if (_stricmp(name, reinterpret_cast<const char *>(row) + 0xec) == 0) {   // 0x0048b850
                if (online != 0)
                    row->hue = 0x170;
                else
                    row->hue = 0x386;
                row->Redraw();                                                      // 0x0041e0f0
            }
            node = node->next;
        }
    }
    char buf[0x50];
    if (online != 0)
        sprintf(buf, "%s is now online.", name);                               // 0x0048b1d0
    else
        sprintf(buf, "%s is now offline.", name);
    Journal_Say(buf);                                                               // 0x00479480
}
HOOK_FUNC(0x0045d2c0, Packet_Handle0x6A);
