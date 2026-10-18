// Mobile drawing through the 64x128 sprite buffer (CAnimOverlayBuffer, the one instance at 0x007226a0):
// the mount lookup, the screen rectangle, the draw and the pixel hit test of a mobile and its equipment, in the
// world and in the "preview" form (paperdoll / status view offsets). 0x00470aa0 .. 0x004725d0.
//
// The CMobile methods are free __fastcall functions (ECX = mobile, EDX unused), as in mobile_draw_ops.cpp.
// The animation index arithmetic that the original repeats inline (175 * body + base[facing] - 0x88a4, or
// - 0x883b for facings 0 and 6) is the static function Mc_AnimIndex here.
// ? The vtable slot 0x84 call (animation update, result unused) and slot 0x8c (compared between the player and
//   the mobile) are not identified.
#include <cstdint>
#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"
#include "mobile_anim.h"
#include "obj.h"
#include "raw_field.h"
#include "vcall.h"
#include "client_misc_ops_ports.h"
#include "file_manager.h"
#include "art_record_ops_ports.h"
#include "paperdoll_gump_ops_ports.h"
#include "mobile_draw_ops_ports.h"
#include "missile_draw_ports.h"
#include "mobile_cached_draw_ports.h"

#define Mc_buffer GLOBAL_AS(CAnimOverlayBuffer, g_animOverlayBuffer_storage)
#define Mc_table GLOBAL_PTR(AnimOverlayEntry, g_animOverlayTable_storage)
#define Mc_targetView GLOBAL_AS(uint8_t *, g_screenGump)   // object whose +0xd4 is the targeted mobile
#define Mc_player GLOBAL_AS(CMobile *, g_player)
#define Mc_mountIndex GLOBAL_AS(int, g_animOverlayBuffer_storage)   // first dword of the buffer: table entry in use

#define Mc_Hidden PORT_FUNC0(&Misc_Return0, int, __cdecl)
#define Mc_LoadAnim(idx, pa, pb) \
    PORT_THISCALL5(&CFileManager::GetRecord, uint8_t *, GLOBAL_PTR(void, g_fileManager_storage), int, int, int, int *, int *)(5, 6, (idx), (pa), (pb))
#define Mc_GumpArt(id) PORT_THISCALL1(&ArtRecord_Get, uint8_t *, GLOBAL_PTR(void, g_artCache_storage), uint32_t)((id))
#define Mc_LayerIsCovered(a1, a2) (Paperdoll_LayerIsCovered(a1, (uint8_t *)(a2)))
#define Mc_IsTileType0b10(a1) (Obj_IsTileType0b10(a1))
#define Mc_NameHue(a1) (Mobile_NameHue(a1))
#define Mc_DrawArt(a1, a2, a3, a4) (MissileDrawTileArt(a1, a2, a3, a4))

static int Mc_AnimIndex(int body, int facing)
{
    int base = g_dirAnimBase[facing & 7] + (body & 0xffff) * 0xaf;
    if (facing == 0 || facing == 6)
        return base - 0x883b;
    return base - 0x88a4;
}

static unsigned Mc_BodyOf(CObj *item)
{
    return *reinterpret_cast<uint16_t *>(g_tileData + item->graphic * 0x28 + 0xc);
}

// 0x00470aa0  size 0xaf  RET 8
int __thiscall CAnimOverlayBuffer::PixelAt(int px, int py)
{
    int sx = px - g_scrollX;
    int sy = py - g_scrollY;
    int dir = direction;
    int off;

    if (dir != 0 && dir != 6)
        off = Mc_table[tableIndex].offsetA;
    else
        off = Mc_table[tableIndex].offsetB;
    y = y + off;
    if (sx >= x && sx < x + 0x40 && sy >= y && sy < y + 0x80) {
        int m = maps[((((dir >> 1) << 7) + sy - y) << 6) + sx - x];
        return pixels[m];
    }
    return 0;
}
HOOK_METHOD(0x00470aa0, CAnimOverlayBuffer, PixelAt);

// 0x00470fa0  size 0x94  RET 4
void __thiscall CAnimOverlayBuffer::NearestAllowedFacing(int *facing)
{
    int f = *facing & 7;
    *facing = f;
    int mask = Mc_table[tableIndex].enabled;
    if (mask & Mc_facingBits[f])
        return;
    f = (f + 1) & 7;
    *facing = f;
    if (Mc_facingBits[f] & mask)
        return;
    f = (f - 2) & 7;
    *facing = f;
    if (Mc_facingBits[f] & mask)
        return;
    f = (f + 3) & 7;
    *facing = f;
    if (Mc_facingBits[f] & mask)
        return;
    f = (f - 4) & 7;
    *facing = f;
    if (Mc_facingBits[f] & mask)
        return;
    f = (f - 3) & 7;
    *facing = f;
    if (Mc_facingBits[f] & mask)
        return;
    f = (f + 2) & 7;
    *facing = f;
    if (Mc_facingBits[f] & mask)
        return;
    f = (f - 1) & 7;
    *facing = f;
}
HOOK_METHOD(0x00470fa0, CAnimOverlayBuffer, NearestAllowedFacing);

// 0x00471040  size 0x144  plain RET, returns 0 / 1
// A body (graphic 0x190..0x3e7) in the world, not hidden, not the player in the 0x00648858 state and without a
// mount equipped, standing on a cell with an item of the same position (z within 1) that is in the table at
// 0x004ae760 with a non-zero mask: remembers its table index in 0x007226a0 and returns 1 when the mask has no
// 0x10 bit or the mobile's +0x148 is set.
int __fastcall Mobile_FindMountEntry(CMobile *self)
{
    if (self->inWorld == 0)
        return 0;
    if (self->graphic < 0x190)
        return 0;
    if (self->graphic >= 0x3e8)
        return 0;
    if (decomp::vcall(PORT_RET(int), self, 0x8c) != 0)
        return 0;
    if (self->unk_13c != 0)
        return 0;
    if (self == Mc_player) {
        if (g_altFacingFlag != 0)
            return 0;
    }
    if (self->equip[25] != nullptr)
        return 0;
    int cell = ((self->y & 0x3f) << 6) + (self->x & 0x3f);
    CWorldObj *o = g_tileBuckets[cell];
    if (o == nullptr)
        return 0;
    do {
        int dz = o->z - self->z;
        if (dz < 0)
            dz = -dz;
        if (dz <= 1 && o->x == self->x && o->y == self->y) {
            if (decomp::vcall(PORT_RET(int), o, 0x18) != 0 && decomp::vcall(PORT_RET(int), o, 0x24) == 0) {
                int found = 0;
                int id = Mc_table[0].id;
                unsigned g = static_cast<CObj *>(o)->graphic;
                if (id != 0) {
                    int n = 0;
                    AnimOverlayEntry *e = Mc_table;
                    for (;;) {
                        if (id == static_cast<int>(g) && e->enabled != 0) {
                            found = n + 1;
                            break;
                        }
                        id = e[1].id;
                        e++;
                        n++;
                        if (id == 0)
                            break;
                    }
                }
                if (found != 0) {
                    Mc_mountIndex = found - 1;
                    if ((Mc_table[found - 1].enabled & 0x10) == 0)
                        return 1;
                    if (self->flag148 != 0)
                        return 1;
                }
            }
        }
        o = o->next;
    } while (o != nullptr);
    return 0;
}
HOOK_FUNC(0x00471040, Mobile_FindMountEntry);

// 0x00471190  size 0x3d2  plain RET
// World screen rectangle of the mobile (frame 0 headers of the body and of each equipped item, widened by 0x10 / 0x20 and by the overhead gumps). A hidden-flag (+0x98 bit 7) mobile that
// 0x00478cd0 reports false for gets the empty rectangle at its origin.
void __fastcall Mobile_ComputeBoundsCached(CMobile *self)
{
    int facing = static_cast<int>(static_cast<uint8_t>(self->facing));
    Mc_buffer.NearestAllowedFacing(&facing);   // 0x00470fa0
    int px = self->x;
    int py = self->y;
    int X = (g_scrollBase - py + px) * 22 - RAWF(int, self, 0x188) + 1;
    int Y0 = (px + py) * 22 + (5 - RAWF(int, self, 0x190)) * 4;
    RAWF(int, self, 0xb4) = Y0;
    int Y = Y0 - RAWF(int, self, 0x18c);
    if ((self->flags98 & 0x80) != 0 && Mc_Hidden() == 0) {   // 0x00478cd0
        self->rectY0 = Y;
        self->rectY1 = Y;
        self->rectX1 = X;
        self->rectX0 = X;
        return;
    }
    decomp::vcall(PORT_RET(int), self, 0x84);
    int ua, ub;
    int mirror = g_dirMirror[facing];
    uint8_t *rec = Mc_LoadAnim(Mc_AnimIndex(self->graphic, facing), &ua, &ub);   // 0x004405d0
    if (rec == nullptr) {
        self->rectX1 = X;
        self->rectY0 = Y;
        self->rectY1 = Y;
        self->rectX0 = X;
        return;
    }
    int count = RAWF(uint16_t, rec, 0x200);
    self->animFrame = static_cast<uint16_t>(count);
    if (count == 0) {
        return;
    }
    int off = RAWF(int, rec, 0x204);
    if (off == 0) {
        self->rectX1 = X;
        self->rectY0 = Y;
        self->rectY1 = Y;
        self->rectX0 = X;
        return;
    }
    uint8_t *fr = rec + 0x200 + off;
    int w0 = RAWF(int16_t, fr, 0);
    int w2 = RAWF(int16_t, fr, 2);
    int w4 = RAWF(int16_t, fr, 4);
    int w6 = RAWF(int16_t, fr, 6);
    if (mirror != 0)
        w0 = static_cast<int16_t>(w4 - w0);
    self->unk_b8 = 0;
    self->rectY1 = Y - w2;
    self->rectX0 = X - w0;
    self->rectY0 = self->rectY1 - w6;
    self->rectX1 = self->rectX0 + w4;
    for (int i = 1; i < 0x1a; i++) {
        CObj *item;
        if (i == 0x15)
            continue;
        item = self->equip[i];
        if (item == nullptr)
            continue;
        unsigned ibody = Mc_BodyOf(item);
        if (static_cast<int>(ibody) < 0x190 || static_cast<int>(ibody) >= 0x3e8)
            continue;
        uint8_t *r2 = Mc_LoadAnim(Mc_AnimIndex(static_cast<int>(ibody), facing), &ua, &ub);   // 0x004405d0
        if (r2 == nullptr)
            continue;
        int o2 = RAWF(int, r2, 0x204);
        if (o2 == 0)
            continue;
        uint8_t *f2 = r2 + 0x200 + o2;
        int a0 = RAWF(int16_t, f2, 0);
        int a2 = RAWF(int16_t, f2, 2);
        int a4 = RAWF(int16_t, f2, 4);
        int a6 = RAWF(int16_t, f2, 6);
        if (mirror != 0)
            a0 = static_cast<int16_t>(a4 - a0);
        int bottom = Y - a2;
        int left = X - a0;
        int top = bottom - a6;
        int right = left + a4;
        if (right > self->rectX1)
            self->rectX1 = right;
        if (left < self->rectX0)
            self->rectX0 = left;
        if (bottom > self->rectY1)
            self->rectY1 = bottom;
        if (top < self->rectY0)
            self->rectY0 = top;
    }
    self->rectY0 = self->rectY0 - 0x10;
    self->rectX0 = self->rectX0 - 0x10;
    self->rectY1 = self->rectY1 + 0x20;
    self->rectX1 = self->rectX1 + 0x10;
    uint8_t *g = RAWF(uint8_t *, self, 0x1c0);
    RAWF(int, self, 0x1cc) = 0;
    RAWF(int, self, 0x1c8) = 0;
    while (g != nullptr) {
        uint8_t *art = Mc_GumpArt(RAWF(uint16_t, g, 0xda) + 0x4000);   // 0x00476130
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
HOOK_FUNC(0x00471190, Mobile_ComputeBoundsCached);

// 0x00471570  size 0x4af  RET 4
// Renders the body, then each equipped item in the per-facing layer order, into the sprite buffer (clipped
// to `clip`), copies the buffer to the back buffer and draws the overhead gumps (+0x1c0 list) centred in the
// mobile's rectangle. Hues: 0x3b2 for a hidden-flag mobile that 0x00478cd0 reports true for, the name hue for
// the targeted mobile, 0x3b2 for everything when the player and this mobile disagree on vtable slot 0x8c,
// else the item's hue (bit 15 added for items whose tiledata flag 0x40000 is set).
void __fastcall Mobile_DrawCached(CMobile *self, int, int *clipArg)
{
    int facing = static_cast<int>(static_cast<uint8_t>(self->facing));
    int hue = 0;
    bool forced = false;
    int clip[4];
    int ua, ub;

    Mc_buffer.NearestAllowedFacing(&facing);   // 0x00470fa0
    if ((self->flags98 & 0x80) != 0) {
        if (Mc_Hidden() == 0)   // 0x00478cd0
            return;
        hue = 0x3b2;
        forced = true;
    }
    if (self == RAWF(CMobile *, Mc_targetView, 0xd4)) {
        hue = Mc_NameHue(self);   // 0x0042dbc0
        forced = true;
    }
    clip[0] = clipArg[0];
    clip[1] = clipArg[1];
    clip[2] = clipArg[2];
    clip[3] = clipArg[3];
    int X = (self->x - self->y + g_scrollBase) * 22 - RAWF(int, self, 0x188) + 1;
    int Y = RAWF(int, self, 0xb4) - RAWF(int, self, 0x18c);
    decomp::vcall(PORT_RET(int), self, 0x84);
    int mirror = g_dirMirror[facing];
    uint8_t *rec = Mc_LoadAnim(Mc_AnimIndex(self->graphic, facing), &ua, &ub);   // 0x004405d0
    if (rec == nullptr)
        return;
    int count = RAWF(uint16_t, rec, 0x200);
    self->animFrame = static_cast<uint16_t>(count);
    if (count == 0)
        return;
    int off = RAWF(int, rec, 0x204);
    if (off == 0) {
        self->rectX1 = X;
        self->rectY0 = Y;
        self->rectY1 = Y;
        self->rectX0 = X;
        return;
    }
    uint32_t *rle = reinterpret_cast<uint32_t *>(rec + 0x200 + off + 8);
    if (!forced)
        hue = self->hue;
    int playerSlot = decomp::vcall(PORT_RET(int), Mc_player, 0x8c);
    if (playerSlot != decomp::vcall(PORT_RET(int), self, 0x8c))
        hue = 0x3b2;
    Mc_buffer.unk_24010 = mirror;
    Mc_buffer.clip = clip;
    memset(Mc_buffer.pixels, 0, sizeof(Mc_buffer.pixels));
    Mc_buffer.direction = facing;
    Mc_buffer.y = Y - g_scrollY - 0x50;
    Mc_buffer.x = X - g_scrollX - 0x20;
    Mc_buffer.DecodeFrame(static_cast<unsigned>(hue), reinterpret_cast<uint16_t *>(rec), rle);   // 0x00470b50

    int mounted = 0;
    CObj *mount = self->equip[22];
    if (mount != nullptr && mount->graphic == 0x204e)
        mounted = 1;
    if (self->graphic >= 0x190) {
        for (int i = 1; i < 0x19; i++) {
            int slot = Mc_layerOrder[(self->facing & 7) * 25 + i];
            if (slot == 0x15)
                continue;
            CObj *item = self->equip[slot];
            if (item == nullptr)
                continue;
            if (Mc_LayerIsCovered(slot, &self->equip[0]) != 0)   // 0x00466b60
                continue;
            if (mounted == 1 && Mc_IsTileType0b10(item) != 0)   // 0x0042db90
                continue;
            unsigned ibody = Mc_BodyOf(item);
            if (static_cast<int>(ibody) < 0x190 || static_cast<int>(ibody) >= 0x3e8)
                continue;
            uint8_t *r2 = Mc_LoadAnim(Mc_AnimIndex(static_cast<int>(ibody), facing), &ua, &ub);   // 0x004405d0
            if (r2 == nullptr)
                continue;
            int o2 = RAWF(int, r2, 0x204);
            if (o2 == 0)
                continue;
            uint32_t *rle2 = reinterpret_cast<uint32_t *>(r2 + 0x200 + o2 + 8);
            if (!forced)
                hue = item->hue;
            int mode = hue;
            int pv = decomp::vcall(PORT_RET(int), Mc_player, 0x8c);
            if (pv != decomp::vcall(PORT_RET(int), self, 0x8c)) {
                hue = 0x3b2;
                mode = hue;
            } else if (!forced) {
                if ((*reinterpret_cast<uint32_t *>(g_tileData + item->graphic * 0x28) & 0x40000) != 0) {
                    mode |= 0x8000;
                    hue = mode;
                }
            }
            Mc_buffer.DecodeFrame(static_cast<unsigned>(mode), reinterpret_cast<uint16_t *>(r2), rle2);   // 0x00470b50
        }
    }
    Mc_buffer.DrawOverlay(self);   // 0x00470870
    for (uint8_t *g = RAWF(uint8_t *, self, 0x1c0); g != nullptr; g = RAWF(uint8_t *, g, 0x100)) {
        uint8_t *art = Mc_GumpArt(RAWF(uint16_t, g, 0xda) + 0x4000);   // 0x00476130
        int w = RAWF(int16_t, art, 4);
        int h = RAWF(int16_t, art, 6);
        int rc[4];
        rc[0] = ((self->rectX0 + self->rectX1) >> 1) - (w >> 1);
        rc[2] = rc[0] + w;
        rc[1] = ((self->rectY1 + self->rectY0) >> 1) - (h >> 1);
        rc[3] = rc[1] + h;
        Mc_DrawArt(RAWF(uint16_t, g, 0xda), RAWF(uint8_t, g, 0xd0), rc, clip);   // 0x00437e60
    }
}
HOOK_FUNC(0x00471570, Mobile_DrawCached);

// 0x00471a20  size 0x3f5  RET 8, returns the pixel (16 bits, zero extended) or 0
// Renders the body and equipment into the sprite buffer exactly as Mobile_DrawCached does (clip = the whole
// 640x480 screen, no blit) and returns the pixel under the screen point (px, py) from the buffer.
// Per item the hue is the item hue, 0x3b2 when the player and this mobile disagree on slot 0x8c, else with bit
// 15 added for tiledata flag 0x40000 (the forced hue of a hidden-flag mobile is not used for items here).
int __thiscall CMobile::HitTest(int px, int py)
{
    CMobile *const self = this;
    int facing = static_cast<int>(static_cast<uint8_t>(self->facing));
    int hue = 0;
    bool forced = false;
    int clip[4];
    int ua, ub;

    Mc_buffer.NearestAllowedFacing(&facing);   // 0x00470fa0
    clip[1] = 0;
    clip[0] = 0;
    clip[2] = 0x280;
    clip[3] = 0x1e0;
    if ((self->flags98 & 0x80) != 0) {
        if (Mc_Hidden() == 0)   // 0x00478cd0
            return 0;
        hue = 0x3b2;
        forced = true;
    }
    int X = (self->x - self->y + g_scrollBase) * 22 - RAWF(int, self, 0x188) + 1;
    int Y = RAWF(int, self, 0xb4) - RAWF(int, self, 0x18c);
    decomp::vcall(PORT_RET(int), self, 0x84);
    int mirror = g_dirMirror[facing];
    uint8_t *rec = Mc_LoadAnim(Mc_AnimIndex(self->graphic, facing), &ua, &ub);   // 0x004405d0
    if (rec == nullptr)
        return 0;
    int count = RAWF(uint16_t, rec, 0x200);
    self->animFrame = static_cast<uint16_t>(count);
    if (count == 0)
        return 0;
    int off = RAWF(int, rec, 0x204);
    if (off == 0) {
        self->rectX1 = X;
        self->rectY0 = Y;
        self->rectY1 = Y;
        self->rectX0 = X;
        return 0;
    }
    uint32_t *rle = reinterpret_cast<uint32_t *>(rec + 0x200 + off + 8);
    if (!forced)
        hue = self->hue;
    int playerSlot = decomp::vcall(PORT_RET(int), Mc_player, 0x8c);
    int mode = hue;
    if (playerSlot != decomp::vcall(PORT_RET(int), self, 0x8c))
        mode = 0x3b2;
    Mc_buffer.clip = clip;
    Mc_buffer.unk_24010 = mirror;
    Mc_buffer.direction = facing;
    memset(Mc_buffer.pixels, 0, sizeof(Mc_buffer.pixels));
    Mc_buffer.x = X - g_scrollX - 0x20;
    Mc_buffer.y = Y - g_scrollY - 0x50;
    Mc_buffer.DecodeFrame(static_cast<unsigned>(mode), reinterpret_cast<uint16_t *>(rec), rle);   // 0x00470b50

    int mounted = 0;
    CObj *mount = self->equip[22];
    if (mount != nullptr && mount->graphic == 0x204e)
        mounted = 1;
    if (self->graphic >= 0x190) {
        for (int i = 1; i < 0x19; i++) {
            int slot = Mc_layerOrder[(self->facing & 7) * 25 + i];
            if (slot == 0x15)
                continue;
            CObj *item = self->equip[slot];
            if (item == nullptr)
                continue;
            if (Mc_LayerIsCovered(slot, &self->equip[0]) != 0)   // 0x00466b60
                continue;
            if (mounted == 1 && Mc_IsTileType0b10(item) != 0)   // 0x0042db90
                continue;
            unsigned ibody = Mc_BodyOf(item);
            if (static_cast<int>(ibody) < 0x190 || static_cast<int>(ibody) >= 0x3e8)
                continue;
            uint8_t *r2 = Mc_LoadAnim(Mc_AnimIndex(static_cast<int>(ibody), facing), &ua, &ub);   // 0x004405d0
            if (r2 == nullptr)
                continue;
            int o2 = RAWF(int, r2, 0x204);
            if (o2 == 0)
                continue;
            uint32_t *rle2 = reinterpret_cast<uint32_t *>(r2 + 0x200 + o2 + 8);
            unsigned imode = item->hue;
            int pv = decomp::vcall(PORT_RET(int), Mc_player, 0x8c);
            if (pv != decomp::vcall(PORT_RET(int), self, 0x8c)) {
                imode = 0x3b2;
            } else if ((*reinterpret_cast<uint32_t *>(g_tileData + item->graphic * 0x28) & 0x40000) != 0) {
                imode |= 0x8000;
            }
            Mc_buffer.DecodeFrame(imode, reinterpret_cast<uint16_t *>(r2), rle2);   // 0x00470b50
        }
    }
    return Mc_buffer.PixelAt(px, py);   // 0x00470aa0
}
HOOK_VMETHOD(0x00471a20, CMobile, HitTest);

// 0x00471e20  size 0x3ce  plain RET
// Preview form of Mobile_ComputeBoundsCached: screen origin from the preview view offsets (0x0069dda4 /
// 0x0069dda8) instead of the scroll fields, no hidden-flag shortcut, and layers covered by other equipment
// are left out of the box. The empty box is also written when the frame count is 0.
void __fastcall Mobile_ComputeBoundsPreview(CMobile *self)
{
    int facing = static_cast<int>(static_cast<uint8_t>(self->facing));
    Mc_buffer.NearestAllowedFacing(&facing);   // 0x00470fa0
    int px = self->x;
    int py = self->y;
    int X = g_viewOffsetX + (g_scrollBase - py + px) * 22 + 1;
    int Y0 = (px + py) * 22 + (5 - RAWF(int, self, 0x190)) * 4;
    RAWF(int, self, 0xb4) = Y0;
    int Y = g_viewOffsetY + Y0;
    decomp::vcall(PORT_RET(int), self, 0x84);
    int ua, ub;
    int mirror = g_dirMirror[facing];
    uint8_t *rec = Mc_LoadAnim(Mc_AnimIndex(self->graphic, facing), &ua, &ub);   // 0x004405d0
    if (rec == nullptr) {
        self->rectX1 = X;
        self->rectY0 = Y;
        self->rectY1 = Y;
        self->rectX0 = X;
        return;
    }
    int count = RAWF(uint16_t, rec, 0x200);
    self->animFrame = static_cast<uint16_t>(count);
    if (count == 0) {
        self->rectX1 = X;
        self->rectY0 = Y;
        self->rectY1 = Y;
        self->rectX0 = X;
        return;
    }
    int off = RAWF(int, rec, 0x204);
    if (off == 0) {
        self->rectX1 = X;
        self->rectY0 = Y;
        self->rectY1 = Y;
        self->rectX0 = X;
        return;
    }
    uint8_t *fr = rec + 0x200 + off;
    int w0 = RAWF(int16_t, fr, 0);
    int w2 = RAWF(int16_t, fr, 2);
    int w4 = RAWF(int16_t, fr, 4);
    int w6 = RAWF(int16_t, fr, 6);
    if (mirror != 0)
        w0 = static_cast<int16_t>(w4 - w0);
    self->unk_b8 = 0;
    self->rectY1 = Y - w2;
    self->rectX0 = X - w0;
    self->rectY0 = self->rectY1 - w6;
    self->rectX1 = self->rectX0 + w4;
    for (int i = 1; i < 0x1a; i++) {
        CObj *item;
        if (i == 0x15)
            continue;
        item = self->equip[i];
        if (item == nullptr)
            continue;
        if (Mc_LayerIsCovered(i, &self->equip[0]) != 0)   // 0x00466b60
            continue;
        unsigned ibody = Mc_BodyOf(item);
        if (static_cast<int>(ibody) < 0x190 || static_cast<int>(ibody) >= 0x3e8)
            continue;
        uint8_t *r2 = Mc_LoadAnim(Mc_AnimIndex(static_cast<int>(ibody), facing), &ua, &ub);   // 0x004405d0
        if (r2 == nullptr)
            continue;
        int o2 = RAWF(int, r2, 0x204);
        if (o2 == 0)
            continue;
        uint8_t *f2 = r2 + 0x200 + o2;
        int a0 = RAWF(int16_t, f2, 0);
        int a2 = RAWF(int16_t, f2, 2);
        int a4 = RAWF(int16_t, f2, 4);
        int a6 = RAWF(int16_t, f2, 6);
        if (mirror != 0)
            a0 = static_cast<int16_t>(a4 - a0);
        int bottom = Y - a2;
        int left = X - a0;
        int top = bottom - a6;
        int right = left + a4;
        if (right > self->rectX1)
            self->rectX1 = right;
        if (left < self->rectX0)
            self->rectX0 = left;
        if (bottom > self->rectY1)
            self->rectY1 = bottom;
        if (top < self->rectY0)
            self->rectY0 = top;
    }
    self->rectY0 = self->rectY0 - 0x10;
    self->rectX0 = self->rectX0 - 0x10;
    self->rectY1 = self->rectY1 + 0x20;
    self->rectX1 = self->rectX1 + 0x10;
    uint8_t *g = RAWF(uint8_t *, self, 0x1c0);
    RAWF(int, self, 0x1cc) = 0;
    RAWF(int, self, 0x1c8) = 0;
    while (g != nullptr) {
        uint8_t *art = Mc_GumpArt(RAWF(uint16_t, g, 0xda) + 0x4000);   // 0x00476130
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
HOOK_FUNC(0x00471e20, Mobile_ComputeBoundsPreview);

// 0x004721f0  size 0x3dd  RET 4
// Preview form of Mobile_DrawCached: origin from the preview view offsets and the stored +0xb4, no
// hidden-flag early return, no target hue, the body is hued 0x3b2 when the +0x98 bit 7 flag is set else its own
// hue, every item is hued the same way (bit 15 added for tiledata flag 0x40000), and the equipment loop also
// runs for graphics below 0x190.
void __fastcall Mobile_DrawPreviewCached(CMobile *self, int, int *clipArg)
{
    int facing = static_cast<int>(static_cast<uint8_t>(self->facing));
    int clip[4];
    int ua, ub;

    Mc_buffer.NearestAllowedFacing(&facing);   // 0x00470fa0
    int Y = RAWF(int, self, 0xb4) + g_viewOffsetY;
    clip[0] = clipArg[0];
    clip[1] = clipArg[1];
    clip[2] = clipArg[2];
    clip[3] = clipArg[3];
    int X = g_viewOffsetX + (self->x - self->y + g_scrollBase) * 22 + 1;
    decomp::vcall(PORT_RET(int), self, 0x84);
    int mirror = g_dirMirror[facing];
    uint8_t *rec = Mc_LoadAnim(Mc_AnimIndex(self->graphic, facing), &ua, &ub);   // 0x004405d0
    if (rec == nullptr)
        return;
    int count = RAWF(uint16_t, rec, 0x200);
    self->animFrame = static_cast<uint16_t>(count);
    if (count == 0)
        return;
    int off = RAWF(int, rec, 0x204);
    if (off == 0) {
        self->rectY0 = Y;
        self->rectY1 = Y;
        self->rectX1 = X;
        self->rectX0 = X;
        return;
    }
    uint32_t *rle = reinterpret_cast<uint32_t *>(rec + 0x200 + off + 8);
    unsigned hue = self->hue;
    if ((self->flags98 & 0x80) != 0)
        hue = 0x3b2;
    Mc_buffer.clip = clip;
    Mc_buffer.unk_24010 = mirror;
    Mc_buffer.direction = facing;
    memset(Mc_buffer.pixels, 0, sizeof(Mc_buffer.pixels));
    Mc_buffer.x = X - g_scrollX - 0x20;
    Mc_buffer.y = Y - g_scrollY - 0x50;
    Mc_buffer.DecodeFrame(hue, reinterpret_cast<uint16_t *>(rec), rle);   // 0x00470b50

    int mounted = 0;
    CObj *mount = self->equip[22];
    if (mount != nullptr && mount->graphic == 0x204e)
        mounted = 1;
    for (int i = 1; i < 0x19; i++) {
        int slot = Mc_layerOrder[facing * 25 + i];
        if (slot == 0x15)
            continue;
        CObj *item = self->equip[slot];
        if (item == nullptr)
            continue;
        if (Mc_LayerIsCovered(slot, &self->equip[0]) != 0)   // 0x00466b60
            continue;
        if (mounted == 1 && Mc_IsTileType0b10(item) != 0)   // 0x0042db90
            continue;
        unsigned ibody = Mc_BodyOf(item);
        if (static_cast<int>(ibody) < 0x190 || static_cast<int>(ibody) >= 0x3e8)
            continue;
        uint8_t *r2 = Mc_LoadAnim(Mc_AnimIndex(static_cast<int>(ibody), facing), &ua, &ub);   // 0x004405d0
        if (r2 == nullptr)
            continue;
        int o2 = RAWF(int, r2, 0x204);
        if (o2 == 0)
            continue;
        uint32_t *rle2 = reinterpret_cast<uint32_t *>(r2 + 0x200 + o2 + 8);
        unsigned imode = item->hue;
        if ((self->flags98 & 0x80) != 0)
            imode = 0x3b2;
        else if ((*reinterpret_cast<uint32_t *>(g_tileData + item->graphic * 0x28) & 0x40000) != 0)
            imode |= 0x8000;
        Mc_buffer.DecodeFrame(imode, reinterpret_cast<uint16_t *>(r2), rle2);   // 0x00470b50
    }
    Mc_buffer.DrawOverlay(self);   // 0x00470870
    for (uint8_t *g = RAWF(uint8_t *, self, 0x1c0); g != nullptr; g = RAWF(uint8_t *, g, 0x100)) {
        uint8_t *art = Mc_GumpArt(RAWF(uint16_t, g, 0xda) + 0x4000);   // 0x00476130
        int w = RAWF(int16_t, art, 4);
        int h = RAWF(int16_t, art, 6);
        int rc[4];
        rc[0] = ((self->rectX0 + self->rectX1) >> 1) - (w >> 1);
        rc[2] = rc[0] + w;
        rc[1] = ((self->rectY0 + self->rectY1) >> 1) - (h >> 1);
        rc[3] = rc[1] + h;
        Mc_DrawArt(RAWF(uint16_t, g, 0xda), RAWF(uint8_t, g, 0xd0), rc, clip);   // 0x00437e60
    }
}
HOOK_FUNC(0x004721f0, Mobile_DrawPreviewCached);

// 0x004725d0  size 0x366  RET 8, returns the pixel or 0
// Preview form of Mobile_HitTestCached (same origin and hue rules as Mobile_DrawPreviewCached, no
// hidden-flag early return, the equipment loop runs for every graphic).
int __thiscall CPlayer::HitTest(int px, int py)
{
    CMobile *const self = this;
    int facing = static_cast<int>(static_cast<uint8_t>(self->facing));
    int clip[4];
    int ua, ub;

    Mc_buffer.NearestAllowedFacing(&facing);   // 0x00470fa0
    clip[2] = 0x280;
    clip[1] = 0;
    clip[0] = 0;
    clip[3] = 0x1e0;
    int X = g_viewOffsetX + (self->x - self->y + g_scrollBase) * 22 + 1;
    int Y = RAWF(int, self, 0xb4) + g_viewOffsetY;
    decomp::vcall(PORT_RET(int), self, 0x84);
    int mirror = g_dirMirror[facing];
    uint8_t *rec = Mc_LoadAnim(Mc_AnimIndex(self->graphic, facing), &ua, &ub);   // 0x004405d0
    if (rec == nullptr)
        return 0;
    int count = RAWF(uint16_t, rec, 0x200);
    self->animFrame = static_cast<uint16_t>(count);
    if (count == 0)
        return 0;
    int off = RAWF(int, rec, 0x204);
    if (off == 0) {
        self->rectY0 = Y;
        self->rectX1 = X;
        self->rectX0 = X;
        self->rectY1 = Y;
        return 0;
    }
    uint32_t *rle = reinterpret_cast<uint32_t *>(rec + 0x200 + off + 8);
    unsigned hue = self->hue;
    if ((self->flags98 & 0x80) != 0)
        hue = 0x3b2;
    Mc_buffer.clip = clip;
    Mc_buffer.unk_24010 = mirror;
    Mc_buffer.direction = facing;
    memset(Mc_buffer.pixels, 0, sizeof(Mc_buffer.pixels));
    Mc_buffer.x = X - g_scrollX - 0x20;
    Mc_buffer.y = Y - g_scrollY - 0x50;
    Mc_buffer.DecodeFrame(hue, reinterpret_cast<uint16_t *>(rec), rle);   // 0x00470b50

    int mounted = 0;
    CObj *mount = self->equip[22];
    if (mount != nullptr && mount->graphic == 0x204e)
        mounted = 1;
    for (int i = 1; i < 0x19; i++) {
        int slot = Mc_layerOrder[facing * 25 + i];
        if (slot == 0x15)
            continue;
        CObj *item = self->equip[slot];
        if (item == nullptr)
            continue;
        if (Mc_LayerIsCovered(slot, &self->equip[0]) != 0)   // 0x00466b60
            continue;
        if (mounted == 1 && Mc_IsTileType0b10(item) != 0)   // 0x0042db90
            continue;
        unsigned ibody = Mc_BodyOf(item);
        if (static_cast<int>(ibody) < 0x190 || static_cast<int>(ibody) >= 0x3e8)
            continue;
        uint8_t *r2 = Mc_LoadAnim(Mc_AnimIndex(static_cast<int>(ibody), facing), &ua, &ub);   // 0x004405d0
        if (r2 == nullptr)
            continue;
        int o2 = RAWF(int, r2, 0x204);
        if (o2 == 0)
            continue;
        uint32_t *rle2 = reinterpret_cast<uint32_t *>(r2 + 0x200 + o2 + 8);
        unsigned imode = item->hue;
        if ((self->flags98 & 0x80) != 0)
            imode = 0x3b2;
        else if ((*reinterpret_cast<uint32_t *>(g_tileData + item->graphic * 0x28) & 0x40000) != 0)
            imode |= 0x8000;
        Mc_buffer.DecodeFrame(imode, reinterpret_cast<uint16_t *>(r2), rle2);   // 0x00470b50
    }
    return Mc_buffer.PixelAt(px, py);   // 0x00470aa0
}
HOOK_VMETHOD(0x004725d0, CPlayer, HitTest);
