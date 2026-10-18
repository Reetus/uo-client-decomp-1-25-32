// CPaperdollGump: constructor, destructor, RedrawFrame, single-click handler.
// 0x00466be0, 0x00467280, 0x00467680, 0x00467dd0.
//
// The SEH frame of the original (try levels around operator new) is omitted.
#include "paperdoll_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"
#include "object_manager.h"
#include "crt.h"
#include "raw_field.h"
#include "vcall.h"
#include "gump_art_ops_ports.h"
#include "batch02_helpers.h"
#include "map_world_ops_ports.h"
#include "font_manager.h"
#include "button_gump.h"
#include "world_obj_ops.h"
#include "client_misc_ops_ports.h"
#include "object_manager_ops_ports.h"
#include "packet_builders_batch24_ports.h"
#include "client_socket.h"
#include "bark_gump.h"
#include "art_row_blits_ports.h"
#include "ui_callbacks_ports.h"
#include "small_methods_u1_ports.h"
#include "book_gump.h"
#include "vtables.h"

// ---------------------------------------------------------------------------
// Globals (single-file use).
// ---------------------------------------------------------------------------
#define g_hueRegister GLOBAL_AS(uint32_t, g_hueOverride)            // hue index read by the hued row blitters
#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)

#define g_rowPlain PTR_AT(void, PORT_ADDR(&RowBlitCopyNonZero))
#define g_rowHuedA PTR_AT(void, PORT_ADDR(&RowBlitLookupNonZero))
#define g_rowHuedB PTR_AT(void, PORT_ADDR(&RowBlitKeyedLookupNonZero))
#define g_rowHeldPlain PTR_AT(void, PORT_ADDR(&RowBlitBlendNonZero))
#define g_rowHeldHuedA PTR_AT(void, PORT_ADDR(&RowBlitLookupBlendNonZero))
#define g_rowHeldHuedB PTR_AT(void, PORT_ADDR(&RowBlitKeyedLookupBlendNonZero))
#define g_fontObj GLOBAL_PTR(void, g_fontMgr_storage)

// Tag strings the original passes to the tagged allocator.
#define g_paperdollVtable reinterpret_cast<uintptr_t>(vtbl_CPaperdollGump)

// ---------------------------------------------------------------------------
// Original code not ported yet.
// ---------------------------------------------------------------------------
static void ArtSize(int artId, int *w, int *h) { GumpArtSizeImpl(artId, w, h); }
static void DrawArt(void *renderer, int artId, int x, int y, const void *pixels, int w, int h)
{
    PORT_FUNC7(&DrawGumpArt, void, __fastcall, void *, int, int, int, const void *, int, int)(renderer, artId, x, y, pixels, w, h);
}
static void *AllocTagged(int size, const char *tag) { return PORT_THISCALL2(&Map_Alloc, void *, g_frameDataAllocator, int, const char *)(size, tag); }
static void FreeTagged(void *p, const char *tag) { PORT_THISCALL2(&Map_Free, void, g_frameDataAllocator, void *, const char *)(p, tag); }
static void TextMeasure(int font, const char *str, int *a, int *b, int *c)
{
    reinterpret_cast<CFontManager *>(g_fontObj)->MeasureText(font, (char *)(str), a, b, c);
}
static void TextDrawWrapped(int x, int y, int font, int hue, const char *str, int wrapWidth, const void *dest, int destW,
                            const int *clip, int a10, int a11)
{
    reinterpret_cast<CFontManager *>(g_fontObj)->DrawTextWrapped(x, y, font, hue, (char *)(str), wrapWidth, (uint16_t *)(dest), destW, clip, a10, a11);
}
static void AddChild(void *child, CGump *parent) { CGump_AddChild(static_cast<CGump *>(child), parent, 1); }
static int HitLayer(CGump *g, int x, int y, int flag) { return reinterpret_cast<CPaperdollGump *>(g)->HitTestLayer(x, y, flag); }
static void Obj_NotifyRedrawn(void *obj) { reinterpret_cast<CWorldObj *>(obj)->Invalidate(); }
static int IsGameMaster() { return Misc_Return0(); }
static void Mgr_BeginUse(void *item) { PORT_THISCALL1(&Object_ShowName, void, &g_objMgr, void *)(item); }
static void Pkt_BuildUse(uint8_t *pkt, uint32_t serial) { PORT_THISCALL2(&Build_Packet_0x09, void, g_packetBuilder, uint8_t *, uint32_t)(pkt, serial); }
static void Net_Send(const uint8_t *pkt) { reinterpret_cast<CClientSocket *>(g_pktSender)->SendPacket(pkt); }

static inline int32_t ptr_to_i32(const void *p) { return static_cast<int32_t>(reinterpret_cast<uintptr_t>(p)); }
static inline uint16_t *bitmap_at(const CGump *g, int slot)
{
    return reinterpret_cast<uint16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(g->frameData[slot])));
}

// ---------------------------------------------------------------------------
// 0x00466be0  size 0x663
// Constructor.
// ---------------------------------------------------------------------------
CPaperdollGump::CPaperdollGump(int px, int py, int /*parent*/, void *obj, int /*a5*/, int /*a6*/, int /*a7*/,
                               char *nameText, uint8_t warFlag)
{
    name = reinterpret_cast<char *>(GADDR(g_paperdollName));
    dirty = 0;
    unk_ec = 0;
    unk_e8 = 0;
    unk_e4 = 0;
    unk_e0 = 0;
    unk_f4 = 0;
    unk_f0 = 0;

    if (g_deskQueueActive == 0) {
        x = px;
        y = py;
    } else {
        int count = g_deskQueueCount;
        int i = 0;
        if (count > 0) {
            // entries are 0x1c bytes at 0x00648930
            for (; i < count; i++) {
                if (g_deskQueue[i * 0x1c] == 0) {
                    x = RAWF(int32_t, g_deskQueue, i * 0x1c + 0x14);
                    y = RAWF(int32_t, g_deskQueue, i * 0x1c + 0x18);
                    unk_60 = RAWF(int32_t, g_deskQueue, i * 0x1c + 0x08);
                    count = g_deskQueueCount;
                    break;
                }
            }
        }
        if (i < count) {
            int j = i;
            if (i < count - 1) {
                uint8_t *dst = g_deskQueue + i * 0x1c;
                do {
                    memcpy(dst, dst + 0x1c, 7 * 4);
                    j++;
                    count = g_deskQueueCount;
                    dst += 0x1c;
                } while (j < count - 1);
            }
            count--;
            g_deskQueueCount = count;
            if (count == 0)
                g_deskQueueActive = 0;
        }
    }

    owner = static_cast<CObj *>(obj);
    if (obj != nullptr)
        RAWF(void *, obj, 0xac) = this;
    if (obj == static_cast<void *>(g_player))
        isPlayer = 1;
    else
        isPlayer = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    unk_50 = 0x7d0;
    ArtSize(0x7d0, &width, &height);
    ArtSize(0x7ee, &unk_24, &unk_28);
    frameIndex = 0;
    frameCount = 2;

    int32_t *fd = static_cast<int32_t *>(AllocTagged(0x10, g_004ae360));
    frameData = fd;
    fd[0] = 0;
    fd[1] = 0;
    fd[1] = ptr_to_i32(AllocTagged(height * width * 2 + 4, s_frameTag));
    memset(reinterpret_cast<char *>(static_cast<uintptr_t>(static_cast<uint32_t>(fd[1]))) + 4, 0, height * width * 2);
    *reinterpret_cast<int16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(fd[1]))) = static_cast<int16_t>(width);
    *reinterpret_cast<int16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(fd[1])) + 2) = static_cast<int16_t>(height);

    fd[3] = ptr_to_i32(AllocTagged(unk_28 * unk_24 * 2 + 4, g_004ae33c));
    memset(reinterpret_cast<char *>(static_cast<uintptr_t>(static_cast<uint32_t>(fd[3]))) + 4, 0, unk_28 * unk_24 * 2);
    *reinterpret_cast<int16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(fd[3]))) = static_cast<int16_t>(unk_24);
    *reinterpret_cast<int16_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(fd[3])) + 2) = static_cast<int16_t>(unk_28);

    nameBMP = nullptr;
    strcpy(title, nameText);
    DrawArt(g_rowPlain, 0x7ee, 0, 0, reinterpret_cast<char *>(static_cast<uintptr_t>(static_cast<uint32_t>(fd[3]))) + 4, unk_24, unk_28);
    heldLayer = -1;
    RedrawFrame();

    // button 1 (not stored)
    void *btn = new CButtonGump(0xb9, 0xe9, (int)(this), 0, 0, PORT_ADDR(&UiCb_OpenStatusGump), 0x7eb, 0x7ec, 0);
    RAWF(int32_t, btn, 0xa4) = 0x7ed;
    AddChild(btn, this);

    if (isPlayer != 0) {
        if (warFlag == 0) {
            btn = new CButtonGump(0xb9, 0xcd, (int)(this), 0, 0, PORT_ADDR(&CPaperdollGump::OnWarButton), 0x7e5, 0x7e6, 0);
            warButton = static_cast<CGump *>(btn);
            RAWF(int32_t, btn, 0xa4) = 0x7e7;
        } else {
            btn = new CButtonGump(0xb9, 0xcd, (int)(this), 0, 0, PORT_ADDR(&CPaperdollGump::OnWarButton), 0x7e8, 0x7e9, 0);
            warButton = static_cast<CGump *>(btn);
            RAWF(int32_t, btn, 0xa4) = 0x7ea;
        }
        AddChild(warButton, this);

        btn = new CButtonGump(0xb9, 0x7c, (int)(this), 0, 0, PORT_ADDR(&U1_OpenGumpType7), 0x7dc, 0x7dd, 0);
        RAWF(int32_t, btn, 0xa4) = 0x7de;
        AddChild(btn, this);

        btn = new CButtonGump(0xb9, 0x97, (int)(this), 0, 0, PORT_ADDR(&UiCb_RequestPlayerStatus), 0x7df, 0x7e0, 0);
        RAWF(int32_t, btn, 0xa4) = 0x7e1;
        AddChild(btn, this);

        btn = new CButtonGump(0xb9, 0xb3, (int)(this), 0, 0, PORT_ADDR(&UiCb_ToggleFightingGump), 0x7e2, 0x7e3, 0);
        RAWF(int32_t, btn, 0xb0) = 1;
        RAWF(int32_t, btn, 0xa4) = 0x7e4;
        AddChild(btn, this);

        btn = new CButtonGump(0xb9, 0x47, (int)(this), 0, 0, PORT_ADDR(&U1_OpenGumpType1388), 0x7d6, 0x7d7, 0);
        RAWF(int32_t, btn, 0xa4) = 0x7d8;
        AddChild(btn, this);

        btn = new CButtonGump(0xb9, 0x2c, (int)(this), 0, 0, PORT_ADDR(&U1_HelpButton), 0x7ef, 0x7f0, 0);
        RAWF(int32_t, btn, 0xa4) = 0x7f1;
        AddChild(btn, this);

        btn = new CButtonGump(0xb9, 0x62, (int)(this), 0, 0, PORT_ADDR(&OpenQuitDialog), 0x7d9, 0x7da, 0);
        RAWF(int32_t, btn, 0xa4) = 0x7db;
        AddChild(btn, this);

        btn = new CButtonGump(0xdf, 0x102, 0xf5, 0x118, this, 0, 0, PORT_ADDR(&CBookGump::Iconify), 0);
        AddChild(btn, this);
    }
}
HOOK_CTOR(0x00466be0, CPaperdollGump, (int, int, int, void *, int, int, int, char *, uint8_t));

// ---------------------------------------------------------------------------
// 0x00467280  size 0x117
// Destructor body: delete the gumps whose owner (+0xb4) is this gump, then free
// the bitmaps and run the CGump base destructor.
// ---------------------------------------------------------------------------
CPaperdollGump::~CPaperdollGump()
{
    CGump *cur = g_gumpListTail;
    CGump *saved = g_gumpDeleteCursor;
    if (cur != nullptr) {
        do {
            g_gumpDeleteCursor = cur->prev;                      // +0x3c
            if (decomp::vcall(PORT_RET(int), cur, 0x84) != 0) {            // slot 0x84
                if (RAWF(CGump *, cur, 0xb4) == this) {
                    if (cur != nullptr)
                        decomp::vdelete(cur);
                }
            }
            cur = g_gumpDeleteCursor;
        } while (cur != nullptr);
    }
    g_gumpDeleteCursor = saved;

    if (nameBMP != nullptr) {
        FreeTagged(nameBMP, s_nameBmpFree);
        nameBMP = nullptr;
    }
    if (frameData[1] != 0) {
        FreeTagged(bitmap_at(this, 1), s_frameFree);
        frameData[1] = 0;
    }
    if (frameData[3] != 0) {
        FreeTagged(bitmap_at(this, 3), g_004ae3a4);
        frameData[3] = 0;
    }
    if (frameData != nullptr) {
        FreeTagged(frameData, g_004ae390);
        frameData = nullptr;
    }
}
HOOK_DTOR(0x00467280, CPaperdollGump);

// ---------------------------------------------------------------------------
// 0x00467680  size 0x55b
// Redraw the paperdoll frame: backdrop, name text, body, equipment layers.
// ---------------------------------------------------------------------------
void __thiscall CPaperdollGump::RedrawFrame()
{
    uint16_t *bmp;
    int rect[4];
    int measA;
    int measB;
    int measC;
    int textId;
    int drawBody;
    int bodyBase;
    int human;
    int hue;
    int sizeA;
    int sizeB;
    int sizeC;

    g_paperdollRedrawFlag = 1;
    if (owner == g_player) {
        bmp = bitmap_at(this, 1);
        DrawArt(g_rowPlain, unk_50, 0, 0, reinterpret_cast<char *>(bmp) + 4, bmp[0], bmp[1]);
    } else {
        bmp = bitmap_at(this, 1);
        DrawArt(g_rowPlain, unk_50 + 1, 0, 0, reinterpret_cast<char *>(bmp) + 4, bmp[0], bmp[1]);
    }

    rect[0] = 0;
    rect[1] = 0;
    rect[2] = width;
    rect[3] = height - 0x16;
    TextMeasure(1, title, &measA, &measB, &measC);
    textId = 0x108;
    if (owner != g_player)
        textId = 0x107;
    TextDrawWrapped(0x27, textId, 1, 0x386, title, 0xba, reinterpret_cast<char *>(bitmap_at(this, 1)) + 4, width, rect, 0, 0xd);

    if (owner == nullptr)
        return;
    if (isPlayer != 0) {
        bmp = bitmap_at(this, 1);
        DrawArt(g_rowPlain, 0x7d2, 0x1b, 0xc4, reinterpret_cast<char *>(bmp) + 4, bmp[0], bmp[1]);
    }

    void *obj = owner;
    human = 0;
    hue = -1;
    drawBody = 1;
    {
        uint16_t body = RAWF(uint16_t, obj, 0x38);
        if (body == 0x190 || body == 0x192) {
            bodyBase = 0x190;
            human = 1;
        } else if (body == 0x191 || body == 0x193) {
            bodyBase = 0x191;
            human = 1;
        } else if (body == 0x3db) {
            bodyBase = 0x190;
            human = 1;
            hue = 0x3ea;
        } else {
            drawBody = 0;
            bodyBase = 0x190;
        }
    }
    if (hue == -1)
        hue = RAWF(uint16_t, obj, 0x3e);
    if (drawBody == 1) {
        if (RAWF(uint16_t, obj, 0x3e) == 0) {
            bmp = bitmap_at(this, 1);
            DrawArt(g_rowPlain, (bodyBase & 0xffff) - 0x184, 8, 0x13, reinterpret_cast<char *>(bmp) + 4, bmp[0], bmp[1]);
        } else if (human == 1) {
            g_hueRegister = static_cast<uint32_t>(hue - 1) & 0x7fff;
            bmp = bitmap_at(this, 1);
            DrawArt(g_rowHuedB, (bodyBase & 0xffff) - 0x184, 8, 0x13, reinterpret_cast<char *>(bmp) + 4, bmp[0], bmp[1]);
        } else {
            g_hueRegister = static_cast<uint32_t>(hue - 1);
            bmp = bitmap_at(this, 1);
            DrawArt(g_rowHuedA, (bodyBase & 0xffff) - 0x184, 8, 0x13, reinterpret_cast<char *>(bmp) + 4, bmp[0], bmp[1]);
        }
    }

    if (RAWF(uint16_t, owner, 0x38) == 0x3db) {
        ArtSize(0xee3b, &sizeA, &sizeC);
        uint16_t h2 = RAWF(uint16_t, owner, 0x3e);
        if (h2 > 0) {
            g_hueRegister = static_cast<uint32_t>(h2 - 1) & 0x7fff;
            bmp = bitmap_at(this, 1);
            DrawArt(g_rowHuedB, 0xc72b, 8, 0x13, reinterpret_cast<char *>(bmp) + 4, bmp[0], bmp[1]);
        } else {
            bmp = bitmap_at(this, 1);
            DrawArt(g_rowPlain, 0xc72b, 8, 0x13, reinterpret_cast<char *>(bmp) + 4, bmp[0], bmp[1]);
        }
    }

    obj = owner;
    for (int i = 0; i < 0x19; i++) {
        int lay = g_equipLayerOrder[i];
        if (drawBody == 0 && lay != 0x15)
            continue;
        void *item = RAWF(void *, obj, 0xc4 + lay * 4);
        int skipItem = 0;
        if (item == nullptr) {
            skipItem = 1;
        } else if (static_cast<unsigned>(lay - 3) <= 0x10) {
            switch (lay) {
            case 3:
            case 4:
                if (RAWF(int32_t, obj, 0x124) != 0)
                    skipItem = 1;
                break;
            case 0xb:
                if (RAWF(void *, obj, 0x11c) != nullptr && RAWF(uint16_t, RAWF(void *, obj, 0x11c), 0x38) == 0x204e)
                    skipItem = 1;
                break;
            case 0xd:
            case 0x13:
                if (RAWF(int32_t, obj, 0x11c) != 0)
                    skipItem = 1;
                break;
            default:
                break;
            }
        }

        if (skipItem == 0) {
            int artId = RAWF(uint16_t, g_objMgr.tileItems, RAWF(uint16_t, item, 0x38) * 0x28 + 0xc) + 0xc350;
            int artIdFemale = artId + 0x2710;
            ArtSize(artIdFemale, &sizeA, &sizeB);
            if (bodyBase == 0x191 && sizeA != 0)
                artId = artIdFemale;
            void *eq = RAWF(void *, obj, 0xc4 + lay * 4);
            if (RAWF(uint16_t, eq, 0x3e) > 0) {
                g_hueRegister = RAWF(uint16_t, eq, 0x3e) - 1u;
                uint32_t flags = decomp::vcall(PORT_RET(uint32_t), eq, 0x40);   // slot 0x40: tiledata flags
                bmp = bitmap_at(this, 1);
                if ((flags & 0x40000) != 0)
                    DrawArt(g_rowHuedB, artId, 8, 0x13, reinterpret_cast<char *>(bmp) + 4, bmp[0], bmp[1]);
                else
                    DrawArt(g_rowHuedA, artId, 8, 0x13, reinterpret_cast<char *>(bmp) + 4, bmp[0], bmp[1]);
            } else {
                bmp = bitmap_at(this, 1);
                DrawArt(g_rowPlain, artId, 8, 0x13, reinterpret_cast<char *>(bmp) + 4, bmp[0], bmp[1]);
            }
        } else if (heldLayer == lay) {
            // the item being dragged from this lay is drawn with the held-item blitters
            if (g_objMgr.heldHue != 0) {
                g_hueRegister = g_objMgr.heldHue - 1u;
                uint8_t *tile = g_objMgr.tileItems + g_objMgr.heldGraphic * 0x28;
                bmp = bitmap_at(this, 1);
                int heldArt = RAWF(uint16_t, tile, 0xc) + 0xc350;
                if ((RAWF(uint32_t, tile, 0) & 0x40000) != 0)
                    DrawArt(g_rowHeldHuedB, heldArt, 8, 0x13, reinterpret_cast<char *>(bmp) + 4, bmp[0], bmp[1]);
                else
                    DrawArt(g_rowHeldHuedA, heldArt, 8, 0x13, reinterpret_cast<char *>(bmp) + 4, bmp[0], bmp[1]);
            } else {
                bmp = bitmap_at(this, 1);
                uint8_t *tile = g_objMgr.tileItems + g_objMgr.heldGraphic * 0x28;
                DrawArt(g_rowHeldPlain, RAWF(uint16_t, tile, 0xc) + 0xc350, 8, 0x13, reinterpret_cast<char *>(bmp) + 4, bmp[0], bmp[1]);
            }
        }
    }
    Obj_NotifyRedrawn(owner);
}
HOOK_METHOD(0x00467680, CPaperdollGump, RedrawFrame);

// ---------------------------------------------------------------------------
// 0x00467dd0  size 0x153
// Single-click handler. RET 8: (x, y).
// ---------------------------------------------------------------------------
void __thiscall CPaperdollGump::OnClick(int mx, int my)
{
    uint8_t pkt[16];

    int slot = HitLayer(this, mx, my, 1);
    if (slot == -1) {
        int slotW;
        int slotH;
        ArtSize(0x7d2, &slotW, &slotH);
        if (isPlayer != 0 && mx >= 0x1b && my >= 0xc4 && mx <= slotW + 0x1b && my <= slotH + 0xc4) {
            new CBarkGump((CObj *)(RAWF(uint32_t, g_player, 0x118)), (char *)(s_guideName), (uint16_t)(0x3b2), (uint16_t)(3), this);
        }
    } else {
        void *item = RAWF(void *, owner, 0xc4 + slot * 4);
        if (item != nullptr) {
            if (g_targetCallbackActive != 0 && g_targetCallback != 0 && g_modalActive == 0) {
                decomp::thiscall_at_void(g_targetCallback, item);
                return;
            }
            if (IsGameMaster() != 0)
                Mgr_BeginUse(item);
            Pkt_BuildUse(pkt, RAWF(uint32_t, item, 0x78));
            Net_Send(pkt);
        }
    }
}
HOOK_METHOD(0x00467dd0, CPaperdollGump, OnClick);
