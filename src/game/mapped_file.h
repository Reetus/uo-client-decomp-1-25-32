// MappedFile: named file mapping wrapper (unifont.mul and friends); the writable variant is in mapped_file_rw.h.
// Layout from decomp/pass2/0x0047e810.cpp, 0x0047e890.cpp, 0x0047ea50.cpp; the object is
// 0x90 bytes (the sites allocate with operator new(0x90)). The writable variant is a class of its own in the original,
// with its own constructor and destructor.
#pragma once

#include <cstddef>
#include <cstdint>

#include <windows.h>

#include "../runtime/ported.h"

class MappedFile {
public:
    // 0x0047e810  size 0x3e  read-only variant; copies the name, zeroes handles and view. new MappedFile(path).
    MappedFile(const char *fileName);
    // 0x0047e850  size 0x3f  destructor: unmap and close, guarded by both the file handle and the view.
    ~MappedFile();
    // 0x0047e890: unmap and close; only guarded by the view pointer.
    void __thiscall Close();
    // 0x0047e8d0: open the named file read-only (records its size at +8), map it; returns the view or 0.
    void *__thiscall OpenReadOnly();
    // 0x0047ea20: copy `count` bytes at `offset` of the view to `dest`; returns count.
    DWORD __thiscall Read(DWORD offset, DWORD count, void *dest);

    HANDLE hFile;       // +0x00
    HANDLE hMapping;    // +0x04
    DWORD size;         // +0x08  set by OpenReadOnly (the file size)
    void *view;         // +0x0c
    char name[0x80];    // +0x10
};

static_assert(offsetof(MappedFile, hFile) == 0x00, "MappedFile::hFile");
static_assert(offsetof(MappedFile, hMapping) == 0x04, "MappedFile::hMapping");
static_assert(offsetof(MappedFile, size) == 0x08, "MappedFile::size");
static_assert(offsetof(MappedFile, view) == 0x0c, "MappedFile::view");
static_assert(offsetof(MappedFile, name) == 0x10, "MappedFile::name");
static_assert(sizeof(MappedFile) == 0x90, "MappedFile size (operator new(0x90) at every site)");
