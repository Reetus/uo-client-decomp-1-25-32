// CFileManager: tables, init, cache, record lookup. (Porting pass 2, batch 15.)
// The SEH frame of 0x0043faf0 (try levels around the `new`s) is omitted.
#include "file_manager.h"

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "app.h"
#include "crt.h"
#include "window_helpers_31_ports.h"
#include "mapped_file.h"
#include "mapped_file_rw.h"
#include "globals.h"

#define g_fileDesc GLOBAL_PTR(FileDesc, g_fileDesc_storage)

// 0x00482c70: ECX = text, EDX = caption, stack = exit flag, RET 4: message box.
#define ErrorBox(a1, a2, a3) (ShowMessageBoxAndExit(a1, a2, a3))

// Verdata patch node (the patch file's index records).
struct VerNode {
    int32_t index;    // +0x00
    int32_t offset;   // +0x04  into the verdata buffer
    int32_t length;   // +0x08  0 = the record was patched to empty
    int32_t extra;    // +0x0c
    VerNode *next;    // +0x10
};

// 0x0047e8d0: map a file read-only; returns the view.
static uint8_t *OpenReadOnly(MappedFile *stream)
{
    return (uint8_t *)stream->MappedFile::OpenReadOnly();
}

// 0x0047eaf0: map the cache file read/write; returns the view.
static uint8_t *OpenReadWrite(MappedFileRW *stream)
{
    return (uint8_t *)stream->MappedFileRW::OpenRW();
}

// 0x0047ea20: copy `size` bytes from the view at `offset` to `dest`. RET 0xc.
static inline void MappedRead(MappedFile *stream, int offset, int size, void *dest)
{
    stream->MappedFile::Read(offset, size, dest);
}

// 0x0043f9e0  size 0x53
CFileManager *__thiscall CFileManager::ClearTables()
{
    memset(&g_hashTable[0], 0, 0x11000 * 4);
    cacheStream = nullptr;
    base = nullptr;
    for (int i = 0; i < 17; i++)
        streams[i] = nullptr;
    if (g_verdataEnabled != 0) {
        GLOBAL_AS(int, g_verdataUnk) = 0;
        memset(&g_verdataTable[0], 0, 0x4400 * 4);
        g_verdataBase = nullptr;
        g_verdataCount = 0;
    }
    return this;
}
HOOK_METHOD(0x0043f9e0, CFileManager, ClearTables);

// 0x0043faf0  size 0x5f5
// Maximum install (0x0069e518 == 1): every file must open from the hard disk and is mapped whole; nothing else is
// set up. Otherwise (or when a file is missing, after the error box) the cache file "cache.uo" is created, zero
// filled, mapped, its entry lists built, and each of the 17 files is opened from the hard disk (flag 1) or the CD
// path (flag 0) and mapped.
// ? the loop that builds hash nodes from an existing cache runs only when header->lruHead != -1, which was just
//   set to -1 above it; it is kept as the assembly has it.
void __thiscall CFileManager::Init()
{
    char msg[0x200];
    uint32_t zeros[0x4000];
    FILE *fp;
    int k;

    if (g_installMode == 1) {
        bool allOpen = true;
        k = 0;
        do {
            fp = fopen(g_app.ResolveDataPath(const_cast<char *>(g_fileDesc[k].name)), "rb");
            if (fp == nullptr) {
                sprintf(msg, g_004a87d4, g_fileDesc[k].name);
                ErrorBox(msg, "Unable to open file", 1);
                allOpen = false;
                break;
            }
            fclose(fp);
            k++;
        } while (k < 17);

        if (allOpen) {
            for (k = 0; k < 17; k++) {
                MappedFile *obj = new MappedFile(g_app.ResolveDataPath(const_cast<char *>(g_fileDesc[k].name)));
                streams[k] = obj;
                views[k] = OpenReadOnly(obj);
                g_fileOnHD[k] = 1;
            }
            return;
        }
    }

    entrySize = 0x18;
    cacheBytes = g_cacheMB * 1000000;
    entryCount = g_cacheMB * 4000;
    headerBytes = (entryCount + 1) * 0x18;
    dataBytes = cacheBytes - headerBytes;

    CacheNode *pool = static_cast<CacheNode *>(::operator new(static_cast<size_t>(entryCount) * 0x18));
    g_nodePool = pool;
    g_nodeFreeList = pool;
    {
        int i = 0;
        if (entryCount - 1 > 0) {
            int off = 0;
            do {
                *reinterpret_cast<CacheNode **>(reinterpret_cast<uint8_t *>(pool) + off + 0x10) =
                    reinterpret_cast<CacheNode *>(reinterpret_cast<uint8_t *>(pool) + off + 0x18);
                i++;
                off += 0x18;
                pool = g_nodePool;
            } while (i < entryCount - 1);
        }
    }
    *reinterpret_cast<int32_t *>(reinterpret_cast<uint8_t *>(pool) + entryCount * 0x18 - 8) = 0;

    strcpy(msg, g_app.ResolveDataPath(const_cast<char *>("cache.uo")));
    fp = fopen(msg, "wb");
    memset(zeros, 0, sizeof(zeros));
    int left = cacheBytes;
    if (left > 0x10000) {
        int chunks = (left - 1) >> 16;
        do {
            fwrite(zeros, 0x10000, 1, fp);
            left -= 0x10000;
            chunks--;
        } while (chunks != 0);
    }
    fwrite(zeros, static_cast<size_t>(left), 1, fp);
    fclose(fp);

    MappedFileRW *cache = new MappedFileRW(g_app.ResolveDataPath(const_cast<char *>("cache.uo")),
                                           static_cast<DWORD>(cacheBytes));
    cacheStream = cache;
    uint8_t *view = OpenReadWrite(cache);
    base = view;
    header = reinterpret_cast<CacheHeader *>(view);
    entries = reinterpret_cast<CacheEntry *>(view + entrySize);
    header->lruHead = -1;
    header->lruTail = -1;
    header->freeHead = 0;
    header->freeTail = 0;
    header->unusedHead = 1;
    header->unusedTail = entryCount - 1;

    entries[0].size = dataBytes;
    entries[0].offset = headerBytes;
    entries[0].next = -1;
    entries[0].prev = -1;
    entries[1].next = 2;
    entries[1].prev = -1;
    {
        int n = 2;
        if (entryCount - 1 > 2) {
            int off = 0x30;
            do {
                int nx = n + 1;
                n--;
                *reinterpret_cast<int32_t *>(reinterpret_cast<uint8_t *>(entries) + off + 0xc) = nx;
                *reinterpret_cast<int32_t *>(reinterpret_cast<uint8_t *>(entries) + off + 0x10) = n;
                n = nx;
                off += 0x18;
            } while (n < entryCount - 1);
        }
    }
    entries[entryCount - 1].next = -1;
    entries[entryCount - 1].prev = entryCount - 2;

    // Hash nodes for blocks already in the cache file (dead here: lruHead was just set to -1).
    {
        int idx = header->lruHead;
        if (idx != -1) {
            for (;;) {
                CacheNode *node = g_nodeFreeList;
                g_nodeFreeList = node->next;
                node->flag = 0;
                node->file = g_cacheEntries[idx].file;
                node->entry = idx;
                node->key = g_cacheEntries[idx].key;
                int slot = (node->file << 12) + (node->key & 0xfff);
                CacheNode *head = g_hashTable[slot];
                node->prev = nullptr;
                node->next = head;
                if (head != nullptr)
                    head->prev = node;
                g_hashTable[(node->file << 12) + (node->key & 0xfff)] = node;
                idx = entries[idx].next;
                if (idx == -1)
                    break;
            }
        }
    }

    // Open the 17 files: hard disk first, then the CD path.
    k = 0;
    do {
        const FileDesc *d = &g_fileDesc[k];
        MappedFile *obj;

        fp = fopen(g_app.ResolveDataPath(const_cast<char *>(d->name)), "rb");
        if (fp != nullptr) {
            fclose(fp);
            g_fileOnHD[k] = 1;
            obj = new MappedFile(g_app.ResolveDataPath(const_cast<char *>(d->name)));
            streams[k] = obj;
        } else {
            fp = fopen(g_app.ResolveDataPath2(const_cast<char *>(d->name)), "rb");
            if (fp != nullptr) {
                fclose(fp);
                g_fileOnHD[k] = 0;
                obj = new MappedFile(g_app.ResolveDataPath2(const_cast<char *>(d->name)));
                streams[k] = obj;
            } else {
                if (k > 0) {
                    int cnt = k;
                    int j = 0;
                    do {
                        streams[j]->Close();
                        j++;
                        cnt--;
                    } while (cnt != 0);
                }
                sprintf(msg, "Failed to open '%s' both from CD and HD.", d->name);
                ErrorBox(msg, "Unable to open file", 1);
            }
        }
        views[k] = OpenReadOnly(streams[k]);
        k++;
    } while (k < 17);
}
HOOK_METHOD(0x0043faf0, CFileManager, Init);

// 0x004400f0  size 0x114
// Only for CD-resident files: find the cache node of the block holding `offset`, take its entry out of the LRU
// list, drop the node into the free list and give the region back.
void __thiscall CFileManager::Invalidate(int file, int offset)
{
    if (g_fileOnHD[file] != 0)
        return;

    int key = offset - offset % g_fileDesc[file].blockSize;
    CacheNode *node = g_hashTable[(file << 12) + (key & 0xfff)];
    while (node != nullptr && node->key != key)
        node = node->next;
    if (node == nullptr)
        return;

    int idx = node->entry;
    CacheEntry *e = entries;
    int pv = e[idx].prev;
    if (pv != -1)
        e[pv].next = e[idx].next;
    else
        header->lruHead = e[idx].next;
    e = entries;
    int nx = e[idx].next;
    if (nx != -1)
        e[nx].prev = e[idx].prev;
    else
        header->lruTail = e[idx].prev;

    CacheNode *after = node->next;
    node->key = node->key & 0xfff;
    if (after != nullptr)
        after->prev = node->prev;
    CacheNode *before = node->prev;
    if (before != nullptr)
        before->next = node->next;
    else
        g_hashTable[(node->file << 12) + node->key] = node->next;
    node->next = g_nodeFreeList;
    g_nodeFreeList = node;
    FreeRegion(idx);
}
HOOK_METHOD(0x004400f0, CFileManager, Invalidate);

// 0x00440210  size 0x26e
// Index record `rec` of idxFile is (offset, length, extra). The new data replaces the old in place when it fits
// (length 0 or less marks the record empty: offset -1, length 0), otherwise it is appended to the data file. Both
// files are closed, rewritten with "r+b" and mapped again. Both files must be on the hard disk, else nothing happens
// after the cache invalidation.
void __thiscall CFileManager::ReplaceRecord(int idxFile, int dataFile, int rec, int extra, void *data, int len)
{
    int offsetV;
    int oldLen;
    FILE *ix;
    FILE *df;

    Invalidate(dataFile, rec);
    if (g_fileOnHD[idxFile] == 0)
        return;
    if (g_fileOnHD[dataFile] == 0)
        return;
    Invalidate(idxFile, rec);
    streams[idxFile]->Close();
    ix = fopen(g_app.ResolveDataPath(const_cast<char *>(g_fileDesc[idxFile].name)), "r+b");
    if (ix == nullptr)
        ErrorBox(g_004a88fc, "Error", 1);
    fseek(ix, g_fileDesc[idxFile].recSize * rec, 0);
    fread(&offsetV, 4, 1, ix);
    fread(&oldLen, 4, 1, ix);
    if (oldLen < 0)
        oldLen = 0;
    fseek(ix, -8, 1);

    if (oldLen < len) {
        streams[dataFile]->Close();
        df = fopen(g_app.ResolveDataPath(const_cast<char *>(g_fileDesc[dataFile].name)), "r+b");
        fseek(df, 0, 2);
        offsetV = ftell(df);
        fwrite(&offsetV, 4, 1, ix);
        fwrite(&len, 4, 1, ix);
        fwrite(&extra, 4, 1, ix);
        fwrite(data, static_cast<size_t>(len), 1, df);
    } else {
        if (len <= 0) {
            offsetV = -1;
            len = 0;
        }
        fwrite(&offsetV, 4, 1, ix);
        fwrite(&len, 4, 1, ix);
        fwrite(&extra, 4, 1, ix);
        if (len == 0) {
            fclose(ix);
            views[idxFile] = OpenReadOnly(streams[idxFile]);
            return;
        }
        streams[dataFile]->Close();
        df = fopen(g_app.ResolveDataPath(const_cast<char *>(g_fileDesc[dataFile].name)), "r+b");
        fseek(df, offsetV, 0);
        fwrite(data, static_cast<size_t>(len), 1, df);
    }
    fclose(df);
    views[dataFile] = OpenReadOnly(streams[dataFile]);
    fclose(ix);
    views[idxFile] = OpenReadOnly(streams[idxFile]);
}
HOOK_METHOD(0x00440210, CFileManager, ReplaceRecord);

// 0x00440480  size 0x148
// Verdata patches first (a patched-empty record gives 0); then a hard-disk file is indexed directly; otherwise the
// block holding the record is looked up in the cache (and read from the CD when missing).
uint8_t *__thiscall CFileManager::GetRecordPtr(int file, int index)
{
    if (g_verdataEnabled != 0) {
        VerNode *v = g_verdataTable[(index & 0x3ff) + (file << 10)];
        if (v != nullptr) {
            do {
                if (v->index == index) {
                    if (v->length != 0)
                        return v->offset + g_verdataBase;
                    return nullptr;
                }
                v = v->next;
            } while (v != nullptr);
        }
    }
    if (g_fileOnHD[file] != 0)
        return g_fileDesc[file].recSize * index + views[file];

    int key = index - index % g_fileDesc[file].blockSize;
    CacheNode *node = g_hashTable[(file << 12) + (key & 0xfff)];
    if (node == nullptr)
        return ReadBlock(file, index);
    do {
        if (node->key == key)
            break;
        node = node->next;
    } while (node != nullptr);
    if (node == nullptr)
        return ReadBlock(file, index);

    if (node->flag == 0)
        g_fileManager.TouchLRU(node->entry);
    if (node->key == index)
        return g_cacheEntries[node->entry].offset + g_cacheBase;
    return (index - node->key) * g_fileDesc[node->file].recSize + g_cacheEntries[node->entry].offset + g_cacheBase;
}
HOOK_METHOD(0x00440480, CFileManager, GetRecordPtr);

// 0x004405d0  size 0x262
// Record `index` of fileA is an index record {offset, length, extra} into fileB. Verdata patches are keyed by
// fileB. Both on the hard disk: pointer into fileB's mapping. Otherwise the record comes from GetRecordPtr and
// the data is cached under (fileB, index).
uint8_t *__thiscall CFileManager::GetRecord(int fileA, int fileB, int index, int *outLen, int *outExtra)
{
    int32_t *rec;
    int offset;
    int length;

    if (g_verdataEnabled != 0) {
        VerNode *v = g_verdataTable[(index & 0x3ff) + (fileB << 10)];
        if (v != nullptr) {
            do {
                if (v->index == index) {
                    *outLen = v->length;
                    *outExtra = v->extra;
                    if (v->length != 0)
                        return v->offset + g_verdataBase;
                    return nullptr;
                }
                v = v->next;
            } while (v != nullptr);
        }
    }

    if (g_fileOnHD[fileA] != 0 && g_fileOnHD[fileB] != 0) {
        int recOff = g_fileDesc[fileA].recSize * index;
        rec = reinterpret_cast<int32_t *>(views[fileA] + recOff);
        length = rec[1];
        offset = rec[0];
        *outLen = length;
        *outExtra = rec[2];
        if (offset < 0) {
            *outLen = 0;
            return nullptr;
        }
        return views[fileB] + offset;
    }

    rec = reinterpret_cast<int32_t *>(GetRecordPtr(fileA, index));
    length = rec[1];
    offset = rec[0];
    *outLen = length;
    *outExtra = rec[2];
    if (offset < 0) {
        *outLen = 0;
        return nullptr;
    }
    length = *outLen;
    if (g_fileOnHD[fileB] != 0)
        return views[fileB] + offset;

    CacheNode *node = g_hashTable[(fileB << 12) + (index & 0xfff)];
    if (node != nullptr) {
        do {
            if (node->key == index)
                break;
            node = node->next;
        } while (node != nullptr);
    }
    if (node == nullptr) {
        int ridx = AllocRegion(fileB, index, length);
        uint8_t *dest = entries[ridx].offset + base;
        MappedRead(streams[fileB], offset, length, dest);
        CacheNode *fresh = g_nodeFreeList;
        g_nodeFreeList = fresh->next;
        fresh->flag = 0;
        fresh->file = g_cacheEntries[ridx].file;
        fresh->entry = ridx;
        int key = g_cacheEntries[ridx].key;
        fresh->key = key;
        int slot = (fresh->file << 12) + (key & 0xfff);
        CacheNode *head = g_hashTable[slot];
        fresh->prev = nullptr;
        fresh->next = head;
        if (head != nullptr)
            head->prev = fresh;
        g_hashTable[(fresh->file << 12) + (key & 0xfff)] = fresh;
        return dest;
    }

    if (node->flag == 0)
        g_fileManager.TouchLRU(node->entry);
    if (node->key == index)
        return g_cacheEntries[node->entry].offset + g_cacheBase;
    return (index - node->key) * g_fileDesc[node->file].recSize + g_cacheEntries[node->entry].offset + g_cacheBase;
}
HOOK_METHOD(0x004405d0, CFileManager, GetRecord);

// 0x00440840  size 0x1a7
// Read the whole cache block that contains record `index` of a CD-resident file into a fresh region and hash it.
uint8_t *__thiscall CFileManager::ReadBlock(int file, int index)
{
    int blockSize = g_fileDesc[file].blockSize;
    int recSize = g_fileDesc[file].recSize;
    int key = index;
    int ridx;
    uint8_t *dest;
    if (blockSize > 1)
        key = index - index % blockSize;

    if (blockSize == 1) {
        ridx = AllocRegion(file, key, recSize);
        dest = entries[ridx].offset + base;
        MappedRead(streams[file], key * recSize, recSize, dest);
        CacheNode *node = g_nodeFreeList;
        g_nodeFreeList = node->next;
        node->flag = 0;
        node->file = g_cacheEntries[ridx].file;
        int k2 = g_cacheEntries[ridx].key;
        node->entry = ridx;
        node->key = k2;
        int slot = (node->file << 12) + (k2 & 0xfff);
        CacheNode *head = g_hashTable[slot];
        node->prev = nullptr;
        node->next = head;
        if (head != nullptr)
            head->prev = node;
        g_hashTable[(node->file << 12) + (k2 & 0xfff)] = node;
        return dest;
    }

    int total = recSize * blockSize;
    ridx = AllocRegion(file, key, total);
    dest = entries[ridx].offset + base;
    MappedRead(streams[file], key * recSize, total, dest);
    CacheNode *node = g_nodeFreeList;
    g_nodeFreeList = node->next;
    node->flag = 0;
    node->file = g_cacheEntries[ridx].file;
    int k2 = g_cacheEntries[ridx].key;
    node->entry = ridx;
    node->key = k2;
    int slot = (node->file << 12) + (k2 & 0xfff);
    CacheNode *head = g_hashTable[slot];
    node->prev = nullptr;
    node->next = head;
    if (head != nullptr)
        head->prev = node;
    g_hashTable[(node->file << 12) + (k2 & 0xfff)] = node;
    return (index - key) * recSize + dest;
}
HOOK_METHOD(0x00440840, CFileManager, ReadBlock);

// 0x004409f0  size 0x1a5
// Find the first free region (lowest offset first) that is big enough, evicting least recently used blocks until
// one is. A region of exactly the size is taken whole; a bigger one gives up its tail to an unused entry. The
// entry becomes the head of the LRU list, tagged (file, key).
int __thiscall CFileManager::AllocRegion(int file, int key, int size)
{
    int r = header->freeHead;
    if (r != -1) {
        CacheEntry *e = entries;
        for (;;) {
            if (e[r].size >= size)
                break;
            r = e[r].next;
            if (r == -1)
                break;
        }
    }
    if (r == -1) {
        do {
            r = EvictLRU();
        } while (entries[r].size < size);
    }
    if (header->unusedHead == -1)
        r = EvictLRU();

    CacheEntry *ents = entries;
    if (ents[r].size == size) {
        int nx = ents[r].next;
        if (nx != -1)
            ents[nx].prev = ents[r].prev;
        else
            header->freeTail = ents[r].prev;
        ents = entries;
        int pv = ents[r].prev;
        if (pv != -1)
            ents[pv].next = ents[r].next;
        else
            header->freeHead = ents[r].next;
    } else {
        int n = header->unusedHead;
        header->unusedHead = ents[n].next;
        if (header->unusedHead != -1)
            entries[header->unusedHead].prev = -1;
        else
            header->unusedTail = -1;
        entries[n].size = size;
        entries[n].offset = entries[r].offset - size + entries[r].size;
        entries[r].size = entries[r].size - size;
        r = n;
    }

    entries[r].prev = -1;
    entries[r].next = header->lruHead;
    int h = header->lruHead;
    if (h != -1)
        entries[h].prev = r;
    else
        header->lruTail = r;
    header->lruHead = r;
    entries[r].file = file;
    entries[r].key = key;
    return r;
}
HOOK_METHOD(0x004409f0, CFileManager, AllocRegion);

// 0x00440ba0  size 0xf8
// Evict the oldest block (error box and exit when there is none): drop its hash node, take its entry out of the LRU
// list and free its region.
int __thiscall CFileManager::EvictLRU()
{
    if (header->lruTail == -1)
        ErrorBox(g_004a89f8, "Caching Error", 1);

    int t = header->lruTail;
    int key = entries[t].key;
    for (CacheNode *node = g_hashTable[(entries[t].file << 12) + (key & 0xfff)]; node != nullptr; node = node->next) {
        if (node->key == key) {
            int nodeKey = node->key;
            CacheNode *after = node->next;
            node->key = nodeKey & 0xfff;
            if (after != nullptr)
                after->prev = node->prev;
            CacheNode *before = node->prev;
            if (before != nullptr)
                before->next = node->next;
            else
                g_hashTable[(node->file << 12) + node->key] = node->next;
            node->next = g_nodeFreeList;
            g_nodeFreeList = node;
            break;
        }
    }

    int pv = entries[t].prev;
    if (pv != -1)
        entries[pv].next = entries[t].next;
    else
        header->lruHead = -1;
    header->lruTail = entries[t].prev;
    return FreeRegion(t);
}
HOOK_METHOD(0x00440ba0, CFileManager, EvictLRU);

// 0x00440ca0  size 0x302
// Put region `idx` into the free list (sorted by offset). It is merged into the region before it when that ends
// where it starts, or into the one after it when only that one touches it (when a previous region exists but does
// not touch, no merge with the next is tried). A merged-away entry goes back to the unused list. Returns the entry
// index that now holds the region.
int __thiscall CFileManager::FreeRegion(int idx)
{
    CacheEntry *e = entries;
    int next = header->freeHead;
    if (next != -1) {
        int off = e[idx].offset;
        for (;;) {
            if (e[next].offset > off)
                break;
            next = e[next].next;
            if (next == -1)
                break;
        }
    }
    int prev;
    if (next == -1)
        prev = header->freeTail;
    else
        prev = e[next].prev;

    int result = idx;
    bool merged = false;

    if (next == -1) {
        if (prev != -1) {
            CacheEntry *pe = &e[prev];
            int psize = pe->size;
            if (pe->offset + psize == entries[idx].offset) {
                pe->size = psize + entries[idx].size;
                result = prev;
                entries[idx].next = header->unusedHead;
                entries[idx].prev = -1;
                header->unusedHead = idx;
                if (header->unusedTail == -1)
                    header->unusedTail = idx;
                merged = true;
            }
        }
    } else if (prev == -1) {
        int off = e[idx].offset;
        if (off + e[idx].size == entries[next].offset) {
            result = next;
            entries[next].offset = off;
            entries[next].size = entries[next].size + entries[idx].size;
            entries[idx].next = header->unusedHead;
            entries[idx].prev = -1;
            header->unusedHead = idx;
            if (header->unusedTail == -1)
                header->unusedTail = idx;
            merged = true;
        }
    } else {
        CacheEntry *pe = &e[prev];
        int psize = pe->size;
        if (psize + pe->offset == e[idx].offset) {
            pe->size = psize + e[idx].size;
            result = prev;
            entries[idx].next = header->unusedHead;
            entries[idx].prev = -1;
            header->unusedHead = idx;
            if (header->unusedTail == -1)
                header->unusedTail = idx;
            merged = true;

            if (pe->offset + pe->size == entries[next].offset) {
                pe->size = pe->size + entries[next].size;
                pe->next = entries[next].next;
                int nn = entries[next].next;
                if (nn != -1)
                    entries[nn].prev = prev;
                entries[next].next = header->unusedHead;
                entries[next].prev = -1;
                header->unusedHead = next;
                if (header->unusedTail == -1)
                    header->unusedTail = next;
            }
        }
    }

    if (!merged) {
        entries[idx].prev = prev;
        entries[idx].next = next;
        if (prev != -1)
            entries[prev].next = idx;
        else
            header->freeHead = idx;
        if (next != -1)
            entries[next].prev = idx;
        else
            header->freeTail = idx;
    }
    return result;
}
HOOK_METHOD(0x00440ca0, CFileManager, FreeRegion);

// 0x00440fb0  size 0xb0
void __thiscall CFileManager::TouchLRU(int idx)
{
    if (header->lruHead == idx)
        return;

    CacheEntry *e = entries;
    int nx = e[idx].next;
    if (nx != -1)
        e[nx].prev = e[idx].prev;
    else
        header->lruTail = e[idx].prev;
    e = entries;
    e[e[idx].prev].next = e[idx].next;
    entries[idx].next = header->lruHead;
    entries[idx].prev = -1;
    int h = header->lruHead;
    if (h != -1) {
        entries[h].prev = idx;
        header->lruHead = idx;
        return;
    }
    header->lruTail = idx;
    header->lruHead = idx;
}
HOOK_METHOD(0x00440fb0, CFileManager, TouchLRU);
