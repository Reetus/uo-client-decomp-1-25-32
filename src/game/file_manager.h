// CFileManager: the data-file manager (one static instance at 0x0068d980). It serves records of the 17 map / art /
// anim / sound / texture / gump / multi / skills .mul files. A file that is on the hard disk is mapped whole
// (g_FileOnHD[file] = 1, views[file] is the mapping); a file that has to come from the CD is served through a block
// cache kept in the memory-mapped file "cache.uo": a list of entries (CacheEntry) that doubles as the LRU list of
// cached blocks and as the sorted list of free regions, plus a hash table of CacheNode keyed by (file, key & 0xfff).
// Layout from decomp/pass2/0x0043faf0.cpp and the assembly of 0x0043f9e0 .. 0x00441190.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"
#include "mapped_file.h"
class MappedFileRW;
#include "globals.h"

// 0x18-byte entry of the cache file. An entry is either a cached block (linked into the LRU list) or a free region
// (linked into the free list, sorted by offset) or unused (linked into the unused list).
struct CacheEntry {
    int32_t size;     // +0x00  bytes of the block / region
    int32_t file;     // +0x04
    int32_t offset;   // +0x08  offset of the data in the cache file (add CFileManager::base)
    int32_t next;     // +0x0c  entry index, -1 = none
    int32_t prev;     // +0x10  entry index, -1 = none
    int32_t key;      // +0x14  first record (block files) or record number (data files) cached here
};
static_assert(sizeof(CacheEntry) == 0x18, "CacheEntry size");

// The six list ends at the start of the cache file.
struct CacheHeader {
    int32_t lruHead;      // +0x00  most recently used entry
    int32_t lruTail;      // +0x04  least recently used entry
    int32_t freeHead;     // +0x08  free regions, lowest offset first
    int32_t freeTail;     // +0x0c
    int32_t unusedHead;   // +0x10  entries that hold neither a block nor a region
    int32_t unusedTail;   // +0x14
};
static_assert(sizeof(CacheHeader) == 0x18, "CacheHeader size");

// 0x18-byte hash node: one per cached block, in the table at 0x004e4350 ([(file << 12) + (key & 0xfff)]).
struct CacheNode {
    int32_t file;       // +0x00
    int32_t key;        // +0x04
    int32_t entry;      // +0x08  index into CFileManager::entries
    int32_t flag;       // +0x0c
    CacheNode *next;    // +0x10
    CacheNode *prev;    // +0x14
};
static_assert(sizeof(CacheNode) == 0x18, "CacheNode size");

// One of the 17 records at 0x0049f2d4.
struct FileDesc {
    int32_t blockSize;    // +0x00  records per cached block (1 = per record)
    int32_t recSize;      // +0x04  bytes per record, -1 for the data files
    const char *name;     // +0x08
    int32_t index;        // +0x0c
};
static_assert(sizeof(FileDesc) == 0x10, "FileDesc size");

class CFileManager {
public:
    // 0x0043f9e0: clear the hash table, the stream slots and (when verdata is enabled) the verdata table. Returns this.
    CFileManager *__thiscall ClearTables();
    // 0x0043faf0: open the 17 files (whole, from the hard disk or the CD) and create the cache file.
    void __thiscall Init();
    // 0x004400f0: RET 8. Drop the cached block holding record `offset` of a CD-resident file.
    void __thiscall Invalidate(int file, int offset);
    // 0x00440210: RET 0x18. Rewrite one index record and its data (both files must be on the hard disk).
    void __thiscall ReplaceRecord(int idxFile, int dataFile, int rec, int extra, void *data, int len);
    // 0x00440480: RET 8. Pointer to record `index` of `file`.
    uint8_t *__thiscall GetRecordPtr(int file, int index);
    // 0x004405d0: RET 0x14. Index record `index` of fileA locates the data in fileB: returns the data, *outLen / *outExtra
    // are the record's length and extra word, 0 when the record is empty.
    uint8_t *__thiscall GetRecord(int fileA, int fileB, int index, int *outLen, int *outExtra);
    // 0x00440840: RET 8. Read the cache block of `file` that holds record `index` and return the record.
    uint8_t *__thiscall ReadBlock(int file, int index);
    // 0x004409f0: RET 0xc. Take a free region of at least `size` bytes (evicting as needed), tag it (file, key), put
    // it at the head of the LRU list; returns its entry index.
    int __thiscall AllocRegion(int file, int key, int size);
    // 0x00440ba0: evict the least recently used block; returns the entry index of the region it became.
    int __thiscall EvictLRU();
    // 0x00440ca0: RET 4. Put entry `idx` into the sorted free list, merging with its neighbours.
    int __thiscall FreeRegion(int idx);
    // 0x00440fb0: RET 4. Move entry `idx` to the head of the LRU list.
    void __thiscall TouchLRU(int idx);

    MappedFileRW *cacheStream;       // +0x00  "cache.uo"
    MappedFile *streams[17];       // +0x04
    uint8_t *views[17];            // +0x48  view of each file (HD files) or 0
    int32_t entrySize;             // +0x8c  0x18
    int32_t headerBytes;           // +0x90  (entryCount + 1) * 0x18, where the cached data starts
    int32_t dataBytes;             // +0x94  cache bytes - headerBytes
    int32_t entryCount;            // +0x98  cache size in MB * 4000
    int32_t cacheBytes;            // +0x9c  cache size in MB * 1000000
    CacheHeader *header;           // +0xa0
    uint8_t *base;                 // +0xa4  view of the cache file
    CacheEntry *entries;           // +0xa8  base + entrySize
};

static_assert(offsetof(CFileManager, streams) == 0x04, "CFileManager::streams");
static_assert(offsetof(CFileManager, views) == 0x48, "CFileManager::views");
static_assert(offsetof(CFileManager, entrySize) == 0x8c, "CFileManager::entrySize");
static_assert(offsetof(CFileManager, headerBytes) == 0x90, "CFileManager::headerBytes");
static_assert(offsetof(CFileManager, dataBytes) == 0x94, "CFileManager::dataBytes");
static_assert(offsetof(CFileManager, entryCount) == 0x98, "CFileManager::entryCount");
static_assert(offsetof(CFileManager, cacheBytes) == 0x9c, "CFileManager::cacheBytes");
static_assert(offsetof(CFileManager, header) == 0xa0, "CFileManager::header");
static_assert(offsetof(CFileManager, base) == 0xa4, "CFileManager::base");
static_assert(offsetof(CFileManager, entries) == 0xa8, "CFileManager::entries");

#define g_fileManager GLOBAL_AS(CFileManager, g_fileManager_storage)
