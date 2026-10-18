#include "mapped_file.h"
#include "mapped_file_rw.h"

#include <cstring>

#include "../runtime/ported.h"

// 0x0047e810  size 0x3e
MappedFile::MappedFile(const char *fileName)
{
    strcpy(name, fileName);
    hMapping = nullptr;
    hFile = nullptr;
    view = nullptr;
}
HOOK_CTOR(0x0047e810, MappedFile, (const char *));

// 0x0047ea50  size 0x44
MappedFileRW::MappedFileRW(const char *fileName, DWORD mapSize)
{
    strcpy(name, fileName);
    hMapping = nullptr;
    hFile = nullptr;
    view = nullptr;
    size = mapSize;
}
HOOK_CTOR(0x0047ea50, MappedFileRW, (const char *, DWORD));

// 0x0047e890  size 0x3a
// Unlike the sibling at 0x0047e850 there is no file-handle test; only the view pointer is checked.
void __thiscall MappedFile::Close()
{
    if (view != nullptr) {
        UnmapViewOfFile(view);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        hMapping = nullptr;
        hFile = nullptr;
        view = nullptr;
    }
}
HOOK_METHOD(0x0047e890, MappedFile, Close);
