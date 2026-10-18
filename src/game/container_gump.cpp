// CContainerGump.
#include "container_gump.h"

#include "../runtime/ported.h"
#include "batch02_helpers.h"
#include "globals.h"
#include "obj.h"
#include "sound_manager.h"
#include "vtables.h"


#define Container_Release(self) (reinterpret_cast<CContainerObj *>(self)->DestroyChildren())    // ECX = container object
// 0x004800f0: MusicManager sound/music request (sound id, then five zero arguments).
#define g_soundObject GLOBAL_PTR(void, g_soundManager_storage)           // the object at 0x0068db30
#define Music_Play(self, a1, a2, a3, a4, a5, a6) (reinterpret_cast<CSoundManager *>(self)->PlaySound(a1, (long)(a2), (long)(a3), a4, a5, a6))

// 0x004261d0  size 0x1a2
CContainerGump::~CContainerGump()
{

    CGump *savedIter = g_gumpDeleteCursor;
    CGump *g = g_gumpListTail;
    while (g != nullptr) {
        g_gumpDeleteCursor = g->prev;      // kept in the global so a delete cannot invalidate the walk
        int r = PORT_VSLOT0(g, 0x84, int)();
        if (r != 0 && RAW_AT(void *, g, 0xb4) == this && g != nullptr)
            PORT_VSLOT1(g, 0, void, int)(1);          // deleting dtor
        g = g_gumpDeleteCursor;
    }

    int savedFlag = g_suppressFlag;
    g_gumpDeleteCursor = savedIter;
    g_suppressFlag = 1;

    void *container = owner;
    if (container != nullptr) {
        int r = PORT_VSLOT0(container, 0x54, int)();
        if (r == 0)
            Container_Release(owner);
    }
    g_suppressFlag = savedFlag;

    if (frameData[1] != 0) {
        FrameData_Free(g_frameDataAllocator, i32_ptr(frameData[1]), "congump frame0 pBMP");
        frameData[1] = 0;
    }
    if (frameCount == 2) {
        if (frameData[3] != 0) {
            FrameData_Free(g_frameDataAllocator, i32_ptr(frameData[3]), "congump frame1 pBMP");
            frameData[3] = 0;
        }
    }
    if (frameData != nullptr) {
        FrameData_Free(g_frameDataAllocator, frameData, "congump framedata");
        frameData = nullptr;
    }

    if (g_openContainerGump == this) {
        g_openContainerGump = nullptr;
        void *c = owner;
        if (c != nullptr) {
            int r = PORT_VSLOT0(c, 0x30, int)();
            if (r != 0)
                RAW_AT(int32_t, owner, 0xac) = 0;
            void *c2 = owner;
            if (c2 != nullptr)
                PORT_VSLOT1(c2, 0, void, int)(1);    // deleting dtor
            owner = nullptr;
        }
    }

    int snd;
    switch (static_cast<uint16_t>(unk_50)) {    // closing sound by container art id
    case 0x3c: case 0x3d: case 0x3f: case 0x41:
        snd = 0x58;
        break;
    case 0x3e: case 0x48: case 0x4d: case 0x51:
        snd = 0x2e;
        break;
    case 0x42: case 0x43: case 0x44: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4e: case 0x4f:
        snd = 0x2c;
        break;
    default:
        snd = -1;
        break;
    }
    if (snd != -1)
        Music_Play(g_soundObject, snd, 0, 0, 0, 0, 0);
}
HOOK_DTOR(0x004261d0, CContainerGump);
