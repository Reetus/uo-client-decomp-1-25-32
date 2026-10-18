// MappedFile: close/open/read of the memory-mapped file wrapper (read-only and read-write variants).
// The RW variants share the layout of MappedFile (CtorSized fills +8 with the map size).
#include "mapped_file.h"
#include "mapped_file_rw.h"

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"
#include "crt.h"
#include "app_support_ports.h"


#define mf_hMainWnd GLOBAL_AS(HWND, g_mainWindow)
// the new-handler (out of memory): 0x0044b200, one stack argument
#define mf_OutOfMemory(a1) (App_OutOfMemoryHandler(a1))

// 0x0047e850  size 0x3f
// Unmap and close; guarded by both the file handle and the view.
MappedFile::~MappedFile()
{
    if (hFile != nullptr && view != nullptr) {
        UnmapViewOfFile(view);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        hMapping = nullptr;
        hFile = nullptr;
        view = nullptr;
    }
}
HOOK_DTOR(0x0047e850, MappedFile);

// 0x0047e8d0  size 0x144
// Read-only open: records the file size (through the original CRT), then CreateFile / mapping / view.
// Failures show a message box and return 0.
void *__thiscall MappedFile::OpenReadOnly()
{
    char msg[0x100];
    char *fname = name;
    FILE *f;
    DWORD err;

    if (fname == nullptr || *fname == 0)
        return nullptr;

    f = fopen(fname, g_fopenReadMode);
    if (f != nullptr) {
        fseek(f, 0, 2);
        size = static_cast<DWORD>(ftell(f));
        fclose(f);
    } else {
        size = 0;
    }

    hFile = CreateFileA(fname, 0x80000000, 1, nullptr, 3, 0x10000080, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        err = GetLastError();
        if (err == 8)
            mf_OutOfMemory(0);
        sprintf(msg, "Failed to open mmf %s: Error %d.", fname, static_cast<int>(err));
        MessageBoxA(mf_hMainWnd, msg, "MMF Error", 0);
        view = nullptr;
    } else {
        hMapping = CreateFileMappingA(hFile, nullptr, 2, 0, 0, nullptr);
        if (hMapping == nullptr) {
            err = GetLastError();
            if (err == 8)
                mf_OutOfMemory(0);
            sprintf(msg, "Failed to create mapping for mmf %s: Error %d.", fname, static_cast<int>(err));
            MessageBoxA(mf_hMainWnd, msg, "MMF Error", 0);
            view = nullptr;
        } else {
            view = MapViewOfFile(hMapping, 4, 0, 0, 0);
            if (view == nullptr)
                mf_OutOfMemory(0);
        }
    }
    return view;
}
HOOK_METHOD(0x0047e8d0, MappedFile, OpenReadOnly);

// 0x0047ea20  size 0x29
// Copy `count` bytes starting at `offset` in the view to `dest`; returns count.
DWORD __thiscall MappedFile::Read(DWORD offset, DWORD count, void *dest)
{
    memcpy(dest, static_cast<uint8_t *>(view) + offset, count);
    return count;
}
HOOK_METHOD(0x0047ea20, MappedFile, Read);

// 0x0047eaa0  size 0x4d
// Read-write close: flush `size` bytes of the view, then unmap and close.
MappedFileRW::~MappedFileRW()
{
    if (hFile != nullptr && view != nullptr) {
        FlushViewOfFile(view, size);
        UnmapViewOfFile(view);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        hMapping = nullptr;
        hFile = nullptr;
        view = nullptr;
    }
}
HOOK_DTOR(0x0047eaa0, MappedFileRW);

// 0x0047eaf0  size 0x129
// Read-write open of a file with a fixed map size (+8).
void *__thiscall MappedFileRW::OpenRW()
{
    char msg[0x100];
    char *fname = name;
    DWORD err;

    hFile = CreateFileA(fname, 0xc0000000, 0, nullptr, 4, 0x10000080, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        err = GetLastError();
        if (err == 8)
            mf_OutOfMemory(0);
        sprintf(msg, "Failed to open mmf_wr %s: Error %d.", fname, static_cast<int>(err));
        MessageBoxA(mf_hMainWnd, msg, "MMF Error", 0);
        view = nullptr;
    } else {
        hMapping = CreateFileMappingA(hFile, nullptr, 4, 0, size, nullptr);
        if (hMapping == nullptr) {
            err = GetLastError();
            if (err == 8)
                mf_OutOfMemory(0);
            sprintf(msg, "Failed to create mapping for mmf_wr %s: Error %d.", fname, static_cast<int>(err));
            MessageBoxA(mf_hMainWnd, msg, "MMF Error", 0);
            view = nullptr;
        } else {
            view = MapViewOfFile(hMapping, 0xf001f, 0, 0, 0);
            if (view == nullptr)
                mf_OutOfMemory(0);
        }
    }
    return view;
}
HOOK_METHOD(0x0047eaf0, MappedFileRW, OpenRW);
