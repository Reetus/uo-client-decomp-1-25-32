// Third porting pass, batch u2: small vtable-only methods, button callbacks, jump thunks and the
// file manager destructor. Methods whose class has no header here are written as __fastcall free
// functions (ECX = this, EDX unused): same register and stack convention as __thiscall.
#include <cstdint>

#include "../runtime/ported.h"
#include "file_manager.h"
#include "crt.h"
#include "raw_field.h"
#include "gump.h"
#include "input_control_gump.h"
#include "init_skill_gump.h"
#include "scroll_choice_gump.h"
#include "char_ui_gumps.h"
#include "login_gumps.h"
#include "char_flow_gumps.h"
#include "circle_trans.h"
#include "mouse_manager.h"
#include "mapped_file.h"
#include "mapped_file_rw.h"
#include "vtable_stubs_u2_ports.h"
#include "vtables.h"
#include "globals.h"
#include "obj.h"
#include "terrain_tile.h"
#include "missile_gump.h"
#include "city_choice_gump.h"

#define g_gumpManager GLOBAL_PTR(void, g_gumpMgr_storage)
#define g_camFlag GLOBAL_AS(int32_t, g_worldPicker_storage)

// 0x0043ba70 CGump::BringToFront (RET 4), 0x0043a720 CommitFocus and 0x0043a820 SetFocus (RET 4) on the gump
// manager at 0x006488c0, 0x0041e0f0 CInputControlGump::Redraw (plain RET).
#define U2_BringToFront(g, f) PORT_THISCALL1(&CGump::BringToFront, void, g, int)(f)
#define U2_CommitFocus() PORT_THISCALL0(&Gump_CommitFocus, void, g_gumpManager)()
#define U2_SetFocus(g) PORT_THISCALL1(&Gump_SetFocus, void, g_gumpManager, void *)(g)
#define U2_Redraw(g) PORT_THISCALL0(&CInputControlGump::Redraw, void, g)()

// 0x0041c900  size 0x5
// Jump thunk to 0x0040e930 (CInitSkillGump::NextFromSkills), plain RET.
void __thiscall CInitSkillGump::OnOk()
{
    void *const self = this;
    PORT_THISCALL0(&CInitSkillGump::NextFromSkills, int, self)();
}
HOOK_VMETHOD(0x0041c900, CInitSkillGump, OnOk);

// 0x0041c910  size 0x5
// Jump thunk to 0x0040ea70 (not ported here), plain RET.
void __thiscall CInitSkillGump::OnCancel()
{
    void *const self = this;
    PORT_THISCALL0(&CInitSkillGump::ShowStatPage, int, self)();
}
HOOK_VMETHOD(0x0041c910, CInitSkillGump, OnCancel);

// 0x0041c920  size 0xe
// Shows the gump at +0x98 (visible = 1); returns that pointer in EAX. Plain RET.
void __thiscall CInitSkillGump::Slot60()
{
    void *const self = this;
    void *child = RAWF(void *, self, 0x98);
    RAWF(int32_t, child, 0x68) = 1;
}
HOOK_VMETHOD(0x0041c920, CInitSkillGump, Slot60);

// 0x0041d6e0  size 0x18
// Scroll choice gump: when a hover row is set, clear it (-1) and redraw (0x0041d830); else returns -1.
void __thiscall CScrollChoiceGump::MouseLeave()
{
    void *const self = this;
    int32_t row = g_scrollChoiceHoverRow;
    if (row == -1)
        return;
    g_scrollChoiceHoverRow = -1;
    PORT_THISCALL0(&CScrollChoiceGump::Redraw, int, self)();
}
HOOK_VMETHOD(0x0041d6e0, CScrollChoiceGump, MouseLeave);

// 0x0041ecf0  size 0x24
// Input control gump: when the pending flag (+0xd4) is set, clear it and redraw (0x0041e0f0) only if the
// word at +0xde is non-zero. Returns the old flag, or Redraw's EAX.
void __thiscall CInputControlGump::MouseLeave()
{
    void *const self = this;
    int32_t pending = RAWF(int32_t, self, 0xd4);
    if (pending != 0) {
        uint16_t w = RAWF(uint16_t, self, 0xde);
        RAWF(int32_t, self, 0xd4) = 0;
        if (w != 0)
            PORT_THISCALL0(&CInputControlGump::Redraw, int, self)();
    }
}
HOOK_VMETHOD(0x0041ecf0, CInputControlGump, MouseLeave);

// 0x004213a0  size 0x5
// Jump thunk to 0x0040ebe0 (CCharSelectGump::Play), plain RET.
void __thiscall CLoginGump::OnOk()
{
    void *const self = this;
    PORT_THISCALL0(&CCharSelectGump::Play, int, self)();
}
HOOK_VMETHOD(0x004213a0, CLoginGump, OnOk);

// 0x004213b0  size 0x5
// Jump thunk to 0x0040ed80 (CCharSelectGump::Disconnect), plain RET.
void __thiscall CLoginGump::OnCancel()
{
    void *const self = this;
    PORT_THISCALL0(&CCharSelectGump::Disconnect, int, self)();
}
HOOK_VMETHOD(0x004213b0, CLoginGump, OnCancel);

// 0x00421b80  size 0xa5
// Tab between the two input controls at +0x94 / +0x98 of a login-flow gump: bring the gump to the front,
// commit the focus, move the focus to the other control, redraw the focused one. Plain RET.
void __thiscall CAcctLoginGump::OnTab()
{
    void *const self = this;
    void *prevFocus = g_focusedInput;
    U2_BringToFront(self, 0);
    U2_CommitFocus();
    void *a = RAWF(void *, self, 0x94);
    if (prevFocus == a) {
        RAWF(int32_t, a, 0xd8) = 0;
        U2_Redraw(RAWF(void *, self, 0x94));
        RAWF(int32_t, RAWF(void *, self, 0x98), 0xd8) = 1;
        U2_SetFocus(RAWF(void *, self, 0x98));
    } else {
        void *b = RAWF(void *, self, 0x98);
        if (prevFocus == b) {
            RAWF(int32_t, b, 0xd8) = 0;
            U2_Redraw(RAWF(void *, self, 0x98));
            RAWF(int32_t, RAWF(void *, self, 0x94), 0xd8) = 1;
            U2_SetFocus(RAWF(void *, self, 0x94));
        }
    }
    void *focus = g_focusedInput;
    if (focus != nullptr)
        U2_Redraw(focus);
}
HOOK_VMETHOD(0x00421b80, CAcctLoginGump, OnTab);

// 0x00421c30  size 0x5
// Jump thunk to 0x0040f3b0 (CAcctLoginGump::Connect), plain RET.
void __thiscall CAcctLoginGump::OnOk()
{
    void *const self = this;
    PORT_THISCALL0(&CAcctLoginGump::Connect, int, self)();
}
HOOK_VMETHOD(0x00421c30, CAcctLoginGump, OnOk);

// 0x00421c40  size 0x5
// Jump thunk to 0x0040f600 (CAcctLoginGump::Cancel), plain RET.
void __thiscall CAcctLoginGump::OnCancel()
{
    void *const self = this;
    PORT_THISCALL0(&CAcctLoginGump::Cancel, int, self)();
}
HOOK_VMETHOD(0x00421c40, CAcctLoginGump, OnCancel);

// 0x00422380  size 0xe2
// Tab among the three input controls at +0xb8 / +0xbc / +0xc0 (b8 -> bc -> c0 -> b8). Unlike the
// two-field version this sets the focus global directly. Plain RET.
void __thiscall CChgPWGump::OnTab()
{
    void *const self = this;
    void *prevFocus = g_focusedInput;
    U2_BringToFront(self, 0);
    U2_CommitFocus();
    if (prevFocus == RAWF(void *, self, 0xb8)) {
        g_focusedInput = RAWF(void *, self, 0xbc);
        RAWF(int32_t, RAWF(void *, self, 0xbc), 0xd8) = 1;
        RAWF(int32_t, RAWF(void *, self, 0xb8), 0xd8) = 0;
        U2_Redraw(RAWF(void *, self, 0xb8));
    }
    if (prevFocus == RAWF(void *, self, 0xbc)) {
        g_focusedInput = RAWF(void *, self, 0xc0);
        RAWF(int32_t, RAWF(void *, self, 0xc0), 0xd8) = 1;
        RAWF(int32_t, RAWF(void *, self, 0xbc), 0xd8) = 0;
        U2_Redraw(RAWF(void *, self, 0xbc));
    } else if (prevFocus == RAWF(void *, self, 0xc0)) {
        g_focusedInput = RAWF(void *, self, 0xb8);
        RAWF(int32_t, RAWF(void *, self, 0xb8), 0xd8) = 1;
        RAWF(int32_t, RAWF(void *, self, 0xc0), 0xd8) = 0;
        U2_Redraw(RAWF(void *, self, 0xc0));
    }
    void *focus = g_focusedInput;
    if (focus != nullptr)
        U2_Redraw(focus);
}
HOOK_VMETHOD(0x00422380, CChgPWGump, OnTab);

// 0x00422580  size 0x5
// Jump thunk to 0x0040f250 (CChgPWGump::Confirm), plain RET.
void __thiscall CChgPWGump::OnOk()
{
    void *const self = this;
    PORT_THISCALL0(&CChgPWGump::Confirm, int, self)();
}
HOOK_VMETHOD(0x00422580, CChgPWGump, OnOk);

// 0x00422a00  size 0x5
// Jump thunk to 0x0040f020 (CDeleteCharGump::DelChar), plain RET.
void __thiscall CDeleteCharGump::OnOk()
{
    void *const self = this;
    PORT_THISCALL0(&CDeleteCharGump::DelChar, int, self)();
}
HOOK_VMETHOD(0x00422a00, CDeleteCharGump, OnOk);

// 0x00422e40  size 0x5
// Jump thunk to 0x0040eab0 (CPlayCharGump::Ok), plain RET.
void __thiscall CPlayCharGump::OnOk()
{
    void *const self = this;
    PORT_THISCALL0(&CPlayCharGump::Ok, int, self)();
}
HOOK_VMETHOD(0x00422e40, CPlayCharGump, OnOk);

// 0x00423ef0  size 0x18
// Shows the gumps at +0x9c and +0xa0 (visible = 1); returns 1. Plain RET.
void __thiscall CCityChoiceGump::Slot60()
{
    void *const self = this;
    RAWF(int32_t, RAWF(void *, self, 0x9c), 0x68) = 1;
    RAWF(int32_t, RAWF(void *, self, 0xa0), 0x68) = 1;
}
HOOK_VMETHOD(0x00423ef0, CCityChoiceGump, Slot60);

// 0x004242c0  size 0x32
// Entry of the init table at 0x004a6000 (called by the CRT): preset the circle transparency radii,
// then CCircleTrans::Recompute (0x00424370) on the object at 0x004b71f8. No arguments.
void __cdecl CircleTrans_Init_004242c0()
{
    g_004b7208 = 0x40;
    g_004b7214 = 0x140;
    g_004b7218 = 0xeb;
    g_004b721c = 0;
    reinterpret_cast<CCircleTrans *>(GLOBAL_PTR(void, g_circleTrans_storage))->Recompute();
}
HOOK_FUNC(0x004242c0, CircleTrans_Init_004242c0);

// 0x0042c0c0  size 0x2b
// Returns 1 when +0x1c is 0, else 1 when the camera flag 0x0069e0c0 is set and the signed word at +0x24
// exceeds 0x0069e0c4.
int __thiscall CObjVt49f730::IsHidden()
{
    void *const self = this;
    if (RAWF(int32_t, self, 0x1c) == 0)
        return 1;
    if (g_camFlag != 0 && static_cast<int32_t>(RAWF(int16_t, self, 0x24)) > g_cutawayZ)
        return 1;
    return 0;
}
HOOK_VMETHOD(0x0042c0c0, CObjVt49f730, IsHidden);

// 0x0042c0f0  size 0x3c
// Returns 1 when +0x1c is 0, else 1 when the camera flag is set and the two signed bytes selected by the
// indexes at +0x64 and +0x68 (read at this + index + 0x6c) both exceed 0x0069e0c4.
int __thiscall CTerrainTileOps::IsHidden()
{
    void *const self = this;
    if (RAWF(int32_t, self, 0x1c) == 0)
        return 1;
    if (g_camFlag != 0) {
        int32_t i = RAWF(int32_t, self, 0x64);
        if (static_cast<int32_t>(RAWF(int8_t, self, static_cast<unsigned>(i) + 0x6c)) > g_cutawayZ) {
            int32_t j = RAWF(int32_t, self, 0x68);
            if (static_cast<int32_t>(RAWF(int8_t, self, static_cast<unsigned>(j) + 0x6c)) > g_cutawayZ)
                return 1;
        }
    }
    return 0;
}
HOOK_VMETHOD(0x0042c0f0, CTerrainTileOps, IsHidden);

// 0x0042eee0  size 0x34
// Screen rect from the cell position: left(+8) = x * 22 - (int16)(+0x24) * 4, right(+0x10) = left + 0x2c,
// top(+4) = y * 22 - 22, bottom(+0xc) = top + 0x2c, with x = +0x2c and y = +0x28. Returns bottom.
void __thiscall CObjVt49f730::ComputeScreenRect()
{
    uint32_t x = static_cast<uint32_t>(RAWF(int32_t, this, 0x2c)) * 11u;
    uint32_t left = x * 2u - (static_cast<uint32_t>(static_cast<int32_t>(RAWF(int16_t, this, 0x24))) << 2);
    RAWF(uint32_t, this, 8) = left;
    RAWF(uint32_t, this, 0x10) = left + 0x2c;
    uint32_t y = static_cast<uint32_t>(RAWF(int32_t, this, 0x28));
    uint32_t top = y * 11u * 2u - 0x16u;
    RAWF(uint32_t, this, 4) = top;
    RAWF(uint32_t, this, 0xc) = top + 0x2c;
}
HOOK_VMETHOD(0x0042eee0, CObjVt49f730, ComputeScreenRect);

// 0x00438140  size 0x6
// Returns 4.
uint32_t __thiscall CMissileHelper::TileFlags()
{
    void *const self = this;
    return 4;
}
HOOK_VMETHOD(0x00438140, CMissileHelper, TileFlags);

// 0x0043fa40  size 0xaa
// CFileManager destructor body (the object at 0x0068d980; reached through the thunk 0x0044a910): free the
// hash node pool, close the cache stream (0x0047eaa0) and the 17 file streams (0x0047e850), free the verdata
// patch node lists and close the verdata stream (0x004d334c).
void __fastcall FileManager_Dtor_0043fa40(CFileManager *self, void *)
{
    void *pool = GLOBAL_AS(void *, g_nodePool);
    if (pool != nullptr)
        operator delete(pool);
    delete self->cacheStream;
    MappedFile **stream = self->streams;
    for (int n = 0x11; n != 0; n--) {
        delete *stream;
        stream++;
    }
    if (g_verdataEnabled != 0) {
        void **slot = GLOBAL_PTR(void *, g_verdataTable);
        do {
            int n = 0x400;
            do {
                void *node = *slot;
                if (node != nullptr) {
                    do {
                        void *next = RAWF(void *, node, 0x10);
                        *slot = next;
                        operator delete(node);
                        node = *slot;
                    } while (node != nullptr);
                }
                slot++;
                n--;
            } while (n != 0);
        } while (reinterpret_cast<intptr_t>(slot) < static_cast<intptr_t>(GEND(g_verdataTable)));
        delete reinterpret_cast<MappedFile *>(g_verdataUnk);
    }
}
HOOK_FUNC(0x0043fa40, FileManager_Dtor_0043fa40);

// 0x004488a0  size 0x2a
// Minimum of the four signed bytes at +0x6c .. +0x6f (sign-extended 16-bit compares, result in AX).
uint32_t __thiscall CTerrainTile::GetBaseZ()
{
    void *const self = this;
    int16_t best = RAWF(int8_t, self, 0x6c);
    int16_t v = RAWF(int8_t, self, 0x6d);
    if (v < best)
        best = v;
    v = RAWF(int8_t, self, 0x6e);
    if (v < best)
        best = v;
    v = RAWF(int8_t, self, 0x6f);
    if (v < best)
        best = v;
    return best;
}
HOOK_VMETHOD(0x004488a0, CTerrainTileOps, GetBaseZ);
