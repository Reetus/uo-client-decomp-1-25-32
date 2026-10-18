// MappedFileRW: the writable file mapping wrapper (cache.uo). A class of its own in the original, with its own
// constructor and destructor; same 0x90-byte layout as MappedFile (mapped_file.h).
#pragma once

#include <cstddef>
#include <cstdint>

#include <windows.h>

#include "../runtime/ported.h"

// The map size is given to the constructor.
class MappedFileRW {
public:
    // 0x0047ea50  size 0x44  copies the name, zeroes handles and view, records the map size. new MappedFileRW(path, size).
    MappedFileRW(const char *fileName, DWORD mapSize);
    // 0x0047eaa0  size 0x4d  destructor: flush `size` bytes of the view, unmap and close; guarded by file handle and view.
    ~MappedFileRW();
    // 0x0047eaf0: open/create the named file read-write and map `size` bytes; returns the view or 0.
    void *__thiscall OpenRW();

    HANDLE hFile;       // +0x00
    HANDLE hMapping;    // +0x04
    DWORD size;         // +0x08  the map size
    void *view;         // +0x0c
    char name[0x80];    // +0x10
};

static_assert(offsetof(MappedFileRW, hFile) == 0x00, "MappedFileRW::hFile");
static_assert(offsetof(MappedFileRW, hMapping) == 0x04, "MappedFileRW::hMapping");
static_assert(offsetof(MappedFileRW, size) == 0x08, "MappedFileRW::size");
static_assert(offsetof(MappedFileRW, view) == 0x0c, "MappedFileRW::view");
static_assert(offsetof(MappedFileRW, name) == 0x10, "MappedFileRW::name");
static_assert(sizeof(MappedFileRW) == 0x90, "MappedFileRW size (operator new(0x90) in CFileManager::Init)");
