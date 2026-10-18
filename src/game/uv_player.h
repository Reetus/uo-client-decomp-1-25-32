// uV intro video player: entry point, stream container reader, audio chunk handling, ADPCM decoders,
// player main loop, worker thread and DirectSound setup (0x004830e0 - 0x004852a?). Porting pass 3, batch v1.
// Only the fields these procedures touch are declared. The surface layer (batch v2) has its own header.
#pragma once

#include <cstddef>
#include <cstdint>

#include <windows.h>
#include <mmsystem.h>

#include "../runtime/ported.h"

struct UvpState;

// The player object the game owns (the intro keeps one at 0x007582b8). The first dword of UvpState points to it.
struct UvpPlayer {
    uint32_t flags;          // +0x00  0x10000 own thread, 0x20000 / 0x30000 thread priority, 0x40000 frame callback,
                             //        0x80000 caller supplies the MMIO handle, 0x400000 / 0x800000 caller supplies
                             //        the DirectSound primary buffer / object
    UvpState *state;         // +0x04
    uint8_t pad_08[0x10];
    void *surf0;             // +0x18  cleared by the entry point (batch v2 owns it)
    void *surf1;             // +0x1c  cleared by the entry point
    uint8_t pad_20[0x28];
    void *ds;                // +0x48  IDirectSound
    void *dsPrimary;         // +0x4c  IDirectSoundBuffer
    void *dsBuffer;          // +0x50  IDirectSoundBuffer (streaming buffer)
    uint8_t pad_54[0x1c];
    int32_t audioStream;     // +0x70  the container is a sound-carrying one (first tag SEAD / lHCS / 1SNh)
    int32_t flag74;          // +0x74
    uint8_t pad_78[8];
    int32_t elapsedMs;       // +0x80
    int32_t ticks;           // +0x84  incremented once per main loop pass
    int32_t lag;             // +0x88
    int32_t runState;        // +0x8c  Interlocked state machine shared with the caller
    HWND hwnd;               // +0x90
    HMMIO hmmio;             // +0x94
    int32_t threadPriority;  // +0x98
    uintptr_t thread;        // +0x9c  _beginthread handle
    int (__cdecl *callback)(); // +0xa0

    void PlayLoop();         // 0x004842e0, __thiscall
};

struct UvpAudioEntry {
    uint8_t *data;           // +0x00
    int32_t size;            // +0x04
    int32_t stamp;           // +0x08
    int32_t stamp2;          // +0x0c
};

struct UvpVideoEntry {
    uint8_t *data;           // +0x00
    int32_t unk04;           // +0x04
    int32_t stamp;           // +0x08
    int32_t type;            // +0x0c
};

// The decoder state calloc'ed by the entry point (0xb2c bytes). Its first dword points back to the player.
struct UvpState {
    UvpPlayer *player;       // +0x00
    uint8_t pad_04[8];
    uint8_t *videoRing;      // +0x0c  0xaae60 bytes
    uint8_t *audioRing;      // +0x10  0x493e0 bytes
    uint8_t *videoWrite;     // +0x14
    uint8_t *audioWrite;     // +0x18
    uint8_t pad_1c[0x10];
    int32_t channels;        // +0x2c
    int32_t rate;            // +0x30
    int32_t bits;            // +0x34
    int32_t codec;           // +0x38  0 PCM, 7 ADPCM, 0xa IMA ADPCM
    UvpAudioEntry audio[24]; // +0x3c
    UvpVideoEntry video[24]; // +0x1bc
    int32_t soundBufMade;    // +0x33c
    int32_t sbSize;          // +0x340
    int32_t sbWrite;         // +0x344
    int32_t audioPlayed;     // +0x348
    int32_t audioWriteIdx;   // +0x34c
    int32_t videoReadIdx;    // +0x350
    int32_t videoWriteIdx;   // +0x354
    int32_t videoQueued;     // +0x358
    int32_t audioQueued;     // +0x35c
    int32_t count360;        // +0x360
    int32_t count364;        // +0x364
    int32_t syncPos;         // +0x368
    uint8_t pad_36c[4];
    int32_t error;           // +0x370
    int32_t state;           // +0x374
    int32_t lastChunk;       // +0x378
    int32_t firstPos;        // +0x37c
    int32_t tick;            // +0x380
    int32_t threadRun;       // +0x384
    HANDLE thread;           // +0x388
    int32_t fakePos;         // +0x38c
    int32_t noSoundBuf;      // +0x390
    int32_t emulDriver;      // +0x394
    int32_t primaryBytes;    // +0x398
    int32_t ownPrimary;      // +0x39c
    int32_t ownDs;           // +0x3a0
    int32_t active;          // +0x3a4
    int32_t adpcmL1;         // +0x3a8
    int32_t adpcmR1;         // +0x3ac
    int32_t adpcmL2;         // +0x3b0
    int32_t adpcmR2;         // +0x3b4
    int32_t imaPred;         // +0x3b8
    uint8_t pad_3bc[4];
    int32_t imaIndex;        // +0x3c0
    uint8_t pad_3c4[0x1c];
    uint32_t startTick;      // +0x3e0
    uint32_t endTick;        // +0x3e4

    void Pump();             // 0x004830e0
    int ReadChunk();         // 0x00483180
    int CloseAll();          // 0x00483be0
    int StartThread();       // 0x00484c70
    int StopThread();        // 0x00484cb0
    int InitDirectSound();   // 0x00484d00
    int ReleaseDirectSound(); // 0x00484e10
    int CreateSoundBuffer(); // 0x00484e70
    int ReleaseDsBuffer();   // 0x00484fd0
};

static_assert(offsetof(UvpPlayer, state) == 0x04, "UvpPlayer::state");
static_assert(offsetof(UvpPlayer, surf1) == 0x1c, "UvpPlayer::surf1");
static_assert(offsetof(UvpPlayer, ds) == 0x48, "UvpPlayer::ds");
static_assert(offsetof(UvpPlayer, dsBuffer) == 0x50, "UvpPlayer::dsBuffer");
static_assert(offsetof(UvpPlayer, audioStream) == 0x70, "UvpPlayer::audioStream");
static_assert(offsetof(UvpPlayer, elapsedMs) == 0x80, "UvpPlayer::elapsedMs");
static_assert(offsetof(UvpPlayer, runState) == 0x8c, "UvpPlayer::runState");
static_assert(offsetof(UvpPlayer, hmmio) == 0x94, "UvpPlayer::hmmio");
static_assert(offsetof(UvpPlayer, thread) == 0x9c, "UvpPlayer::thread");
static_assert(offsetof(UvpPlayer, callback) == 0xa0, "UvpPlayer::callback");
static_assert(offsetof(UvpState, videoRing) == 0x0c, "UvpState::videoRing");
static_assert(offsetof(UvpState, channels) == 0x2c, "UvpState::channels");
static_assert(offsetof(UvpState, audio) == 0x3c, "UvpState::audio");
static_assert(offsetof(UvpState, video) == 0x1bc, "UvpState::video");
static_assert(offsetof(UvpState, soundBufMade) == 0x33c, "UvpState::soundBufMade");
static_assert(offsetof(UvpState, videoReadIdx) == 0x350, "UvpState::videoReadIdx");
static_assert(offsetof(UvpState, count360) == 0x360, "UvpState::count360");
static_assert(offsetof(UvpState, syncPos) == 0x368, "UvpState::syncPos");
static_assert(offsetof(UvpState, error) == 0x370, "UvpState::error");
static_assert(offsetof(UvpState, threadRun) == 0x384, "UvpState::threadRun");
static_assert(offsetof(UvpState, thread) == 0x388, "UvpState::thread");
static_assert(offsetof(UvpState, noSoundBuf) == 0x390, "UvpState::noSoundBuf");
static_assert(offsetof(UvpState, active) == 0x3a4, "UvpState::active");
static_assert(offsetof(UvpState, adpcmL1) == 0x3a8, "UvpState::adpcmL1");
static_assert(offsetof(UvpState, imaIndex) == 0x3c0, "UvpState::imaIndex");
static_assert(offsetof(UvpState, startTick) == 0x3e0, "UvpState::startTick");
static_assert(sizeof(UvpAudioEntry) == 0x10 && sizeof(UvpVideoEntry) == 0x10, "uv entries");

// Entry points other code reaches by address.
int __cdecl UvpPlayerStart(const char *path, HWND hwnd, UvpPlayer *player);   // 0x00483f40

// The decoders in uv_audio.cpp (hooked there; the chunk reader calls them by name).
void __cdecl UvpAdpcmDecodeMono(int count, int32_t *progress, int32_t *s1p, int32_t *s2p, const uint8_t *data,
                                uint8_t **outp);                                                 // 0x00484560
void __cdecl UvpAdpcmDecodeStereo(int count, int32_t *progress, int32_t *l1p, int32_t *l2p, int32_t *r1p,
                                  int32_t *r2p, const uint8_t *data, uint8_t **outp);            // 0x004846e0
void __cdecl UvpImaAdpcmDecodeStereo(const uint8_t *src, int16_t *dst, int count, int32_t *pred,
                                     int32_t *index);                                            // 0x00485093
