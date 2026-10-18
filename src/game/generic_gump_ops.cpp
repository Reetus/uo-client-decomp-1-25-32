// CGenericGump / GumpNode (the original CGumpLayoutNode): destructors, widget adders, frame
// rebuild, mouse handling and the per-node helpers. The SEH frames of the original (try levels
// around operator new) are omitted. See generic_gump.cpp for the constructor and the renderer.
#include "generic_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch02_helpers.h"
#include "globals.h"
#include "raw_field.h"
#include "crt.h"
#include "unicode_speech_ops_ports.h"
#include "unicode_font_ports.h"
#include "art_record_ops_ports.h"
#include "packet_buffer_ops_ports.h"
#include "gump.h"
#include "vtables.h"

// 0x0047c060 wcslen (ECX = string, result in EAX), 0x0047c080 wcscpy (ECX = dst, EDX = src).
#define GGO_WcsLen(o) PORT_THISCALL0(&WcsLenThunk, int, (o))()
#define GGO_WcsCpy(a1, a2) (WcsCpy((uint16_t *)(a1), (const uint16_t *)(a2)))
// 0x0047cc80: ECX = 16-bit string, EDX = &width, stack (&height, &yOffset), RET 8.
#define GGO_MeasureUnicode(a1, a2, a3, a4) (MeasureUnicodeString((const uint16_t *)(a1), a2, a3, a4))
// 0x0047ca80: ECX = character; true when the unicode font has a glyph for it.
#define GGO_UnifontHasGlyph(a1) (UnifontHasGlyph((uint16_t)(a1)))
// 0x00476130 on the tile art table (0x0068d2d8): art record of tile id + 0x4000; u16 width at +4, height at +6.
#define GGO_TileArtGet(o, a1) PORT_THISCALL1(&ArtRecord_Get, uint8_t *, (o), int)(a1)
#define GGO_TileArtTable GLOBAL_PTR(void, g_artCache_storage)
// 0x004574e0.. packet writers, ECX = packet, EDX = value.
#define GGO_PktDword(a1, a2) (PacketBuf_WriteU32((uint8_t *)(a1), (uint32_t)(a2)))
// The original passes the value in DX as a 16-bit quantity (no zero extension), so the call goes through
// a prototype whose second parameter is uint16_t.
typedef void(__fastcall *GGO_PktWordFn)(uint8_t *, uint16_t);
#define GGO_PktWord(a1, a2) ((reinterpret_cast<GGO_PktWordFn>(&PacketBuf_WriteU16))((uint8_t *)(a1), (uint16_t)(a2)))
// Gump manager (0x006488c0): 0x0043a720 commit the focused text entry, 0x0043a820 give a gump the focus.
#define GGO_CommitFocus(o) PORT_THISCALL0(&Gump_CommitFocus, void, (o))()
#define GGO_SetFocus(o, a1) PORT_THISCALL1(&Gump_SetFocus, void, (o), CGump *)(a1)
#define GGO_FrameTag "generic frame"   // 0x004a81d0

static inline GumpNode *gg_node(int32_t v)
{
    return reinterpret_cast<GumpNode *>(static_cast<uintptr_t>(static_cast<uint32_t>(v)));
}

static inline int32_t gg_ptr(const GumpNode *n)
{
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(n));
}

// The group lookup at the top of every Add* method: group 0 is the top level list; otherwise the
// group node is searched in the top level groups, their children, and (0x00435e60) below those.
// On success *parent is the group node (0 for the top level) and *slot the link after its last child.
static inline bool gg_find_slot(CGenericGump *g, int group, GumpNode **parent, GumpNode ***slot)
{
    GumpNode *found = nullptr;
    GumpNode **s;

    if (group == 0) {
        s = &g->groups;
    } else {
        GumpNode *n = g->groups;
        while (n != nullptr) {
            if (n->group == group) {
                found = n;
                break;
            }
            GumpNode *c = n->children;
            GumpNode *hit = nullptr;
            while (c != nullptr) {
                if (c->group == group) {
                    hit = c;
                    break;
                }
                if (c->children != nullptr) {
                    hit = c->FindDescendantByGroup(group);   // 0x00435e60
                    if (hit != nullptr)
                        break;
                }
                c = c->next;
            }
            if (hit != nullptr) {
                found = hit;
                break;
            }
            n = n->next;
        }
        if (found == nullptr)
            return false;
        s = &found->children;
    }
    while (*s != nullptr)
        s = &(*s)->next;
    *parent = found;
    *slot = s;
    return true;
}

// 0x00435db0  size 0xab
// Node constructor. The text is copied to a new 2 * (length + 1) byte buffer. Check and radio
// nodes (types 6, 7) take their initial state from the target field.
GumpNode::GumpNode(GumpNode *parent, int nodeType, int nodePage, int nodeGroup, int nx, int ny, int nw, int nh, int a0,
                   int a1, const uint16_t *wtext, int f38, int entry, int f40)
{
    page = nodePage;
    group = nodeGroup;
    unk_00 = static_cast<int32_t>(reinterpret_cast<uintptr_t>(parent));
    y = ny;
    w = nw;
    x = nx;
    h = nh;
    art0 = a0;
    art1 = a1;
    type = nodeType;
    unk_38 = f38;
    curPage = 1;
    children = nullptr;
    next = nullptr;
    text = nullptr;
    entryId = entry;
    unk_40 = f40;
    flag = 0;
    if (wtext != nullptr) {
        int n = GGO_WcsLen(const_cast<uint16_t *>(wtext));          // 0x0047c060
        text = ::operator new(static_cast<size_t>(n + n + 2));          // 0x0048ad70
        GGO_WcsCpy(text, wtext);                                     // 0x0047c080
    }
    if (type == 6 || type == 7)
        flag = unk_40;
}
HOOK_CTOR(0x00435db0, GumpNode, (GumpNode *, int, int, int, int, int, int, int, int, int, const uint16_t *, int, int, int));

// 0x00434e00  size 0x49
GumpNode *__thiscall GumpNode::ScalarDeletingDtor(int flags)
{
    GumpNode *c = children;
    while (c != nullptr) {
        GumpNode *nextChild = c->next;
        if (c != nullptr)
            c->ScalarDeletingDtor(1);
        c = nextChild;
    }
    if (text != nullptr)
        operator delete(text);                 // 0x0048ad60
    if (flags & 1)
        operator delete(this);                 // 0x0048ad60
    return this;
}
HOOK_METHOD(0x00434e00, GumpNode, ScalarDeletingDtor);

// 0x00435e60  size 0x46
GumpNode *__thiscall GumpNode::FindDescendantByGroup(int groupId)
{
    GumpNode *c = children;
    while (c != nullptr) {
        if (c->group == groupId)
            return c;
        if (c->children != nullptr) {
            GumpNode *r = c->FindDescendantByGroup(groupId);
            if (r != nullptr)
                return r;
        }
        c = c->next;
    }
    return nullptr;
}
HOOK_METHOD(0x00435e60, GumpNode, FindDescendantByGroup);

// 0x00436340  size 0x85
// Deepest node containing (px, py): children on the current page first (the last hit wins),
// then this node itself when it has a type and its rect contains the point.
GumpNode *__thiscall GumpNode::HitTest(int px, int py)
{
    GumpNode *hit = nullptr;
    GumpNode *c = children;

    if (c != nullptr) {
        do {
            if (c->page == 0 || c->page == curPage) {
                GumpNode *r = c->HitTest(px, py);
                if (r != nullptr)
                    hit = r;
            }
            c = c->next;
        } while (c != nullptr);
    }
    if (hit != nullptr)
        return hit;
    if (type != 0 && px >= x && py >= y && px < w + x && py < h + y)
        return this;
    return nullptr;
}
HOOK_METHOD(0x00436340, GumpNode, HitTest);

// 0x004363d0  size 0x180
// Mouse released at (px, py) over this node. Buttons (5) either send the response and close the
// gump (flag 1) or change page (flag 2); check (6) and radio (7) nodes toggle. Returns 0 when the
// gump was destroyed, else 1.
int __thiscall GumpNode::OnMouseUp(int px, int py, CGenericGump *gump)
{
    gump->unk_bc = 0;

    if (type == 5) {
        if (px >= x && px < w + x && py >= y && py < h + y) {
            if (unk_38 & 1) {
                gump->SendResponse(entryId);                                         // 0x00434e50
                if (gump != nullptr)
                    PORT_VSLOT1(gump, kGumpSlot_DeletingDtor, void *, int)(1);   // delete gump
                return 0;
            }
            if (unk_38 & 2) {
                GumpNode *parent = gg_node(unk_00);
                if (parent != nullptr) {
                    parent->curPage = unk_40;
                    flag = 0;
                    gump->RebuildFrame();                                            // 0x00435a00
                    return 1;
                }
                gump->unk_94 = unk_40;
            }
        }
        flag = 0;
        gump->RebuildFrame();                                                        // 0x00435a00
        return 1;
    }
    if (type == 6) {
        if (px >= x && px < w + x && py >= y && py < h + y) {
            flag = (flag == 0) ? 1 : 0;
            gump->RebuildFrame();                                                    // 0x00435a00
            return 1;
        }
        return 1;
    }
    if (type == 7) {
        if (px >= x && px < w + x && py >= y && py < h + y) {
            if (flag == 0) {
                GumpNode *parent = gg_node(unk_00);
                GumpNode *n = (parent != nullptr) ? parent->children : gump->groups;
                while (n != nullptr) {
                    if (n->type == 7)
                        n->flag = 0;
                    n = n->next;
                }
                flag = 1;
            }
            gump->RebuildFrame();                                                    // 0x00435a00
        }
    }
    return 1;
}
HOOK_METHOD(0x004363d0, GumpNode, OnMouseUp);

// 0x00436550  size 0xbd
// Key for a text entry node: backspace (8, 0x105) drops the last character, Enter and Escape
// commit the focus, 0x09 and 0xfa..0xff are ignored, any character the unicode font has is
// appended (up to 0xee characters). The frame is redrawn after a change.
void __thiscall GumpNode::OnKey(int key, CGenericGump *gump)
{
    int len;

    switch (key) {
    case 0x0d:
    case 0x1b:
        GGO_CommitFocus(g_gumpManager);                                              // 0x0043a720
        return;
    case 0x08:
    case 0x105:
        if (*static_cast<uint16_t *>(text) == 0)
            return;
        len = GGO_WcsLen(static_cast<uint16_t *>(text));                             // 0x0047c060
        static_cast<uint16_t *>(text)[len - 1] = 0;
        gump->RebuildFrame();                                                        // 0x00435a00
        return;
    case 0x09:
    case 0xfa:
    case 0xfb:
    case 0xfc:
    case 0xfd:
    case 0xfe:
    case 0xff:
        return;
    default:
        break;
    }

    if (GGO_UnifontHasGlyph(key) == 0)                                               // 0x0047ca80
        return;
    len = GGO_WcsLen(text);                                                          // 0x0047c060
    if (len >= 0xef)
        return;
    uint16_t *buf = static_cast<uint16_t *>(::operator new(static_cast<size_t>(len + len + 4)));   // 0x0048ad70
    GGO_WcsCpy(buf, text);                                                           // 0x0047c080
    buf[len] = static_cast<uint16_t>(key);
    buf[len + 1] = 0;
    operator delete(text);                                                                 // 0x0048ad60
    text = buf;
    gump->RebuildFrame();                                                            // 0x00435a00
}
HOOK_METHOD(0x00436550, GumpNode, OnKey);

// 0x00436730  size 0x39
// Number of selected check / radio nodes in this subtree.
int __thiscall GumpNode::CountSwitches()
{
    int n = 0;
    GumpNode *c = children;
    while (c != nullptr) {
        n += c->CountSwitches();
        c = c->next;
    }
    if ((type == 6 || type == 7) && flag != 0)
        n++;
    return n;
}
HOOK_METHOD(0x00436730, GumpNode, CountSwitches);

// 0x00436770  size 0x46
// Write the switch id of every selected check / radio node of this subtree.
void __thiscall GumpNode::WriteSwitches(char *packet)
{
    GumpNode *c = children;
    while (c != nullptr) {
        c->WriteSwitches(packet);
        c = c->next;
    }
    if ((type == 6 || type == 7) && flag != 0)
        GGO_PktDword(packet, entryId);                                               // 0x004575f0
}
HOOK_METHOD(0x00436770, GumpNode, WriteSwitches);

// 0x004367c0  size 0x2b
// Number of text entry nodes (type 8) in this subtree.
int __thiscall GumpNode::CountEntries()
{
    int n = 0;
    GumpNode *c = children;
    while (c != nullptr) {
        n += c->CountEntries();
        c = c->next;
    }
    if (type == 8)
        n++;
    return n;
}
HOOK_METHOD(0x004367c0, GumpNode, CountEntries);

// 0x004367f0  size 0x7d
// Write id, length (at most 0xef) and characters of every text entry node of this subtree.
void __thiscall GumpNode::WriteEntries(char *packet)
{
    GumpNode *c = children;
    while (c != nullptr) {
        c->WriteEntries(packet);
        c = c->next;
    }
    if (type == 8) {
        GGO_PktWord(packet, static_cast<uint16_t>(entryId));                        // 0x00457570
        int n = GGO_WcsLen(text);                                                    // 0x0047c060
        int cnt = (static_cast<uint16_t>(n) > 0xef) ? 0xef : n;
        GGO_PktWord(packet, static_cast<uint16_t>(cnt));                             // 0x00457570
        for (int i = 0; i < (cnt & 0xffff); i++)
            GGO_PktWord(packet, static_cast<const uint16_t *>(text)[i]);             // 0x00457570
    }
}
HOOK_METHOD(0x004367f0, GumpNode, WriteEntries);

HOOK_VTABLE(0x0049eab0, CGenericGump);
HOOK_SDD(0x00434d30, CGenericGump);

// 0x00434d60  size 0x97
// Frees the widget tree, then runs the CGump base destructor. The SEH frame is omitted.
CGenericGump::~CGenericGump()
{
    GumpNode *g = groups;
    while (g != nullptr) {
        GumpNode *nextGroup = g->next;
        GumpNode *c = g->children;
        while (c != nullptr) {
            GumpNode *nextChild = c->next;
            c->ScalarDeletingDtor(1);    // 0x00434e00
            c = nextChild;
        }
        if (g->text != nullptr)
            operator delete(g->text);          // 0x0048ad60
        operator delete(g);                    // 0x0048ad60
        g = nextGroup;
    }
}
HOOK_DTOR(0x00434d60, CGenericGump);

// 0x00435060  size 0x1d
void __thiscall CGenericGump::OnKey(int key)
{
    GumpNode *entry = gg_node(unk_c0);
    if (entry != nullptr)
        entry->OnKey(key, this);         // 0x00436550
}
HOOK_VMETHOD(0x00435060, CGenericGump, OnKey);

// 0x00435080  size 0x2d
void __thiscall CGenericGump::ClearEntryFocus()
{
    GumpNode *entry = gg_node(unk_c0);
    if (entry != nullptr) {
        if (entry->type == 8) {
            entry->flag = 0;
            RebuildFrame();              // 0x00435a00
        }
        unk_c0 = 0;
    }
}
HOOK_METHOD(0x00435080, CGenericGump, ClearEntryFocus);

// 0x004350b0  size 0x64
int __thiscall CGenericGump::HitTest(int px, int py)
{
    GumpNode *n = groups;
    while (n != nullptr) {
        if (n->page == 0 || n->page == unk_94) {
            if (n->HitTest(px, py) != nullptr)       // 0x00436340
                return 1;
        }
        n = n->next;
    }
    return 0;
}
HOOK_VMETHOD(0x004350b0, CGenericGump, HitTest);

// 0x00435120  size 0x10a
// Gump art node (type 1) sized from the art.
void __thiscall CGenericGump::AddGumpPic(int px, int py, int artId, int nodePage, int group)
{
    GumpNode *grp;
    GumpNode **slot;
    int w, h;

    if (!gg_find_slot(this, group, &grp, &slot))      // 0x00435e60
        return;
    GumpArtSize(artId, &w, &h);                          // 0x00433e70
    GumpNode *node = new GumpNode(grp, 1, nodePage, group, px, py, w, h, artId, 0, nullptr, 0, 0, 0);   // 0x00435db0
    *slot = node;
}
HOOK_METHOD(0x00435120, CGenericGump, AddGumpPic);

// 0x00435230  size 0xf6
// Resizable frame node (type 3) of the given size.
void __thiscall CGenericGump::AddResizePic(int px, int py, int artId, int w, int h, int nodePage, int group)
{
    GumpNode *grp;
    GumpNode **slot;

    if (!gg_find_slot(this, group, &grp, &slot))      // 0x00435e60
        return;
    GumpNode *node = new GumpNode(grp, 3, nodePage, group, px, py, w, h, artId, 0, nullptr, 0, 0, 0);   // 0x00435db0
    *slot = node;
}
HOOK_METHOD(0x00435230, CGenericGump, AddResizePic);

// 0x00435330  size 0x133
// Item art node (type 2) sized from tile art tileId + 0x4000.
void __thiscall CGenericGump::AddTilePic(int px, int py, int tileId, int nodePage, int group)
{
    GumpNode *grp;
    GumpNode **slot;

    if (!gg_find_slot(this, group, &grp, &slot))      // 0x00435e60
        return;
    int w = 0;
    int h = 0;
    uint8_t *art = GGO_TileArtGet(GGO_TileArtTable, tileId + 0x4000);   // 0x00476130
    if (art != nullptr) {
        w = *reinterpret_cast<uint16_t *>(art + 4);
        h = *reinterpret_cast<uint16_t *>(art + 6);
    }
    GumpNode *node = new GumpNode(grp, 2, nodePage, group, px, py, w, h, tileId, 0, nullptr, 0, 0, 0);   // 0x00435db0
    *slot = node;
}
HOOK_METHOD(0x00435330, CGenericGump, AddTilePic);

// 0x00435470  size 0x12f
// Text node (type 4): hue in art0, size from the unicode measure plus 2.
void __thiscall CGenericGump::AddText(int px, int py, int hue, const uint16_t *text, int nodePage, int group)
{
    GumpNode *grp;
    GumpNode **slot;
    int w = 0;
    int h = 0;
    int yOff = 0;

    if (!gg_find_slot(this, group, &grp, &slot))      // 0x00435e60
        return;
    GGO_MeasureUnicode(text, &w, &h, &yOff);             // 0x0047cc80
    GumpNode *node = new GumpNode(grp, 4, nodePage, group, px, py, w + 2, h + 2, hue, 0, text, 0, 0, 0);   // 0x00435db0
    *slot = node;
}
HOOK_METHOD(0x00435470, CGenericGump, AddText);

// 0x004355a0  size 0x100
// Text entry node (type 8): font in art0, size + 2.
void __thiscall CGenericGump::AddTextEntry(int px, int py, int w, int h, int font, const uint16_t *text, int entryId, int nodePage, int group)
{
    GumpNode *grp;
    GumpNode **slot;

    if (!gg_find_slot(this, group, &grp, &slot))      // 0x00435e60
        return;
    GumpNode *node = new GumpNode(grp, 8, nodePage, group, px, py, w + 2, h + 2, font, 0, text, 0, entryId, 0);   // 0x00435db0
    *slot = node;
}
HOOK_METHOD(0x004355a0, CGenericGump, AddTextEntry);

// 0x004356a0  size 0x13f
// Button node (type 5). flags: bit 0 when replies is nonzero (send the response), bit 1 when
// targetPage is nonzero (change page).
void __thiscall CGenericGump::AddButton(int px, int py, int upArt, int downArt, int replies, int targetPage, int buttonId, int nodePage, int group)
{
    GumpNode *grp;
    GumpNode **slot;
    int w, h;

    if (!gg_find_slot(this, group, &grp, &slot))      // 0x00435e60
        return;
    GumpArtSize(upArt, &w, &h);                          // 0x00433e70
    int flags = ((targetPage != 0) ? 2 : 0) | ((replies != 0) ? 1 : 0);
    GumpNode *node = new GumpNode(grp, 5, nodePage, group, px, py, w, h, upArt, downArt, nullptr, flags, buttonId, targetPage);   // 0x00435db0
    *slot = node;
}
HOOK_METHOD(0x004356a0, CGenericGump, AddButton);

// 0x004357e0  size 0x110
// Check box node (type 6); state is the initial value.
void __thiscall CGenericGump::AddCheckbox(int px, int py, int offArt, int onArt, int state, int switchId, int nodePage, int group)
{
    GumpNode *grp;
    GumpNode **slot;
    int w, h;

    if (!gg_find_slot(this, group, &grp, &slot))      // 0x00435e60
        return;
    GumpArtSize(offArt, &w, &h);                         // 0x00433e70
    GumpNode *node = new GumpNode(grp, 6, nodePage, group, px, py, w, h, offArt, onArt, nullptr, 0, switchId, state);   // 0x00435db0
    *slot = node;
}
HOOK_METHOD(0x004357e0, CGenericGump, AddCheckbox);

// 0x004358f0  size 0x110
// Radio button node (type 7); state is the initial value.
void __thiscall CGenericGump::AddRadio(int px, int py, int offArt, int onArt, int state, int switchId, int nodePage, int group)
{
    GumpNode *grp;
    GumpNode **slot;
    int w, h;

    if (!gg_find_slot(this, group, &grp, &slot))      // 0x00435e60
        return;
    GumpArtSize(offArt, &w, &h);                         // 0x00433e70
    GumpNode *node = new GumpNode(grp, 7, nodePage, group, px, py, w, h, offArt, onArt, nullptr, 0, switchId, state);   // 0x00435db0
    *slot = node;
}
HOOK_METHOD(0x004358f0, CGenericGump, AddRadio);

// 0x00435a00  size 0xa7
// Throw the frame bitmap away, start a 1x1 one and draw every node of the current page into it
// (the nodes grow the bitmap through GrowFrame).
void __thiscall CGenericGump::RebuildFrame()
{
    FrameData_Free(g_frameDataAllocator, i32_ptr(frameData[1]), GGO_FrameTag);   // 0x00446310
    height = 1;
    rowStride = 1;
    width = 1;
    frameData[1] = ptr_i32(FrameData_Alloc(g_frameDataAllocator, 6, GGO_FrameTag));   // 0x004462f0
    memset(static_cast<char *>(i32_ptr(frameData[1])) + 4, 0, static_cast<size_t>((width * height) << 1));
    *static_cast<uint16_t *>(i32_ptr(frameData[1])) = static_cast<uint16_t>(width);
    *(static_cast<uint16_t *>(i32_ptr(frameData[1])) + 1) = static_cast<uint16_t>(height);
    GumpNode *n = groups;
    while (n != nullptr) {
        if (n->page == 0 || n->page == unk_94)
            n->Render(this);                 // 0x00435eb0
        n = n->next;
    }
}
HOOK_METHOD(0x00435a00, CGenericGump, RebuildFrame);

// 0x00435ab0  size 0xfc
// Grow the frame bitmap to max(right, width) x max(bottom, height), keeping the old pixels in the
// top left corner.
void __thiscall CGenericGump::GrowFrame(int right, int bottom)
{
    int newW = right;
    if (!(newW > width))
        newW = width;
    int newH = bottom;
    if (!(newH > height))
        newH = height;

    int count = newH * newW;
    uint16_t *buf = reinterpret_cast<uint16_t *>(FrameData_Alloc(g_frameDataAllocator, count + count + 4, GGO_FrameTag));   // 0x004462f0
    memset(reinterpret_cast<char *>(buf) + 4, 0, static_cast<size_t>(count + count));
    buf[0] = static_cast<uint16_t>(newW);
    buf[1] = static_cast<uint16_t>(newH);

    int dstIdx = 2;
    int srcIdx = 2;
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            const uint16_t *src = static_cast<const uint16_t *>(i32_ptr(frameData[1]));
            buf[dstIdx] = src[srcIdx];
            dstIdx++;
            srcIdx++;
        }
        dstIdx += newW - width;
    }
    FrameData_Free(g_frameDataAllocator, i32_ptr(frameData[1]), GGO_FrameTag);   // 0x00446310
    frameData[1] = ptr_i32(buf);
    rowStride = newW;
    width = newW;
    height = newH;
}
HOOK_METHOD(0x00435ab0, CGenericGump, GrowFrame);

// 0x00435bb0  size 0x32
// Clears the drag gump; when the gump may be closed sends response 0 and deletes it.
void __thiscall CGenericGump::RButtonUp(int a, int b)
{
    (void)a;
    (void)b;
    g_dragGump = nullptr;
    if (canClose != 0) {
        SendResponse(0);                                                         // 0x00434e50
        if (this != nullptr)
            PORT_VSLOT1(this, kGumpSlot_DeletingDtor, void *, int)(1);      // delete this
    }
}
HOOK_VMETHOD(0x00435bb0, CGenericGump, RButtonUp);

// 0x00435bf0  size 0xd6
// Mouse pressed at (px, py): bring the gump to the front, remember the press, and let the node under
// the cursor react (button: pressed, check / radio: remembered, text entry: takes the focus).
void __thiscall CGenericGump::MouseDown(int px, int py)
{
    CGump_BringToFront(this, 0);                         // 0x0043ba70
    GumpNode *n = groups;
    unk_98 = 1;
    unk_9c = px;
    unk_a0 = py;
    int curPage = unk_94;
    while (n != nullptr) {
        if (n->page == 0 || n->page == curPage) {
            GumpNode *hit = n->HitTest(px, py);          // 0x00436340
            if (hit != nullptr) {
                if (hit->type == 5) {
                    hit->flag = 1;
                    unk_bc = gg_ptr(hit);
                    RebuildFrame();                      // 0x00435a00
                } else if (hit->type == 6 || hit->type == 7) {
                    unk_bc = gg_ptr(hit);
                } else if (hit->type == 8) {
                    GGO_CommitFocus(g_gumpManager);      // 0x0043a720
                    GGO_SetFocus(g_gumpManager, this);   // 0x0043a820
                    hit->flag = 1;
                    unk_c0 = gg_ptr(hit);
                    RebuildFrame();                      // 0x00435a00
                }
            }
        }
        n = n->next;
    }
}
HOOK_VMETHOD(0x00435bf0, CGenericGump, MouseDown);

// 0x00435cd0  size 0x79
// Mouse released at (px, py): the node that was pressed (hit-tested at the press position) handles
// the release; stops at the first node that returns 0.
void __thiscall CGenericGump::MouseUp(int px, int py)
{
    GumpNode *n = groups;
    unk_98 = 0;
    int curPage = unk_94;
    while (n != nullptr) {
        if (n->page == 0 || n->page == curPage) {
            GumpNode *hit = n->HitTest(unk_9c, unk_a0);      // 0x00436340
            if (hit != nullptr) {
                if (hit->OnMouseUp(px, py, this) == 0)       // 0x004363d0
                    break;
            }
        }
        n = n->next;
    }
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x00435cd0, CGenericGump, MouseUp);

// 0x00435d60  size 0x46
// Mouse moved with the button down and no node pressed: drag the gump if it may be moved.
void __thiscall CGenericGump::MouseMove(int px, int py)
{
    if (canMove != 0 && unk_98 != 0 && unk_bc == 0)
        Move(px - unk_9c, py - unk_a0, 1, nullptr, 1);   // 0x0043b140
}
HOOK_VMETHOD(0x00435d60, CGenericGump, MouseMove);

// 0x00435d50  size 0xe  tag slot 0x80: whether the pointer at +0xa8 is non-null (was the free HasPtr_00435d50).
int __thiscall CGenericGump::IsTag80()
{
    return RAWF(int32_t, this, 0xa8) != 0 ? 1 : 0;
}
HOOK_VMETHOD(0x00435d50, CGenericGump, IsTag80);
