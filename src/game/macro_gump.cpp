#include "macro_gump.h"

#include <windows.h>

#include <cstring>
#include <cstdlib>

#include "../runtime/ported.h"
#include "batch01_helpers.h"
#include "globals.h"
#include "list_box_control_gump.h"
#include "crt.h"
#include "vcall.h"
#include "bb_msg_gump_ports.h"
#include "mouse_manager.h"
#include "gump.h"
#include "gump_base_ops_ports.h"
#include "char_flow_gumps.h"
#include "vtables.h"
#include "input_control_gump.h"

// The SEH frames of the original (state counter around every operator new, handlers 0x499208 / 0x499236 /
// 0x499277 / 0x4991f3 / 0x4992ae / 0x4992c8 / 0x4992eb) are omitted. Every "p = operator new(size); obj = p ? ctor(p,
// ...) : 0" of the original is kept inside the helpers below; the code after a helper dereferences the result without
// a NULL check, as the original does.

#define g_mouseObj GLOBAL_PTR(void, g_mouse_storage)

// 0x00406a80: name up to the first '(' copied into a static buffer; ECX = the name, returns the buffer.
#define Orig_StripParenSuffix(a1) (StripParenSuffix(a1))
// 0x00420a70: add an item to a list box, RET 8, (id, text).
#define Orig_ListBoxAddItem(self, a1, a2) (reinterpret_cast<CListBoxControlGump *>(self)->AddItem(a1, a2))
// 0x00420290: list box redraw, RET 4, (-1).
#define Orig_ListBoxRedraw(self, a1) (reinterpret_cast<CListBoxControlGump *>(self)->Redraw(a1))

// 0x00417890  size 0x37d
// Constructor of one macro row: frame0 bitmap (w x h), the command type box, then the argument widgets.
CMacroCmdGump::CMacroCmdGump(int x_, int y_, int w, int h, int cmdType, int cmdArg, const char *text, int val1, int val2)
{
    x = x_;
    y = y_;
    name = const_cast<char *>("macro cmd gump");
    dirty = 0;
    owner = nullptr;
    unk_50 = 0;
    pressed = 0;
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    width = w;
    height = h;
    frameIndex = 0;
    frameCount = 1;
    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CMacroCmdGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    frameData[1] = static_cast<int32_t>(reinterpret_cast<uintptr_t>(gfx::AllocTagged(height * width * 2 + 4, "macrocmd frame0")));
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(height * width * 2));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);

    typeBox = new CInputControlGump(4, 4, this, 0x98d, 0x98d, 2, 0x10, 1, 0x386, 0, 1, 0);
    typeBox->marginY = 4;
    typeBox->value = cmdType;
    if (typeBox->value != 0)
        strcpy(typeBox->text, g_macroCmdNames[cmdType]);
    typeBox->Redraw();
    gfx::AddChild(typeBox, this);

    argList = nullptr;
    argText = nullptr;
    val1Box = nullptr;
    val2Box = nullptr;
    RebuildArgWidgets();

    if (argList != nullptr) {
        argList->value = cmdArg;
        int type = typeBox->value;
        if (type == 5) {
            strcpy(argList->text, g_macroArgNames5[cmdArg]);
        } else if (static_cast<unsigned>(type) >= 8 && static_cast<unsigned>(type) <= 0xb) {
            strcpy(argList->text, g_macroArgNames8[cmdArg]);
        } else if (type == 0xd) {
            int i = cmdArg;
            while ((g_skillFlags[i] & 1) == 0)
                i++;
            strcpy(argList->text, g_skillNames + i * 0x50);
        } else if (type == 0xf) {
            // The entries are 8 bytes apart: {name, ?}.
            strcpy(argList->text, Orig_StripParenSuffix(g_spellNames[cmdArg * 2]));
        }
        argList->Redraw();
    }
    if (argText != nullptr) {
        strcpy(argText->text, text);
        argText->Redraw();
    }
    if (val1Box != nullptr) {
        val1Box->value = val1;
        val1Box->Redraw();
    }
    if (val2Box != nullptr) {
        val2Box->value = val2;
        val2Box->Redraw();
    }
}
HOOK_CTOR(0x00417890, CMacroCmdGump, (int, int, int, int, int, int, const char *, int, int));

HOOK_VTABLE(0x0049d008, CMacroCmdGump);
HOOK_SDD(0x00417c10, CMacroCmdGump);

// 0x00417c40  size 0x8b
// Destructor body: frees the frame bitmap and the frame table.
CMacroCmdGump::~CMacroCmdGump()
{
    if (frameData[1] != 0) {
        gfx::FreeTagged(gfx::bitmapOf(this), "macro cmd frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "macro cmd framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00417c40, CMacroCmdGump);

// 0x00417cd0  size 0x54
// The type box changed: choosing 0x63 deletes this row, any other type rebuilds the argument widgets.
void __thiscall CMacroCmdGump::OnChildChanged(int childArg)
{
    CInputControlGump * child = reinterpret_cast<CInputControlGump *>(childArg);
    CGump *par = parent;
    if (child == typeBox) {
        if (typeBox->value == 0x63) {
            static_cast<CMacroListGump *>(par)->RemoveRow(this);
            if (par != nullptr)
                PORT_VSLOT1(par, 0x44, void, CGump *)(nullptr);
            return;
        }
        RebuildArgWidgets();
    }
    if (par != nullptr)
        PORT_VSLOT1(par, 0x44, void, CGump *)(this);
}
HOOK_VMETHOD(0x00417cd0, CMacroCmdGump, OnChildChanged);

// 0x00417d30  size 0x17
// Passes (0, 0) on to the parent's slot 0x1c.
void __thiscall CMacroCmdGump::RButtonUp(int, int)
{
    CGump *par = parent;
    if (par != nullptr)
        PORT_VSLOT2(par, 0x1c, void, int, int)(0, 0);
}
HOOK_VMETHOD(0x00417d30, CMacroCmdGump, RButtonUp);

// 0x00417d50  size 0x1f
// Mouse down: remember the press, bring the parent to the front.
void __thiscall CMacroCmdGump::MouseDown(int, int)
{
    pressed = 1;
    if (parent != nullptr)
        CGump_BringToFront(parent, 0);
}
HOOK_VMETHOD(0x00417d50, CMacroCmdGump, MouseDown);

// 0x00417d70  size 0x14
// Mouse up: release.
void __thiscall CMacroCmdGump::MouseUp(int, int)
{
    pressed = 0;
    g_dragGump = nullptr;
}
HOOK_VMETHOD(0x00417d70, CMacroCmdGump, MouseUp);

// 0x00417d90  size 0x28c
// Deletes the four argument widgets and creates the ones the command type needs: a list-picked argument for
// types 5, 8-11, 13 and 15, a free text argument for types 1-4 (and 22 when more than 21 commands exist).
void __thiscall CMacroCmdGump::RebuildArgWidgets()
{
    int listArg = 0;
    int textArg = 0;

    if (argList != nullptr) {
        decomp::vdelete(argList);
        argList = nullptr;
    }
    if (argText != nullptr) {
        decomp::vdelete(argText);
        argText = nullptr;
    }
    if (val1Box != nullptr) {
        decomp::vdelete(val1Box);
        val1Box = nullptr;
    }
    if (val2Box != nullptr) {
        decomp::vdelete(val2Box);
        val2Box = nullptr;
    }

    int type = typeBox->value;
    // Jump table at 0x0041801c over (type - 1), 0..0x15.
    switch (type) {
    case 1: case 2: case 3: case 4:
        textArg = 1;
        break;
    case 5: case 8: case 9: case 10: case 11: case 13: case 15:
        listArg = 1;
        break;
    case 22:
        if (g_macroCmdCount >= 0x16)
            textArg = 1;
        break;
    default:
        break;
    }

    int ax = typeBox->width + typeBox->x + 4;
    if (listArg != 0) {
        argList = new CInputControlGump(ax, 4, this, 0x98e, 0x98e, 2, 0x10, 1, 0x386, 0, 1, 0);
        argList->marginY = 4;
        argList->value = 0;
        if (type == 5) {
            strcpy(argList->text, g_macroArgNames5[0]);
        } else if (static_cast<unsigned>(type) >= 8 && static_cast<unsigned>(type) <= 0xb) {
            strcpy(argList->text, g_macroArgNames8[0]);
        } else if (type == 0xd) {
            int i = 0;
            while ((g_skillFlags[i] & 1) == 0)
                i++;
            strcpy(argList->text, g_skillNames + i * 0x50);
        } else if (type == 0xf) {
            strcpy(argList->text, Orig_StripParenSuffix(g_spellNames[0]));
        }
        argList->Redraw();
        gfx::AddChild(argList, this);
    }
    if (textArg != 0) {
        argText = new CInputControlGump(ax, 4, this, 0x98e, 0x98e, 0, 0x50, 1, 0x386, 1, 0, 0);
        argText->marginX = 0xa;
        argText->marginY = 4;
        gfx::AddChild(argText, this);
    }
}
HOOK_METHOD(0x00417d90, CMacroCmdGump, RebuildArgWidgets);

// 0x00418070  size 0x465
// Opens the drop-down list for the type box (all commands plus "(NONE)") or for the argument box (contents depend on
// the command type), positions it under `anchor` and hands it the mouse.
void __thiscall CMacroCmdGump::Slot64(int anchorArg)
{
    CGump * anchor = reinterpret_cast<CGump *>(anchorArg);
    int ox = 0;
    int oy = 0;
    CListBoxControlGump *list;

    gfx::GumpOrigin(this, &ox, &oy);
    CInputControlGump *typeEntry = typeBox;
    if (anchor == typeEntry) {
        list = new CListBoxControlGump(anchor->x + ox - 2, anchor->height + anchor->y + oy + 1, 0x9b0, anchor->width + 4, 0, 1, 0x386, 0);
        if (g_macroCmdCount + 1 > 1) {
            int i = 1;
            const char *const *p = g_macroCmdNames + 1;
            do {
                Orig_ListBoxAddItem(list, i, *p);
                i++;
                p++;
            } while (i < g_macroCmdCount + 1);
        }
        Orig_ListBoxAddItem(list, 0x63, "(NONE)");
    } else {
        CInputControlGump *argEntry = argList;
        if (anchor != argEntry)
            return;
        int type = typeEntry->value;
        if (type == 5) {
            list = new CListBoxControlGump(anchor->x + ox - 2, anchor->height + anchor->y + oy + 1, 0x9b5, anchor->width + 4, 0, 1, 0x386, 0);
            const char *const *p = g_macroArgNames5;
            int i = 0;
            do {
                Orig_ListBoxAddItem(list, i, *p);
                p++;
                i++;
            } while (p < g_macroArgNames5 + 8);
        } else {
            if (anchor != argEntry)
                return;
            if (static_cast<unsigned>(type) >= 8 && static_cast<unsigned>(type) <= 0xb) {
                list = new CListBoxControlGump(anchor->x + ox - 2, anchor->height + anchor->y + oy + 1, 0x9b5, anchor->width + 4, 0, 1, 0x386, 0);
                if (g_macroArgNames8[0] != nullptr) {
                    int i = 0;
                    const char *nextName;
                    do {
                        int t = typeBox->value;
                        if (static_cast<unsigned>(t) >= 0xa && static_cast<unsigned>(t) <= 0xb) {
                            if (g_macroArgFlags8[i] != 0)
                                Orig_ListBoxAddItem(list, i, g_macroArgNames8[i]);
                        } else {
                            Orig_ListBoxAddItem(list, i, g_macroArgNames8[i]);
                        }
                        nextName = g_macroArgNames8[i + 1];
                        i++;
                    } while (nextName != nullptr);
                }
            } else {
                if (anchor != argEntry)
                    return;
                if (typeEntry->value == 0xd) {
                    list = new CListBoxControlGump(anchor->x + ox - 2, anchor->height + anchor->y + oy + 1, 0x9b5, anchor->width + 4, 0, 1, 0x386, 10);
                    if (g_skillCount > 0) {
                        int i = 0;
                        do {
                            int id = g_skillOrder[i];
                            if ((g_skillFlags[id] & 1) != 0)
                                Orig_ListBoxAddItem(list, id, g_skillNames + id * 0x50);
                            i++;
                        } while (i < g_skillCount);
                    }
                } else {
                    if (anchor != argEntry)
                        return;
                    if (typeEntry->value != 0xf)
                        return;
                    list = new CListBoxControlGump(anchor->x + ox - 2, anchor->height + anchor->y + oy + 1, 0x9b5, anchor->width + 4, 0, 1, 0x386, 10);
                    const char *const *entry = g_spellNames;
                    const char *spell = entry[0];
                    int i = 0;
                    if (strlen(spell) != 0) {
                        do {
                            Orig_ListBoxAddItem(list, i, Orig_StripParenSuffix(spell));
                            spell = entry[2];
                            entry += 2;
                            i++;
                        } while (strlen(spell) != 0);
                    }
                }
            }
        }
    }

    Orig_ListBoxRedraw(list, -1);
    list->visible = 1;
    gfx::AddChild(list, nullptr);
    list->unk_94 = static_cast<int32_t>(reinterpret_cast<uintptr_t>(anchor));
    g_dragGump = list;
    int mx;
    int my;
    reinterpret_cast<CMouseManager *>(g_mouseObj)->GetMousePos(&mx, &my);   // CMouseManager::GetMousePos
    int lx = mx;
    int ly = my;
    list->CGump::ScreenToLocal(&lx, &ly);          // CGump::ScreenToLocal
    PORT_VSLOT2(list, 0x28, void, int, int)(lx, ly);
}
HOOK_VMETHOD(0x00418070, CMacroCmdGump, Slot64);

// 0x004184e0  size 0x247
// Constructor of the macro list: frame0 bitmap (w x h, left empty), scroll up / down buttons.
CMacroListGump::CMacroListGump(int x_, int y_, int w, int h, int artId, int arg6)
{
    x = x_;
    name = const_cast<char *>("macro list gump");
    y = y_;
    dirty = 0;
    owner = nullptr;
    unk_a8 = 0;
    pressed = 0;
    unk_a0 = 0;
    unk_9c = 0;
    clickState = 0;
    pressTick = 0;
    unk_60 = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    width = w;
    height = h;
    frameIndex = 0;
    frameCount = 1;
    frameData = static_cast<int32_t *>(gfx::AllocTagged(8, "framedata for CMacroListGump"));
    memset(frameData, 0, static_cast<size_t>(frameCount) * 8);
    frameData[1] = static_cast<int32_t>(reinterpret_cast<uintptr_t>(gfx::AllocTagged(height * width * 2 + 4, "cfg frame0")));
    memset(reinterpret_cast<uint8_t *>(gfx::bitmapOf(this)) + 4, 0, static_cast<size_t>(height * width * 2));
    gfx::bitmapOf(this)[0] = static_cast<uint16_t>(width);
    gfx::bitmapOf(this)[1] = static_cast<uint16_t>(height);
    arg_c8 = arg6;
    unk_cc = 0;
    unk_c4 = 0;
    unk_bc = 0;
    unk_b8 = 0;
    head = nullptr;
    unk_c0 = 0;
    unk_50 = artId;

        CButtonGump *b;
    b = new CButtonGump(width - 8, 4, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 2, 0, PORT_ADDR(&CGump_004106a0::ScrollRowsDown), 0x983, 0x983, 0);
    child_94 = b;
    child_94->visible = 0;
    child_94->artHover = 0x984;
    gfx::AddChild(child_94, this);

    b = new CButtonGump(width - 8, height - 0xe, static_cast<int>(reinterpret_cast<uintptr_t>(this)), 2, 0, PORT_ADDR(&CGump_004106a0::ScrollRowsUp), 0x985, 0x985, 0);
    child_98 = b;
    child_94->visible = 0;   // (sic) the first button again, as in the original; the second is never hidden
    child_98->artHover = 0x986;
    gfx::AddChild(child_98, this);
}
HOOK_CTOR(0x004184e0, CMacroListGump, (int, int, int, int, int, int));

HOOK_VTABLE(0x0049d120, CMacroListGump);
HOOK_SDD(0x00418730, CMacroListGump);

// 0x00418760  size 0xc7
// Destructor body: deletes the row list, frees the frame bitmap and the frame table.
CMacroListGump::~CMacroListGump()
{
    if (head != nullptr) {
        CMacroListNode *nextNode;
        do {
            nextNode = head->next;
            if (head->row != nullptr)
                decomp::vdelete(head->row);
            operator delete(head);
            head = nextNode;
        } while (nextNode != nullptr);
    }
    if (frameData[1] != 0) {
        gfx::FreeTagged(gfx::bitmapOf(this), "MacroList frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameData != nullptr) {
        gfx::FreeTagged(frameData, "MacroList framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00418760, CMacroListGump);

// 0x00418830  size 0xf1
// Appends a macro row (x 10, 0x10e x 0x1a) one row below the lowest existing row; the first row goes to y = val2.
// A child counts as a row when its virtual slot 0xd8 returns non-zero.
void __thiscall CMacroListGump::AddRow(int cmdType, int cmdArg, const char *text, int val1, int val2)
{
    CGump *last = nullptr;
    int rows = 0;
    int maxY = 0;
    for (CGump *c = lastChild; c != nullptr; c = c->prev) {
        if (PORT_VSLOT0(c, 0xd8, int)() != 0 && c->y > maxY) {
            maxY = c->y;
            rows++;
            last = c;
        }
    }

    int rowY = maxY;
    if (last != nullptr)
        rowY = rowY + 0x1a;
    else
        rowY = val2;

    CMacroCmdGump *row = new CMacroCmdGump(10, rowY, 0x10e, 0x1a, cmdType, cmdArg, text, val1, val2);
    if (rowY < 8 || rowY + 0x1a > height)
        row->visible = 0;
    gfx::AddChild(row, this);
    if (rows == 1)
        child_98->visible = 1;
}
HOOK_METHOD(0x00418830, CMacroListGump, AddRow);

// 0x00418930  size 0xdf
// Deletes `row` and moves the rows below it up by 0x1a; appends an empty row when the list was full with a used last
// row or when the lowest row was removed.
void __thiscall CMacroListGump::RemoveRow(CMacroCmdGump *row)
{
    int maxY = 0;
    int rows = 0;
    CGump *lastRow = nullptr;
    CGump *lowest = nullptr;
    int needBlank = 0;

    for (CGump *c = lastChild; c != nullptr; c = c->prev) {
        if (PORT_VSLOT0(c, 0xd8, int)() != 0) {
            rows++;
            lastRow = c;
            if (c->y > maxY) {
                maxY = c->y;
                lowest = c;
            }
        }
    }
    if (rows == 10 && static_cast<CMacroCmdGump *>(lastRow)->typeBox->value != 0)
        needBlank = 1;
    else if (row == lowest)
        needBlank = 1;

    for (CGump *c = lastChild; c != nullptr; c = c->prev) {
        if (PORT_VSLOT0(c, 0xd8, int)() != 0 && c->y > row->y) {
            c->y = c->y + -0x1a;
            if (c->y + 0x1a < height)
                c->visible = 1;
        }
    }
    if (row != nullptr)
        decomp::vdelete(row);
    if (needBlank != 0)
        AddRow(0, 0, nullptr, 4, 0xe);
}
HOOK_METHOD(0x00418930, CMacroListGump, RemoveRow);

// 0x00418a10  size 0x99
// A row changed: when it is the lowest row and fewer than 10 rows exist, append an empty row; tell the parent.
void __thiscall CMacroListGump::OnChildChanged(int rowArg)
{
    CGump * row = reinterpret_cast<CGump *>(rowArg);
    CGump *target = nullptr;
    CGump *lowest = nullptr;
    int maxY = 0;

    if (row != nullptr && PORT_VSLOT0(row, 0xd8, int)() != 0)
        target = row;
    if (target != nullptr) {
        int rows = 0;
        for (CGump *c = lastChild; c != nullptr; c = c->prev) {
            if (PORT_VSLOT0(c, 0xd8, int)() != 0) {
                rows++;
                if (c->y > maxY) {
                    maxY = c->y;
                    lowest = c;
                }
            }
        }
        if (rows < 10 && lowest == target)
            AddRow(0, 0, nullptr, 4, 0xe);
    }
    if (parent != nullptr)
        PORT_VSLOT1(parent, 0x44, void, CGump *)(this);
}
HOOK_VMETHOD(0x00418a10, CMacroListGump, OnChildChanged);

// 0x00418ab0  size 0x5d
// Mouse down: remembers the press, brings the parent to the front, tracks the double-click state.
void __thiscall CMacroListGump::MouseDown(int mx, int my)
{
    pressed = 1;
    unk_6c = mx;
    unk_70 = my;
    if (parent != nullptr)
        CGump_BringToFront(parent, 0);
    if (clickState == 1) {
        clickState = 2;
        return;
    }
    clickState = 1;
    pressTick = static_cast<int32_t>(GetTickCount());
}
HOOK_VMETHOD(0x00418ab0, CMacroListGump, MouseDown);


void __thiscall CMacroListGump::MouseUp(int, int)
{
    pressed = 0;
    if (unk_9c != 0) {
        unk_9c = 0;
        clickState = 0;
        g_dragGump = nullptr;
        return;
    }
    if (clickState == 2)
        clickState = 0;
    g_dragGump = nullptr;
}

void __thiscall CMacroListGump::MouseMove(int mx, int my)
{
    int wasPressed = pressed;
    if (wasPressed != 0 && static_cast<uint32_t>(clickState) > 0) {
        if (abs(mx - unk_6c) > 4 || abs(my - unk_70) > 4) {
            unk_9c = 1;
            clickState = 0;
        }
    }
    if (wasPressed != 0 && parent != nullptr)
        parent->Move(mx - unk_6c, my - unk_70, 1, nullptr, 1);
}

void __thiscall CMacroListGump::Tick()
{
    DWORD now = GetTickCount();
    DWORD dbl = GetDoubleClickTime();
    if (clickState == 1 && now - static_cast<DWORD>(pressTick) > dbl) {
        clickState = 0;
        if (pressed != 0)
            unk_9c = 1;
    }
}

// 0x00418b10  size 0xbf
// Draws frame 0 (or frame 1 when +0x60 is set) at the absolute origin when the clip allows it, then the children.
void __thiscall CMacroListGump::Draw(int *clip)
{
    int ox = 0;
    int oy = 0;
    gfx::GumpOrigin(this, &ox, &oy);

    uint16_t *bmp;
    if (unk_60 == 0)
        bmp = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[1])));
    else
        bmp = reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(frameData[3])));
    int rect[4];
    rect[0] = ox;
    rect[1] = oy;
    rect[2] = bmp[0] + ox;
    rect[3] = oy + bmp[1];

    // 0x0043b540: thiscall on the gump manager object, (clip, &rect), RET 8
    if (PORT_THISCALL2(&GumpRectIntersects, int, g_gumpManager, void *, int *)(clip, rect) != 0) {
        GFX_BlitFrame(gfx::artPlain(), bmp + 2, clip, bmp[0], bmp[1], ox, oy, g_backBuffer, 0x280, 0x1e0);
        for (CGump *c = firstChild; c != nullptr; c = c->next)
            PORT_VSLOT1(c, kGumpSlot_Draw, void, void *)(clip);
    }
}
HOOK_VMETHOD(0x00418b10, CMacroListGump, Draw);
