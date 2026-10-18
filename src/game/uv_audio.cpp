// uV intro video player, audio side: the two block ADPCM decoders (0x00484560 mono, 0x004846e0 stereo), the
// audio worker thread (0x00484970), thread start / stop, DirectSound set-up and tear-down, and the two IMA ADPCM
// decoders (0x00484ff0, 0x00485093). Porting pass 3, batch v1. See uv_player.h for the layout.
//
// DirectSound is reached through COM vtable slots (com_call.h): IDirectSound 0x08 Release, 0x0c CreateSoundBuffer,
// 0x10 GetCaps, 0x18 SetCooperativeLevel; IDirectSoundBuffer 0x08 Release, 0x0c GetCaps, 0x10 GetCurrentPosition,
// 0x2c Lock, 0x30 Play, 0x44 SetFrequency, 0x4c Unlock, 0x50 Restore.
#include "uv_player.h"

#include <cstring>

#include <mmsystem.h>

#include "../runtime/ported.h"
#include "com_call.h"
#include "crt.h"
#include <dsound.h>
#include "uv_audio_ports.h"
#include "globals.h"

#pragma comment(lib, "winmm.lib")

// DirectSoundCreate thunk (IAT jmp) at 0x0049869a: (guid, &IDirectSound, outer), __stdcall.

#define UVP_L(x) reinterpret_cast<LONG *>(&(x))

// Tables in the image: ADPCM coefficient pairs (16 dwords each), the IMA step table and index adjust table.
// Scratch globals the stereo IMA decoder keeps its running state in.

static inline int32_t uvp_mulhi_s(int32_t a, int32_t b)
{
    return static_cast<int32_t>((static_cast<int64_t>(a) * static_cast<int64_t>(b)) >> 32);
}

static inline uint32_t uvp_mulhi_u(uint32_t a, uint32_t b)
{
    return static_cast<uint32_t>((static_cast<uint64_t>(a) * static_cast<uint64_t>(b)) >> 32);
}

static inline int32_t uvp_clamp16(int32_t v)
{
    if (v < -0x8000) {
        return -0x8000;
    }
    if (v > 0x7fff) {
        return 0x7fff;
    }
    return v;
}

// (n * mult) / 28 truncated toward zero, the way the compiler's reciprocal multiply does it.
static inline int32_t uvp_div28(int32_t n)
{
    int32_t edx = uvp_mulhi_s(n, static_cast<int32_t>(0x92492493));
    edx = static_cast<int32_t>(static_cast<uint32_t>(edx) + static_cast<uint32_t>(n));
    edx >>= 4;
    return static_cast<int32_t>(static_cast<uint32_t>(edx) + (static_cast<uint32_t>(edx) >> 31));
}

// Number of 28-sample blocks needed for `count` samples: (count + 27) / 28.
static inline uint32_t uvp_blocks(int32_t count)
{
    uint32_t x = static_cast<uint32_t>(count) + 0x1b;
    uint32_t hi = uvp_mulhi_u(x, 0x24924925);
    return (((x - hi) >> 1) + hi) >> 4;
}

// 0x00484560  size 0x179
// Mono block ADPCM: 15-byte blocks (header byte + 14 data bytes) to 28 samples, each written to both channels
// of a 16-bit stereo frame. `progress` counts samples already consumed in the chunk (used only to offset the
// source); s1 / s2 are the two previous samples, out the running output pointer. cdecl, six arguments.
void __cdecl UvpAdpcmDecodeMono(int count, int32_t *progress, int32_t *s1p, int32_t *s2p, const uint8_t *data,
                                       uint8_t **outp)
{
    int32_t progress15 = static_cast<int32_t>(static_cast<uint32_t>(*progress) * 15u);
    data += uvp_div28(progress15);
    int32_t prev1 = *s1p;
    uint8_t *out = *outp;
    int32_t prev2 = *s2p;
    if (count > 0) {
        uint32_t blocks = uvp_blocks(count);
        do {
            *progress += 0x1c;
            uint32_t header = *data;
            uint32_t sel = header >> 4;
            int32_t shift = static_cast<int32_t>(header & 0xf) + 8;
            data++;
            int32_t c1 = uvp_coef1[sel];
            int32_t c2 = uvp_coef2[sel];
            uint8_t *end = out + 0x70;
            if (out < end) {
                do {
                    int32_t t = static_cast<int32_t>(static_cast<uint32_t>(prev2) * static_cast<uint32_t>(c2));
                    int32_t m = static_cast<int32_t>(static_cast<uint32_t>(prev1) * static_cast<uint32_t>(c1));
                    uint32_t d = *data;
                    t = static_cast<int32_t>(static_cast<uint32_t>(t) + static_cast<uint32_t>(m));
                    int32_t nib = static_cast<int32_t>((d >> 4) << 28) >> shift;
                    int32_t v = static_cast<int32_t>(static_cast<uint32_t>(t) + static_cast<uint32_t>(nib) + 0x80u) >> 8;
                    v = uvp_clamp16(v);
                    prev1 = static_cast<int32_t>(static_cast<uint32_t>(prev1) * static_cast<uint32_t>(c2));
                    reinterpret_cast<int16_t *>(out)[0] = static_cast<int16_t>(v);
                    reinterpret_cast<int16_t *>(out)[1] = static_cast<int16_t>(v);
                    prev2 = v;
                    int32_t m2 = static_cast<int32_t>(static_cast<uint32_t>(v) * static_cast<uint32_t>(c1));
                    d = *data;
                    m2 = static_cast<int32_t>(static_cast<uint32_t>(m2) + static_cast<uint32_t>(prev1));
                    int32_t nib2 = static_cast<int32_t>(d << 28) >> shift;
                    int32_t v2 = static_cast<int32_t>(static_cast<uint32_t>(m2) + static_cast<uint32_t>(nib2) + 0x80u) >> 8;
                    v2 = uvp_clamp16(v2);
                    reinterpret_cast<int16_t *>(out)[2] = static_cast<int16_t>(v2);
                    reinterpret_cast<int16_t *>(out)[3] = static_cast<int16_t>(v2);
                    prev1 = v2;
                    out += 8;
                    data++;
                } while (out < end);
            }
            blocks--;
        } while (blocks != 0);
    }
    *s1p = prev1;
    *s2p = prev2;
    *outp = out;
}
HOOK_FUNC(0x00484560, UvpAdpcmDecodeMono);

// 0x004846e0  size 0x283
// Stereo block ADPCM: 30-byte blocks (two header bytes + 28 data bytes, one byte per channel pair per sample
// pair) to 28 stereo frames. The first header byte holds the coefficient selectors (high nibble left, low nibble
// right), the second the shifts (high nibble left, low nibble right, both plus 8). Argument order as the caller
// at 0x004835a0 pushes it: (count, &progress, &L1, &L2, &R1, &R2, data, &out).
void __cdecl UvpAdpcmDecodeStereo(int count, int32_t *progress, int32_t *l1p, int32_t *l2p, int32_t *r1p,
                                         int32_t *r2p, const uint8_t *data, uint8_t **outp)
{
    int32_t progress30 = static_cast<int32_t>(static_cast<uint32_t>(*progress) * 30u);
    data += uvp_div28(progress30);
    int32_t l1 = *l1p;
    uint8_t *out = *outp;
    int32_t l2 = *l2p;
    int32_t r1 = *r1p;
    int32_t r2 = *r2p;
    if (count > 0) {
        uint32_t blocks = uvp_blocks(count);
        do {
            *progress += 0x1c;
            data += 2;
            uint32_t h0 = data[-2];
            uint32_t h1 = data[-1];
            uint32_t hiSel = h0 >> 4;
            uint32_t loSel = h0 & 0xf;
            int32_t c1L = uvp_coef1[hiSel];
            int32_t c2L = uvp_coef2[hiSel];
            int32_t c1R = uvp_coef1[loSel];
            int32_t c2R = uvp_coef2[loSel];
            int32_t shiftR = static_cast<int32_t>(h1 & 0xf) + 8;
            int32_t shiftL = static_cast<int32_t>(h1 >> 4) + 8;
            uint8_t *end = out + 0x70;
            if (out < end) {
                do {
                    uint32_t d0 = data[0];
                    uint32_t a = static_cast<uint32_t>(l2) * static_cast<uint32_t>(c2L);
                    uint32_t b = static_cast<uint32_t>(static_cast<int32_t>((d0 & 0xfffffff0u) << 24) >> shiftL);
                    b += static_cast<uint32_t>(l1) * static_cast<uint32_t>(c1L);
                    uint32_t r = static_cast<uint32_t>(static_cast<int32_t>(d0 << 28) >> shiftR);
                    r += static_cast<uint32_t>(r1) * static_cast<uint32_t>(c1R);
                    uint32_t rr = static_cast<uint32_t>(r2) * static_cast<uint32_t>(c2R);
                    int32_t vl = static_cast<int32_t>(b + a + 0x80u) >> 8;
                    int32_t vr = static_cast<int32_t>(r + rr + 0x80u) >> 8;
                    vl = uvp_clamp16(vl);
                    vr = uvp_clamp16(vr);
                    reinterpret_cast<int16_t *>(out)[0] = static_cast<int16_t>(vl);
                    reinterpret_cast<int16_t *>(out)[1] = static_cast<int16_t>(vr);
                    r1 = static_cast<int32_t>(static_cast<uint32_t>(r1) * static_cast<uint32_t>(c2R));
                    l1 = static_cast<int32_t>(static_cast<uint32_t>(l1) * static_cast<uint32_t>(c2L));
                    uint32_t d1 = data[1];
                    l2 = vl;
                    uint32_t pl = static_cast<uint32_t>(vl) * static_cast<uint32_t>(c1L);
                    r2 = vr;
                    uint32_t pr = static_cast<uint32_t>(vr) * static_cast<uint32_t>(c1R);
                    uint32_t b2 = static_cast<uint32_t>(static_cast<int32_t>((d1 & 0xfffffff0u) << 24) >> shiftL);
                    uint32_t r3 = static_cast<uint32_t>(static_cast<int32_t>(d1 << 28) >> shiftR);
                    b2 += pl;
                    int32_t vl2 = static_cast<int32_t>(b2 + static_cast<uint32_t>(l1) + 0x80u) >> 8;
                    r3 += static_cast<uint32_t>(r1);
                    int32_t vr2 = static_cast<int32_t>(r3 + pr + 0x80u) >> 8;
                    vl2 = uvp_clamp16(vl2);
                    vr2 = uvp_clamp16(vr2);
                    l1 = vl2;
                    reinterpret_cast<int16_t *>(out)[2] = static_cast<int16_t>(vl2);
                    reinterpret_cast<int16_t *>(out)[3] = static_cast<int16_t>(vr2);
                    r1 = vr2;
                    out += 8;
                    data += 2;
                } while (out < end);
            }
            blocks--;
        } while (blocks != 0);
    }
    *l1p = l1;
    *l2p = l2;
    *r1p = r1;
    *r2p = r2;
    *outp = out;
}
HOOK_FUNC(0x004846e0, UvpAdpcmDecodeStereo);

// 0x00484970  size 0x2ff
// Audio worker thread (argument: the decoder state, from CreateThread at 0x00484c70; the original returns with a
// plain `ret`). Every 15 ms it asks the streaming buffer how far playback got, and when the queue holds a chunk
// that fits in the free space it locks the buffer and copies the chunk in; with no chunk queued it fills the gap
// with silence. Without a streaming buffer (noSoundBuf) it advances a fake play position instead.
DWORD __cdecl UvpAudioThread(UvpState *s)
{
    Sleep(0xf);
    if (InterlockedExchange(UVP_L(s->threadRun), 1) == 0) {
        return 0;
    }
    do {
        if (s->soundBufMade != 0) {
            int32_t playPos;
            int32_t writePos;   // filled by GetCurrentPosition, not used
            if (s->noSoundBuf != 0) {
                s->fakePos += 0x372;
                if (s->fakePos >= s->sbSize) {
                    s->fakePos -= s->sbSize;
                }
                playPos = s->fakePos;
            } else {
                com_call(PORT_RET(long), s->player->dsBuffer, 0x10, &playPos, &writePos);
            }
            int32_t free;
            if (s->sbWrite > playPos) {
                free = playPos - s->sbWrite + s->sbSize;
            } else {
                free = playPos - s->sbWrite;
            }
            if (free >= s->audio[s->audioPlayed % 24].size) {
                UvpAudioEntry *e = &s->audio[s->audioPlayed % 24];
                void *ptr1;
                void *ptr2;
                DWORD bytes1;
                DWORD bytes2;
                if (s->audioQueued != 0) {
                    if (s->noSoundBuf == 0) {
                        while (com_call(PORT_RET(long), s->player->dsBuffer, 0x2c, static_cast<DWORD>(s->sbWrite), static_cast<DWORD>(e->size), &ptr1, &bytes1, &ptr2, &bytes2, static_cast<DWORD>(0)) != 0) {
                            com_call(PORT_RET(long), s->player->dsBuffer, 0x50);   // Restore
                        }
                        memcpy(ptr1, e->data, bytes1);
                        if (bytes2 != 0) {
                            memcpy(ptr2, e->data + bytes1, bytes2);
                        }
                        com_call(PORT_RET(long), s->player->dsBuffer, 0x4c, ptr1, bytes1, ptr2, bytes2);   // Unlock
                    }
                    s->sbWrite = (s->sbWrite + e->size) % s->sbSize;
                    InterlockedIncrement(UVP_L(s->audioPlayed));
                    InterlockedExchange(UVP_L(s->syncPos), e->stamp);
                    InterlockedDecrement(UVP_L(s->audioQueued));
                    e->data = 0;
                    e->size = 0;
                } else {
                    if (s->noSoundBuf == 0) {
                        if (com_call(PORT_RET(long), s->player->dsBuffer, 0x2c, static_cast<DWORD>(s->sbWrite), static_cast<DWORD>(free), &ptr1, &bytes1, &ptr2, &bytes2, static_cast<DWORD>(0)) == 0) {
                            memset(ptr1, 0, bytes1);
                            if (bytes2 != 0) {
                                memset(ptr2, 0, bytes2);
                            }
                            com_call(PORT_RET(long), s->player->dsBuffer, 0x4c, ptr1, bytes1, ptr2, bytes2);   // Unlock
                        }
                    }
                    s->sbWrite = (s->sbWrite + free) % s->sbSize;
                    InterlockedIncrement(UVP_L(s->syncPos));
                }
            }
        }
        InterlockedIncrement(UVP_L(s->tick));
        Sleep(0xf);
    } while (InterlockedExchange(UVP_L(s->threadRun), 1) != 0);
    return 0;
}
HOOK_FUNC(0x00484970, UvpAudioThread);

// 0x00484c70  size 0x3c
// Start the audio worker thread. Returns 1 when CreateThread failed.
int UvpState::StartThread()
{
    DWORD tid;
    InterlockedExchange(UVP_L(threadRun), 1);
    // The thread procedure is the original address (0x00484970); the hook routes it to UvpAudioThread.
    thread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(static_cast<uintptr_t>(PORT_ADDR(&UvpAudioThread))),
                          this, 0, &tid);
    return thread == nullptr;
}
HOOK_METHOD(0x00484c70, UvpState, StartThread);

// 0x00484cb0  size 0x46
// Stop the worker thread: clear the run flag and wait until the thread has seen it. Returns 1 when no thread was
// started, 0 otherwise.
int UvpState::StopThread()
{
    if (thread != nullptr) {
        InterlockedExchange(UVP_L(threadRun), 0);
        if (InterlockedExchange(UVP_L(threadRun), 0) == 0) {
            do {
                Sleep(10);
            } while (InterlockedExchange(UVP_L(threadRun), 0) == 0);
        }
        return 0;
    }
    return 1;
}
HOOK_METHOD(0x00484cb0, UvpState, StopThread);

// 0x00484d00  size 0x10b
// Create the DirectSound object when the caller did not supply one (flag 0x800000 means sound is wanted), set
// the cooperative level, read the driver caps (emulated driver bit) and the primary buffer size.
int UvpState::InitDirectSound()
{
    UvpPlayer *p = player;
    if (p->flags & 0x800000) {
        if (p->ds == nullptr) {
            ownDs = 1;
            HWND h = p->hwnd;
            if (DirectSoundCreate(nullptr, reinterpret_cast<IDirectSound **>(&p->ds), nullptr) != 0) {
                p->ds = nullptr;
            } else {
                if (com_call(PORT_RET(long), p->ds, 0x18, h, static_cast<DWORD>(2)) != 0) {   // SetCooperativeLevel(hwnd, 2)
                    com_call(PORT_RET(long), p->ds, 0x08);                                   // Release
                    p->ds = nullptr;
                }
            }
        }
        p = player;
        if (p->ds != nullptr) {
            uint32_t caps[24];
            memset(caps, 0, sizeof(caps));
            caps[0] = 0x60;                                  // DSCAPS.dwSize
            com_call(PORT_RET(long), p->ds, 0x10, caps);               // GetCaps
            emulDriver = (caps[1] >> 5) & 1;                 // dwFlags & DSCAPS_EMULDRIVER
            p = player;
            if ((p->flags & 0x400000) && p->dsPrimary != nullptr) {
                uint32_t bcaps[5] = {0x14, 0, 0, 0, 0};      // DSBCAPS
                com_call(PORT_RET(long), p->dsPrimary, 0x0c, bcaps);   // GetCaps
                primaryBytes = static_cast<int32_t>(bcaps[2]);
                return 0;
            }
            primaryBytes = 0x8000;
            return 0;
        }
        p->dsPrimary = nullptr;
    }
    return 0;
}
HOOK_METHOD(0x00484d00, UvpState, InitDirectSound);

// 0x00484e10  size 0x52
// Release what InitDirectSound / the caller made: the primary buffer when ownPrimary is set, the DirectSound
// object when ownDs is set.
int UvpState::ReleaseDirectSound()
{
    UvpPlayer *p = player;
    if (p->flags & 0x800000) {
        if (ownPrimary != 0) {
            if (p->dsPrimary != nullptr) {
                com_call(PORT_RET(long), p->dsPrimary, 0x08);   // Release
                player->dsPrimary = nullptr;
            }
        }
        if (ownDs != 0) {
            void **dsp = &player->ds;
            if (*dsp != nullptr) {
                com_call(PORT_RET(long), *dsp, 0x08);           // Release
                *dsp = nullptr;
            }
        }
    }
    return 0;
}
HOOK_METHOD(0x00484e10, UvpState, ReleaseDirectSound);

// 0x00484e70  size 0x153
// Create the streaming secondary buffer (16-bit stereo PCM at the stream's rate, looping), read its real size
// and start it playing. Any failure leaves the player without a buffer: noSoundBuf is set and the worker thread
// advances a fake play position of 0x8000 bytes instead.
int UvpState::CreateSoundBuffer()
{
    UvpPlayer *p = player;
    bool failed = false;
    if (p->ds == nullptr) {
        failed = true;
    } else {
        WAVEFORMATEX wfx;
        memset(&wfx, 0, sizeof(wfx));
        wfx.wFormatTag = 1;
        wfx.nChannels = 2;
        wfx.nSamplesPerSec = static_cast<DWORD>(rate);
        wfx.nAvgBytesPerSec = static_cast<DWORD>(rate) << 2;
        wfx.nBlockAlign = 4;
        wfx.wBitsPerSample = 0x10;
        wfx.cbSize = 0;
        uint32_t desc[5];                                   // DSBUFFERDESC
        desc[0] = 0x14;
        desc[1] = 0x100e0;
        desc[2] = static_cast<uint32_t>(primaryBytes);
        desc[3] = 0;
        desc[4] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&wfx));
        if (com_call(PORT_RET(long), p->ds, 0x0c, desc, &p->dsBuffer, static_cast<void *>(nullptr)) != 0) {   // CreateSoundBuffer
            failed = true;
        } else {
            com_call(PORT_RET(long), player->dsBuffer, 0x44, static_cast<DWORD>(rate));   // SetFrequency
            uint32_t bcaps[5] = {0x14, 0, 0, 0, 0};                             // DSBCAPS
            if (com_call(PORT_RET(long), player->dsBuffer, 0x0c, bcaps) != 0) {           // GetCaps
                com_call(PORT_RET(long), player->dsBuffer, 0x08);                         // Release
                failed = true;
            } else {
                sbSize = static_cast<int32_t>(bcaps[2]);
                noSoundBuf = 0;
                sbWrite = 0;
                if (com_call(PORT_RET(long), player->dsBuffer, 0x30, static_cast<DWORD>(0), static_cast<DWORD>(0), static_cast<DWORD>(1)) != 0) {                // Play(0, 0, DSBPLAY_LOOPING)
                    com_call(PORT_RET(long), player->dsBuffer, 0x08);                      // Release
                    failed = true;
                }
            }
        }
    }
    if (failed) {
        player->dsBuffer = nullptr;
        fakePos = 0;
        sbSize = 0x8000;
        noSoundBuf = 1;
        sbWrite = 0;
    }
    soundBufMade = 1;
    return 0;
}
HOOK_METHOD(0x00484e70, UvpState, CreateSoundBuffer);

// 0x00484fd0  size 0x1f
// Release the streaming buffer and forget it.
int UvpState::ReleaseDsBuffer()
{
    void *buf = player->dsBuffer;
    if (buf != nullptr) {
        com_call(PORT_RET(long), buf, 0x08);   // Release
        player->dsBuffer = nullptr;
    }
    return 0;
}
HOOK_METHOD(0x00484fd0, UvpState, ReleaseDsBuffer);

// 0x00484ff0  size 0xa3
// IMA ADPCM, mono: decode `count` nibbles (high nibble first, starting at nibble index `start`) from `src` to
// 16-bit samples at `dst`. *pred and *index carry the predictor and step offset (0 - 0x1600) between calls.
// cdecl, saves every register (pushal / popal).
static void __cdecl UvpImaAdpcmDecodeMono(int start, const uint8_t *src, int16_t *dst, int count, int32_t *pred,
                                          int32_t *index)
{
    int32_t n = start;
    int32_t end = start + count;
    int32_t idx = *index;
    int32_t sample = static_cast<int16_t>(*pred);
    do {
        uint32_t nib = *src;
        if ((n & 1) == 0) {
            nib = static_cast<uint8_t>(static_cast<uint8_t>(nib) >> 4);
        } else {
            nib &= 0xf;
            src++;
        }
        sample += *reinterpret_cast<const int32_t *>(uvp_imaStep + idx + nib * 4);
        if (sample > 0x7fff) {
            sample = 0x7fff;
        } else if (sample < -0x8000) {
            sample = -0x8000;
        }
        *dst++ = static_cast<int16_t>(sample);
        n++;
        nib &= 7;
        idx += uvp_imaAdjust[nib];
        if (idx > 0x1600) {
            idx = 0x1600;
        } else if (idx < 0) {
            idx = 0;
        }
    } while (n < end);
    *pred = sample;
    *index = idx;
}
HOOK_FUNC(0x00484ff0, UvpImaAdpcmDecodeMono);

// 0x00485093  size 0x177
// IMA ADPCM, stereo: `count` bytes, each one left (high nibble) and one right (low nibble) sample, to `count`
// 16-bit stereo frames. *pred and *index each pack left (low half) and right (high half). The running state is
// kept in four globals at 0x004b3190 - 0x004b319c, as in the original. cdecl, saves every register.
void __cdecl UvpImaAdpcmDecodeStereo(const uint8_t *src, int16_t *dst, int count, int32_t *pred,
                                            int32_t *index)
{
    int32_t n = 0;
    uint32_t idxPacked = static_cast<uint32_t>(*index);
    uvp_g3198 = static_cast<int16_t>(idxPacked);
    uvp_g319c = static_cast<int16_t>(idxPacked >> 16);
    uint32_t predPacked = static_cast<uint32_t>(*pred);
    uvp_g3190 = static_cast<int16_t>(predPacked);
    uvp_g3194 = static_cast<int16_t>(predPacked >> 16);
    do {
        uint32_t nib = static_cast<uint8_t>(*src >> 4);
        int32_t idx = uvp_g3198;
        int32_t sample = uvp_g3190 + *reinterpret_cast<const int32_t *>(uvp_imaStep + idx + nib * 4);
        if (sample > 0x7fff) {
            sample = 0x7fff;
        } else if (sample < -0x8000) {
            sample = -0x8000;
        }
        dst[n * 2] = static_cast<int16_t>(sample);
        uvp_g3190 = sample;
        nib &= 7;
        idx += uvp_imaAdjust[nib];
        if (idx > 0x1600) {
            idx = 0x1600;
        } else if (idx < 0) {
            idx = 0;
        }
        uvp_g3198 = idx;
        nib = *src & 0xf;
        idx = uvp_g319c;
        sample = uvp_g3194 + *reinterpret_cast<const int32_t *>(uvp_imaStep + idx + nib * 4);
        if (sample > 0x7fff) {
            sample = 0x7fff;
        } else if (sample < -0x8000) {
            sample = -0x8000;
        }
        dst[n * 2 + 1] = static_cast<int16_t>(sample);
        uvp_g3194 = sample;
        nib &= 7;
        idx += uvp_imaAdjust[nib];
        if (idx > 0x1600) {
            idx = 0x1600;
        } else if (idx < 0) {
            idx = 0;
        }
        uvp_g319c = idx;
        n++;
        src++;
    } while (n < count);
    *pred = static_cast<int32_t>((static_cast<uint32_t>(uvp_g3194) << 16) | (static_cast<uint32_t>(uvp_g3190) & 0xffffu));
    *index = static_cast<int32_t>((static_cast<uint32_t>(uvp_g319c) << 16) | (static_cast<uint32_t>(uvp_g3198) & 0xffffu));
}
HOOK_FUNC(0x00485093, UvpImaAdpcmDecodeStereo);
