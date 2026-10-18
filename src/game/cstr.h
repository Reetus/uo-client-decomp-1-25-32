// CStr: the growable C string used as a member by a few classes (the app object, the object manager).
// Layout from the constructors at 0x004703a0 / 0x004703d0 and the assignment at 0x00470460. 0x10 bytes.
#pragma once

#include <cstddef>
#include <cstdint>
#include <new>

class CStr {
public:
    // 0x004703a0: empty string (no buffer), grow step 0x40. Returns this.
    CStr();
    // 0x004703c0: free the buffer.
    ~CStr();
    CStr *Ctor() { return new (this) CStr(); }
    void Dtor() { this->~CStr(); }
    // 0x004703d0: RET 4. Copy of a C string (buffer of strlen + 0x40 bytes); null gives an empty string. Returns this.
    CStr(const char *src);
    // 0x00470460: RET 4. Assign a C string, reallocating when the buffer is too small; null empties it. Returns this.
    CStr *__thiscall Assign(const char *src);
    // 0x00470500: the buffer pointer.
    char *__thiscall GetBuffer();

    char *buf;      // +0x00
    int32_t len;    // +0x04  strlen of the contents
    int32_t grow;   // +0x08  0x40
    int32_t cap;    // +0x0c  allocated bytes
};

static_assert(offsetof(CStr, len) == 0x04, "CStr::len");
static_assert(offsetof(CStr, grow) == 0x08, "CStr::grow");
static_assert(offsetof(CStr, cap) == 0x0c, "CStr::cap");
static_assert(sizeof(CStr) == 0x10, "CStr size");
