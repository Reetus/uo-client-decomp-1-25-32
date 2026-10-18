// What the ported code needs from the C runtime beyond the plain names.
//
// The original is linked with the multithreaded static CRT of Visual C++ 5, and so is this build. The ported
// code calls the runtime as the original does: ::operator new(n) (0x0048ad70), operator delete, malloc,
// calloc, free, atoi, qsort and the rest by their own names (docs/building.md, "The C runtime"). The only
// helpers left here are the two array iterators of the compiler's own runtime, which have no public name.
//
#pragma once

#include "../runtime/ported.h"
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <new>
#include <new.h>
#include <process.h>

// 0x0048b540 / 0x0048b400, `eh vector constructor iterator` / `eh vector destructor iterator`:
// (array, element size, count, constructor[, destructor]), each called with ECX = element. Without the
// unwinding: no constructor of the client throws.
inline void __stdcall crt_vec_ctor(void *array, size_t size, int count, void *ctor, void *)
{
    char *at = static_cast<char *>(array);
    for (int i = 0; i < count; i++, at += size)
        PORT_THISCALL0(ctor, void, at)();
}

inline void __stdcall crt_vec_dtor(void *array, size_t size, int count, void *dtor)
{
    char *at = static_cast<char *>(array) + size * count;
    while (--count >= 0) {
        at -= size;
        PORT_THISCALL0(dtor, void, at)();
    }
}
