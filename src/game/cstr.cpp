// CStr methods (0x004703a0 .. 0x00470500).
#include "cstr.h"

#include <cstring>

#include "../runtime/ported.h"
#include "crt.h"


// 0x004703a0  size 0x14
CStr::CStr()
{
    buf = nullptr;
    len = 0;
    grow = 0x40;
    cap = 0;
}
HOOK_CTOR(0x004703a0, CStr, ());

// 0x004703c0  size 0x10
CStr::~CStr()
{
    if (buf != nullptr)
        operator delete(buf);   // 0x0048ad60
}
HOOK_DTOR(0x004703c0, CStr);

// 0x004703d0  size 0x8f  RET 4
// A non-null source gets a buffer of strlen + 0x40 bytes; if that allocation yields null the string ends
// up empty (cap and len 0), after the copy has been attempted as in the original.
CStr::CStr(const char *src)
{
    buf = nullptr;
    len = 0;
    grow = 0x40;
    cap = 0;
    if (src == nullptr) {
        len = 0;
    } else {
        if (static_cast<int>(strlen(src)) + 1 > 0) {
            cap = static_cast<int>(strlen(src)) + 1 + 0x3f;
            buf = static_cast<char *>(::operator new(static_cast<size_t>(cap)));   // 0x0048ad70
        }
        strcpy(buf, src);
        len = static_cast<int>(strlen(buf));
    }
    if (buf == nullptr) {
        cap = 0;
        len = 0;
    }
}
HOOK_CTOR(0x004703d0, CStr, (const char *));

// 0x00470460  size 0x9c  RET 4
CStr *__thiscall CStr::Assign(const char *src)
{
    if (src == nullptr) {
        if (buf != nullptr)
            *buf = 0;
        len = 0;
        return this;
    }
    int n = static_cast<int>(strlen(src)) + 1;
    if (n > cap) {
        if (buf != nullptr)
            operator delete(buf);   // 0x0048ad60
        cap = grow + static_cast<int>(strlen(src));
        buf = static_cast<char *>(::operator new(static_cast<size_t>(cap)));   // 0x0048ad70
    }
    strcpy(buf, src);
    len = static_cast<int>(strlen(buf));
    return this;
}
HOOK_METHOD(0x00470460, CStr, Assign);

// 0x00470500  size 0x3: mov eax, [ecx]; ret
char *__thiscall CStr::GetBuffer()
{
    return buf;
}
HOOK_METHOD(0x00470500, CStr, GetBuffer);
