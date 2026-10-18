// CGenericGump: server-described gump (layout string with page / group / gumppic / button / text ...
// commands; name "generic gump", vtable 0x0049eab0), and GumpNode, the 0x48-byte widget tree node it
// builds. Layout partial; see decomp/pass2/0x004344b0.cpp, 0x00434e50.cpp, 0x00435eb0.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gump.h"

class CGenericGump;

// One widget (or group) of the gump. Groups hold their widgets in `children`; widgets of a group are
// chained through `next`. Built by the original node constructor 0x00435db0.
struct GumpNode {
    // 0x00435eb0: draw this node into the gump frame bitmap (growing it when needed), then
    // recurse into the children that belong to the current page.
    void __thiscall Render(CGenericGump *gump);
    // 0x00435db0  size 0xab  constructor (parent, type, page, group, x, y, w, h, art0, art1, text, flags, entryId, target);
    // `text` is a wide string that is copied; types 6 / 7 start with flag = target. RET 0x38.
    GumpNode(GumpNode *parent, int nodeType, int nodePage, int nodeGroup, int nx, int ny, int nw, int nh, int a0, int a1,
             const uint16_t *wtext, int f38, int entry, int f40);
    // 0x00434e00: scalar deleting destructor (children first, then the text, then the node when flags & 1).
    GumpNode *__thiscall ScalarDeletingDtor(int flags);
    // 0x00435e60: first descendant whose group id matches, depth first; 0 when none.
    GumpNode *__thiscall FindDescendantByGroup(int groupId);
    // 0x00436340: deepest node under (px, py) that is on the current page; 0 when none.
    GumpNode *__thiscall HitTest(int px, int py);
    // 0x004363d0: mouse released at (px, py); returns 0 when the gump was closed by a button.
    int __thiscall OnMouseUp(int px, int py, CGenericGump *gump);
    // 0x00436550: key typed while this text entry node has the focus.
    void __thiscall OnKey(int key, CGenericGump *gump);
    // 0x00436730 / 0x00436770: count the selected check / radio nodes below, and write their ids.
    int __thiscall CountSwitches();
    void __thiscall WriteSwitches(char *packet);
    // 0x004367c0 / 0x004367f0: count the text entry nodes below, and write their ids and text.
    int __thiscall CountEntries();
    void __thiscall WriteEntries(char *packet);

    int32_t unk_00;         // +0x00  first constructor argument (parent group)  // ?
    int32_t page;           // +0x04  page this node is shown on (0 = every page)
    int32_t curPage;        // +0x08  page currently selected for the children  // ?
    int32_t group;          // +0x0c  group id
    int32_t x;              // +0x10
    int32_t y;              // +0x14
    int32_t w;              // +0x18
    int32_t h;              // +0x1c
    int32_t art0;           // +0x20  art id / font id / tile id  // ?
    int32_t art1;           // +0x24  second (pressed / selected) art id
    GumpNode *children;     // +0x28
    GumpNode *next;         // +0x2c  next sibling
    int32_t type;           // +0x30  1,5,6,7 gump art; 2 tile art; 3 resizepic; 4,8 text; 6,7 radio / check; 8 text entry
    void *text;             // +0x34  text / edit string
    int32_t unk_38;
    int32_t entryId;        // +0x3c  switch / text entry id
    int32_t unk_40;
    int32_t flag;           // +0x44  selected (radio / check) or editing (text entry)
};

static_assert(sizeof(GumpNode) == 0x48, "GumpNode size (operator new 0x48)");
static_assert(offsetof(GumpNode, page) == 0x04, "GumpNode::page");
static_assert(offsetof(GumpNode, group) == 0x0c, "GumpNode::group");
static_assert(offsetof(GumpNode, x) == 0x10, "GumpNode::x");
static_assert(offsetof(GumpNode, art0) == 0x20, "GumpNode::art0");
static_assert(offsetof(GumpNode, children) == 0x28, "GumpNode::children");
static_assert(offsetof(GumpNode, next) == 0x2c, "GumpNode::next");
static_assert(offsetof(GumpNode, type) == 0x30, "GumpNode::type");
static_assert(offsetof(GumpNode, text) == 0x34, "GumpNode::text");
static_assert(offsetof(GumpNode, entryId) == 0x3c, "GumpNode::entryId");
static_assert(offsetof(GumpNode, flag) == 0x44, "GumpNode::flag");
static_assert(sizeof(GumpNode) == 0x48, "GumpNode size (operator new 0x48)");

class CGenericGump : public CGump {
public:
    virtual int IsTag6c() { return 1; }
    // slot override (gump virtual-slot prep): own copy of the shared body
    virtual int IsTag80();
    // 0x004344b0: CGenericGump(x, y, layout, textTable, owner, serial). Parses the layout
    // string block by block and builds the widget tree, then the frame bitmap.
    CGenericGump(int x, int y, char *layout, char **textTable, int owner, int serial);
    // 0x00434e50: send the gump menu choice packet 0xB1 (owner serial, gump id, button id,
    // switches, text entries).  // ? name from the first pass
    void __thiscall SendResponse(int buttonId);
    ~CGenericGump();
    void Dtor() { this->CGenericGump::~CGenericGump(); }
    // 0x00435060: a key went to the text entry that has the focus (unk_c0), if any.
    virtual void OnKey(int key);
    // 0x00435080: drop the text entry focus (unk_c0) and redraw.
    void __thiscall ClearEntryFocus();
    // 0x004350b0: true when (px, py) is on a node of the current page.
    virtual int HitTest(int x, int y);
    // Widget adders: append a node to group `group` (0 = top level); nothing happens when the group does not exist.
    void __thiscall AddGumpPic(int px, int py, int artId, int nodePage, int group);               // 0x00435120
    void __thiscall AddResizePic(int px, int py, int artId, int w, int h, int nodePage, int group); // 0x00435230
    void __thiscall AddTilePic(int px, int py, int tileId, int nodePage, int group);               // 0x00435330
    void __thiscall AddText(int px, int py, int hue, const uint16_t *text, int nodePage, int group);   // 0x00435470
    void __thiscall AddTextEntry(int px, int py, int w, int h, int font, const uint16_t *text, int entryId, int nodePage, int group);   // 0x004355a0
    void __thiscall AddButton(int px, int py, int upArt, int downArt, int replies, int targetPage, int buttonId, int nodePage, int group);   // 0x004356a0
    void __thiscall AddCheckbox(int px, int py, int offArt, int onArt, int state, int switchId, int nodePage, int group);   // 0x004357e0
    void __thiscall AddRadio(int px, int py, int offArt, int onArt, int state, int switchId, int nodePage, int group);      // 0x004358f0
    // 0x00435a00: free and clear the frame bitmap, then draw every node of the current page into it.
    void __thiscall RebuildFrame();
    // 0x00435ab0: grow the frame bitmap to at least (right, bottom), keeping its pixels.
    void __thiscall GrowFrame(int right, int bottom);
    // 0x00435bb0: sends response 0 and closes the gump when it may be closed. Arguments unused.  // ?
    virtual void RButtonUp(int x, int y);
    // 0x00435bf0 / 0x00435cd0 / 0x00435d60: mouse down, up and move at (px, py).
    virtual void MouseDown(int x, int y);
    virtual void MouseUp(int x, int y);
    virtual void MouseMove(int x, int y);

    int32_t unk_94;         // +0x94  set to 1
    int32_t unk_98;         // +0x98
    int32_t unk_9c;         // +0x9c
    int32_t unk_a0;         // +0xa0
    int32_t canMove;        // +0xa4  1, "nomove" clears it
    int32_t canDispose;     // +0xa8  1, "nodispose" clears it
    int32_t canClose;       // +0xac  1, "noclose" clears it
    int32_t ownerSerial;    // +0xb0
    int32_t gumpId;         // +0xb4  serial argument of the constructor
    GumpNode *groups;       // +0xb8  head of the root group list
    int32_t unk_bc;         // +0xbc
    int32_t unk_c0;         // +0xc0
};

static_assert(offsetof(CGenericGump, unk_94) == 0x94, "CGenericGump::unk_94");
static_assert(offsetof(CGenericGump, canMove) == 0xa4, "CGenericGump::canMove");
static_assert(offsetof(CGenericGump, canDispose) == 0xa8, "CGenericGump::canDispose");
static_assert(offsetof(CGenericGump, canClose) == 0xac, "CGenericGump::canClose");
static_assert(offsetof(CGenericGump, ownerSerial) == 0xb0, "CGenericGump::ownerSerial");
static_assert(offsetof(CGenericGump, gumpId) == 0xb4, "CGenericGump::gumpId");
static_assert(offsetof(CGenericGump, groups) == 0xb8, "CGenericGump::groups");
static_assert(offsetof(CGenericGump, unk_c0) == 0xc0, "CGenericGump::unk_c0");
static_assert(sizeof(CGenericGump) == 0xc4, "CGenericGump size (operator new(0xc4))");
