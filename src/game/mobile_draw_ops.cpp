// Mobile (CMobile / player) screen rectangle and sprite drawing, 0x0042caa0 .. 0x0042ee90.
//
// Everything here is a free function with the original register convention:
// a __thiscall method with its arguments on the stack is written as
// __fastcall(this, <unused EDX>, args...), which is the same ECX + callee-cleaned
// stack layout. Offsets of CMobile that obj.h does not name are reached with rawf.
//
// ? obj.h names +0xbc / +0xc4 as listNext / equip[]. This code reads +0xc4 + layer * 4 as an item
//   pointer (layers 1..0x19) and, for mobiles whose serial has bit 31 set, +0xbc + layer * 4 of a
//   second object as an item serial. One of the two descriptions of the array is wrong; the code
//   here follows the assembly.
#include <cstdint>

#include "../runtime/ported.h"
#include "globals.h"
#include "obj.h"
#include "raw_field.h"
#include "mobile_cached_draw_ports.h"
#include "client_misc_ops_ports.h"
#include "paperdoll_gump_ops_ports.h"
#include "world_obj_ops.h"
#include "world_renderer.h"
#include "missile_draw_ports.h"
#include "mobile_draw_ops_ports.h"
#include "file_manager.h"
#include "art_record_ops_ports.h"
#include "object_manager.h"

#define g_targetView GLOBAL_AS(uint8_t *, g_screenGump)       // object whose +0xd4 is the targeted mobile

// Original procedures called from here.
#define Mob_Draw(a1, a2, a3) (Mobile_DrawCached(a1, a2, a3))             // 0x00471570 CMobile::Draw(clip)
#define Mob_DrawPreview(a1, a2, a3) (Mobile_DrawPreviewCached(a1, a2, a3))      // 0x004721f0
#define Mob_IsPreview(a1) (Mobile_FindMountEntry(a1))                      // 0x00471040
#define Mob_ComputeBounds(a1) (Mobile_ComputeBoundsCached(a1))                 // 0x00471190
#define Mob_ComputeBoundsPreview(a1) (Mobile_ComputeBoundsPreview(a1))          // 0x00471e20
#define Mob_IsHidden PORT_FUNC0(&Misc_Return0, int, __cdecl)                               // 0x00478cd0 (returns 0)
#define Mob_PendingUpdate(a1, a2) (PendingUpdate_Has((uint32_t)(a1), (uint32_t)(a2)))                   // 0x004522f0 (serial, 0)
#define Mob_FindBySerialIdx(a1) ((CMobile *)PendingUpdate_FindObj((uint32_t)(a1)))                // 0x00452290
#define Mob_Stat PORT_FUNC1(&CMobile::Div24, int, __fastcall, void *)                              // 0x004544c0
#define Mob_LayerIsCovered(a1, a2) (Paperdoll_LayerIsCovered(a1, (uint8_t *)(a2)))               // 0x00466b60
#define Mob_Item_Draw(self, a1) (reinterpret_cast<CObj *>(self)->CObj::Draw(a1))                                 // 0x0042c730 CItem::Draw
#define Mob_DrawShadow(a1, a2, a3, a4, a5) (WorldDrawShadowRLE(a1, a2, a3, (const int *)(a4), (const uint8_t *)(a5)))   // 0x0042a670
#define Mob_Blit(a1, a2, a3, a4, a5, a6, a7) (WorldBlitRLESprite(a1, (unsigned)(a2), a3, a4, (const int *)(a5), (const uint16_t *)(a6), (const uint8_t *)(a7)))   // 0x0042a7f0
#define Mob_DrawArt(a1, a2, a3, a4) (MissileDrawTileArt((uint16_t)(a1), (uint8_t)(a2), a3, a4))          // 0x00437e60
#define Mob_StandIndex(a1, a2, a3, a4) (Mobile_GetStandAnimIndex(a1, a2, a3, a4))  // 0x0042cff0
#define Mob_AnimFrame(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) (Mobile_GetAnimFrame(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10))   // 0x0042caf0
#define Mob_LoadAnim(idx, pa, pb) PORT_THISCALL5(&CFileManager::GetRecord, uint8_t *, GLOBAL_PTR(void, g_fileManager_storage), int, int, int, int *, int *)(5, 6, (idx), (pa), (pb))   // 0x004405d0
#define Mob_GumpArt(id) PORT_THISCALL1(&ArtRecord_Get, uint8_t *, GLOBAL_PTR(void, g_artCache_storage), uint32_t)((id))   // 0x00476130

// vtable +0x8c: read as a boolean; vtable +0x84: forced action, -1 for none.
static inline int V_HIDDEN(void *o)
{
    return PORT_VSLOT0(o, 0x8c, int)();
}
static inline int V_ACTION(void *o)
{
    return PORT_VSLOT0(o, 0x84, int)();
}

static inline uint16_t tile16(const void *obj)
{
    // tiledata[graphic].+0x0c (the body / animation id of an equipment tile)
    return RAWF(uint16_t, g_tileData + RAWF(uint16_t, obj, 0x38) * 0x28, 0x0c);
}
static inline uint32_t tileFlags(const void *obj)
{
    return RAWF(uint32_t, g_tileData + RAWF(uint16_t, obj, 0x38) * 0x28, 0x00);
}
static inline uint8_t tileType(const void *obj)
{
    return RAWF(uint8_t, g_tileData + RAWF(uint16_t, obj, 0x38) * 0x28, 0x05);
}

// Scan the zero-terminated table the way the original does and return the entry it stops on
// (non-zero when `v` is in the list).
static int tableStop(const int *tab, int v)
{
    int i = 0;
    if (tab[0] != 0) {
        int cur = tab[0];
        while (v != cur) {
            cur = tab[i + 1];
            i++;
            if (cur == 0)
                break;
        }
    }
    return tab[i];
}

// 0x0042caa0  size 0x50
// Draw an item with tiledata flag 0x8 forced on for the call, then restore the original flag dword.
// ECX = item, stack: clip, RET 4.
void __thiscall CObjVt49fb90::Draw(int *clip)
{
    CObj *const self = this;
    uint32_t *e = reinterpret_cast<uint32_t *>(g_tileData + (self->graphic + static_cast<int8_t>(self->unk_3a)) * 0x28);
    uint32_t saved = *e;
    *e = saved | 8;
    Mob_Item_Draw(self, clip);                       // 0x0042c730
    e = reinterpret_cast<uint32_t *>(g_tileData + (self->graphic + static_cast<int8_t>(self->unk_3a)) * 0x28);
    *e = saved;
}
HOOK_VMETHOD(0x0042caa0, CObjVt49fb90, Draw);

// 0x0042caf0  size 0x4f1
// Animation table index and frame for a mobile: (body, facing, tick, specific frame, unused, action
// override, moving flag) -> *outIdx (index into the animation file pair 5/6), *outFrame.
// ECX = body (low 16 bits), EDX = facing (bit 7 as in the caller), stack (RET 0x20): tick, specific,
// unused, act, run, outIdx, outFrame, mobile.
// ? the third stack argument is never read.
void __fastcall Mobile_GetAnimFrame(uint32_t bodyReg, uint32_t facingReg, int tick, int specific, int unusedArg, int act,
                                    int run, int *outIdx, int *outFrame, CMobile *mob)
{
    (void)unusedArg;
    uint16_t body = static_cast<uint16_t>(bodyReg);
    int sel;      // esi: action selector
    int cnt;      // eax: frame count
    int idx;      // edi: body index
    int base;
    int mult;
    int mounted;
    int weaponAnim;

    if (body < 0x96) {
        if (RAWF(int, mob, 0x148) != 0) {
            if (body == 0xd || body == 0xf || body == 0x10) {
                sel = 8;
                cnt = 6;
            } else {
                sel = 1;
                cnt = 1;
            }
        } else {
            sel = 0;
            cnt = 0xa;
            if (run != 0) {
                if (body == 4 || body == 5 || body == 6 || body == 9 || body == 0xa || body == 0xc || body == 0x1e ||
                    body == 0x27 || body == 0x3b || body == 0x3c || body == 0x3d)
                    sel = 0x13;
            }
        }
        idx = body;
        base = 0;
        mult = 0x6e;
    } else if (body < 0xc8) {
        if (RAWF(int, mob, 0x148) != 0) {
            sel = 2;
            cnt = 1;
        } else {
            sel = (run != 0) ? 1 : 0;
            cnt = 0xa;
        }
        idx = body;
        base = 0;
        mult = 0x6e;
    } else if (body < 0x190) {
        if (RAWF(int, mob, 0x148) != 0) {
            sel = 2;
            cnt = 1;
        } else {
            sel = (run != 0) ? 1 : 0;
            cnt = 5;
        }
        idx = body - 0xc8;
        base = 0x55f0;
        mult = 0x41;
    } else {
        int idxSaved = body - 0x190;
        idx = idxSaved;
        base = 0x88b8;
        mult = 0xaf;
        mounted = 0;
        if (RAWF(uint16_t, mob, 0x38) >= 0x190 && RAWF(int, mob, 0x128) != 0)
            mounted = 1;
        weaponAnim = 0;
        {
            void *w = RAWF(void *, mob, 0xcc);
            if (w != nullptr && tile16(w) == body) {
                if ((PORT_VSLOT0(w, 0x40, int)() & 0x800000) != 0)
                    weaponAnim = 1;
            }
        }
        if (RAWF(int, mob, 0x148) != 0) {
            if (body == 0x23e) {
                sel = 0;
                cnt = 0xa;
            } else if (mounted != 0) {
                if (weaponAnim != 0) {
                    sel = 0x1c;
                    cnt = 7;
                } else {
                    sel = 0x19;
                    cnt = 1;
                }
            } else if (RAWF(int, mob, 0x150) != 0 && V_HIDDEN(mob) == 0) {
                if (weaponAnim != 0) {
                    sel = 2;
                    cnt = 0xa;
                } else {
                    int a18 = 7;
                    void *w1 = RAWF(void *, mob, 0xc8);
                    if (w1 != nullptr) {
                        int v = tile16(w1);
                        if (v >= g_tableMountLo && v <= g_tableMountHi) {
                            if (tableStop(g_tableMountList, v) != 0)
                                a18 = 8;
                        }
                    }
                    void *w2 = RAWF(void *, mob, 0xcc);
                    if (w2 != nullptr) {
                        int v = tile16(w2);
                        if (v >= g_tableMountLo && v <= g_tableMountHi) {
                            if (tableStop(g_tableMountList, v) != 0)
                                a18 = 8;
                        }
                    }
                    sel = a18;
                    idx = idxSaved;
                    cnt = 1;
                }
            } else {
                if (weaponAnim != 0) {
                    sel = 0;
                    cnt = 0xa;
                } else {
                    sel = 4;
                    cnt = 1;
                }
            }
        } else {
            if (mounted != 0) {
                sel = (run != 0) ? 0x18 : 0x17;
                cnt = 5;
            } else if (run != 0) {
                void *w2 = RAWF(void *, mob, 0xcc);
                sel = 2;
                if (w2 != nullptr) {
                    int v = tile16(w2);
                    if (v < g_tableWalkLo || v > g_tableWalkHi || tableStop(g_tableWalkList, v) == 0)
                        sel = 3;
                }
                cnt = 0xa;
            } else if (RAWF(int, mob, 0x150) != 0 && V_HIDDEN(mob) == 0) {
                sel = 0xf;
                cnt = 0xa;
            } else {
                void *w2 = RAWF(void *, mob, 0xcc);
                sel = 0;
                if (w2 != nullptr) {
                    int v = tile16(w2);
                    if (v < g_tableWalkLo || v > g_tableWalkHi || tableStop(g_tableWalkList, v) == 0)
                        sel = 1;
                }
                cnt = 0xa;
            }
        }
    }

    if (specific == -1) {
        idx *= mult;
        *outIdx = sel * 5 + (base + (g_dirAnimBase[facingReg & 7] + idx));
        if (cnt == 1)
            *outFrame = 0;
        else
            *outFrame = tick % (cnt - 1);
    } else {
        int a = act;
        int fr = specific;
        if (a > 0x22) {
            a = 0;
            fr = 0;
        }
        idx *= mult;
        *outIdx = a * 5 + (base + (g_dirAnimBase[facingReg & 7] + idx));
        *outFrame = fr;
    }
}
HOOK_FUNC(0x0042caf0, Mobile_GetAnimFrame);

// 0x0042cff0  size 0xe2
// Base animation index of the standing pose for a body. ECX = body (signed compares), EDX = facing
// (bit 7 = second pose), stack (RET 8): outIdx, outCount (3 or 5, the frame the sitting code uses).
void __fastcall Mobile_GetStandAnimIndex(int body, uint32_t dir, int *outIdx, int *outCount)
{
    *outCount = 5;
    if (body < 0x96) {
        *outCount = 3;
        int t = ((dir & 0x80) | 0x100) >> 7;
        *outIdx = (t + body * 11 * 2) * 5 + g_dirAnimBase[dir & 7];
        return;
    }
    if (body < 0xc8) {
        *outIdx = g_dirAnimBase[dir & 7] + body * 11 * 5 * 2 + 0x28;
        return;
    }
    if (body < 0x190) {
        int t = ((dir & 0x80) | 0x100) >> 5;
        *outIdx = (body * 13 + t + 0x708) * 5 + g_dirAnimBase[dir & 7];
        return;
    }
    {
        int b = body - 200;
        int bit = ((dir & 0x80) != 0) ? 1 : 0;
        *outIdx = (b * 7 * 5 + (bit + 0x15)) * 5 + g_dirAnimBase[dir & 7];
    }
}
HOOK_FUNC(0x0042cff0, Mobile_GetStandAnimIndex);

// 0x0042d0e0  size 0x30b
// Screen rectangle of a mobile in its standing pose (rect at +4..+0x10, ybase at +0xb4, width in
// tiles at +0xb8). Plain ECX = mobile, no stack arguments.
void __thiscall CObjVt49f9e0::ComputeScreenRect()
{
    CMobile *const self = reinterpret_cast<CMobile *>(this);
    int px = self->x;
    int py = self->y;
    int e = g_scrollBase + px - py;
    int X0 = e * 11 * 2 + 1;
    int Y0 = (px + py) * 22 + (5 - self->z) * 4;
    int idx;
    int frameIdx;
    int mirror;
    uint8_t *rec;
    int ua, ub;
    int count;
    int off;
    uint8_t *fr;
    int w0, w2, w4, w6;
    int ecx;
    int t;

    RAWF(int, self, 0xb4) = Y0;
    if (self->unk_3c >= 0x3e8) {
        self->rectY0 = Y0;
        self->rectY1 = Y0;
        self->rectX1 = X0;
        self->rectX0 = X0;
        return;
    }
    if ((self->flags98 & 0x80) != 0) {
        CMobile *player = reinterpret_cast<CMobile *>(g_player);
        if (Mob_IsHidden() == 0 && player != nullptr && player->graphic != 0x3db) {   // 0x00478cd0
            self->rectY0 = Y0;
            self->rectY1 = Y0;
            self->rectX1 = X0;
            self->rectX0 = X0;
            return;
        }
    }
    Mob_StandIndex(self->unk_3c, self->facing, &idx, &frameIdx);   // 0x0042cff0
    mirror = g_dirMirror[self->facing & 7];
    rec = Mob_LoadAnim(idx, &ua, &ub);                              // 0x004405d0
    if (rec == nullptr) {
        self->rectY0 = Y0;
        self->rectY1 = Y0;
        self->rectX1 = X0;
        self->rectX0 = X0;
        return;
    }
    count = RAWF(int, rec, 0x200);
    if (frameIdx >= count) {
        self->rectY0 = Y0;
        self->rectY1 = Y0;
        self->rectX1 = X0;
        self->rectX0 = X0;
        return;
    }
    off = RAWF(int, rec, 0x200 + frameIdx * 4 + 4);
    if (off == 0) {
        self->rectY0 = Y0;
        self->rectY1 = Y0;
        self->rectX1 = X0;
        self->rectX0 = X0;
        return;
    }
    fr = rec + 0x200 + off;
    w0 = RAWF(int16_t, fr, 0);
    w2 = RAWF(int16_t, fr, 2);
    w4 = RAWF(int16_t, fr, 4);
    w6 = RAWF(int16_t, fr, 6);
    ecx = (mirror != 0) ? static_cast<int16_t>(w4 - w0) : w0;
    self->rectY1 = Y0 - w2;
    self->rectX0 = X0 - ecx;
    self->rectY0 = self->rectY1 - w6;
    self->rectX1 = self->rectX0 + w4;
    if (w2 >= 0)
        self->rectY1 = Y0;
    t = self->rectX1 - X0 + 0x16;
    if (t <= 0x2c)
        RAWF(int, self, 0xb8) = 0;
    else
        RAWF(int, self, 0xb8) = t / 0x2c;
    if (self->unk_3c >= 0x190) {
        for (int i = 0; i < 0x19; i++) {
            int serial;
            CObj *child;
            if (i == 0x15)
                continue;
            serial = RAWF(int, self, 0xbc + i * 4);
            if (serial == 0)
                continue;
            child = RAWF(CObj *, self, 0xa8);
            if (child == nullptr)
                continue;
            while (RAWF(int, child, 0x78) != serial) {
                child = child->link40;
                if (child == nullptr)
                    break;
            }
            if (child == nullptr)
                continue;
            int body = tile16(child);
            if (body < 0x190 || body >= 0x3e8)
                continue;
            Mob_StandIndex(body, self->facing, &idx, &frameIdx);   // 0x0042cff0
            rec = Mob_LoadAnim(idx, &ua, &ub);                      // 0x004405d0
            if (rec == nullptr)
                continue;
            off = RAWF(int, rec, 0x200 + frameIdx * 4 + 4);
            if (off == 0)
                continue;
            fr = rec + 0x200 + off;
            w0 = RAWF(int16_t, fr, 0);
            w2 = RAWF(int16_t, fr, 2);
            w4 = RAWF(int16_t, fr, 4);
            w6 = RAWF(int16_t, fr, 6);
            if (mirror != 0)
                w0 = static_cast<int16_t>(w4 - w0);
            int bottom = Y0 - w2;
            int left = X0 - w0;
            int top = bottom - w6;
            int right = left + w4;
            if (right > self->rectX1)
                self->rectX1 = right;
            if (left < self->rectX0)
                self->rectX0 = left;
            if (bottom > self->rectY1)
                self->rectY1 = bottom;
            if (top < self->rectY0)
                self->rectY0 = top;
        }
    }
}
HOOK_VMETHOD(0x0042d0e0, CObjVt49f9e0, ComputeScreenRect);

// 0x0042d3f0  size 0x2d9
// Draw a mobile in its standing pose, clipped to `clip`. ECX = mobile, stack: clip, RET 4.
void __thiscall CObjVt49f9e0::Draw(int *clip)
{
    CMobile *const self = reinterpret_cast<CMobile *>(this);
    int hue = 0;
    uint8_t ghost = 0;
    int rect[4];
    int idx;
    int frameIdx;
    int ua, ub;
    int mirror;
    int ybase;
    int x;
    uint8_t *rec;
    int off;
    uint8_t *fd;

    if (self->unk_3c >= 0x3e8)
        return;
    if (Mob_PendingUpdate(RAWF(int, self, 0x78), 0) != 0)   // 0x004522f0
        return;
    if ((self->flags98 & 0x80) != 0) {
        if (Mob_IsHidden() == 0) {                           // 0x00478cd0
            CMobile *player = reinterpret_cast<CMobile *>(g_player);
            if (player == nullptr)
                return;
            if (player->graphic != 0x3db)
                return;
        }
        hue = 0x3b2;
        ghost = 1;
    }
    ybase = RAWF(int, self, 0xb4);
    rect[0] = clip[0];
    rect[1] = clip[1];
    rect[2] = clip[2];
    rect[3] = clip[3];
    x = (self->x - self->y + g_scrollBase) * 11 * 2 + 1;
    if (g_clipBusyBB == 0) {
        if (rect[3] > ybase - g_scrollY)
            rect[3] = ybase - g_scrollY;
    }
    Mob_StandIndex(self->unk_3c, self->facing, &idx, &frameIdx);   // 0x0042cff0
    mirror = g_dirMirror[self->facing & 7];
    rec = Mob_LoadAnim(idx, &ua, &ub);                              // 0x004405d0
    if (rec == nullptr)
        return;
    if (frameIdx >= RAWF(int, rec, 0x200))
        return;
    off = RAWF(int, rec, 0x200 + frameIdx * 4 + 4);
    if (off == 0) {
        self->rectY0 = ybase;
        self->rectY1 = ybase;
        self->rectX1 = x;
        self->rectX0 = x;
        return;
    }
    fd = rec + 0x200 + off + 8;
    {
        CMobile *player = reinterpret_cast<CMobile *>(g_player);
        if (V_HIDDEN(player) != 0)
            hue = 0x3b2;
        else if (ghost == 0)
            hue = self->hue;
    }
    Mob_Blit(mirror, hue, x, ybase, rect, rec, fd);   // 0x0042a7f0
    if (self->unk_3c < 0x190)
        return;
    for (int i = 0; i < 0x19; i++) {
        int layer = Mc_layerOrder[(self->facing & 7) * 25 + i];
        int serial;
        CObj *child;
        uint8_t *rec2;
        uint8_t *fd2;
        int hue2;
        int body;
        if (layer == 0x15 || layer == 0x19)
            continue;
        serial = RAWF(int, self, 0xbc + layer * 4);
        if (serial == 0)
            continue;
        child = RAWF(CObj *, self, 0xa8);
        if (child == nullptr)
            continue;
        while (RAWF(int, child, 0x78) != serial) {
            child = child->link40;
            if (child == nullptr)
                break;
        }
        if (child == nullptr)
            continue;
        body = tile16(child);
        if (body < 0x190 || body >= 0x3e8)
            continue;
        Mob_StandIndex(body, self->facing, &idx, &frameIdx);   // 0x0042cff0
        rec2 = Mob_LoadAnim(idx, &ua, &ub);                     // 0x004405d0
        if (rec2 == nullptr)
            continue;
        off = RAWF(int, rec2, 0x200 + frameIdx * 4 + 4);
        if (off == 0)
            continue;
        fd2 = rec2 + 0x200 + off + 8;
        {
            CMobile *player = reinterpret_cast<CMobile *>(g_player);
            if (V_HIDDEN(player) != 0) {
                hue2 = 0x3b2;
            } else {
                hue2 = child->hue;
                if ((tileFlags(child) & 0x40000) != 0)
                    hue2 |= 0x8000;
            }
        }
        Mob_Blit(mirror, hue2, x, ybase, rect, rec2, fd2);   // 0x0042a7f0
    }
}
HOOK_VMETHOD(0x0042d3f0, CObjVt49f9e0, Draw);

// Union of the extent of the attached gumps into the rect (shared tail of the CalcRect variants).
// 0x0042d6d0 / 0x0042e430 end with identical loops over the gump list at +0x1c0 (next at +0x100).
//
// 0x0042d6d0  size 0x4bf
// Screen rectangle of an animated mobile, including mount and equipment layers.
// ECX = mobile, plain ret.
void __thiscall CMobile::ComputeScreenRect()
{
    CMobile *const self = this;
    int px;
    int py;
    int X;
    int Y;
    int Y0;
    int body;
    int a2;
    int idx;
    int frame;
    int ua, ub;
    int mirror;
    uint8_t *rec;
    uint8_t *fr;
    int off;
    int w0, w2, w4, w6;
    int h16;
    int ecx;
    int t;
    int count;

    if (Mob_IsPreview(self) != 0) {   // 0x00471040
        Mob_ComputeBounds(self);      // 0x00471190
        return;
    }
    py = self->y;
    px = self->x;
    X = (g_scrollBase - py + px) * 22 - RAWF(int, self, 0x188) + 1;
    Y0 = (px + py) * 22 + (5 - RAWF(int, self, 0x190)) * 4;
    RAWF(int, self, 0xb4) = Y0;
    Y = Y0 - RAWF(int, self, 0x18c);
    if (self->graphic >= 0x3e8) {
        self->rectY0 = Y;
        self->rectY1 = Y;
        self->rectX1 = X;
        self->rectX0 = X;
        return;
    }
    if ((self->flags98 & 0x80) != 0) {
        CMobile *player = reinterpret_cast<CMobile *>(g_player);
        if (Mob_IsHidden() == 0 && player != nullptr && player->graphic != 0x3db) {   // 0x00478cd0
            self->rectY0 = Y;
            self->rectY1 = Y;
            self->rectX1 = X;
            self->rectX0 = X;
            return;
        }
    }
    if (V_HIDDEN(self) != 0)
        body = self->graphic - 2;
    else
        body = self->graphic;
    a2 = V_ACTION(self);
    Mob_AnimFrame(body, self->facing, RAWF(int, self, 0x140), a2, RAWF(uint16_t, self, 0x1aa),
                  RAWF(uint16_t, self, 0x1b8) + RAWF(uint16_t, self, 0x1ac), RAWF(int, self, 0x144), &idx, &frame, self);   // 0x0042caf0
    mirror = g_dirMirror[self->facing & 7];
    rec = Mob_LoadAnim(idx, &ua, &ub);   // 0x004405d0
    if (rec == nullptr) {
        self->rectX1 = X;
        self->rectY0 = Y;
        self->rectY1 = Y;
        self->rectX0 = X;
        return;
    }
    count = RAWF(uint16_t, rec, 0x200);
    RAWF(uint16_t, self, 0x1aa) = static_cast<uint16_t>(count);
    if (frame >= count)
        return;
    off = RAWF(int, rec, 0x200 + frame * 4 + 4);
    if (off == 0) {
        self->rectX1 = X;
        self->rectY0 = Y;
        self->rectY1 = Y;
        self->rectX0 = X;
        return;
    }
    fr = rec + 0x200 + off;
    w0 = RAWF(int16_t, fr, 0);
    w2 = RAWF(int16_t, fr, 2);
    w4 = RAWF(int16_t, fr, 4);
    w6 = RAWF(int16_t, fr, 6);
    h16 = w6;
    ecx = (mirror != 0) ? static_cast<int16_t>(w4 - w0) : w0;
    self->rectY1 = Y - w2;
    self->rectX0 = X - ecx;
    self->rectY0 = self->rectY1 - w6;
    self->rectX1 = self->rectX0 + w4;
    if (w2 >= 0) {
        self->rectY1 = Y;
        h16 = static_cast<int16_t>(h16 + w2);
    }
    t = self->rectX1 - X + 0x16;
    if (t <= 0x2c)
        RAWF(int, self, 0xb8) = 0;
    else
        RAWF(int, self, 0xb8) = t / 0x2c;
    {
        int g = self->graphic;
        if (g != 0xdd && g != 0x97 && g != 0x96 && g != 0x34 && g != 0x15)
            self->rectX1 = self->rectX1 + (h16 >> 1);
    }
    if (self->graphic >= 0x190) {
        for (int i = 1; i < 0x1a; i++) {
            CObj *obj;
            if (i == 0x15)
                continue;
            obj = RAWF(CObj *, self, 0xc4 + i * 4);
            if (obj == nullptr) {
                if ((RAWF(uint32_t, self, 0x78) & 0x80000000u) != 0) {
                    CMobile *other = Mob_FindBySerialIdx(RAWF(int, self, 0x78));   // 0x00452290
                    if (other != nullptr) {
                        if (PORT_VSLOT0(other, 0x54, int)() != 0) {
                            CObj *c = RAWF(CObj *, other, 0xa8);
                            if (c != nullptr) {
                                int serial = RAWF(int, other, 0xbc + i * 4);
                                while (RAWF(int, c, 0x78) != serial) {
                                    c = c->link40;
                                    if (c == nullptr)
                                        break;
                                }
                                if (c != nullptr)
                                    obj = c;
                            }
                        }
                    }
                }
                if (obj == nullptr)
                    continue;
            }
            int ibody = tile16(obj);
            if (ibody < 0x190 || ibody >= 0x3e8)
                continue;
            Mob_AnimFrame(ibody, self->facing, RAWF(int, self, 0x140), a2, RAWF(uint16_t, self, 0x1aa),
                          RAWF(uint16_t, self, 0x1b8) + RAWF(uint16_t, self, 0x1ac), RAWF(int, self, 0x144), &idx, &frame, self);   // 0x0042caf0
            rec = Mob_LoadAnim(idx, &ua, &ub);   // 0x004405d0
            if (rec == nullptr)
                continue;
            off = RAWF(int, rec, 0x200 + frame * 4 + 4);
            if (off == 0)
                continue;
            fr = rec + 0x200 + off;
            w0 = RAWF(int16_t, fr, 0);
            w2 = RAWF(int16_t, fr, 2);
            w4 = RAWF(int16_t, fr, 4);
            w6 = RAWF(int16_t, fr, 6);
            if (mirror != 0)
                w0 = static_cast<int16_t>(w4 - w0);
            int bottom = Y - w2;
            int left = X - w0;
            int top = bottom - w6;
            int right = left + w4;
            if (i == 0x19)
                right += w4 >> 1;
            if (right > self->rectX1)
                self->rectX1 = right;
            if (left < self->rectX0)
                self->rectX0 = left;
            if (bottom > self->rectY1)
                self->rectY1 = bottom;
            if (top < self->rectY0)
                self->rectY0 = top;
        }
    }
    {
        uint8_t *g = RAWF(uint8_t *, self, 0x1c0);
        RAWF(int, self, 0x1cc) = 0;
        RAWF(int, self, 0x1c8) = 0;
        while (g != nullptr) {
            uint8_t *art = Mob_GumpArt(RAWF(uint16_t, g, 0xda) + 0x4000);   // 0x00476130
            int h = RAWF(uint16_t, art, 6);
            int d = RAWF(int16_t, art, 4) - self->rectX1 + self->rectX0;
            if (d > 0) {
                d >>= 1;
                RAWF(int, self, 0x1c8) += d;
                self->rectX0 -= d;
                self->rectX1 += d;
            }
            d = static_cast<int16_t>(h) - self->rectY1 + self->rectY0;
            if (d > 0) {
                d >>= 1;
                RAWF(int, self, 0x1cc) += d;
                self->rectY0 -= d;
                self->rectY1 += d;
            }
            g = RAWF(uint8_t *, g, 0x100);
        }
    }
}
HOOK_VMETHOD(0x0042d6d0, CMobile, ComputeScreenRect);

// 0x0042db90  size 0x28
// 1 when the object exists and its tiledata type byte (+5) is 0x0b or 0x10. ECX = object.
int __fastcall Obj_IsTileType0b10(CObj *obj)
{
    if (obj != nullptr) {
        uint8_t t = tileType(obj);
        if (t == 0xb || t == 0x10)
            return 1;
    }
    return 0;
}
HOOK_FUNC(0x0042db90, Obj_IsTileType0b10);

// 0x0042dbc0  size 0x66
// Hue (0x59 default, 0x22, 0x3b2) of a mobile relative to the player. ECX = mobile.
// Hue choice, also inlined twice into 0x0042dc30.
static int mobile_name_hue(CMobile *self, CMobile *player)
{
    if (self != nullptr && player != nullptr) {
        int a = Mob_Stat(self);   // 0x004544c0
        if (a >= 0) {
            if (self->graphic >= 0x190 || a > 0)
                return 0x59;
            return 0x3b2;
        } else {
            int b = Mob_Stat(player);   // 0x004544c0
            if (b > 0)
                b = 0;
            b -= a;
            if (b >= 2)
                return 0x22;
            if (b <= 0)
                return 0x59;
            return 0x3b2;
        }
    }
    return 0x59;
}

int __fastcall Mobile_NameHue(CMobile *self)
{
    return mobile_name_hue(self, reinterpret_cast<CMobile *>(g_player));
}
HOOK_FUNC(0x0042dbc0, Mobile_NameHue);

// 0x0042dc30  size 0x7fa
// Draw an animated mobile with mount and equipment layers clipped to `clip`. ECX = mobile, stack: clip,
// RET 4. The name-hue block at the top is the inlined 0x0042dbc0 (twice: 0x004544c0 appears four times).
void __thiscall CMobile::Draw(int *clip)
{
    CMobile *const self = this;
    int hue = 0;
    uint8_t ghost = 0;
    int rect[4];
    int x;
    int Y;
    int ybase0;
    int body;
    int a2;
    int idx;
    int frame;
    int ua, ub;
    int mirror;
    uint8_t *rec;
    int off;
    uint8_t *fd;
    CMobile *player;
    int deathRobe;

    if (self->graphic >= 0x3e8)
        return;
    if (Mob_IsPreview(self) != 0) {   // 0x00471040
        Mob_Draw(self, 0, clip);      // 0x00471570
        return;
    }
    player = reinterpret_cast<CMobile *>(g_player);
    if (((g_targetCallbackActive != 0 || RAWF(int, player, 0x150) != 0) && self == RAWF(CMobile *, g_targetView, 0xd4)) ||
        g_attackSerial == RAWF(int, self, 0x78)) {
        hue = mobile_name_hue(self, player);   // inlined 0x0042dbc0: 0x004544c0, 0x004544c0, 0x004544c0, 0x004544c0
        ghost = 1;
    }
    if ((self->flags98 & 0x80) != 0) {
        if (Mob_IsHidden() == 0) {   // 0x00478cd0
            CMobile *p = reinterpret_cast<CMobile *>(g_player);
            if (p == nullptr)
                return;
            if (p->graphic != 0x3db)
                return;
        }
        hue = 0x3b2;
        ghost = 1;
    }
    rect[0] = clip[0];
    rect[1] = clip[1];
    rect[2] = clip[2];
    rect[3] = clip[3];
    x = (self->x - self->y + g_scrollBase) * 22 - RAWF(int, self, 0x188) + 1;
    ybase0 = RAWF(int, self, 0xb4);
    Y = ybase0 - RAWF(int, self, 0x18c);
    if (g_clipBusyBB == 0 && self->graphic < 0x190) {
        if (rect[3] > ybase0 - g_scrollY)
            rect[3] = ybase0 - g_scrollY;
    }
    if (V_HIDDEN(self) != 0)
        body = self->graphic - 2;
    else
        body = self->graphic;
    a2 = V_ACTION(self);
    Mob_AnimFrame(body, self->facing, RAWF(int, self, 0x140), a2, RAWF(uint16_t, self, 0x1aa),
                  RAWF(uint16_t, self, 0x1b8) + RAWF(uint16_t, self, 0x1ac), RAWF(int, self, 0x144), &idx, &frame, self);   // 0x0042caf0
    mirror = g_dirMirror[self->facing & 0x7f];
    rec = Mob_LoadAnim(idx, &ua, &ub);   // 0x004405d0
    if (rec == nullptr)
        return;
    RAWF(uint16_t, self, 0x1aa) = RAWF(uint16_t, rec, 0x200);
    if (frame >= RAWF(uint16_t, rec, 0x200))
        return;
    off = RAWF(int, rec, 0x200 + frame * 4 + 4);
    if (off == 0) {
        self->rectX1 = x;
        self->rectY0 = Y;
        self->rectY1 = Y;
        self->rectX0 = x;
        return;
    }
    fd = rec + 0x200 + off + 8;
    if (ghost == 0)
        hue = self->hue;
    {
        int p = V_HIDDEN(reinterpret_cast<CMobile *>(g_player));
        int q = V_HIDDEN(self);
        if (p != q)
            hue = 0x3b2;
    }
    {
        int g = self->graphic;
        if (g != 0xdd && g != 0x97 && g != 0x96 && g != 0x34 && g != 0x15)
            Mob_DrawShadow(mirror, x, Y, rect, fd);   // 0x0042a670
    }
    if (RAWF(void *, self, 0x128) != nullptr) {
        void *mount = RAWF(void *, self, 0x128);
        int mbody = tile16(mount);
        uint8_t *rec2;
        Mob_AnimFrame(mbody, self->facing, RAWF(int, self, 0x140), a2, RAWF(uint16_t, self, 0x1aa),
                      RAWF(uint16_t, self, 0x1b8) + RAWF(uint16_t, self, 0x1ac), RAWF(int, self, 0x144), &idx, &frame, self);   // 0x0042caf0
        rec2 = Mob_LoadAnim(idx, &ua, &ub);   // 0x004405d0
        if (rec2 != nullptr) {
            off = RAWF(int, rec2, 0x200 + frame * 4 + 4);
            if (off != 0) {
                uint8_t *fd2 = rec2 + 0x200 + off + 8;
                int riderHue = hue;
                int mhue = RAWF(uint16_t, mount, 0x3e);
                int p = V_HIDDEN(reinterpret_cast<CMobile *>(g_player));
                int q = V_HIDDEN(self);
                if (p != q)
                    mhue = 0x3b2;
                Mob_DrawShadow(mirror, x, Y, rect, fd2);        // 0x0042a670
                Mob_Blit(mirror, mhue, x, Y, rect, rec2, fd2);  // 0x0042a7f0
                hue = riderHue;
            }
        }
        if (ghost == 0)
            hue = self->hue;
        {
            int p = V_HIDDEN(reinterpret_cast<CMobile *>(g_player));
            int q = V_HIDDEN(self);
            if (p != q)
                hue = 0x3b2;
        }
        Mob_AnimFrame(self->graphic, self->facing, RAWF(int, self, 0x140), a2, RAWF(uint16_t, self, 0x1aa),
                      RAWF(uint16_t, self, 0x1b8) + RAWF(uint16_t, self, 0x1ac), RAWF(int, self, 0x144), &idx, &frame, self);   // 0x0042caf0
        rec = Mob_LoadAnim(idx, &ua, &ub);   // 0x004405d0
        if (rec == nullptr)
            return;
        RAWF(uint16_t, self, 0x1aa) = RAWF(uint16_t, rec, 0x200);
        if (frame >= RAWF(uint16_t, rec, 0x200))
            return;
        fd = rec + 0x200 + RAWF(int, rec, 0x200 + frame * 4 + 4) + 8;
    }
    Mob_Blit(mirror, hue, x, Y, rect, rec, fd);   // 0x0042a7f0

    deathRobe = 0;
    {
        void *robe = RAWF(void *, self, 0x11c);
        if (robe != nullptr && RAWF(uint16_t, robe, 0x38) == 0x204e)
            deathRobe = 1;
    }
    if (self->graphic >= 0x190) {
        for (int i = 1; i < 0x19; i++) {
            int layer = Mc_layerOrder[(self->facing & 7) * 25 + i];
            CObj *obj;
            int ibody;
            uint8_t *rec3;
            if (layer == 0x15)
                continue;
            obj = RAWF(CObj *, self, 0xc4 + layer * 4);
            if (obj == nullptr) {
                if ((RAWF(uint32_t, self, 0x78) & 0x80000000u) != 0) {
                    CMobile *other = Mob_FindBySerialIdx(RAWF(int, self, 0x78));   // 0x00452290
                    if (other != nullptr) {
                        if (PORT_VSLOT0(other, 0x54, int)() != 0) {
                            CObj *c = RAWF(CObj *, other, 0xa8);
                            if (c != nullptr) {
                                int serial = RAWF(int, other, 0xbc + layer * 4);
                                while (RAWF(int, c, 0x78) != serial) {
                                    c = c->link40;
                                    if (c == nullptr)
                                        break;
                                }
                                if (c != nullptr)
                                    obj = c;
                            }
                        }
                    }
                }
            } else {
                if (Mob_LayerIsCovered(layer, reinterpret_cast<uint8_t *>(self) + 0xc4) != 0)   // 0x00466b60
                    continue;
            }
            if (obj == nullptr)
                continue;
            if (deathRobe == 1) {
                uint8_t tt = tileType(obj);
                if (tt == 0xb || tt == 0x10)
                    continue;
            }
            ibody = tile16(obj);
            if (ibody < 0x190 || ibody >= 0x3e8)
                continue;
            Mob_AnimFrame(ibody, self->facing, RAWF(int, self, 0x140), a2, RAWF(uint16_t, self, 0x1aa),
                          RAWF(uint16_t, self, 0x1b8) + RAWF(uint16_t, self, 0x1ac), RAWF(int, self, 0x144), &idx, &frame, self);   // 0x0042caf0
            rec3 = Mob_LoadAnim(idx, &ua, &ub);   // 0x004405d0
            if (rec3 == nullptr)
                continue;
            off = RAWF(int, rec3, 0x200 + frame * 4 + 4);
            if (off == 0)
                continue;
            fd = rec3 + 0x200 + off + 8;
            if (ghost == 0)
                hue = obj->hue;
            {
                int p = V_HIDDEN(reinterpret_cast<CMobile *>(g_player));
                int q = V_HIDDEN(self);
                if (p != q) {
                    hue = 0x3b2;
                } else if ((tileFlags(obj) & 0x40000) != 0) {
                    hue |= 0x8000;
                }
            }
            Mob_Blit(mirror, hue, x, Y, rect, rec3, fd);   // 0x0042a7f0
        }
    }
    for (uint8_t *g = RAWF(uint8_t *, self, 0x1c0); g != nullptr; g = RAWF(uint8_t *, g, 0x100)) {
        uint8_t *art = Mob_GumpArt(RAWF(uint16_t, g, 0xda) + 0x4000);   // 0x00476130
        int w = RAWF(int16_t, art, 4);
        int h = RAWF(int16_t, art, 6);
        int r[4];
        r[0] = ((self->rectX1 + self->rectX0) >> 1) - (w >> 1);
        r[2] = r[0] + w;
        r[1] = ((self->rectY0 + self->rectY1) >> 1) - (h >> 1);
        r[3] = r[1] + h;
        Mob_DrawArt(RAWF(uint16_t, g, 0xda), RAWF(uint8_t, g, 0xd0), r, rect);   // 0x00437e60
    }
}
HOOK_VMETHOD(0x0042dc30, CMobile, Draw);

// 0x0042e430  size 0x473
// CalcRect of the player object: same as 0x0042d6d0 but the pixel origin comes from the view
// (0x0069dda4 / 0x0069dda8), the animation tick and moving flag from the player globals
// (0x0064885c / 0x00648868), and the equipment loop has no serial fallback. ECX = player, plain ret.
void __thiscall CPlayer::ComputeScreenRect()
{
    CMobile *const self = this;
    int px;
    int py;
    int X;
    int Y;
    int body;
    int a2;
    int idx;
    int frame;
    int ua, ub;
    int mirror;
    uint8_t *rec;
    uint8_t *fr;
    int off;
    int w0, w2, w4, w6;
    int h16;
    int ecx;
    int t;
    int count;

    if (Mob_IsPreview(self) != 0) {        // 0x00471040
        Mob_ComputeBoundsPreview(self);    // 0x00471e20
        return;
    }
    py = self->y;
    px = self->x;
    X = g_viewOffsetX + (g_scrollBase - py + px) * 22 + 1;
    {
        int y0 = (px + py) * 22 + (5 - RAWF(int, self, 0x190)) * 4;
        RAWF(int, self, 0xb4) = y0;
        Y = g_viewOffsetY + y0;
    }
    if (self->graphic >= 0x3e8) {
        self->rectY0 = Y;
        self->rectY1 = Y;
        self->rectX1 = X;
        self->rectX0 = X;
        return;
    }
    if (V_HIDDEN(self) != 0)
        body = self->graphic - 2;
    else
        body = self->graphic;
    a2 = V_ACTION(self);
    Mob_AnimFrame(body, self->facing, g_walkTick, a2, RAWF(uint16_t, self, 0x1aa),
                  RAWF(uint16_t, self, 0x1b8) + RAWF(uint16_t, self, 0x1ac), g_walkDirState, &idx, &frame, self);   // 0x0042caf0
    mirror = g_dirMirror[self->facing & 0x7f];
    rec = Mob_LoadAnim(idx, &ua, &ub);   // 0x004405d0
    if (rec == nullptr) {
        self->rectY0 = Y;
        self->rectY1 = Y;
        self->rectX1 = X;
        self->rectX0 = X;
        return;
    }
    count = RAWF(uint16_t, rec, 0x200);
    RAWF(uint16_t, self, 0x1aa) = static_cast<uint16_t>(count);
    if (frame >= count) {
        self->rectY0 = Y;
        self->rectY1 = Y;
        self->rectX1 = X;
        self->rectX0 = X;
        return;
    }
    off = RAWF(int, rec, 0x200 + frame * 4 + 4);
    if (off == 0) {
        self->rectY0 = Y;
        self->rectY1 = Y;
        self->rectX1 = X;
        self->rectX0 = X;
        return;
    }
    fr = rec + 0x200 + off;
    w0 = RAWF(int16_t, fr, 0);
    w2 = RAWF(int16_t, fr, 2);
    w4 = RAWF(int16_t, fr, 4);
    w6 = RAWF(int16_t, fr, 6);
    h16 = w6;
    ecx = (mirror != 0) ? static_cast<int16_t>(w4 - w0) : w0;
    self->rectY1 = Y - w2;
    self->rectX0 = X - ecx;
    self->rectY0 = self->rectY1 - w6;
    self->rectX1 = self->rectX0 + w4;
    if (w2 >= 0) {
        self->rectY1 = Y;
        h16 = static_cast<int16_t>(h16 + w2);
    }
    t = self->rectX1 - X + 0x16;
    if (t <= 0x2c)
        RAWF(int, self, 0xb8) = 0;
    else
        RAWF(int, self, 0xb8) = t / 0x2c;
    if ((self->flags98 & 0x80) == 0 && V_HIDDEN(self) == 0) {
        int g = self->graphic;
        if (g != 0xdd && g != 0x97 && g != 0x96 && g != 0x34 && g != 0x15)
            self->rectX1 = self->rectX1 + (h16 >> 1);
    }
    if (self->graphic >= 0x190) {
        for (int i = 1; i < 0x1a; i++) {
            CObj *obj;
            int ibody;
            if (i == 0x15)
                continue;
            obj = RAWF(CObj *, self, 0xc4 + i * 4);
            if (obj == nullptr)
                continue;
            if (Mob_LayerIsCovered(i, reinterpret_cast<uint8_t *>(self) + 0xc4) != 0)   // 0x00466b60
                continue;
            ibody = tile16(RAWF(CObj *, self, 0xc4 + i * 4));
            if (ibody < 0x190 || ibody >= 0x3e8)
                continue;
            Mob_AnimFrame(ibody, self->facing, g_walkTick, a2, RAWF(uint16_t, self, 0x1aa),
                          RAWF(uint16_t, self, 0x1b8) + RAWF(uint16_t, self, 0x1ac), g_walkDirState, &idx, &frame, self);   // 0x0042caf0
            rec = Mob_LoadAnim(idx, &ua, &ub);   // 0x004405d0
            if (rec == nullptr)
                continue;
            off = RAWF(int, rec, 0x200 + frame * 4 + 4);
            if (off == 0)
                continue;
            fr = rec + 0x200 + off;
            w0 = RAWF(int16_t, fr, 0);
            w2 = RAWF(int16_t, fr, 2);
            w4 = RAWF(int16_t, fr, 4);
            w6 = RAWF(int16_t, fr, 6);
            if (mirror != 0)
                w0 = static_cast<int16_t>(w4 - w0);
            int bottom = Y - w2;
            int left = X - w0;
            int top = bottom - w6;
            int right = left + w4;
            if (i == 0x19)
                right += w4 >> 1;
            if (right > self->rectX1)
                self->rectX1 = right;
            if (left < self->rectX0)
                self->rectX0 = left;
            if (bottom > self->rectY1)
                self->rectY1 = bottom;
            if (top < self->rectY0)
                self->rectY0 = top;
        }
    }
    {
        uint8_t *g = RAWF(uint8_t *, self, 0x1c0);
        RAWF(int, self, 0x1cc) = 0;
        RAWF(int, self, 0x1c8) = 0;
        while (g != nullptr) {
            uint8_t *art = Mob_GumpArt(RAWF(uint16_t, g, 0xda) + 0x4000);   // 0x00476130
            int h = RAWF(uint16_t, art, 6);
            int d = RAWF(int16_t, art, 4) - self->rectX1 + self->rectX0;
            if (d > 0) {
                d >>= 1;
                RAWF(int, self, 0x1c8) += d;
                self->rectX0 -= d;
                self->rectX1 += d;
            }
            d = static_cast<int16_t>(h) - self->rectY1 + self->rectY0;
            if (d > 0) {
                d >>= 1;
                RAWF(int, self, 0x1cc) += d;
                self->rectY0 -= d;
                self->rectY1 += d;
            }
            g = RAWF(uint8_t *, g, 0x100);
        }
    }
}
HOOK_VMETHOD(0x0042e430, CPlayer, ComputeScreenRect);

// 0x0042e8b0  size 0x5de
// Draw the player: like 0x0042dc30 without the target/ghost hue logic and without the serial
// fallback for equipment; the hue is the tile's hue or 0x3b2 for a mobile with flag 0x80 at +0x98.
// ECX = player, stack: clip, RET 4.
void __thiscall CPlayer::Draw(int *clip)
{
    CMobile *const self = this;
    int rect[4];
    int X;
    int Y;
    int body;
    int a2;
    int idx;
    int frame;
    int ua, ub;
    int mirror;
    uint8_t *rec;
    uint8_t *fd;
    int off;
    int hue;
    int flag80;
    int deathRobe;

    if (self->graphic >= 0x3e8)
        return;
    if (Mob_IsPreview(self) != 0) {        // 0x00471040
        Mob_DrawPreview(self, 0, clip);    // 0x004721f0
        return;
    }
    rect[0] = clip[0];
    rect[1] = clip[1];
    rect[2] = clip[2];
    rect[3] = clip[3];
    X = g_viewOffsetX + (self->x - self->y + g_scrollBase) * 22 + 1;
    {
        int y0 = RAWF(int, self, 0xb4);
        Y = g_viewOffsetY + y0;
        if (g_clipBusyBB == 0 && self->graphic < 0x190) {
            if (rect[3] > y0 - g_scrollY)
                rect[3] = y0 - g_scrollY;
        }
    }
    if (V_HIDDEN(self) != 0)
        body = self->graphic - 2;
    else
        body = self->graphic;
    a2 = V_ACTION(self);
    Mob_AnimFrame(body, self->facing, g_walkTick, a2, RAWF(uint16_t, self, 0x1aa),
                  RAWF(uint16_t, self, 0x1ac) + RAWF(uint16_t, self, 0x1b8), g_walkDirState, &idx, &frame, self);   // 0x0042caf0
    mirror = g_dirMirror[self->facing & 0x7f];
    rec = Mob_LoadAnim(idx, &ua, &ub);   // 0x004405d0
    if (rec == nullptr)
        return;
    RAWF(uint16_t, self, 0x1aa) = RAWF(uint16_t, rec, 0x200);
    if (frame >= RAWF(uint16_t, rec, 0x200))
        return;
    off = RAWF(int, rec, 0x200 + frame * 4 + 4);
    if (off == 0) {
        self->rectX1 = X;
        self->rectY0 = Y;
        self->rectY1 = Y;
        self->rectX0 = X;
        return;
    }
    fd = rec + 0x200 + off + 8;
    flag80 = self->flags98 & 0x80;
    hue = self->hue;
    if (flag80 != 0)
        hue = 0x3b2;
    if (flag80 == 0 && V_HIDDEN(self) == 0) {
        int g = self->graphic;
        if (g != 0xdd && g != 0x97 && g != 0x96 && g != 0x34 && g != 0x15)
            Mob_DrawShadow(mirror, X, Y, rect, fd);   // 0x0042a670
    }
    if (RAWF(void *, self, 0x128) != nullptr) {
        void *mount = RAWF(void *, self, 0x128);
        int mbody = tile16(mount);
        uint8_t *rec2;
        Mob_AnimFrame(mbody, self->facing, g_walkTick, a2, RAWF(uint16_t, self, 0x1aa),
                      RAWF(uint16_t, self, 0x1ac) + RAWF(uint16_t, self, 0x1b8), g_walkDirState, &idx, &frame, self);   // 0x0042caf0
        rec2 = Mob_LoadAnim(idx, &ua, &ub);   // 0x004405d0
        if (rec2 != nullptr) {
            off = RAWF(int, rec2, 0x200 + frame * 4 + 4);
            if (off != 0) {
                uint8_t *fd2 = rec2 + 0x200 + off + 8;
                int mhue;
                if ((self->flags98 & 0x80) != 0)
                    mhue = 0x3b2;
                else
                    mhue = RAWF(uint16_t, RAWF(void *, self, 0x128), 0x3e);
                Mob_DrawShadow(mirror, X, Y, rect, fd2);        // 0x0042a670
                Mob_Blit(mirror, mhue, X, Y, rect, rec2, fd2);  // 0x0042a7f0
            }
        }
        Mob_AnimFrame(self->graphic, self->facing, g_walkTick, a2, RAWF(uint16_t, self, 0x1aa),
                      RAWF(uint16_t, self, 0x1ac) + RAWF(uint16_t, self, 0x1b8), g_walkDirState, &idx, &frame, self);   // 0x0042caf0
        rec = Mob_LoadAnim(idx, &ua, &ub);   // 0x004405d0
        if (rec == nullptr)
            return;
        RAWF(uint16_t, self, 0x1aa) = RAWF(uint16_t, rec, 0x200);
        if (frame >= RAWF(uint16_t, rec, 0x200))
            return;
        hue = self->hue;
        fd = rec + 0x200 + RAWF(int, rec, 0x200 + frame * 4 + 4) + 8;
        if ((self->flags98 & 0x80) != 0)
            hue = 0x3b2;
    }
    Mob_Blit(mirror, hue, X, Y, rect, rec, fd);   // 0x0042a7f0

    deathRobe = 0;
    {
        void *robe = RAWF(void *, self, 0x11c);
        if (robe != nullptr && RAWF(uint16_t, robe, 0x38) == 0x204e)
            deathRobe = 1;
    }
    if (self->graphic >= 0x190) {
        for (int i = 1; i < 0x19; i++) {
            int layer = Mc_layerOrder[(self->facing & 7) * 25 + i];
            CObj *obj;
            int ibody;
            uint8_t *rec3;
            int hue3;
            if (layer == 0x15)
                continue;
            if (RAWF(CObj *, self, 0xc4 + layer * 4) == nullptr)
                continue;
            if (Mob_LayerIsCovered(layer, reinterpret_cast<uint8_t *>(self) + 0xc4) != 0)   // 0x00466b60
                continue;
            if (deathRobe == 1) {
                CObj *o = RAWF(CObj *, self, 0xc4 + layer * 4);
                if (o != nullptr) {
                    uint8_t tt = tileType(o);
                    if (tt == 0xb || tt == 0x10)
                        continue;
                }
            }
            obj = RAWF(CObj *, self, 0xc4 + layer * 4);
            ibody = tile16(obj);
            if (ibody < 0x190 || ibody >= 0x3e8)
                continue;
            Mob_AnimFrame(ibody, self->facing, g_walkTick, a2, RAWF(uint16_t, self, 0x1aa),
                          RAWF(uint16_t, self, 0x1ac) + RAWF(uint16_t, self, 0x1b8), g_walkDirState, &idx, &frame, self);   // 0x0042caf0
            rec3 = Mob_LoadAnim(idx, &ua, &ub);   // 0x004405d0
            if (rec3 == nullptr)
                continue;
            off = RAWF(int, rec3, 0x200 + frame * 4 + 4);
            if (off == 0)
                continue;
            fd = rec3 + 0x200 + off + 8;
            obj = RAWF(CObj *, self, 0xc4 + layer * 4);
            hue3 = obj->hue;
            if ((self->flags98 & 0x80) != 0) {
                hue3 = 0x3b2;
            } else if ((RAWF(uint8_t, g_tileData + obj->graphic * 0x28, 2) & 4) != 0) {
                hue3 |= 0x8000;
            }
            Mob_Blit(mirror, hue3, X, Y, rect, rec3, fd);   // 0x0042a7f0
        }
    }
    for (uint8_t *g = RAWF(uint8_t *, self, 0x1c0); g != nullptr; g = RAWF(uint8_t *, g, 0x100)) {
        uint8_t *art = Mob_GumpArt(RAWF(uint16_t, g, 0xda) + 0x4000);   // 0x00476130
        int w = RAWF(int16_t, art, 4);
        int h = RAWF(int16_t, art, 6);
        int r[4];
        r[0] = ((self->rectX1 + self->rectX0) >> 1) - (w >> 1);
        r[2] = r[0] + w;
        r[1] = ((self->rectY0 + self->rectY1) >> 1) - (h >> 1);
        r[3] = r[1] + h;
        Mob_DrawArt(RAWF(uint16_t, g, 0xda), RAWF(uint8_t, g, 0xd0), r, rect);   // 0x00437e60
    }
}
HOOK_VMETHOD(0x0042e8b0, CPlayer, Draw);
