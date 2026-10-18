// CGump: base class of every gump (window) in the client. gumps.cxx.
//
// Layout from decomp/FINDINGS.md (base ctor 0x0043c3d0, dtor 0x0043c470,
// vtable 0x0049ed40). Derived classes start their own fields at +0x94.
// The class has the compiler's vptr (+0) and vftable (HOOK_VTABLE in gump_base_ops.cpp); the virtual
// functions are the 69 slots of the original table 0x0049ed40, declared in slot order below. The
// derived gumps override them (docs/matching.md "Gump virtual slots").
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"
#include "map_world_ops_ports.h"
#include "globals.h"

class CObj;

class CGump {
public:
    char *name;             // +0x04  "bark gump", "paperdoll gump", ...
    int32_t unk_08;
    int32_t unk_0c;
    int32_t frameIndex;     // +0x10
    int32_t frameCount;     // +0x14
    int32_t rowStride;      // +0x18  (width + 7) & ~7
    int32_t width;          // +0x1c
    int32_t height;         // +0x20
    int32_t unk_24;
    int32_t unk_28;
    int32_t x;              // +0x2c  relative to parent
    int32_t y;              // +0x30
    CObj *owner;            // +0x34
    int32_t *frameData;     // +0x38  from 0x004462f0; slot i bitmap at +4 + i*8
    CGump *prev;            // +0x3c  previous sibling
    CGump *next;            // +0x40  next sibling
    CGump *lastChild;       // +0x44
    CGump *parent;          // +0x48
    CGump *firstChild;      // +0x4c
    int32_t unk_50;
    uint8_t layer;          // +0x54  tab number in the config gump
    uint8_t unk_55[3];
    int32_t unk_58;
    int32_t dirty;          // +0x5c
    int32_t unk_60;
    int32_t unk_64;
    int32_t visible;        // +0x68  default 1
    int32_t unk_6c;
    int32_t unk_70;
    int32_t unk_74;
    int32_t unk_78;
    int32_t unk_7c;
    int32_t unk_80;
    int32_t unk_84;
    int32_t unk_88;
    int32_t unk_8c;
    int32_t unk_90;

    // 0x0043b140: move by (dx, dy), accumulate the dirty rect, move owned gumps.
    void __thiscall Move(int dx, int dy, int isRoot, int *dirty, int clamp);
    // 0x0043b970: deepest child gump under (x, y), or 0.
    CGump *__thiscall FindGumpAt(int x, int y, CGump *skip);   // not the slot HitTest(int,int)
    // 0x004011c0: true when a later sibling (+0x40) that answers slot 0x84 intersects this gump's rect.
    int __thiscall AnyChildOverlapsSelf();

    // Second pass, gump_base_ops.cpp (gumps.cxx 0x0043a850..0x0043c640).
    // 0x0043c2a0: full constructor, RET 0x1c; (x, y, a3 (not read), owner, frameCount, width, height).
    CGump(int x, int y, int a3, CObj *owner, int frameCount, int width, int height);
    // 0x0043b4e0 / 0x0043b510: add / subtract the offsets of this gump and every ancestor, RET 8.
    void __thiscall LocalToScreen(int *x, int *y);
    void __thiscall ScreenToLocal(int *x, int *y);
    // 0x0043ba70: move to the end of its sibling list, RET 4.
    void __thiscall BringToFront(int flag);
    // 0x0043bcc0: link as the last child of `parent` (or into the top-level list when null), RET 8 (flag not read).
    void __thiscall LinkInto(CGump *parent, int flag);

    // The virtual slots, in table order. The bodies are in gump_base_ops.cpp (the defaults of the original's table 0x0049ed40;
    // the empty ones are the original's shared empty procedures, which the linker folds onto the free stubs Stub_00401250/60,
    // CircleTrans_Nullsub, Nop_00406720, Nop_0040caf0, Nop_00431ce0, Stub_00421390, kept as plain functions too: they are
    // called on non-gump objects and used as button callbacks). The 43 tag queries (class-identity tests, 0x68..0x110) are
    // inline virtuals returning 0, except 0x7c and 0x80 (1); the folding gives the original's U1_Return0 / U1_Return1.
    virtual ~CGump();                                          // +0x00 default CGump::ScalarDeletingDtor 0x0043c3a0
    virtual void PreDraw(int *clip);                           // +0x04 default CGump::PreDraw 0x0043b5f0
    virtual void Draw(int *clip);                              // +0x08 default CGump::Draw 0x0043b7b0
    virtual int HitTest(int x, int y);                         // +0x0c default CGump::HitTest 0x0043a850
    virtual void MouseDown(int x, int y);                      // +0x10 default CGump::MouseDown 0x004242b0
    virtual void MouseUp(int x, int y);                        // +0x14 default CGump::MouseUp 0x0043c640
    virtual void RButtonDown(int x, int y);                    // +0x18 default CGump::RButtonDown 0x004242b0
    virtual void RButtonUp(int x, int y);                      // +0x1c default CGump::RButtonUp 0x004242b0
    virtual void MouseOver(int x, int y);                      // +0x20 default CGump::MouseOver 0x0040c990
    virtual void MouseLeave();                                 // +0x24 default CGump::MouseLeave 0x0043c660
    virtual void MouseMove(int x, int y);                      // +0x28 default CGump::MouseMove 0x004242b0
    virtual void Tick();                                       // +0x2c default CGump::Tick 0x00406720
    virtual void OnScroll(int a, int b, int c);                // +0x30 default CGump::OnScroll 0x00431ce0
    virtual void OnScrollEnd(int a, int b, int c);             // +0x34 default CGump::OnScrollEnd 0x00431ce0
    virtual void Resize(int w, int h);                         // +0x38 default CGump::Resize 0x00401250
    virtual void OnKey(int key);                               // +0x3c default CGump::OnKey 0x00421390
    virtual void Slot40(int a);                                // +0x40 default CGump::Slot40 0x00401260
    virtual void OnChildChanged(int child);                    // +0x44 default CGump::OnChildChanged 0x00401260
    virtual void OnTab();                                      // +0x48 default CGump::OnTab 0x0040caf0
    virtual void OnOk();                                       // +0x4c default CGump::OnOk 0x0040caf0
    virtual void OnCancel();                                   // +0x50 default CGump::OnCancel 0x0040caf0
    virtual void OnFocus(int field);                           // +0x54 default CGump::OnFocus 0x00401260
    virtual void OnEnter(int a);                               // +0x58 default CGump::OnEnter 0x00401260
    virtual void Update();                                     // +0x5c default CGump::Update 0x0040caf0
    virtual void Slot60();                                     // +0x60 default CGump::Slot60 0x0040caf0
    virtual void Slot64(int a);                                // +0x64 default CGump::Slot64 0x00401260
    virtual int IsTag68() { return 0; }                        // +0x68 default ::U1_Return0 0x0040c7c0
    virtual int IsTag6c() { return 0; }                        // +0x6c default ::U1_Return0 0x0040c7c0
    virtual int IsTag70() { return 0; }                        // +0x70 default ::U1_Return0 0x0040c7c0
    virtual int IsTag74() { return 0; }                        // +0x74 default ::U1_Return0 0x0040c7c0
    virtual int IsTag78() { return 0; }                        // +0x78 default ::U1_Return0 0x0040c7c0
    virtual int IsTag7c() { return 1; }                        // +0x7c default ::U1_Return1 0x0040c7d0
    virtual int IsTag80() { return 1; }                        // +0x80 default ::U1_Return1 0x0040c7d0
    virtual int IsTag84() { return 0; }                        // +0x84 default ::U1_Return0 0x0040c7c0
    virtual int IsTag88() { return 0; }                        // +0x88 default ::U1_Return0 0x0040c7c0
    virtual int IsTag8c() { return 0; }                        // +0x8c default ::U1_Return0 0x0040c7c0
    virtual int IsTag90() { return 0; }                        // +0x90 default ::U1_Return0 0x0040c7c0
    virtual int IsTag94() { return 0; }                        // +0x94 default ::U1_Return0 0x0040c7c0
    virtual int IsTag98() { return 0; }                        // +0x98 default ::U1_Return0 0x0040c7c0
    virtual int IsTag9c() { return 0; }                        // +0x9c default ::U1_Return0 0x0040c7c0
    virtual int IsTaga0() { return 0; }                        // +0xa0 default ::U1_Return0 0x0040c7c0
    virtual int IsTaga4() { return 0; }                        // +0xa4 default ::U1_Return0 0x0040c7c0
    virtual int IsTaga8() { return 0; }                        // +0xa8 default ::U1_Return0 0x0040c7c0
    virtual int IsTagac() { return 0; }                        // +0xac default ::U1_Return0 0x0040c7c0
    virtual int IsTagb0() { return 0; }                        // +0xb0 default ::U1_Return0 0x0040c7c0
    virtual int IsTagb4() { return 0; }                        // +0xb4 default ::U1_Return0 0x0040c7c0
    virtual int IsTagb8() { return 0; }                        // +0xb8 default ::U1_Return0 0x0040c7c0
    virtual int IsTagbc() { return 0; }                        // +0xbc default ::U1_Return0 0x0040c7c0
    virtual int IsTagc0() { return 0; }                        // +0xc0 default ::U1_Return0 0x0040c7c0
    virtual int IsTagc4() { return 0; }                        // +0xc4 default ::U1_Return0 0x0040c7c0
    virtual int IsTagc8() { return 0; }                        // +0xc8 default ::U1_Return0 0x0040c7c0
    virtual int IsTagcc() { return 0; }                        // +0xcc default ::U1_Return0 0x0040c7c0
    virtual int IsTagd0() { return 0; }                        // +0xd0 default ::U1_Return0 0x0040c7c0
    virtual int IsTagd4() { return 0; }                        // +0xd4 default ::U1_Return0 0x0040c7c0
    virtual int IsTagd8() { return 0; }                        // +0xd8 default ::U1_Return0 0x0040c7c0
    virtual int IsTagdc() { return 0; }                        // +0xdc default ::U1_Return0 0x0040c7c0
    virtual int IsTage0() { return 0; }                        // +0xe0 default ::U1_Return0 0x0040c7c0
    virtual int IsTage4() { return 0; }                        // +0xe4 default ::U1_Return0 0x0040c7c0
    virtual int IsTage8() { return 0; }                        // +0xe8 default ::U1_Return0 0x0040c7c0
    virtual int IsTagec() { return 0; }                        // +0xec default ::U1_Return0 0x0040c7c0
    virtual int IsTagf0() { return 0; }                        // +0xf0 default ::U1_Return0 0x0040c7c0
    virtual int IsTagf4() { return 0; }                        // +0xf4 default ::U1_Return0 0x0040c7c0
    virtual int IsTagf8() { return 0; }                        // +0xf8 default ::U1_Return0 0x0040c7c0
    virtual int IsTagfc() { return 0; }                        // +0xfc default ::U1_Return0 0x0040c7c0
    virtual int IsTag100() { return 0; }                       // +0x100 default ::U1_Return0 0x0040c7c0
    virtual int IsTag104() { return 0; }                       // +0x104 default ::U1_Return0 0x0040c7c0
    virtual int IsTag108() { return 0; }                       // +0x108 default ::U1_Return0 0x0040c7c0
    virtual int IsTag10c() { return 0; }                       // +0x10c default ::U1_Return0 0x0040c7c0
    virtual int IsTag110() { return 0; }                       // +0x110 default ::U1_Return0 0x0040c7c0

    // The constructor and destructor a derived class's constructor and destructor call implicitly (they give a
    // derived constructor its state 0 and its funclet). The constructor is the old CtorBase, the destructor is 0x0043c470.
    CGump();
    void DtorBody() { this->CGump::~CGump(); }   // direct call: ~CGump is virtual now
};

static_assert(sizeof(CGump) == 0x94, "CGump size");
static_assert(offsetof(CGump, frameIndex) == 0x10, "CGump::frameIndex");
static_assert(offsetof(CGump, x) == 0x2c, "CGump::x");
static_assert(offsetof(CGump, owner) == 0x34, "CGump::owner");
static_assert(offsetof(CGump, firstChild) == 0x4c, "CGump::firstChild");
static_assert(offsetof(CGump, layer) == 0x54, "CGump::layer");
static_assert(offsetof(CGump, dirty) == 0x5c, "CGump::dirty");
static_assert(offsetof(CGump, visible) == 0x68, "CGump::visible");

// Known CGump vtable slots (byte offsets), from FINDINGS.md.
enum CGumpSlot {
    kGumpSlot_DeletingDtor = 0x00,   // (int flags)
    kGumpSlot_Draw = 0x08,           // (int clip)
    kGumpSlot_HitTest = 0x0c,        // (int x, int y)
    kGumpSlot_IsWaitingGump = 0x108,
};

// Original CGump methods not ported yet.
#define CGump_Dtor(self) (reinterpret_cast<CGump *>(self)->DtorBody())                          // base dtor body
#define CGump_AddChild(self, a1, a2) (reinterpret_cast<CGump *>(self)->LinkInto(a1, a2))        // link into parent (or top-level list), invalidate
#define CGump_BringToFront(self, a1) (reinterpret_cast<CGump *>(self)->BringToFront(a1))   // one int arg (0 at all known call sites), RET 4
#define FrameData_Alloc(o, a1, a2) PORT_THISCALL2(&Map_Alloc, int32_t *, (o), int, const char *)(a1, a2)   // on the allocator object at 0x0064cdac
#define g_frameDataAllocator GLOBAL_PTR(void, g_frameDataAllocator_storage)

// Top-level gump list.

// 0x0043a720 / 0x0043a820: commit the focused input gump (calls its "done editing" handler) / set the focus gump (0x006488d0).
void __cdecl Gump_CommitFocus();
void __stdcall Gump_SetFocus(CGump *gump);
