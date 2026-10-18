// CGenericGump and GumpNode. The SEH frame of the original (try level 1 around the
// new(0x48)) is omitted.
#include "generic_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch02_helpers.h"
#include "globals.h"
#include "crt.h"
#include "text_entry_ops_ports.h"
#include "packet_buffer_ops_ports.h"
#include "client_socket.h"
#include "unicode_speech_ops_ports.h"
#include "art_record_ops_ports.h"
#include "art_row_blits_ports.h"
#include "gump_frame.h"
#include "unicode_font_ports.h"
#include "bark_gump_ui_ports.h"
#include "art_row_blits_hue_ports.h"
#include "vtables.h"

// 0x00442c60: split the first word of block into cmd and return the rest. ECX = block, EDX = cmd.
#define SplitFirstWord(a1, a2) ((char *)CopyFirstWordLower((const char *)(a1), a2))

// Widget adders on CGenericGump (ECX = gump), all arguments ints unless noted.
#define Gump_AddGumpPic(self, a1, a2, a3, a4, a5) (reinterpret_cast<CGenericGump *>(self)->AddGumpPic(a1, a2, a3, a4, a5))             // x, y, id, page, group
#define Gump_AddTilePic(self, a1, a2, a3, a4, a5) (reinterpret_cast<CGenericGump *>(self)->AddTilePic(a1, a2, a3, a4, a5))
#define Gump_AddText(self, a1, a2, a3, a4, a5, a6) (reinterpret_cast<CGenericGump *>(self)->AddText(a1, a2, a3, (const uint16_t *)(a4), a5, a6))        // x, y, hue, text, page, group
#define Gump_AddTextEntry(self, a1, a2, a3, a4, a5, a6, a7, a8, a9) (reinterpret_cast<CGenericGump *>(self)->AddTextEntry(a1, a2, a3, a4, a5, (const uint16_t *)(a6), a7, a8, a9))
#define Gump_AddRadio(self, a1, a2, a3, a4, a5, a6, a7, a8) (reinterpret_cast<CGenericGump *>(self)->AddRadio(a1, a2, a3, a4, a5, a6, a7, a8))
#define Gump_AddButton(self, a1, a2, a3, a4, a5, a6, a7, a8, a9) (reinterpret_cast<CGenericGump *>(self)->AddButton(a1, a2, a3, a4, a5, a6, a7, a8, a9))
#define Gump_AddCheckBox(self, a1, a2, a3, a4, a5, a6, a7, a8) (reinterpret_cast<CGenericGump *>(self)->AddCheckbox(a1, a2, a3, a4, a5, a6, a7, a8))
#define Gump_AddResizePic(self, a1, a2, a3, a4, a5, a6, a7) (reinterpret_cast<CGenericGump *>(self)->AddResizePic(a1, a2, a3, a4, a5, a6, a7))
#define Gump_FinishLayout(self) (reinterpret_cast<CGenericGump *>(self)->RebuildFrame())                                    // draw the whole tree into the frame
#define Gump_GrowFrame(self, a1, a2) (reinterpret_cast<CGenericGump *>(self)->GrowFrame(a1, a2))                             // (right, bottom)
// GumpNode: constructor (parent, 0, page, group, ten zeros) and recursive group lookup.
#define Node_FindGroup(self, a1) (reinterpret_cast<GumpNode *>(self)->FindDescendantByGroup(a1))

// 0x004574e0 / 0x004575f0 / 0x00457570: packet writers, ECX = packet, EDX = value.
#define Pkt_Init(a1, a2) (PacketBuf_Begin((uint8_t *)(a1), (uint8_t)(a2)))
#define Pkt_Dword(a1, a2) (PacketBuf_WriteU32((uint8_t *)(a1), (uint32_t)(a2)))
#define Pkt_Word(a1, a2) (PacketBuf_WriteU16((uint8_t *)(a1), (uint16_t)(a2)))
#define Net_Send(self, a1) (reinterpret_cast<CClientSocket *>(self)->SendPacket((const uint8_t *)(a1)))
// Per-node serialisers (ECX = node).
#define Node_CountSwitches(self) (reinterpret_cast<GumpNode *>(self)->CountSwitches())
#define Node_WriteSwitches(self, a1) (reinterpret_cast<GumpNode *>(self)->WriteSwitches(a1))
#define Node_CountEntries(self) (reinterpret_cast<GumpNode *>(self)->CountEntries())
#define Node_WriteEntries(self, a1) (reinterpret_cast<GumpNode *>(self)->WriteEntries(a1))
#define Str_Length(o) PORT_THISCALL0(&WcsLenThunk, uint16_t, (o))()                                       // length of a wide string (ECX)

// Text / bitmap helpers for Render.
#define g_tileArtTable GLOBAL_PTR(void, g_artCache_storage)
#define TileArt_Get(o, a1) PORT_THISCALL1(&ArtRecord_Get, void *, (o), int)(a1)
#define Tile_SetTarget(a1, a2) (SetBlitSurface((uint16_t *)(a1), a2))                  // ECX = buffer, EDX = width
#define Tile_Draw PORT_FUNC4(&RleArtWalkUnclipped, void, __fastcall, void *, void *, int, int)          // ECX = blitter 0x4761e0, EDX = tile
#define DrawFrameArt(a1, a2, a3) (PaintGumpFrame(a1, (uint16_t *)(a2), a3))
#define WStr_Copy(a1, a2) (WcsCpy((uint16_t *)(a1), (const uint16_t *)(a2)))                    // ECX = dst, EDX = src
#define WStr_Append(a1, a2) (WcsCat((uint16_t *)(a1), (const uint16_t *)(a2)))
#define Text_Render(a1, a2, a3, a4, a5, a6, a7) (DrawUnicodeString((uint16_t *)(a1), a2, (const uint16_t *)(a3), a4, a5, (uint16_t)(a6), (const int *)(a7)))
#define Text_Finish(a1, a2, a3, a4) (CBark_BitmapOutline((uint16_t *)(a1), a2, a3, (uint32_t)(a4)))
#define g_caretString GLOBAL_PTR(const void, g_caretString_storage)

static inline uint16_t *frame_pixels(CGenericGump *g)
{
    return reinterpret_cast<uint16_t *>(static_cast<char *>(i32_ptr(g->frameData[1])) + 4);
}

// 0x004344b0  size 0x874
CGenericGump::CGenericGump(int px, int py, char *layout, char **textTable, int owner_, int serial)
{
    char cmd[1024];
    char block[1024];
    int page, group;
    int v10 = 0, v14 = 0, v18 = 0, v1c = 0, v20 = 0, v24 = 0, v30 = 0, v34 = 0, v38 = 0, v3c = 0;
    int v40 = 0, v44 = 0, v48 = 0, v4c = 0, v50 = 0, v54 = 0;
    int p8 = 0, pc = 0;
    char *cur, *p, *args, *q;
    int oldGroup;
    GumpNode **slot;
    GumpNode *parentNode;
    GumpNode *node;
    int32_t *fd;
    int al;

    ownerSerial = owner_;
    unk_98 = 0;
    unk_9c = 0;
    unk_a0 = 0;
    groups = nullptr;
    unk_bc = 0;
    unk_c0 = 0;
    page = 0;
    group = 0;
    gumpId = serial;
    unk_94 = 1;
    canMove = 1;
    canDispose = 1;
    canClose = 1;
    name = const_cast<char *>("generic gump");
    x = px;
    y = py;

    cur = layout;
    if (*cur != 0) {
        do {
            p = cur;
            // find the open brace
            al = *p;
            if (al != 0) {
                while (al != 0x7b) {
                    al = p[1];
                    p++;
                    if (al == 0)
                        break;
                }
                cur = p;
            }
            if (*p != 0) {
                p++;
                cur = p;
            }
            // skip whitespace
            al = *p;
            if (al != 0) {
                while (isspace(static_cast<int>(static_cast<char>(al))) != 0) {
                    al = p[1];
                    p++;
                    if (al == 0)
                        break;
                }
                cur = p;
            }
            // copy up to the close brace
            al = *p;
            q = block;
            if (al != 0) {
                while (al != 0x7d) {
                    *q = static_cast<char>(al);
                    al = p[1];
                    q++;
                    p++;
                    if (al == 0)
                        break;
                }
                cur = p;
            }
            if (*p != 0) {
                p++;
                cur = p;
            }
            // trim trailing whitespace
            if (q > block) {
                do {
                    if (isspace(static_cast<int>(q[-1])) == 0)
                        break;
                    q--;
                } while (q > block);
            }
            *q = 0;
            args = SplitFirstWord(block, cmd);

            if (strcmp(cmd, "page") == 0) {
                page = atoi(args);
            } else if (strcmp(cmd, "nodispose") == 0) {
                canDispose = 0;
            } else if (strcmp(cmd, "nomove") == 0) {
                canMove = 0;
            } else if (strcmp(cmd, "noclose") == 0) {
                canClose = 0;
            } else if (strcmp(cmd, "group") == 0) {
                oldGroup = group;
                group = atoi(args);
                parentNode = nullptr;
                slot = nullptr;
                if (oldGroup == 0) {
                    slot = &groups;
                } else {
                    parentNode = groups;
                    if (parentNode != nullptr) {
                        for (;;) {
                            if (parentNode->group == oldGroup)
                                break;
                            GumpNode *s = parentNode->children;
                            GumpNode *r = nullptr;
                            if (s != nullptr) {
                                for (;;) {
                                    if (s->group == oldGroup) {
                                        r = s;
                                        break;
                                    }
                                    if (s->children != nullptr) {
                                        r = Node_FindGroup(s, oldGroup);
                                        if (r != nullptr)
                                            break;
                                    }
                                    s = s->next;
                                    if (s == nullptr) {
                                        r = nullptr;
                                        break;
                                    }
                                }
                            }
                            if (r != nullptr) {
                                parentNode = r;
                                break;
                            }
                            parentNode = parentNode->next;
                            if (parentNode == nullptr)
                                break;
                        }
                    }
                    if (parentNode != nullptr)
                        slot = &parentNode->children;
                }
                if (slot != nullptr) {
                    // walk to the tail of the sibling list
                    if (*slot != nullptr) {
                        GumpNode *e = *slot;
                        do {
                            slot = &e->next;
                            e = e->next;
                        } while (e != nullptr);
                    }
                    node = new GumpNode(parentNode, 0, page, group, 0, 0, 0, 0, 0, 0, nullptr, 0, 0, 0);
                    *slot = node;
                }
            } else if (strcmp(cmd, "gumppic") == 0) {
                sscanf(args, "%d %d %d", &v10, &v14, &v18);
                Gump_AddGumpPic(this, v10, v14, v18, page, group);
            } else if (strcmp(cmd, "tilepic") == 0) {
                sscanf(args, "%d %d %d", &v10, &v14, &v18);
                Gump_AddTilePic(this, v10, v14, v18, page, group);
            } else if (strcmp(cmd, "text") == 0) {
                sscanf(args, "%d %d %d %d", &v10, &v14, &pc, &p8);
                Gump_AddText(this, v10, v14, pc, textTable[p8], page, group);
            } else if (strcmp(cmd, "textentry") == 0) {
                sscanf(args, "%d %d %d %d %d %d %d", &v10, &v14, &v20, &v24, &v30, &v3c, &v4c);
                Gump_AddTextEntry(this, v10, v14, v20, v24, v30, textTable[v4c], v3c, page, group);
            } else if (strcmp(cmd, "radio") == 0) {
                sscanf(args, "%d %d %d %d %d %d", &v10, &v14, &v18, &v1c, &v44, &v54);
                Gump_AddRadio(this, v10, v14, v18, v1c, v44, v54, page, group);
            } else if (strcmp(cmd, "button") == 0) {
                sscanf(args, "%d %d %d %d %d %d %d", &v10, &v14, &v18, &v1c, &v38, &v34, &v50);
                Gump_AddButton(this, v10, v14, v18, v1c, v38, v34, v50, page, group);
            } else if (strcmp(cmd, "checkbox") == 0) {
                sscanf(args, "%d %d %d %d %d %d", &v10, &v14, &v18, &v1c, &v48, &v40);
                Gump_AddCheckBox(this, v10, v14, v18, v1c, v48, v40, page, group);
            } else if (strcmp(cmd, "resizepic") == 0) {
                sscanf(args, "%d %d %d %d %d", &v10, &v14, &v18, &v20, &v24);
                Gump_AddResizePic(this, v10, v14, v18, v20, v24, page, group);
            }
        } while (*cur != 0);
    }

    // finish
    frameCount = 1;
    height = 1;
    rowStride = 1;
    width = 1;
    fd = FrameData_Alloc(g_frameDataAllocator, 8, "framedata for GenericGump");
    frameData = fd;
    memset(fd, 0, frameCount * 8);
    frameData[1] = ptr_i32(FrameData_Alloc(g_frameDataAllocator, width * height * 2 + 4, "generic frame"));
    memset(static_cast<char *>(i32_ptr(frameData[1])) + 4, 0, width * height * 2);
    *static_cast<uint16_t *>(i32_ptr(frameData[1])) = static_cast<uint16_t>(width);
    *(static_cast<uint16_t *>(i32_ptr(frameData[1])) + 1) = static_cast<uint16_t>(height);
    Gump_FinishLayout(this);
    CGump_AddChild(this, nullptr, 1);
}
HOOK_CTOR(0x004344b0, CGenericGump, (int, int, char *, char **, int, int));

// 0x00434e50  size 0x20c
void __thiscall CGenericGump::SendResponse(int buttonId)
{
    char packet[0x1e58];
    if (ownerSerial != 0 && g_pktSender != nullptr) {
        Pkt_Init(packet, 0xb1);
        Pkt_Dword(packet, ownerSerial);      // serial
        Pkt_Dword(packet, gumpId);           // gump id
        Pkt_Dword(packet, buttonId);

        // pass 1: count switches
        int total = 0;
        for (GumpNode *g = groups; g != nullptr; g = g->next) {
            int n = 0;
            for (GumpNode *c = g->children; c != nullptr; c = c->next)
                n += Node_CountSwitches(c);
            if (g->type == 6 || g->type == 7) {
                if (g->flag != 0)
                    n++;
            }
            total += n;
        }
        Pkt_Dword(packet, total);

        // pass 2: write switches
        for (GumpNode *g = groups; g != nullptr; g = g->next) {
            for (GumpNode *c = g->children; c != nullptr; c = c->next)
                Node_WriteSwitches(c, packet);
            if (g->type == 6 || g->type == 7) {
                if (g->flag != 0)
                    Pkt_Dword(packet, g->entryId);
            }
        }

        // pass 3: count text entries
        total = 0;
        for (GumpNode *g = groups; g != nullptr; g = g->next) {
            int n = 0;
            for (GumpNode *c = g->children; c != nullptr; c = c->next)
                n += Node_CountEntries(c);
            if (g->type == 8)
                n++;
            total += n;
        }
        Pkt_Dword(packet, total);

        // pass 4: write text entries
        for (GumpNode *g = groups; g != nullptr; g = g->next) {
            for (GumpNode *c = g->children; c != nullptr; c = c->next)
                Node_WriteEntries(c, packet);
            if (g->type == 8) {
                Pkt_Word(packet, static_cast<uint16_t>(g->entryId));       // entry id
                uint16_t len = Str_Length(g->text);                        // wide string length
                uint16_t cnt = (len > 0xef) ? 0xef : len;
                Pkt_Word(packet, cnt);
                for (int i = 0; i < (cnt & 0xffff); i++)
                    Pkt_Word(packet, static_cast<const uint16_t *>(g->text)[i]);
            }
        }
        Net_Send(g_pktSender, packet);
    }
}
HOOK_METHOD(0x00434e50, CGenericGump, SendResponse);

// Copy a w x h block of 16-bit pixels onto the frame, skipping zero (colour-key) pixels,
// optionally converting 555 to 565 on the way.
static void keyed_blit(uint16_t *dst, const uint16_t *src, int w, int h, int frameW, bool convert565)
{
    for (int yy = 0; yy < h; yy++) {
        for (int xx = 0; xx < w; xx++) {
            unsigned short px = *src;
            if (px != 0)
                *dst = convert565 ? static_cast<unsigned short>(((px & 0xffe0) << 1) | (px & 0x1f)) : px;
            dst++;
            src++;
        }
        dst += frameW - w;
    }
}

// 0x00435eb0  size 0x470
void __thiscall GumpNode::Render(CGenericGump *gump)
{
    uint8_t textBuf[0x800];
    int rect[4];

    if (type != 0) {
        int right = x + w;
        if (gump->width < right || gump->height < y + h)
            Gump_GrowFrame(gump, right, y + h);          // grow frame

        switch (type) {
        case 1: case 5: case 6: case 7: {
            int artId = flag ? art1 : art0;
            DrawGumpArt(ROWBLIT_COPY, artId, x, y, frame_pixels(gump), gump->width, gump->height);
            break;
        }
        case 2: {
            void *tile = TileArt_Get(g_tileArtTable, art0 + 0x4000);
            if (tile == nullptr)
                break;
            uint16_t *tmp = static_cast<uint16_t *>(::operator new(static_cast<size_t>(h * w * 2)));
            memset(tmp, 0, static_cast<size_t>(h * w * 2));
            uint16_t *src = tmp;
            Tile_SetTarget(tmp, w);
            Tile_Draw(reinterpret_cast<void *>(PORT_ADDR(&RowBlitCopyAll)), tile, 0, 0);
            uint16_t *dst = frame_pixels(gump) + y * gump->width + x;
            keyed_blit(dst, src, w, h, gump->width, g_is565 != 0);
            operator delete(tmp);
            break;
        }
        case 3: {
            int16_t *hdr = static_cast<int16_t *>(::operator new(static_cast<size_t>(h * w * 2 + 4)));
            hdr[0] = static_cast<int16_t>(w);
            hdr[1] = static_cast<int16_t>(h);
            uint16_t *src = reinterpret_cast<uint16_t *>(hdr + 2);
            memset(src, 0, static_cast<size_t>(h * w * 2));
            DrawFrameArt(art0, hdr, 1);                   // draw the 9-slice into hdr
            uint16_t *dst = frame_pixels(gump) + y * gump->width + x;
            keyed_blit(dst, src, w, h, gump->width, false);
            operator delete(hdr);
            break;
        }
        case 4: case 8: {
            rect[0] = 0;
            rect[1] = 0;
            rect[2] = w;
            rect[3] = h;
            uint16_t *buf = static_cast<uint16_t *>(::operator new(static_cast<size_t>(h * w * 2)));
            memset(buf, 0, static_cast<size_t>(h * w * 2));
            uint16_t hue = *reinterpret_cast<uint16_t *>(g_fontHueTable + art0 * 0x58);
            if (type == 8 && flag != 0) {
                WStr_Copy(textBuf, text);                 // copy the edit string
                WStr_Append(textBuf, g_caretString);      // append the caret
                Text_Render(buf, w, textBuf, 1, 1, hue, rect);
            } else {
                Text_Render(buf, w, text, 1, 1, hue, rect);
            }
            Text_Finish(buf, w, h, 1);
            uint16_t *src = buf;
            uint16_t *dst = frame_pixels(gump) + y * gump->width + x;
            keyed_blit(dst, src, w, h, gump->width, g_is565 != 0);
            operator delete(buf);
            break;
        }
        default:
            break;
        }
    }

    for (GumpNode *c = children; c != nullptr; c = c->next) {
        if (c->page == 0 || c->page == curPage)
            c->Render(gump);
    }
}
HOOK_METHOD(0x00435eb0, GumpNode, Render);
