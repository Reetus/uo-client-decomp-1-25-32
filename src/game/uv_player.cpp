// uV intro video player (plays music\intro.wve): the entry point, the stream container reader (chunk tags, video
// and audio chunk queues, format chunk), the player main loop and the thread that runs it. Porting pass 3, batch
// v1. The audio side (decoders, worker thread, DirectSound) is in uv_audio.cpp; the DirectDraw surface layer is
// batch v2 and is called at its addresses. See uv_player.h for the layout.
//
// Calling conventions of the original: every method takes the decoder state in ECX (UvpState, "this"); the
// chunk readers take the chunk size in EDX as well (__fastcall here), 0x004834c0 also a stack argument.
#include "uv_player.h"

#include <cstdio>
#include <cstring>

#include <mmsystem.h>

#include "../runtime/ported.h"
#include "crt.h"
#include "uv_video_surface_ports.h"
#include "uv_video_decode_ports.h"
#include "uv_player_ports.h"

#pragma comment(lib, "winmm.lib")

// Batch v2, all with the decoder state in ECX and a plain `ret`.
#define uvp_BlitFrame PORT_FUNC1(&UV_BlitFrame, int, __fastcall, UvpState *)               // 0x00485210
#define uvp_SwapFrameBuffers PORT_FUNC1(&UV_SwapFrameBuffers, int, __fastcall, UvpState *)        // 0x00485d70
#define uvp_InitVideoSurface PORT_FUNC1(&UV_InitVideoSurface, int, __fastcall, UvpState *)        // 0x00486590
#define uvp_ReleaseVideoSurface PORT_FUNC1(&UV_ReleaseVideoSurface, int, __fastcall, UvpState *)     // 0x00486770
#define uvp_DrawEntry PORT_FUNC2(&UV_DecodeChunk, int, __fastcall, UvpState *, UvpVideoEntry *)   // 0x00486850, EDX = entry

// CRT _beginthread (0x0048cc80): (start, stack size, argument).

#define UVP_L(x) reinterpret_cast<LONG *>(&(x))

static inline uint32_t uvp_le32(const uint8_t *p)
{
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

// 0x004830e0  size 0x87
// Read the next chunk when both queues have room (fewer than 0x16 queued), remember its result and react to it:
// -1 stores the state, 1 (sound container) clears the "audio stream" flag once a chunk of that kind was read,
// 2 stores the state unless an audio-less player already has 0x16 audio chunks.
void UvpState::Pump()
{
    if (state == -1) {
        return;
    }
    if (audioQueued >= 0x16) {
        return;
    }
    if (videoQueued >= 0x16) {
        return;
    }
    int r = ReadChunk();
    lastChunk = r;
    switch (r) {
    case 2:
        if (player->flag74 != 0) {
            if (audioQueued >= 0x16) {
                return;
            }
        }
        state = ReadChunk();
        return;
    case 1:
        if (player->audioStream == 0) {
            return;
        }
        r = ReadChunk();
        state = r;
        if (r == 1) {
            player->audioStream = 0;
        }
        return;
    case -1:
        state = r;
        return;
    case -2:
    case 0:
        return;
    }
}
HOOK_METHOD(0x004830e0, UvpState, Pump);

// 0x004834c0  size 0xdf
// Queue a video chunk: read `size` bytes into the video ring (wrapping to the start when it would not fit),
// record where, its type (1 key frame ... 4, from the tag) and the current counter in the next video entry, and
// bump the write index, the queued count and the counter. Returns 1; -1 on end of stream (and sets error), -2 on
// a read error. __fastcall (ECX state, EDX size) plus one stack argument (the type).
static int __fastcall UvpReadVideo(UvpState *s, int size, int type)
{
    uint8_t *ring = s->videoRing;
    if (reinterpret_cast<uintptr_t>(s->videoWrite) + static_cast<uintptr_t>(size) >=
        reinterpret_cast<uintptr_t>(ring) + 0xaae60) {
        s->videoWrite = ring;
    }
    LONG got = mmioRead(s->player->hmmio, reinterpret_cast<HPSTR>(s->videoWrite), size);
    if (got == 0) {
        s->error = 1;
        return -1;
    }
    if (got == -1) {
        return -2;
    }
    s->video[s->videoWriteIdx % 24].data = s->videoWrite;
    s->video[s->videoWriteIdx % 24].type = type;
    s->video[s->videoWriteIdx % 24].stamp = s->count360;
    s->videoWrite += size;
    InterlockedIncrement(UVP_L(s->videoWriteIdx));
    InterlockedIncrement(UVP_L(s->videoQueued));
    InterlockedIncrement(UVP_L(s->count360));
    return 1;
}
HOOK_FUNC(0x004834c0, UvpReadVideo);

// 0x004835a0  size 0x384
// Queue an audio chunk: read it, decode it into the PCM ring according to the format (0 PCM, 7 block ADPCM,
// 0xa IMA ADPCM; mono is doubled to stereo frames) and add an entry (position, byte count, counters). Returns 2;
// -1 on end of stream (sets error), -2 on a read error. __fastcall (ECX state, EDX size).
static int __fastcall UvpReadAudio(UvpState *s, int size)
{
    uint8_t buf[0x8000];
    int32_t progress = 0;           // the two stack cells the decoders take as their `progress` argument
    uint8_t *outPtr;
    int32_t bytes;                  // PCM bytes produced (the original's EDI)
    LONG got = mmioRead(s->player->hmmio, reinterpret_cast<HPSTR>(buf), size);
    if (got == 0) {
        s->error = 1;
        return -1;
    }
    if (got == -1) {
        return -2;
    }
    if (s->codec == 0) {
        if (s->channels == 2) {
            bytes = static_cast<int32_t>(uvp_le32(buf) << 2);
            if (reinterpret_cast<uintptr_t>(s->audioWrite) + static_cast<uintptr_t>(bytes) >=
                reinterpret_cast<uintptr_t>(s->audioRing) + 0x493e0) {
                s->audioWrite = s->audioRing;
            }
            int32_t frames = (bytes + ((bytes >> 31) & 3)) >> 2;
            uint8_t *dst = s->audioWrite;
            if (frames > 0) {
                const uint8_t *src = buf + 4;
                do {
                    *reinterpret_cast<uint32_t *>(dst) = *reinterpret_cast<const uint32_t *>(src);
                    src += 4;
                    dst += 4;
                    frames--;
                } while (frames != 0);
            }
        } else {
            bytes = static_cast<int32_t>(uvp_le32(buf) << 2);
            if (reinterpret_cast<uintptr_t>(s->audioWrite) + static_cast<uintptr_t>(bytes) >=
                reinterpret_cast<uintptr_t>(s->audioRing) + 0x493e0) {
                s->audioWrite = s->audioRing;
            }
            int32_t frames = (bytes + ((bytes >> 31) & 3)) >> 2;
            uint8_t *dst = s->audioWrite;
            if (frames > 0) {
                const uint8_t *src = buf + 4;
                do {
                    uint16_t w = *reinterpret_cast<const uint16_t *>(src);
                    src += 2;
                    *reinterpret_cast<uint16_t *>(dst + 2) = w;
                    *reinterpret_cast<uint16_t *>(dst) = w;
                    dst += 4;
                    frames--;
                } while (frames != 0);
            }
        }
    } else if (s->codec == 7) {
        uint32_t rest = static_cast<uint32_t>(size) - 0xc;
        if (s->channels == 2) {
            progress = 0;
            s->adpcmL2 = *reinterpret_cast<const int16_t *>(buf + 6);
            s->adpcmL1 = *reinterpret_cast<const int16_t *>(buf + 4);
            s->adpcmR2 = *reinterpret_cast<const int16_t *>(buf + 0xa);
            s->adpcmR1 = *reinterpret_cast<const int16_t *>(buf + 8);
            uint32_t blocks = static_cast<uint32_t>((static_cast<uint64_t>(rest) * 0x88888889u) >> 32) >> 4;
            bytes = static_cast<int32_t>(blocks * 7 * 16);
            if (reinterpret_cast<uintptr_t>(s->audioWrite) + static_cast<uintptr_t>(bytes) >=
                reinterpret_cast<uintptr_t>(s->audioRing) + 0x493e0) {
                s->audioWrite = s->audioRing;
            }
            outPtr = s->audioWrite;
            UvpAdpcmDecodeStereo((bytes + ((bytes >> 31) & 3)) >> 2, &progress, &s->adpcmL1, &s->adpcmL2,
                                 &s->adpcmR1, &s->adpcmR2, buf + 0xc, &outPtr);
        } else {
            progress = 0;
            s->adpcmL2 = *reinterpret_cast<const int16_t *>(buf + 6);
            s->adpcmL1 = *reinterpret_cast<const int16_t *>(buf + 4);
            s->adpcmR2 = *reinterpret_cast<const int16_t *>(buf + 0xa);
            s->adpcmR1 = *reinterpret_cast<const int16_t *>(buf + 8);
            uint32_t blocks = static_cast<uint32_t>((static_cast<uint64_t>(rest) * 0x88888889u) >> 32) >> 3;
            bytes = static_cast<int32_t>(blocks * 7 * 16);
            if (reinterpret_cast<uintptr_t>(s->audioWrite) + static_cast<uintptr_t>(bytes) >=
                reinterpret_cast<uintptr_t>(s->audioRing) + 0x493e0) {
                s->audioWrite = s->audioRing;
            }
            outPtr = s->audioWrite;
            UvpAdpcmDecodeMono((bytes + ((bytes >> 31) & 3)) >> 2, &progress, &s->adpcmL1, &s->adpcmL2, buf + 0xc,
                               &outPtr);
        }
    } else if (s->codec == 0xa) {
        bytes = static_cast<int32_t>(static_cast<uint32_t>(size) * 4);
        if (reinterpret_cast<uintptr_t>(s->audioWrite) + static_cast<uintptr_t>(bytes) >=
            reinterpret_cast<uintptr_t>(s->audioRing) + 0x493e0) {
            s->audioWrite = s->audioRing;
        }
        UvpImaAdpcmDecodeStereo(buf, reinterpret_cast<int16_t *>(s->audioWrite), size, &s->imaPred, &s->imaIndex);
    } else {
        bytes = 0;   // ? the original reads an uninitialised stack cell here
    }
    if (s->state == 2) {
        s->player->flag74 = 1;
    }
    if (s->player->flag74 != 0) {
        InterlockedIncrement(UVP_L(s->count360));
    }
    int32_t idx = s->audioWriteIdx % 24;
    uint8_t *start = s->audioWrite;
    s->audio[idx].data = start;
    s->audio[idx].size = bytes;
    s->audio[idx].stamp = s->count360;
    s->audio[idx].stamp2 = s->count364;
    s->audioWrite += bytes;
    InterlockedIncrement(UVP_L(s->audioWriteIdx));
    InterlockedIncrement(UVP_L(s->audioQueued));
    InterlockedIncrement(UVP_L(s->count364));
    return 2;
}
HOOK_FUNC(0x004835a0, UvpReadAudio);

// 0x00483930  size 0x29f
// Format chunk of a sound container: defaults 16 bit, mono, 22050 Hz, PCM, then a list of records (tag byte,
// length byte, big-endian value) ended by 0xfd; after it a list of codes 0x81 - 0x84 (bits, channels, codec, rate)
// ended by 0x8a, each a length byte and a big-endian value, sign-extended when the top bit of a 1 - 3 byte value
// is set. Other codes are skipped. __fastcall (ECX state, EDX size). Returns 0; -1 / -2 as the other readers
// (here without setting error).
static int __fastcall UvpReadFormat(UvpState *s, int size)
{
    uint8_t buf[0x8000];
    LONG got = mmioRead(s->player->hmmio, reinterpret_cast<HPSTR>(buf), size);
    if (got == 0) {
        return -1;
    }
    if (got == -1) {
        return -2;
    }
    s->bits = 0x10;
    s->channels = 1;
    s->rate = 0x5622;
    s->codec = 0;
    const uint8_t *p = buf + 4;
    if (*p != 0xfd) {
        do {
            p++;
            uint32_t len = *p++;
            p += len;
        } while (*p != 0xfd);
    }
    p++;
    uint32_t op = *p;
    if (op != 0x8a) {
        do {
            int32_t *dst = nullptr;
            switch (op - 0x81) {   // the original's jump table at 0x00483bd0
            case 0:
                dst = &s->bits;
                break;
            case 1:
                dst = &s->channels;
                break;
            case 2:
                dst = &s->codec;
                break;
            case 3:
                dst = &s->rate;
                break;
            default:
                break;
            }
            p++;
            uint32_t len = *p++;
            uint32_t value = 0;
            for (uint32_t k = 0; k < len; k++) {
                value = (value << 8) + *p++;
            }
            if (dst != nullptr) {
                // The four cases are the same code with a different destination field.
                int32_t v = static_cast<int32_t>(value);
                if (len == 1 && v > 0x7f) {
                    v -= 0x100;
                } else if (len == 2 && v > 0x7fff) {
                    v -= 0x10000;
                } else if (len == 3 && v > 0x7fffff) {
                    v -= 0x1000000;
                }
                *dst = v;
            }
            op = *p;
        } while (op != 0x8a);
    }
    return 0;
}
HOOK_FUNC(0x00483930, UvpReadFormat);

// 0x00483180  size 0x339
// Read one chunk header (8 bytes: tag, then size including the header) and dispatch on the tag:
//   SEAD (0x44414553): the sound container's header: stereo 16 bit 22050 Hz codec 0xa; returns 0
//   SNDC / CDNS, 1SNd, SCDl: sound data, handed to the audio reader when the container carries sound
//   1SNh, lHCS, SCHl: format chunk (UvpReadFormat); lCCS, lLCS, SCCl, SCLl: skipped, returns 4
//   pIQT, UV2f -> video type 4; fVGT, TGVf -> 2; pQGT, MUVf -> 3; kVGT, TGVk -> 1 (UvpReadVideo)
//   SEND, 1SNe, SCEl: end of stream, returns -1
// Returns -1 at the end of the stream, -2 on a short read or an unknown tag; sets error when the file ended.
int UvpState::ReadChunk()
{
    uint8_t buf[0x800c];
    LONG got = mmioRead(player->hmmio, reinterpret_cast<HPSTR>(buf), 8);
    if (got == 0) {
        error = 1;
        return -1;
    }
    if (got == -1 || got != 8) {
        return -2;
    }
    int32_t chunk = static_cast<int32_t>(uvp_le32(buf + 4)) - 8;
    uint32_t tag = uvp_le32(buf);
    switch (tag) {
    case 0x44414553:   // SEAD
        got = mmioRead(player->hmmio, reinterpret_cast<HPSTR>(buf + 0xc), chunk);
        if (got == 0) {
            error = 1;
            return -1;
        }
        if (got == -1) {
            return -2;
        }
        bits = 0x10;
        channels = 2;
        rate = 0x5622;
        codec = 0xa;
        return 0;
    case 0x43444e53:   // SNDC
    case 0x534e4443:   // CDNS
        if (player->audioStream == 0) {
            return -2;
        }
        return UvpReadAudio(this, chunk);
    case 0x5343436c:   // lCCS
    case 0x53434c6c:   // lLCS
    case 0x6c434353:   // SCCl
    case 0x6c4c4353:   // SCLl
        mmioRead(player->hmmio, reinterpret_cast<HPSTR>(buf + 0xc), chunk);
        return 4;
    case 0x5343486c:   // lHCS
    case 0x6c484353:   // SCHl
        return UvpReadFormat(this, chunk);
    case 0x54475170:   // pQGT
    case 0x6656554d:   // MUVf
        return UvpReadVideo(this, chunk, 3);
    case 0x5447566b:   // kVGT
    case 0x6b564754:   // TGVk
        return UvpReadVideo(this, chunk, 1);
    case 0x54475666:   // fVGT
    case 0x66564754:   // TGVf
        return UvpReadVideo(this, chunk, 2);
    case 0x644e5331:   // 1SNd
    case 0x6c444353:   // SCDl
        if (player->audioStream == 0) {
            return -2;
        }
        return UvpReadAudio(this, chunk);
    case 0x444e4553:   // SEND
    case 0x654e5331:   // 1SNe
    case 0x6c454353:   // SCEl
        return -1;
    case 0x54514970:   // pIQT
    case 0x66325655:   // UV2f
        return UvpReadVideo(this, chunk, 4);
    case 0x684e5331:   // 1SNh
        return UvpReadFormat(this, chunk);
    default:
        return -2;
    }
}
HOOK_METHOD(0x00483180, UvpState, ReadChunk);

// 0x00483be0  size 0x78
// Close the MMIO handle (unless the caller owns it), free the two rings and clear both chunk tables.
int UvpState::CloseAll()
{
    UvpPlayer *p = player;
    if (!(p->flags & 0x80000)) {
        if (p->hmmio != nullptr) {
            mmioClose(p->hmmio, 0);
        }
    }
    if (videoRing != nullptr) {
        free(videoRing);
    }
    if (audioRing != nullptr) {
        free(audioRing);
    }
    for (int i = 0; i < 24; i++) {
        video[i].data = nullptr;
    }
    if (player->audioStream != 0) {
        for (int i = 0; i < 24; i++) {
            audio[i].data = nullptr;
        }
    }
    return 0;
}
HOOK_METHOD(0x00483be0, UvpState, CloseAll);

// 0x00484100  size 0x1da
// Open the stream (mmioOpenA unless the caller supplied a handle), set a 16 KB MMIO buffer, peek at the first tag
// (SEAD / lHCS / 1SNh mean a sound container), clear the chunk tables, allocate and clear the two rings and
// prime the queues with the first chunks. fastcall: ECX = path, EDX = decoder state. Returns 0, 1 when the file
// cannot be opened, 2 when memory ran out.
static int __fastcall UvpOpenStream(const char *path, UvpState *s)
{
    s->error = 0;
    if (!(s->player->flags & 0x80000)) {
        s->player->hmmio = mmioOpenA(const_cast<LPSTR>(path), nullptr, 0);
    }
    if (s->player->hmmio == nullptr) {
        return 1;
    }
    if (mmioSetBuffer(s->player->hmmio, nullptr, 0x4000, 0) != 0) {
        return 1;
    }
    DWORD tag;
    mmioRead(s->player->hmmio, reinterpret_cast<HPSTR>(&tag), 4);
    s->firstPos = mmioSeek(s->player->hmmio, -4, SEEK_CUR);
    if (tag == 0x44414553 || tag == 0x6c484353 || tag == 0x684e5331) {
        s->player->audioStream = 1;
    } else {
        s->player->audioStream = 0;
    }
    for (int i = 0; i < 24; i++) {
        s->audio[i].data = nullptr;
    }
    for (int i = 0; i < 24; i++) {
        s->video[i].data = nullptr;
    }
    s->videoRing = static_cast<uint8_t *>(calloc(1, 0xaae60));
    s->videoWrite = s->videoRing;
    if (s->videoRing == nullptr) {
        return 2;
    }
    s->audioRing = static_cast<uint8_t *>(calloc(1, 0x493e0));
    s->audioWrite = s->audioRing;
    if (s->audioRing == nullptr) {
        return 2;
    }
    for (int pass = 0; pass < 4; pass++) {
        memset(s->videoRing, 0, 0xaae60);
        memset(s->audioRing, 0, 0x493e0);
    }
    s->count360 = 0;
    s->count364 = 0;
    s->videoReadIdx = 0;
    s->videoWriteIdx = 0;
    s->videoQueued = 0;
    s->audioQueued = 0;
    s->state = 4;
    if (s->player->audioStream != 0) {
        if (s->audioQueued < 0x15) {
            do {
                if (s->ReadChunk() == -1) {
                    break;
                }
                if (s->ReadChunk() == -1) {
                    break;
                }
            } while (s->audioQueued < 0x15);
        }
    } else {
        if (s->videoQueued < 0x14) {
            do {
                if (s->ReadChunk() == -1) {
                    break;
                }
            } while (s->videoQueued < 0x14);
        }
    }
    s->player->flag74 = (s->count360 == 0) ? 1 : 0;
    return 0;
}
HOOK_FUNC(0x00484100, UvpOpenStream);

// 0x004842e0  size 0x27f
// The player main loop, run on the caller's thread when the player has no thread of its own (flag 0x10000 clear);
// this = the player object. Starts the audio worker, then once per pass: negotiates the run state with the
// caller (3 = paused, 4 = stop), reads chunks ahead, draws the oldest queued video frame (the frame is only
// presented when the player is not behind), calls the frame callback and swaps the frame buffers; when the
// sound is the clock it waits (Sleep 0 / 3, at most 0x66 times) until the audio position catches up and records
// how far behind it is. Ends on stop, on the end of the stream or when the callback returns non-zero.
void UvpPlayer::PlayLoop()
{
    UvpState *s = state;
    if ((s->player->flags & 0x30000) == 0x30000) {
        SetThreadPriority(GetCurrentThread(), static_cast<int>(s->player->threadPriority));
    }
    if (s->StartThread() == 0) {
        s->player->lag = 0;
        s->startTick = GetTickCount();
        InterlockedExchange(UVP_L(s->player->runState), 1);
        for (;;) {
            Sleep(0);
            LONG run = InterlockedExchange(UVP_L(s->player->runState), 2);
            UvpVideoEntry *e = &s->video[s->videoReadIdx % 24];
            if (run == 3) {
                InterlockedExchange(UVP_L(s->player->runState), run);
                do {
                    Sleep(0x14);
                    run = InterlockedExchange(UVP_L(s->player->runState), 3);
                } while (run == 3);
                InterlockedExchange(UVP_L(s->player->runState), run);
            }
            if (run == 4) {
                break;
            }
            if (run != 2) {
                continue;
            }
            s->Pump();
            if (s->soundBufMade == 0 && s->player->audioStream != 0) {
                s->CreateSoundBuffer();
                s->syncPos = 0;
            }
            if (e->data != nullptr) {
                if (s->player->lag <= 1) {
                    uvp_DrawEntry(s, e);
                    uvp_BlitFrame(s);
                } else if (e->type == 2 || e->type == 1) {
                    uvp_DrawEntry(s, e);
                }
                if (s->player->flags & 0x40000) {
                    if (s->player->callback != nullptr) {
                        if (s->player->callback() != 0) {
                            break;
                        }
                    }
                }
                e->data = nullptr;
                e->unk04 = 0;
                InterlockedIncrement(UVP_L(s->videoReadIdx));
                InterlockedDecrement(UVP_L(s->videoQueued));
                uvp_SwapFrameBuffers(s);
            }
            InterlockedIncrement(UVP_L(s->player->ticks));
            if (s->videoQueued == 0) {
                if (s->player->audioStream == 0) {
                    break;
                }
                if (s->audioQueued == 0) {
                    break;
                }
            }
            if (s->player->audioStream == 0) {
                continue;
            }
            int32_t target = s->syncPos - 5;
            int32_t now = s->player->ticks;
            int32_t spins = 0;
            if (now - target >= 1) {
                do {
                    Sleep(0);
                    Sleep(3);
                    target = s->syncPos - 5;
                    int32_t spin = spins;
                    spins++;
                    if (spin > 0x64) {
                        break;
                    }
                } while (now - target >= 1);
            }
            s->player->lag = target - now;
            if (s->player->lag < 0) {
                s->player->lag = 0;
            }
        }
        s->endTick = GetTickCount();
        s->StopThread();
    }
    s->ReleaseDsBuffer();
    InterlockedExchange(UVP_L(s->player->runState), 5);
}
HOOK_METHOD(0x004842e0, UvpPlayer, PlayLoop);

// 0x00483c60  size 0x2e0
// Thread procedure of the player when it runs on its own thread (flag 0x10000; started by UvpPlayerStart through
// _beginthread; cdecl, argument = the player object). The same loop as UvpPlayer::PlayLoop, followed by the
// tear-down that UvpPlayerStart does inline when it runs the loop itself (frees the decoder state).
void __cdecl UvpPlayerThread(UvpPlayer *player)
{
    UvpState *s = player->state;
    if ((s->player->flags & 0x30000) == 0x30000) {
        SetThreadPriority(GetCurrentThread(), static_cast<int>(s->player->threadPriority));
    }
    if (s->StartThread() == 0) {
        s->player->lag = 0;
        s->startTick = GetTickCount();
        InterlockedExchange(UVP_L(s->player->runState), 1);
        for (;;) {
            Sleep(0);
            LONG run = InterlockedExchange(UVP_L(s->player->runState), 2);
            UvpVideoEntry *e = &s->video[s->videoReadIdx % 24];
            if (run == 3) {
                InterlockedExchange(UVP_L(s->player->runState), run);
                do {
                    Sleep(0x14);
                    run = InterlockedExchange(UVP_L(s->player->runState), 3);
                } while (run == 3);
                InterlockedExchange(UVP_L(s->player->runState), run);
            }
            if (run == 4) {
                break;
            }
            if (run != 2) {
                continue;
            }
            s->Pump();
            if (s->soundBufMade == 0 && s->player->audioStream != 0) {
                s->CreateSoundBuffer();
                s->syncPos = 0;
            }
            if (e->data != nullptr) {
                if (s->player->lag <= 1) {
                    uvp_DrawEntry(s, e);
                    uvp_BlitFrame(s);
                } else if (e->type == 2 || e->type == 1) {
                    uvp_DrawEntry(s, e);
                }
                if (s->player->flags & 0x40000) {
                    if (s->player->callback != nullptr) {
                        if (s->player->callback() != 0) {
                            break;
                        }
                    }
                }
                e->data = nullptr;
                e->unk04 = 0;
                InterlockedIncrement(UVP_L(s->videoReadIdx));
                InterlockedDecrement(UVP_L(s->videoQueued));
                uvp_SwapFrameBuffers(s);
            }
            InterlockedIncrement(UVP_L(s->player->ticks));
            if (s->videoQueued == 0) {
                if (s->player->audioStream == 0) {
                    break;
                }
                if (s->audioQueued == 0) {
                    break;
                }
            }
            if (s->player->audioStream == 0) {
                continue;
            }
            int32_t target = s->syncPos - 5;
            int32_t now = s->player->ticks;
            int32_t spins = 0;
            if (now - target >= 1) {
                do {
                    Sleep(0);
                    Sleep(3);
                    target = s->syncPos - 5;
                    int32_t spin = spins;
                    spins++;
                    if (spin > 0x64) {
                        break;
                    }
                } while (now - target >= 1);
            }
            s->player->lag = target - now;
            if (s->player->lag < 0) {
                s->player->lag = 0;
            }
        }
        s->endTick = GetTickCount();
        s->StopThread();
    }
    s->ReleaseDsBuffer();
    InterlockedExchange(UVP_L(s->player->runState), 5);
    if (s->active != 0) {
        UvpPlayer *p = player->state->player;
        s = player->state;
        p->elapsedMs = static_cast<int32_t>(s->endTick - s->startTick);
        s->CloseAll();
        s->ReleaseDirectSound();
        uvp_ReleaseVideoSurface(s);
        InterlockedExchange(UVP_L(s->player->runState), 6);
        s->player->state = nullptr;
        free(s);
    }
}
HOOK_FUNC(0x00483c60, UvpPlayerThread);

// 0x00483f40  size 0x1b3
// uV player entry (cdecl): allocate the decoder state, open the stream, set up sound and the video surface, then
// either start the player thread (flag 0x10000; waits until it has taken the run state, returns the player
// object) or run the main loop here and tear everything down (returns 0). Returns 0 when the stream cannot be
// opened or memory runs out.
int __cdecl UvpPlayerStart(const char *path, HWND hwnd, UvpPlayer *player)
{
    UvpState *s = static_cast<UvpState *>(calloc(1, 0xb2c));
    UvpPlayer *p = nullptr;
    if (s != nullptr) {
        p = player;
        s->player = p;
        p->hwnd = hwnd;
        s->player->surf0 = nullptr;
        s->player->surf1 = nullptr;
        s->player->ticks = 0;
        if (UvpOpenStream(path, s) == 0) {
            p->state = s;
            s->InitDirectSound();
            uvp_InitVideoSurface(s);
            InterlockedExchange(UVP_L(s->player->runState), 0);
        } else {
            free(s);
            p = nullptr;
        }
    }
    if (p == nullptr) {
        return 0;
    }
    p->state->active = 1;
    if (p->flags & 0x10000) {
        p->thread = _beginthread(reinterpret_cast<void(__cdecl *)(void *)>(static_cast<uintptr_t>(PORT_ADDR(&UvpPlayerThread))), 0, p);
        if (p->thread == static_cast<uintptr_t>(-1)) {
            return 0;
        }
        LONG *run = UVP_L(p->runState);
        if (InterlockedExchange(run, 1) != 2) {
            do {
                Sleep(0x14);
            } while (InterlockedExchange(run, 1) != 2);
        }
        InterlockedExchange(run, 2);
        return static_cast<int>(reinterpret_cast<intptr_t>(p));
    }
    UvpState *st = p->state;
    p->PlayLoop();
    if (st->active == 0) {
        return 0;
    }
    st = p->state;
    st->player->elapsedMs = static_cast<int32_t>(st->endTick - st->startTick);
    if (!(st->player->flags & 0x80000)) {
        if (st->player->hmmio != nullptr) {
            mmioClose(st->player->hmmio, 0);
        }
    }
    if (st->videoRing != nullptr) {
        free(st->videoRing);
    }
    if (st->audioRing != nullptr) {
        free(st->audioRing);
    }
    for (int i = 0; i < 24; i++) {
        st->video[i].data = nullptr;
    }
    if (st->player->audioStream != 0) {
        for (int i = 0; i < 24; i++) {
            st->audio[i].data = nullptr;
        }
    }
    st->ReleaseDirectSound();
    uvp_ReleaseVideoSurface(st);
    InterlockedExchange(UVP_L(st->player->runState), 6);
    st->player->state = nullptr;
    free(st);
    return 0;
}
HOOK_FUNC(0x00483f40, UvpPlayerStart);
