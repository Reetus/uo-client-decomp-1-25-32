#include "obj_picker_gump.h"

#include <cstring>

#include "../runtime/ported.h"
#include "batch06_helpers.h"
#include "globals.h"
#include "input_control_gump.h"
#include "button_gump.h"
#include "obj.h"
#include "packet_builders_b25_ports.h"
#include "client_socket.h"
#include "gump.h"
#include "vtables.h"
#include "crt.h"

// 0x00413110 button ctor, 9 stack args: (x, y, x2, y2, parent, 2, 0, callback, 0).
// 0x00453050: default display name of item id (ECX = id, EDX = 0).
#define Item_DefaultName(a1, a2) (BuildItemName((int)(a1), a2))
// 0x00465190 on the packet builder object 0x0069e0e0: (buf, serial, modelId, selection, id, hue).
#define Packet_BuildPick(o, a1, a2, a3, a4, a5, a6) PORT_THISCALL6(&Pkt_Build_0x7D_MenuResponse, void, (o), void *, unsigned int, uint16_t, int16_t, uint16_t, uint16_t)(a1, a2, a3, a4, a5, a6)
// 0x00478610 on the socket object: send a built packet.
#define Socket_Send(self, a1) (reinterpret_cast<CClientSocket *>(self)->SendPacket((const uint8_t *)(a1)))
#define g_packetBuilder GLOBAL_PTR(void, g_packetBuilder_storage)

// 0x0046eb80  size 0x492  CObjPickerGump::CObjPickerGump(...)
// Copies the id / hue / name arrays, draws the frame and title, adds the list control and two buttons.
CObjPickerGump::CObjPickerGump(int x_, int y_, char *title_, unsigned int serial_, uint16_t modelId_, int count_,
                               uint16_t *ids_, uint16_t *hues_, char **names_)
{
    int rect[4];
    int tw, th, ty;

    x = x_;
    y = y_;
    name = const_cast<char *>("objpicker gump");
    dirty = 0;
    unk_50 = 0x910;   // gump art id
    unk_60 = 0;
    unk_ac = 0;
    unk_a0 = 0;
    unk_9c = 0;
    unk_98 = 0;
    unk_94 = 0;
    unk_a8 = 0;
    unk_a4 = 0;
    modelId = modelId_;
    serial = serial_;
    strcpy(title, title_);
    count = count_;
    ids = static_cast<uint16_t *>(::operator new(count_ * 2));
    memcpy(ids, ids_, count * 2);
    hues = static_cast<uint16_t *>(::operator new(count * 2));
    memcpy(hues, hues_, count * 2);

    int i = 0;
    if (count > 0) {
        char **src = names_;
        char **dst = names;
        do {
            const char *s;
            char *copy;
            if (*src == nullptr) {
                // no name supplied: use the default name of item id ids[i]
                s = Item_DefaultName(ids[i], 0);
                copy = static_cast<char *>(::operator new(strlen(s) + 1));
                *dst = copy;
            } else {
                copy = static_cast<char *>(::operator new(strlen(*src) + 1));
                *dst = copy;
                s = *src;
            }
            strcpy(copy, s);
            i++;
            dst++;
            src++;
        } while (i < count);
    }

    unk_b8 = 0;
    unk_c4 = -1;
    selection = 0;
    firstChild = nullptr;
    parent = nullptr;
    lastChild = nullptr;
    next = nullptr;
    prev = nullptr;
    b6_text_measure(b6_font, 1, title_, &tw, &th, &ty);

    b6_art_size(unk_50, &width, &height);
    frameIndex = 0;
    frameCount = 1;
    frameData = static_cast<int32_t *>(b6_alloc(g_frameDataAllocator, 8, "framedata for CDumbGump"));
    memset(frameData, 0, frameCount * 8);
    b6_set_frame_bitmap(this, 0, b6_alloc(g_frameDataAllocator, width * height * 2 + 4, "dumb frame0"));
    memset(b6_frame_bitmap(this, 0)->px, 0, (width * height) * 2);
    b6_frame_bitmap(this, 0)->w = static_cast<uint16_t>(width);
    b6_frame_bitmap(this, 0)->h = static_cast<uint16_t>(height);
    {
        B6Bitmap *pb = b6_frame_bitmap(this, 0);
        b6_draw_art(B6_ROWBLIT_COPY, unk_50, 0, 0, pb->px, pb->w, pb->h);
    }

    rect[0] = 0;
    rect[1] = 0;
    rect[2] = width;
    rect[3] = height;
    pickRect[0] = 0x28;
    pickRect[1] = 0x2f;
    pickRect[2] = 0x101;
    pickRect[3] = 0x5b;
    b6_text_measure(b6_font, 1, title, &tw, &th, &ty);
    b6_text_draw(b6_font, 0x27, ty + 0x12, 1, 0x386, title, b6_frame_bitmap(this, 0)->px, width, rect);

    uint8_t *ctl = reinterpret_cast<uint8_t *>(new CInputControlGump(0x27, 0x67, this, static_cast<uint16_t>(0xda), static_cast<uint16_t>(0x10), 0, 0x1e, 1, 0x386, 1, 0, 0));
    listControl = ctl;
    *reinterpret_cast<int32_t *>(ctl + 0xb4) = 1;   // ? unconditional in the original (null deref if the allocation failed)
    CGump_AddChild(listControl, this, 1);
    this->CObjPickerGump::RedrawList();   // refresh picker list

    uint8_t *btn = (uint8_t *)new CButtonGump(0x16, 0x3d, 0x23, 0x4f, this, 2, 0, (int)(reinterpret_cast<void *>(PORT_ADDR(&CObjPickerGump::PrevPage))), 0);
    okButton = btn;
    CGump_AddChild(btn, this, 1);

    btn = (uint8_t *)new CButtonGump(0x105, 0x3d, 0x112, 0x4f, this, 2, 0, (int)(reinterpret_cast<void *>(PORT_ADDR(&CObjPickerGump::NextPage))), 0);
    cancelButton = btn;
    CGump_AddChild(btn, this, 1);
}
HOOK_CTOR(0x0046eb80, CObjPickerGump, (int, int, char *, unsigned int, uint16_t, int, uint16_t *, uint16_t *, char **));

// 0x0046f050  size 0x14b  CObjPickerGump::~CObjPickerGump
// If the socket exists, sends the selection (id / hue of the chosen entry, 0 when none); frees the arrays,
// the list control and the frame bitmap, then runs the CGump destructor.
CObjPickerGump::~CObjPickerGump()
{
    uint16_t id = 0;
    uint16_t hue = 0;
    uint8_t buf[0x20];

    if (selection != 0) {
        id = ids[selection - 1];
        hue = hues[selection - 1];
    }
    if (g_pktSender != nullptr) {
        Packet_BuildPick(g_packetBuilder, buf, serial, modelId, static_cast<int16_t>(selection), id, hue);
        Socket_Send(g_pktSender, buf);
    }
    operator delete(ids);
    operator delete(hues);
    int i = 0;
    if (count > 0) {
        char **p = names;
        do {
            operator delete(*p);
            i++;
            p++;
        } while (i < count);
    }
    if (listControl != nullptr) {
        b6_delete_gump(listControl);
        listControl = nullptr;
    }
    if (b6_frame_bitmap(this, 0) != nullptr) {
        b6_free(g_frameDataAllocator, b6_frame_bitmap(this, 0), "objpicker frame0 pBMP");
        b6_set_frame_bitmap(this, 0, nullptr);
    }
    if (frameData != nullptr) {
        b6_free(g_frameDataAllocator, frameData, "objpicker framedata");
        frameData = nullptr;
    }
}
HOOK_DTOR(0x0046f050, CObjPickerGump);
