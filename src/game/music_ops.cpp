// MusicManager: constructor, destructor, volume, enable flag and the track cycle.
#include "music_manager.h"

#include <windows.h>
#include <mmsystem.h>

#include "../runtime/ported.h"
#include "globals.h"
#include "raw_field.h"
#include "crt.h"

#pragma comment(lib, "winmm.lib")

// original CRT srand

// midiOutGetDevCapsA(-1, ...) and test MIDICAPS_VOLUME (dwSupport & 1).
static inline bool MoMidiHasVolume()
{
    MIDIOUTCAPSA caps;
    midiOutGetDevCapsA(static_cast<UINT_PTR>(-1), &caps, 0x34);
    return (caps.dwSupport & 1) != 0;
}

// 0x0047ec20  size 0x7b
MusicManager *__thiscall MusicManager::Ctor()
{
    RAWF(int32_t, this, 0x00) = -1;           // saved MIDI volume, filled in below
    volumeInitialised = 0;
    curTrack = 0;
    RAWF(uint16_t, this, 0x0e) = 0;           // next track of the cycle
    enabled = mo_defaultEnabled;
    volume = g_musicDefaultVolume;
    fading = 0;
    fadeAccum = 0;
    lastStartTick = 0;
    mciDevice = 0;
    musicQuality = 0;
    srand(GetTickCount());
    // midiOutGetDevCapsA is called inside MoMidiHasVolume
    if (MoMidiHasVolume())
        midiOutGetVolume(reinterpret_cast<HMIDIOUT>(static_cast<UINT_PTR>(-1)), reinterpret_cast<LPDWORD>(this));
    return this;
}
HOOK_METHOD(0x0047ec20, MusicManager, Ctor);

// 0x0047eca0  size 0x51
void __thiscall MusicManager::Dtor()
{
    if (mciDevice != 0) {
        mciSendCommandA(mciDevice, 0x804, 0, 0);   // MCI_CLOSE
        mciDevice = 0;
    }
    // midiOutGetDevCapsA is called inside MoMidiHasVolume
    if (MoMidiHasVolume())
        midiOutSetVolume(reinterpret_cast<HMIDIOUT>(static_cast<UINT_PTR>(-1)), RAWF(uint32_t, this, 0x00));
}
HOOK_METHOD(0x0047eca0, MusicManager, Dtor);

// 0x0047f3b0  size 0x5f
void __thiscall MusicManager::SetVolume(int vol)
{
    int v = vol & 0xff;
    int eff;

    volume = v;
    eff = v - fadeAccum;
    if (eff < 0)
        eff = 0;
    if (eff > 0xff)
        eff = 0xff;
    // midiOutGetDevCapsA is called inside MoMidiHasVolume
    if (MoMidiHasVolume()) {
        DWORD b = static_cast<DWORD>(eff & 0xff);
        midiOutSetVolume(reinterpret_cast<HMIDIOUT>(static_cast<UINT_PTR>(-1)), ((b << 16) | b) << 8);
    }
}
HOOK_METHOD(0x0047f3b0, MusicManager, SetVolume);

// 0x0047f410  size 0x34
void __thiscall MusicManager::SetEnabled(int enable)
{
    enabled = enable;
    if (enable == 0 && mo_mciDeviceGlobal != 0) {
        mciSendCommandA(mo_mciDeviceGlobal, 0x804, 0, 0);   // MCI_CLOSE
        mo_mciDeviceGlobal = 0;
    }
}
HOOK_METHOD(0x0047f410, MusicManager, SetEnabled);

// 0x0047f450  size 0x5b
// The cycle wraps at 0x31 without a player, and by the player's +0x150 flag otherwise:
// flag set -> back to 0x26 at 0x29, flag clear -> back to 0 at 9.
// ? EAX on return is not a meaningful value (whatever was left in AX); callers are assumed to ignore it
void __thiscall MusicManager::PlayNextInCycle()
{
    uint16_t &next = RAWF(uint16_t, this, 0x0e);

    if (PlayTrack(next, 0, 0) == 0)
        return;
    next++;
    if (g_player != nullptr) {
        if (RAWF(int32_t, g_player, 0x150) != 0) {
            if (next >= 0x29)
                next = 0x26;
        } else if (next >= 9) {
            next = 0;
        }
    } else if (next >= 0x31) {
        next = 0;
    }
}
HOOK_METHOD(0x0047f450, MusicManager, PlayNextInCycle);
