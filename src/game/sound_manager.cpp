// CSoundManager / CSoundVoice (DirectSound). See sound_manager.h for the
// layout and the COM vtable offsets used.
#include "sound_manager.h"

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "com_call.h"
#include "globals.h"
#include "crt.h"
#include "sound_ops_ports.h"
#include "file_manager.h"
#include <dsound.h>
#include "mouse_manager.h"

#pragma comment(lib, "winmm.lib")

// Original rand() (0x0048c6c0) keeps its own state.

// Sound lock bracket (critical section 0x00758288, busy flag 0x00758a14).
#define SoundLockAcquire() (LockSoundBuffers())
#define SoundLockRelease() (UnlockSoundBuffers())

// Float constants in the image used by the volume math.
// 1.0f at 0x004a0ca8 is written as a literal.

#define g_soundFileTable GLOBAL_PTR(void, g_fileManager_storage)

// Sound file table record fetches (FileMgr, this = 0x0068d980).
#define FileTable_GetItem(self, a1, a2, a3, a4, a5) (reinterpret_cast<CFileManager *>(self)->GetRecord(a1, a2, (int)(a3), (int *)(a4), (int *)(a5)))
#define FileTable_GetRecord(self, a1, a2) ((int *)reinterpret_cast<CFileManager *>(self)->GetRecordPtr(a1, (int)(a2)))

// DirectSoundCreate thunk (IAT jmp).

// IDirectSoundBuffer / IDirectSound vtable byte offsets.
enum {
    kDsRelease = 0x08,
    kDsCreateSoundBuffer = 0x0c,
    kDsSetCooperativeLevel = 0x18,
    kDsbGetCurrentPosition = 0x10,
    kDsbGetVolume = 0x18,
    kDsbGetStatus = 0x24,
    kDsbLock = 0x2c,
    kDsbPlay = 0x30,
    kDsbSetFormat = 0x38,
    kDsbSetVolume = 0x3c,
    kDsbSetPan = 0x40,
    kDsbSetFrequency = 0x44,
    kDsbStop = 0x48,
    kDsbUnlock = 0x4c,
    kDsbRestore = 0x50,
};

// The volume formula shared by Init, SetVolume, Enable and Start:
// ftol((1 - (1 - cur * C1) * scale) * -2500)
static long ComputeBufferVolume(long cur, float scale)
{
    float x = (1.0f - (float)cur * kVolumeC1) * scale;
    return (long)((1.0f - x) * kVolumeRange);
}

// 1/255 scaling of the volume byte, then GetVolume / SetVolume on a voice's buffer.
static void ApplyVoiceVolume(CSoundVoice *voice, int soundVol)
{
    float scale = (float)soundVol * kVolumeByteScale;
    voice->volumeScale = scale;
    long curVol;
    com_call(PORT_RET(long), voice->dsBuffer, kDsbGetVolume, &curVol);
    void *buf = voice->dsBuffer;
    long newVol = ComputeBufferVolume(curVol, scale);
    com_call(PORT_RET(long), buf, kDsbSetVolume, newVol);
}

// 0x0047f4b0  size 0x1c2
// SEH frame (handler 0x0049ba46, state byte around the node allocation) omitted.
// Init settings, build the name->index hash table (0x125 buckets) over the sound-name table at
// 0x004aa128, default WAVEFORMATEX, clear the voice slots.
CSoundManager *__thiscall CSoundManager::Ctor()
{
    int i;
    int hash;
    char **entry;
    const char *name;
    const char *p;
    SoundNameNode *node;
    SoundNameNode *last;

    pDirectSound = nullptr;
    enabled = g_soundEnabledSetting;
    volume = g_soundVolumeSetting;
    volumeB = g_soundVolumeSetting;
    bucketCount = 0x125;
    unk_28 = 0;

    hashTable = (SoundNameNode **)::operator new(0x494);
    memset(hashTable, 0, 0x125 * 4);

    ambientSound = 0;
    ambientVoice = -1;
    unk_5c = 0;
    currentVoice = -1;

    i = 0;
    if (g_soundNameTable[0] != nullptr) {
        entry = g_soundNameTable;
        do {
            name = *entry;
            hash = 0;
            p = name;
            if (*name != '\0') {
                do {
                    hash = (int)(((hash << 6) + (signed char)*p) % bucketCount);
                    p++;
                } while (*p != '\0');
            }

            node = (SoundNameNode *)::operator new(0xc);
            if (node != nullptr) {
                node->name = (char *)::operator new(strlen(name) + 1);
                strcpy(node->name, name);
                node->index = i;
                node->next = nullptr;
            }

            if (hashTable[hash] == nullptr) {
                hashTable[hash] = node;
            } else {
                last = hashTable[hash];
                while (last->next != nullptr)
                    last = last->next;
                last->next = node;
            }

            entry++;
            i++;
        } while (*entry != nullptr);
    }

    pDirectSound = nullptr;
    primaryBuffer = nullptr;
    wfx.wFormatTag = 1;
    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 0x5620;
    wfx.wBitsPerSample = 0x10;
    wfx.nBlockAlign = 4;
    wfx.nAvgBytesPerSec = 0x15880;
    wfx.cbSize = 0;
    memset(voices, 0, 8 * 4);
    memset(unk_68, 0xff, 8 * 4);
    return this;
}
HOOK_METHOD(0x0047f4b0, CSoundManager, Ctor);

// 0x0047f680  size 0xea
// SEH frame (handler 0x0049ba5b) omitted. Release the voices, the primary buffer and DirectSound,
// then free the name table.
void __thiscall CSoundManager::Dtor()
{
    if (primaryBuffer != nullptr) {
        CSoundVoice **slot = &voices[0];
        int n = 8;
        do {
            CSoundVoice *voice = *slot;
            if (voice != nullptr) {
                void *buf = voice->dsBuffer;
                if (buf != nullptr)
                    com_call(PORT_RET(long), buf, kDsRelease);
                operator delete(voice);
            }
            *slot = nullptr;
            slot++;
            n--;
        } while (n != 0);

        com_call(PORT_RET(long), primaryBuffer, kDsRelease);
        void *ds = pDirectSound;
        primaryBuffer = nullptr;
        if (ds != nullptr)
            com_call(PORT_RET(long), ds, kDsRelease);
        pDirectSound = nullptr;
    }

    int i = 0;
    if (bucketCount > 0) {
        do {
            SoundNameNode *head = hashTable[i];
            SoundNameNode *p = head;
            if (head != nullptr) {
                do {
                    SoundNameNode *next = p->next;
                    p = next;
                } while (p != nullptr);   // walks to the end of the chain, result unused
                operator delete(head->name);
                operator delete(head);
            }
            i++;
        } while (i < bucketCount);
    }
    operator delete(hashTable);
}
HOOK_METHOD(0x0047f680, CSoundManager, Dtor);

// 0x0047f770  size 0x1ea
// SEH frame (handler 0x0049ba7b) omitted. DirectSoundCreate, priority cooperative level, primary
// buffer, 8 voices, per-voice volume, start the primary buffer looping.
void __thiscall CSoundManager::Init(HWND hwnd)
{
    char errbuf[256];
    DsBufferDesc desc;
    WAVEFORMATEX fmt;

    DirectSoundCreate(nullptr, reinterpret_cast<IDirectSound **>(&pDirectSound), nullptr);

    if (pDirectSound == nullptr)
        return;

    long hr = com_call(PORT_RET(long), pDirectSound, kDsSetCooperativeLevel, hwnd, (DWORD)3);   // DSSCL_PRIORITY
    if (hr != 0) {
        errbuf[0] = 0;
        if (hr == (long)0x80070057)   // E_INVALIDARG
            sprintf(errbuf, "Window Handle == %d", reinterpret_cast<int>(hwnd));
        ShowError("Failed to set cooperative level", hr, errbuf);
        return;
    }

    desc.dwSize = 0x14;
    desc.dwFlags = 0x81;   // DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME
    desc.dwBufferBytes = 0;
    desc.dwReserved = 0;
    desc.lpwfxFormat = nullptr;
    hr = com_call(PORT_RET(long), pDirectSound, kDsCreateSoundBuffer, &desc, &primaryBuffer, (void *)nullptr);
    if (hr != 0) {
        ShowError("Failed to create primary sound buffer", hr, g_emptyString);
        return;
    }

    memcpy(&fmt, &wfx, 18);
    com_call(PORT_RET(long), primaryBuffer, kDsbSetFormat, &fmt);   // result ignored

    CSoundVoice **slot = &voices[0];
    int n = 8;
    do {
        *slot = new CSoundVoice();
        slot++;
        n--;
    } while (n != 0);

    if (enabled == 0)
        return;
    if (primaryBuffer == nullptr)
        return;

    CSoundVoice **p = &voices[0];
    int cnt = 8;
    do {
        CSoundVoice *voice = *p;
        if (voice != nullptr)
            ApplyVoiceVolume(voice, volume);
        p++;
        cnt--;
    } while (cnt != 0);

    hr = com_call(PORT_RET(long), primaryBuffer, kDsbPlay, (DWORD)0, (DWORD)0, (DWORD)1);   // DSBPLAY_LOOPING
    if (hr != 0)
        ShowError("Failed to start playing primary buffer", hr, g_emptyString);
}
HOOK_METHOD(0x0047f770, CSoundManager, Init);

// 0x0047f9e0  size 0x94
// Store the volume byte and rescale every voice.
void __thiscall CSoundManager::SetVolume(unsigned char vol)
{
    volume = vol;
    if (primaryBuffer == nullptr)
        return;

    CSoundVoice **p = &voices[0];
    int n = 8;
    do {
        CSoundVoice *voice = *p;
        if (voice != nullptr)
            ApplyVoiceVolume(voice, volume);
        p++;
        n--;
    } while (n != 0);
}
HOOK_METHOD(0x0047f9e0, CSoundManager, SetVolume);

// 0x0047fa80  size 0x196
// Enable or disable sound. Note the loops run over the global manager (0x0068db30), not over `this`.
void __thiscall CSoundManager::Enable(int enable)
{
    enabled = enable;
    if (enable != 0) {
        if (g_soundManager.primaryBuffer != nullptr) {
            unsigned off = 0x30;
            do {
                CSoundVoice *voice = *(CSoundVoice **)((char *)&g_soundManager + off);
                if (voice != nullptr)
                    ApplyVoiceVolume(voice, g_soundManager.volume & 0xff);
                off += 4;
            } while (off < 0x50);

            long hr = com_call(PORT_RET(long), g_soundManager.primaryBuffer, kDsbPlay, (DWORD)0, (DWORD)0, (DWORD)1);
            if (hr != 0)
                g_soundManager.ShowError("Failed to start playing primary buffer", hr, g_emptyString);
        }
        uint16_t ambientId = ambientSound;
        if (ambientId != 0 && ambientId != 0xffff && ambientVoice == -1)
            ambientVoice = PlaySound(ambientId, 0, ambientParam, 1, 0, 0);
        return;
    }

    // disable
    unk_5c = 0;
    unk_64 = -10000;
    if (primaryBuffer != nullptr && currentVoice != -1) {
        CSoundVoice *cv = voices[currentVoice];
        com_call(PORT_RET(long), cv->dsBuffer, kDsbStop);
        currentVoice = -1;
    }
    ambientParam = -2500;
    if (g_soundManager.primaryBuffer != nullptr) {
        unsigned off = 0x30;
        do {
            CSoundVoice *voice = *(CSoundVoice **)((char *)&g_soundManager + off);
            if (voice != nullptr)
                com_call(PORT_RET(long), voice->dsBuffer, kDsbStop);
            off += 4;
        } while (off < 0x50);
        long hr = com_call(PORT_RET(long), g_soundManager.primaryBuffer, kDsbStop);
        if (hr != 0)
            g_soundManager.ShowError("Failed to stop playing primary buffer", hr, g_emptyString);
    }
}
HOOK_METHOD(0x0047fa80, CSoundManager, Enable);

// 0x0047fc20  size 0x225
// Show a DirectSound error dialog: msg + " " + DSERR name + " " + extra. `this` is unused.
// The original maps hr through a byte index table (0x0047fe6c) and jump table; the switch below
// has the same 14 named cases (table bytes read from the image).
void __thiscall CSoundManager::ShowError(const char *msg, long hr, const char *extra)
{
    char buf[0x800];

    strcpy(buf, msg);
    strcat(buf, " ");

    switch (hr) {
    case (long)0x8878001eu: strcat(buf, "(DSERR_CONTROLUNAVAIL)"); break;
    case (long)0x88780032u: strcat(buf, "(DSERR_INVALIDCALL)"); break;
    case (long)0x88780046u: strcat(buf, "(DSERR_PRIOLEVELNEEDED)"); break;
    case (long)0x88780064u: strcat(buf, "(DSERR_BADFORMAT)"); break;
    case (long)0x88780078u: strcat(buf, "(DSERR_NODRIVER)"); break;
    case (long)0x88780082u: strcat(buf, "(DSERR_ALREADYINITIALIZED)"); break;
    case (long)0x88780096u: strcat(buf, "(DSERR_BUFFERLOST)"); break;
    case (long)0x887800a0u: strcat(buf, "(DSERR_OTHERAPPHASPRIO)"); break;
    case (long)0x8878000au: strcat(buf, "(DSERR_ALLOCATED)"); break;
    case (long)0x80070057u: strcat(buf, "(DSERR_INVALIDPARAM)"); break;
    case (long)0x8007000eu: strcat(buf, "(DSERR_OUTOFMEMORY)"); break;
    case (long)0x80040110u: strcat(buf, "(DSERR_NOAGGREGATION)"); break;
    case (long)0x80004005u: strcat(buf, "(DSERR_GENERIC)"); break;
    case (long)0x80004001u: strcat(buf, "(DSERR_UNSUPPORTED)"); break;
    default: break;
    }
    strcat(buf, " ");
    strcat(buf, extra);
    MessageBoxA(nullptr, buf, "DirectSound Error", 0);
}
HOOK_METHOD(0x0047fc20, CSoundManager, ShowError);

// 0x004800f0  size 0x153
// Pick a voice (duplicate check, free voice, else steal the quietest non-ambient one), fetch the
// sound record and start it. Returns the voice index or -1.
int __thiscall CSoundManager::PlaySound(int soundId, long pan, long vol, int loopFlag, int freqJitter, int allowDuplicate)
{
    long curVol;

    if (enabled != 0 && soundId >= 0 && soundId < 0x24b && primaryBuffer != nullptr) {
        if (allowDuplicate == 0) {
            int i = 0;
            CSoundVoice **p = &voices[0];
            do {
                CSoundVoice *v = *p;
                if (v != nullptr) {
                    if (v->soundId == soundId)
                        return i;
                }
                i++;
                p++;
            } while (i < 8);
        }

        CSoundVoice **vp = &voices[0];
        allowDuplicate = -1;   // reused as the chosen voice index
        int k = 0;
        CSoundVoice **q = vp;
        for (; k < 8; k++, q++) {
            CSoundVoice *v = *q;
            if (v != nullptr) {
                if (v->soundId == 0xffff) {
                    allowDuplicate = k;
                    break;
                }
            }
        }

        if (allowDuplicate == -1) {
            // no idle voice: take the one with the lowest volume, skipping the ambient voice
            long best = 0;
            int s = 0;
            do {
                if (s != ambientVoice) {
                    com_call(PORT_RET(long), vp[s]->dsBuffer, kDsbGetVolume, &curVol);
                    if (!(curVol > best)) {
                        best = curVol;
                        allowDuplicate = s;
                    }
                }
                s++;
            } while (s < 8);
        }

        int *rec = FileTable_GetRecord(g_soundFileTable, 7, (unsigned)soundId);
        void *data = (void *)rec[0];
        int length = rec[1];
        if ((int)data != -1 && length > 0x28) {
            int freq = 0;
            if (freqJitter != 0) {
                int r = rand();
                int rem = r % freqJitter;
                float f = (float)rem;
                f *= kJitterPercent;
                f *= kJitterBase;
                int j = (int)f;
                if ((rand() & 1) == 0)
                    j = -j;
                freq = j + 0x5620;
            }

            // ? when no voice was chosen allowDuplicate is still -1 here and the original indexes voices[-1]
            (&voices[0])[allowDuplicate]->Start(soundId, data, length, pan, vol, loopFlag, freq);
            return allowDuplicate;
        }
    }
    return -1;
}
HOOK_METHOD(0x004800f0, CSoundManager, PlaySound);

// 0x00480330  size 0x114
// Create the 64KB looping secondary buffer (mono 16-bit 22048 Hz), zero it and start it.
CSoundVoice::CSoundVoice()
{
    DsBufferDesc desc;
    WAVEFORMATEX fmt;
    void *ptr1;
    void *ptr2;
    DWORD bytes1;
    DWORD bytes2;

    fmt.wFormatTag = 1;
    fmt.nChannels = 1;
    doneLow = 1;
    doneHigh = 1;
    half = 1;
    loopStart = 0;
    total = 0;
    soundId = 0xffff;
    rawData = nullptr;
    rawLength = 0;
    volumeScale = 1.0f;
    dsBuffer = nullptr;
    remaining = 0;
    consumed = 0;
    startTick = 0;
    unk_0c = 0;
    loop = 0;

    desc.lpwfxFormat = &fmt;
    void *ds = g_soundManager.pDirectSound;
    fmt.nSamplesPerSec = 0x5620;
    fmt.wBitsPerSample = 0x10;
    fmt.nBlockAlign = 2;
    fmt.nAvgBytesPerSec = 0xac40;
    fmt.cbSize = 0;
    desc.dwSize = 0x14;
    desc.dwFlags = 0x100e0;   // GETCURRENTPOSITION2 | CTRLPAN | CTRLVOLUME | CTRLFREQUENCY
    desc.dwBufferBytes = 0x10000;
    desc.dwReserved = 0;

    long hr = com_call(PORT_RET(long), ds, kDsCreateSoundBuffer, &desc, &dsBuffer, (void *)nullptr);
    if (hr != 0) {
        g_soundManager.ShowError("Failed to create sound buffer", hr, g_emptyString);
    } else {
        com_call(PORT_RET(long), dsBuffer, kDsbLock, (DWORD)0, (DWORD)0x10000, &ptr1, &bytes1, &ptr2, &bytes2, (DWORD)0);
        memset(ptr1, 0, 0x4000 * 4);
        com_call(PORT_RET(long), dsBuffer, kDsbUnlock, ptr1, bytes1, ptr2, bytes2);
        com_call(PORT_RET(long), dsBuffer, kDsbPlay, (DWORD)0, (DWORD)0, (DWORD)1);
    }
}
HOOK_CTOR(0x00480330, CSoundVoice, ());

// Zero `words` 16-bit samples after the `usedBytes` already copied (REP STOSD + REP STOSW in the original).
static void ZeroTail(void *base, int usedBytes, int words)
{
    uint16_t *dst = (uint16_t *)base + (usedBytes >> 1);
    if (words > 0)
        memset(dst, 0, words * 2);
}

// 0x00480450  size 0x3a9
// Start `id` on this voice: set pan/volume/frequency, then fill the free part of the buffer
// from the current play/write cursors. The sound data starts 0x28 bytes (WAV header) into the record.
void __thiscall CSoundVoice::Start(int id, void *data, int length, long pan, long vol, int loopFlag, long frequency)
{
    DWORD status;
    unsigned localRec;
    DWORD playCursor;
    DWORD writeCursor;
    void *ptr1;
    void *ptr2;
    DWORD bytes2;
    DWORD bytes1;
    const uint8_t *src;

    if (id == 0xffff)
        return;
    if (id < 0)
        return;

    rawData = data;
    rawLength = length;

    soundId = id;
    uint8_t *rec = FileTable_GetItem(g_soundFileTable, 7, 8, (unsigned)id, (unsigned *)&length, &localRec);   // overwrites length
    SoundLockAcquire();

    com_call(PORT_RET(long), dsBuffer, kDsbGetStatus, &status);
    if (status & 0x2)   // DSBSTATUS_BUFFERLOST
        com_call(PORT_RET(long), dsBuffer, kDsbRestore);

    loopStart = 0;
    src = rec + 0x28;
    remaining = length - 0x28;
    total = length - 0x28;
    loop = loopFlag;

    long hr = com_call(PORT_RET(long), dsBuffer, kDsbSetPan, pan);
    if (hr != 0) {
        SoundLockRelease();
        g_soundManager.ShowError("DirectSound SetPan failed.", hr, g_emptyString);
        return;
    }

    {
        long v = ComputeBufferVolume(vol, volumeScale);
        hr = com_call(PORT_RET(long), dsBuffer, kDsbSetVolume, v);
    }
    if (hr != 0) {
        SoundLockRelease();
        g_soundManager.ShowError("DirectSound SetVolume failed.", hr, g_emptyString);
        return;
    }

    hr = com_call(PORT_RET(long), dsBuffer, kDsbSetFrequency, frequency);
    if (hr != 0) {
        SoundLockRelease();
        g_soundManager.ShowError("DirectSound SetFrequency failed.", hr, g_emptyString);
        return;
    }

    half = 1;
    startTick = GetTickCount();
    unk_0c = 0;

    com_call(PORT_RET(long), dsBuffer, kDsbGetCurrentPosition, &playCursor, &writeCursor);

    if (writeCursor < playCursor) {
        com_call(PORT_RET(long), dsBuffer, kDsbLock, writeCursor, playCursor - writeCursor, &ptr1, &bytes1, &ptr2, &bytes2, (DWORD)0);
        if (remaining < (int)bytes1) {
            if (loop != 0) {
                // looping sound
                unsigned copied = 0;
                unsigned srcOff = 0;
                if (bytes1 != 0) {
                    unsigned tot = (unsigned)total;
                    for (;;) {
                        unsigned chunk = tot - srcOff;
                        unsigned want = bytes1 - copied;
                        if (!(chunk < want))
                            chunk = want;
                        memcpy((uint8_t *)ptr1 + copied, src + srcOff, chunk);
                        srcOff += chunk;
                        tot = (unsigned)total;
                        if (!(srcOff < tot))
                            srcOff = (unsigned)loopStart;
                        copied += chunk;
                        if (!(copied < bytes1))
                            break;
                    }
                }
            } else {
                memcpy(ptr1, src, remaining);
                ZeroTail(ptr1, remaining, (int)(bytes1 - remaining) >> 1);
                consumed = remaining;
            }
        } else {
            memcpy(ptr1, src, bytes1);
            consumed = bytes1;
        }
        half = 0;
        doneLow = 0;
    } else {
        // write cursor >= play cursor: lock including the wrap-around
        com_call(PORT_RET(long), dsBuffer, kDsbLock, writeCursor, playCursor - writeCursor + 0x10000, &ptr1, &bytes1, &ptr2, &bytes2, (DWORD)0);
        if (remaining < (int)bytes1) {
            memcpy(ptr1, src, remaining);
            ZeroTail(ptr1, remaining, (int)(bytes1 - remaining) >> 1);
            consumed = remaining;
        } else {
            memcpy(ptr1, src, bytes1);
            unsigned rest = remaining - bytes1;
            if (rest < bytes2) {
                memcpy(ptr2, src + bytes1, rest);
                ZeroTail(ptr2, (int)rest, (int)(bytes2 + bytes1 - remaining) >> 1);
                consumed = remaining;
            } else {
                memcpy(ptr2, src + bytes1, bytes2);
                consumed = bytes2 + bytes1;
            }
        }
        if (playCursor < 0x8000) {
            half = 1;
            doneHigh = 0;
        } else {
            half = 0;
            doneLow = 0;
        }
    }
    com_call(PORT_RET(long), dsBuffer, kDsbUnlock, ptr1, bytes1, ptr2, bytes2);
    if (!(status & 0x1))   // DSBSTATUS_PLAYING not set
        com_call(PORT_RET(long), dsBuffer, kDsbPlay, (DWORD)0, (DWORD)0, (DWORD)1);
    SoundLockRelease();
}
HOOK_METHOD(0x00480450, CSoundVoice, Start);

// 0x00480800  size 0x2d8
// Per-frame: refill the half of the 64KB buffer the play cursor just left, handle loop wrap, and
// retire the voice once both halves are finished.
void __thiscall CSoundVoice::Update()
{
    DWORD status;
    unsigned recLength;
    unsigned recAux;
    DWORD playCursor;
    DWORD writeCursor;
    void *ptr1;
    void *ptr2;
    DWORD bytes1;
    DWORD bytes2;
    DWORD written;
    int want;
    uint8_t *rec;

    if (soundId == 0xffff)
        return;

    rec = FileTable_GetItem(g_soundFileTable, 7, 8, (unsigned)soundId, &recLength, &recAux);
    if (rec == nullptr)
        return;

    SoundLockAcquire();

    com_call(PORT_RET(long), dsBuffer, kDsbGetStatus, &status);
    if (status & 0x2) {   // DSBSTATUS_BUFFERLOST
        com_call(PORT_RET(long), dsBuffer, kDsbRestore);
        com_call(PORT_RET(long), dsBuffer, kDsbLock, (DWORD)0, (DWORD)0x10000, &ptr1, &bytes1, &ptr2, &bytes2, (DWORD)0);
        memset(ptr1, 0, 0x4000 * 4);
        doneLow = 1;
        doneHigh = 1;
        soundId = 0xffff;
        com_call(PORT_RET(long), dsBuffer, kDsbUnlock, ptr1, bytes1, ptr2, bytes2);
        remaining = 0;
        consumed = 0;
        half = 1;
        startTick = 0;
        unk_0c = 0;
        loop = 0;
        com_call(PORT_RET(long), dsBuffer, kDsbPlay, (DWORD)0, (DWORD)0, (DWORD)1);
    } else if (!(status & 0x1)) {   // not playing
        com_call(PORT_RET(long), dsBuffer, kDsbPlay, (DWORD)0, (DWORD)0, (DWORD)1);
    }

    com_call(PORT_RET(long), dsBuffer, kDsbGetCurrentPosition, &playCursor, &writeCursor);

    if (playCursor >= 0x8000) {
        if (half == 0)
            goto end_checks;
        // lock the low half
        com_call(PORT_RET(long), dsBuffer, kDsbLock, (DWORD)0, (DWORD)0x8000, &ptr1, &bytes1, &ptr2, &bytes2, (DWORD)0);
        doneLow = 0;
    } else {
        if (half != 0)
            goto end_checks;
        // lock the high half
        com_call(PORT_RET(long), dsBuffer, kDsbLock, (DWORD)0x8000, (DWORD)0x8000, &ptr1, &bytes1, &ptr2, &bytes2, (DWORD)0);
        doneHigh = 0;
    }

    // fill the locked half
    if (loop != 0 && (int)(bytes1 + consumed) >= total) {
        // looping sound that wraps inside this half
        written = 0;
        if (bytes1 != 0) {
            do {
                unsigned pos = consumed;
                int chunk = total - (int)pos;
                if (chunk == 0) {
                    consumed = loopStart;
                } else {
                    want = (int)(bytes1 - written);
                    if (!(want >= chunk))
                        chunk = want;
                    memcpy((uint8_t *)ptr1 + written, rec + pos + 0x28, chunk);
                    if (chunk == want)
                        consumed = consumed + chunk;
                    else
                        consumed = loopStart;
                    written += chunk;
                }
            } while (written < bytes1);
        }
    } else {
        unsigned pos = (unsigned)consumed;
        unsigned tot = (unsigned)remaining;
        if (pos == tot) {
            // sound finished: silence the whole half
            memset(ptr1, 0, 0x2000 * 4);
            if (half != 0)
                doneLow = 1;
            else
                doneHigh = 1;
        } else {
            int left = (int)(tot - pos);
            if (left >= (int)bytes1) {
                memcpy(ptr1, rec + pos + 0x28, bytes1);
                consumed = consumed + bytes1;
            } else {
                memcpy(ptr1, rec + pos + 0x28, left);
                ZeroTail(ptr1, left, (int)(bytes1 - left) >> 1);
                consumed = remaining;
            }
        }
    }

    com_call(PORT_RET(long), dsBuffer, kDsbUnlock, ptr1, bytes1, ptr2, bytes2);
    half = (half == 0) ? 1 : 0;

end_checks:
    if (doneLow != 0 && doneHigh != 0) {
        loop = 0;
        soundId = 0xffff;
        com_call(PORT_RET(long), dsBuffer, kDsbSetFrequency, (DWORD)0);
    }
    SoundLockRelease();
}
HOOK_METHOD(0x00480800, CSoundVoice, Update);

// Globals of the timer callback.
#define g_soundCS GLOBAL_AS(CRITICAL_SECTION, g_soundLock_storage)
#define g_mainWindowHandle GLOBAL_AS(HWND, g_mainWindow)
#define g_cursorObject GLOBAL_PTR(void, g_mouse_storage)

// 0x00480c40  size 0x11e
// timeSetEvent callback: while the sound lock is free, sync the software cursor to the mouse position.
void __stdcall SoundTimerProc(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
    (void)uMsg;
    (void)dw1;
    (void)dw2;
    POINT pt;

    EnterCriticalSection(&g_soundCS);
    if (g_soundBusy == 0) {
        g_soundBusy = 1;
        LeaveCriticalSection(&g_soundCS);

        if (uTimerID == g_timerId && dwUser == 1 && g_timerReentry == 0) {
            g_timerReentry = 1;
            GetCursorPos(&pt);
            ScreenToClient(g_mainWindowHandle, &pt);
            // out: current cursor x (over uTimerID) and y (over dwUser)
            reinterpret_cast<CMouseManager *>(g_cursorObject)->GetMousePos((int *)(&uTimerID), (int *)(&dwUser));
            reinterpret_cast<CMouseManager *>(g_cursorObject)->UpdateCursorShape();
            if (pt.x != (LONG)uTimerID || pt.y != (LONG)dwUser || reinterpret_cast<CMouseManager *>(g_cursorObject)->GetChangedFlag() != 0) {
                reinterpret_cast<CMouseManager *>(g_cursorObject)->SetPosUpdateShape(pt.x, pt.y);
                reinterpret_cast<CMouseManager *>(g_cursorObject)->DrawCursor(1);
            }
            g_timerReentry = 0;
        }

        EnterCriticalSection(&g_soundCS);
        g_soundBusy = 0;
        LeaveCriticalSection(&g_soundCS);
    } else {
        LeaveCriticalSection(&g_soundCS);
    }
}
HOOK_FUNC(0x00480c40, SoundTimerProc);
