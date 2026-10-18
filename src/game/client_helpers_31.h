// The two unidentified objects of client_helpers_31.cpp (placeholder names by address).
#pragma once

class CObj0047d430 {
public:
    // 0x0047d430: delete the buffer at +0 and clear the pointer.
    void __thiscall FreeBuffer();
    void *buffer;   // +0x00
};

class CObj0047d490 {
public:
    // 0x0047d490: set the size/offset globals at 0x00758210..0x0075825c, then call the no-op at 0x00406720. Returns this.
    CObj0047d490 *__thiscall InitSizes();
    // 0x0047d560: call the no-op at 0x00406720 twice.
    void __thiscall NopTwice();
    // 0x0047d580: call the no-op at 0x00406720 when the versioned-data flag (0x006f4768) is set.
    void __thiscall NopIfVersioned();
};

