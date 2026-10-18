// CSoundManager: shutdown, per-frame update, 3D positioned play, ambient switching; the sound lock helpers.
#include "sound_manager.h"

#include "../runtime/ported.h"
#include "com_call.h"
#include "globals.h"
#include "crt.h"
#include "raw_field.h"
#include "map_gump_ops_ports.h"

#define so_critSect reinterpret_cast<CRITICAL_SECTION *>(static_cast<uintptr_t>(GADDR(g_soundLock_storage)))
#define so_mainWnd GLOBAL_AS(HWND, g_mainWindow)
// x87 constants of the original image
static const float kVolSlope = -0.00039999998989515007f;   // 0x004a0ca4
static const float kOne = 1.0f;                             // 0x004a0ca8
static const float kVolScale = -2500.0f;                    // 0x004a0cac
static const float kDistScale = 0.0555555559694767f;        // 0x004a0cb8
static const float kHalf = 0.5f;                            // 0x004a0cbc
static const float kMinusOne = -1.0f;                       // 0x004a0cc0
static const float kPanScale = 2500.0f;                     // 0x004a0cc4
// map distance between two (x, y) int16 pairs; ECX = map object (unused by the callee)
#define so_MapDistance(a1, a2) (MapDistancePoints(a1, a2))

// Volume the update loop gives a fading voice: the x87 sequence at 0x0047ff64 / 0x0047ffc6.
// t = scale * (1 - v * slope) is stored as a float; result = (1 - t) * -2500, truncated.
static long SoFadeVolume(CSoundVoice *voice, int v)
{
    double a = static_cast<double>(v) * static_cast<double>(kVolSlope);
    double b = static_cast<double>(kOne) - a;
    float t = static_cast<float>(static_cast<double>(voice->volumeScale) * b);
    return static_cast<long>((static_cast<double>(kOne) - static_cast<double>(t)) * static_cast<double>(kVolScale));
}

// 0x0047f960  size 0x6f
void __thiscall CSoundManager::Shutdown()
{
    if (primaryBuffer == nullptr)
        return;
    for (int i = 0; i < 8; i++) {
        CSoundVoice *v = voices[i];
        if (v != nullptr) {
            if (v->dsBuffer != nullptr)
                com_call(PORT_RET(long), v->dsBuffer, 0x08);   // IDirectSoundBuffer::Release
            operator delete(v);
        }
        voices[i] = nullptr;
    }
    com_call(PORT_RET(long), primaryBuffer, 0x08);
    primaryBuffer = nullptr;
    if (pDirectSound != nullptr)
        com_call(PORT_RET(long), pDirectSound, 0x08);   // IDirectSound::Release
    pDirectSound = nullptr;
}
HOOK_METHOD(0x0047f960, CSoundManager, Shutdown);

// 0x0047f9d0  size 0xb
int __thiscall CSoundManager::IsInitialized()
{
    return primaryBuffer != nullptr ? 1 : 0;
}
HOOK_METHOD(0x0047f9d0, CSoundManager, IsInitialized);

// 0x0047fef0  size 0x11d
// When enabled and initialised: refill every voice; ramp down the outgoing ambient voice (stop it when the
// parameter reaches -2500, otherwise SetVolume), and ramp the current ambient voice towards -2200.
void __thiscall CSoundManager::Update()
{
    if (enabled == 0 || primaryBuffer == nullptr)
        return;
    for (int i = 0; i < 8; i++)
        voices[i]->Update();

    if (currentVoice != -1) {
        int v = unk_64 - 0x14;
        unk_64 = v;
        if (v <= -2500) {
            CSoundVoice *voice = voices[currentVoice];
            com_call(PORT_RET(long), voice->dsBuffer, 0x48);   // Stop
            unk_64 = -10000;
            unk_5c = 0;
            currentVoice = -1;
        } else {
            CSoundVoice *voice = voices[currentVoice];
            com_call(PORT_RET(long), voice->dsBuffer, 0x3c, SoFadeVolume(voice, v));   // SetVolume
        }
    }
    if (ambientVoice != -1) {
        int v = ambientParam + 0x14;
        ambientParam = v;
        if (v > -2200)
            ambientParam = -2200;
        CSoundVoice *voice = voices[ambientVoice];
        com_call(PORT_RET(long), voice->dsBuffer, 0x3c, SoFadeVolume(voice, ambientParam));   // SetVolume
    }
}
HOOK_METHOD(0x0047fef0, CSoundManager, Update);

// 0x00480010  size 0xdc
// Positional sound (not a method: ECX is loaded with the manager). Stack: sound id, world position
// (int16 x, y), loudness scale (float), loop flag, frequency jitter, allow-duplicate; RET 0x18.
// Distance (map units / 18, capped at 1) scales the volume; the x/y offset to the player sets the pan.
// ? the parameter meanings are inferred from the arithmetic
int __stdcall PlaySound3D(int soundId, const int16_t *pos, float loudness, int loopFlag, int freqJitter,
                                 int allowDuplicate)
{
    const int16_t *playerPos = reinterpret_cast<const int16_t *>(reinterpret_cast<char *>(g_player) + 0x20);
    int dist = so_MapDistance(pos, playerPos);   // 0x00445280
    double d = static_cast<double>(dist) * static_cast<double>(kDistScale);
    long vol;
    long pan;

    if (!(d <= static_cast<double>(kOne)))
        d = static_cast<double>(kOne);
    vol = static_cast<long>(d * static_cast<double>(loudness) * static_cast<double>(kVolScale));

    int dx = static_cast<int>(playerPos[1]) - static_cast<int>(pos[1]) - static_cast<int>(playerPos[0]) +
             static_cast<int>(pos[0]);
    double p = static_cast<double>(dx) * static_cast<double>(kHalf) * static_cast<double>(kDistScale);
    if (!(p <= static_cast<double>(kOne)))
        p = static_cast<double>(kOne);
    if (p < static_cast<double>(kMinusOne))
        p = static_cast<double>(kMinusOne);
    pan = static_cast<long>(p * static_cast<double>(kPanScale));
    if (so_stereoFlip != 0)
        pan = -pan;
    return g_soundManager.PlaySound(soundId, pan, vol, loopFlag, freqJitter, allowDuplicate);
}
HOOK_FUNC(0x00480010, PlaySound3D);

// 0x00480250  size 0xd9
// Make `soundId` the ambient loop. The outgoing ambient sound moves to the fade-out slot (+0x5c/+0x60/+0x64);
// asking for the sound that is currently fading out swaps the two.
void __thiscall CSoundManager::SetAmbientSound(int soundId)
{
    uint16_t oldAmbient = ambientSound;
    int oldId = oldAmbient;

    if (soundId == oldId)
        return;
    if (soundId == static_cast<int>(unk_5c) && soundId != 0xffff && soundId != 0 && currentVoice != -1) {
        int fadeVoice = currentVoice;
        int fadeParam = unk_64;

        ambientSound = unk_5c;
        currentVoice = ambientVoice;
        int curParam = ambientParam;
        unk_5c = oldId;
        ambientVoice = fadeVoice;
        ambientParam = fadeParam;
        unk_64 = curParam;
        return;
    }
    int voiceSave = ambientVoice;
    int paramSave = ambientParam;

    unk_5c = oldAmbient;
    currentVoice = voiceSave;
    unk_64 = paramSave;
    ambientSound = static_cast<uint16_t>(soundId);
    ambientParam = -2500;
    if (static_cast<uint16_t>(soundId) == 0xffff || static_cast<uint16_t>(soundId) == 0) {
        ambientVoice = -1;
        return;
    }
    ambientVoice = PlaySound(soundId, 0, -2500, 1, 0, 0);
    if (ambientVoice == currentVoice && ambientVoice != -1) {
        unk_5c = 0;
        currentVoice = -1;
        unk_64 = -10000;
    }
}
HOOK_METHOD(0x00480250, CSoundManager, SetAmbientSound);

// 0x00480ae0  size 0x26
// ECX = sound id, EDX = pan, stack = volume (RET 4). Plays on the manager when it is enabled. No return value (EAX is whatever is left).
void __fastcall PlaySoundEffectIfEnabled(int soundId, int pan, long vol)
{
    if (g_soundManager.enabled != 0)
        g_soundManager.PlaySound(soundId, pan, vol, 0, 0, 0);
}
HOOK_FUNC(0x00480ae0, PlaySoundEffectIfEnabled);

// 0x00480bc0  size 0x47
// Wait until no other thread holds the sound buffers, then mark them held.
void __cdecl LockSoundBuffers()
{
    for (;;) {
        EnterCriticalSection(so_critSect);
        if (g_soundBusy == 0)
            break;
        LeaveCriticalSection(so_critSect);
        Sleep(1);
    }
    g_soundBusy = 1;
    LeaveCriticalSection(so_critSect);
}
HOOK_FUNC(0x00480bc0, LockSoundBuffers);

// 0x00480c10  size 0x21
void __cdecl UnlockSoundBuffers()
{
    EnterCriticalSection(so_critSect);
    g_soundBusy = 0;
    LeaveCriticalSection(so_critSect);
}
HOOK_FUNC(0x00480c10, UnlockSoundBuffers);

// 0x00480e20  size 0x19
// WM_COMMAND 0x9c99 to the main window.
LRESULT __cdecl PostMenuCommand9c99()
{
    return SendMessageA(so_mainWnd, WM_COMMAND, 0x9c99, 0);
}
HOOK_FUNC(0x00480e20, PostMenuCommand9c99);
