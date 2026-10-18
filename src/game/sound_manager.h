// CSoundManager and CSoundVoice: DirectSound playback. The one manager lives
// at 0x0068db30; it owns 8 streaming voices (64KB looping secondary buffers).
// Layout from decomp/pass2/0x0047f4b0.cpp and the other sound methods.
//
// The interface pointers (IDirectSound, IDirectSoundBuffer) are kept as
// void * and called through com_call.h with their vtable byte offsets:
//   IDirectSound:       0x08 Release, 0x0c CreateSoundBuffer, 0x18 SetCooperativeLevel
//   IDirectSoundBuffer: 0x08 Release, 0x10 GetCurrentPosition, 0x18 GetVolume, 0x24 GetStatus,
//                       0x2c Lock, 0x30 Play, 0x38 SetFormat, 0x3c SetVolume, 0x40 SetPan,
//                       0x44 SetFrequency, 0x48 Stop, 0x4c Unlock, 0x50 Restore
#pragma once

#include <cstddef>
#include <cstdint>

#include <windows.h>
#include <mmsystem.h>

#include "../runtime/ported.h"
#include "globals.h"

class CSoundVoice;

// Chain node of the sound-name hash table (operator new(0xc)).
struct SoundNameNode {
    char *name;            // +0x00
    int index;             // +0x04  index in the name table at 0x004aa128
    SoundNameNode *next;   // +0x08
};
static_assert(sizeof(SoundNameNode) == 0xc, "SoundNameNode size");

// DSBUFFERDESC as the 1998 DirectSound used it (5 dwords).
struct DsBufferDesc {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwBufferBytes;
    DWORD dwReserved;
    WAVEFORMATEX *lpwfxFormat;
};
static_assert(sizeof(DsBufferDesc) == 0x14, "DsBufferDesc size");

// One streaming voice. operator new(0x3c) in CSoundManager::Init.
class CSoundVoice {
public:
    // 0x00480330: create the 64KB looping secondary buffer, zero it and start it.
    CSoundVoice();
    // 0x00480450: start sound `soundId` (PCM record `data`/`length`) on this voice.
    void __thiscall Start(int id, void *data, int length, long pan, long vol, int loopFlag, long frequency);
    // 0x00480800: per-frame refill of one half of the streaming buffer.
    void __thiscall Update();

    void *dsBuffer;        // +0x00  IDirectSoundBuffer *
    uint32_t startTick;    // +0x04
    int half;              // +0x08  which half of the buffer is written next (0 low / 1 high)
    int unk_0c;            // +0x0c
    int consumed;          // +0x10  bytes of the sound already copied
    int remaining;         // +0x14  bytes of the sound (length - 0x28)
    int loop;              // +0x18
    int loopStart;         // +0x1c
    int total;             // +0x20  bytes of the sound (length - 0x28)
    int soundId;           // +0x24  0xffff when idle
    void *rawData;         // +0x28  data argument of Start
    int rawLength;         // +0x2c
    int doneLow;           // +0x30  low half finished
    int doneHigh;          // +0x34  high half finished
    float volumeScale;     // +0x38
};

static_assert(offsetof(CSoundVoice, startTick) == 0x04, "CSoundVoice::startTick");
static_assert(offsetof(CSoundVoice, half) == 0x08, "CSoundVoice::half");
static_assert(offsetof(CSoundVoice, consumed) == 0x10, "CSoundVoice::consumed");
static_assert(offsetof(CSoundVoice, remaining) == 0x14, "CSoundVoice::remaining");
static_assert(offsetof(CSoundVoice, loop) == 0x18, "CSoundVoice::loop");
static_assert(offsetof(CSoundVoice, loopStart) == 0x1c, "CSoundVoice::loopStart");
static_assert(offsetof(CSoundVoice, total) == 0x20, "CSoundVoice::total");
static_assert(offsetof(CSoundVoice, soundId) == 0x24, "CSoundVoice::soundId");
static_assert(offsetof(CSoundVoice, rawData) == 0x28, "CSoundVoice::rawData");
static_assert(offsetof(CSoundVoice, doneLow) == 0x30, "CSoundVoice::doneLow");
static_assert(offsetof(CSoundVoice, doneHigh) == 0x34, "CSoundVoice::doneHigh");
static_assert(offsetof(CSoundVoice, volumeScale) == 0x38, "CSoundVoice::volumeScale");
static_assert(sizeof(CSoundVoice) == 0x3c, "CSoundVoice size");

class CSoundManager {
public:
    // 0x0047f4b0: settings, name->index hash table (0x125 buckets), default format, clear voice slots.
    CSoundManager *__thiscall Ctor();
    // 0x0047f680
    void __thiscall Dtor();
    // 0x0047f770
    void __thiscall Init(HWND hwnd);
    // 0x0047f9e0
    void __thiscall SetVolume(unsigned char vol);
    // 0x0047fa80
    void __thiscall Enable(int enable);
    // 0x0047fc20: ECX (this) is unused
    void __thiscall ShowError(const char *msg, long hr, const char *extra);
    // 0x004800f0: returns the voice index or -1
    int __thiscall PlaySound(int soundId, long pan, long vol, int loopFlag, int freqJitter, int allowDuplicate);
    // 0x0047f960: release the voices, the primary buffer and IDirectSound.
    void __thiscall Shutdown();
    // 0x0047f9d0: primary buffer exists.
    int __thiscall IsInitialized();
    // 0x0047fef0: per-frame voice refill and ambient/crossfade volume ramps.
    void __thiscall Update();
    // 0x00480250: switch the looping ambient sound, keeping the previous one for fading out.
    void __thiscall SetAmbientSound(int soundId);

    void *pDirectSound;           // +0x00  IDirectSound *
    WAVEFORMATEX wfx;             // +0x04  18 bytes, default PCM stereo 22048 Hz 16 bit
    uint8_t pad_016[2];
    int enabled;                  // +0x18  sound enabled (from DAT_004a8f54)
    uint8_t volume;               // +0x1c  (from DAT_004a8f58)
    uint8_t volumeB;              // +0x1d
    uint8_t pad_01e[2];
    SoundNameNode **hashTable;    // +0x20
    int bucketCount;              // +0x24  0x125
    uint8_t unk_28;               // +0x28
    uint8_t pad_029[3];
    void *primaryBuffer;          // +0x2c  IDirectSoundBuffer *
    CSoundVoice *voices[8];       // +0x30
    uint16_t ambientSound;        // +0x50
    uint8_t pad_052[2];
    int ambientVoice;             // +0x54  voice index, -1 none
    int ambientParam;             // +0x58  passed as the volume of the ambient PlaySound
    uint16_t unk_5c;              // +0x5c
    uint8_t pad_05e[2];
    int currentVoice;             // +0x60  -1 none
    int unk_64;                   // +0x64
    int unk_68[8];                // +0x68  filled with -1 by the constructor
};

static_assert(sizeof(WAVEFORMATEX) == 18, "WAVEFORMATEX must be packed to 18 bytes");
static_assert(offsetof(CSoundManager, wfx) == 0x04, "CSoundManager::wfx");
static_assert(offsetof(CSoundManager, enabled) == 0x18, "CSoundManager::enabled");
static_assert(offsetof(CSoundManager, volume) == 0x1c, "CSoundManager::volume");
static_assert(offsetof(CSoundManager, volumeB) == 0x1d, "CSoundManager::volumeB");
static_assert(offsetof(CSoundManager, hashTable) == 0x20, "CSoundManager::hashTable");
static_assert(offsetof(CSoundManager, bucketCount) == 0x24, "CSoundManager::bucketCount");
static_assert(offsetof(CSoundManager, unk_28) == 0x28, "CSoundManager::unk_28");
static_assert(offsetof(CSoundManager, primaryBuffer) == 0x2c, "CSoundManager::primaryBuffer");
static_assert(offsetof(CSoundManager, voices) == 0x30, "CSoundManager::voices");
static_assert(offsetof(CSoundManager, ambientSound) == 0x50, "CSoundManager::ambientSound");
static_assert(offsetof(CSoundManager, ambientVoice) == 0x54, "CSoundManager::ambientVoice");
static_assert(offsetof(CSoundManager, ambientParam) == 0x58, "CSoundManager::ambientParam");
static_assert(offsetof(CSoundManager, unk_5c) == 0x5c, "CSoundManager::unk_5c");
static_assert(offsetof(CSoundManager, currentVoice) == 0x60, "CSoundManager::currentVoice");
static_assert(offsetof(CSoundManager, unk_64) == 0x64, "CSoundManager::unk_64");
static_assert(offsetof(CSoundManager, unk_68) == 0x68, "CSoundManager::unk_68");

#define g_soundManager GLOBAL_AS(CSoundManager, g_soundManager_storage)
