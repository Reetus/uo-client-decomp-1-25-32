// MusicManager: MIDI music playback. One static instance lives at 0x0068db30.
// Layout partial; see decomp/pass2/0x0047f0d0.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"
#include "globals.h"

class MusicManager {
public:
    // 0x0047f0d0: "music\\<quality dir><name>", resolved against both data paths; empty if neither opens.
    void __thiscall BuildMusicPath(const char *name, char *out);

    // 0x0047ed00: once per frame; random-track timer and fade-out of the current track.
    void __thiscall Tick();
    // 0x0047ef60: play track `track` (0..0x30) from the table at 0x004afc48.
    int __thiscall PlayTrack(uint16_t track, int arg2, int force);
    // 0x0047f210: open and play a MIDI file through MCI "sequencer".
    int __thiscall PlayMidi(const char *name, int forceClose);
    // 0x0047ec20: initial state, seed the original rand with GetTickCount, remember the MIDI volume at +0. Returns this.
    MusicManager *__thiscall Ctor();
    // 0x0047eca0: close the MCI device, restore the saved MIDI volume.
    void __thiscall Dtor();
    // 0x0047f3b0: store the volume (low byte) and apply it minus the fade accumulator to the MIDI output.
    void __thiscall SetVolume(int vol);
    // 0x0047f410: set the enabled flag; when disabling, close the MCI device.
    void __thiscall SetEnabled(int enable);
    // 0x0047f450: play the cycle track at +0xe; on success advance it, wrapping by player state.
    void __thiscall PlayNextInCycle();

    uint8_t pad_000[4];
    int volumeInitialised;   // +0x04
    int tickCountdown;       // +0x08  reset to 0x24
    uint16_t curTrack;       // +0x0c
    uint8_t pad_00e[2];
    int enabled;             // +0x10  music enabled
    int volume;              // +0x14
    int fading;              // +0x18
    int fadeAccum;           // +0x1c
    char pendingName[0x100]; // +0x20  name queued while a track is playing/fading  // ? real size of the buffer unknown, ends before +0x120
    uint32_t lastStartTick;  // +0x120
    uint32_t mciDevice;      // +0x124  MCI device id, 0 when closed
    int musicQuality;        // +0x128  0 plain, 1 "512K\\", 2 "4MB\\"
};

static_assert(offsetof(MusicManager, volumeInitialised) == 0x04, "MusicManager::volumeInitialised");
static_assert(offsetof(MusicManager, tickCountdown) == 0x08, "MusicManager::tickCountdown");
static_assert(offsetof(MusicManager, curTrack) == 0x0c, "MusicManager::curTrack");
static_assert(offsetof(MusicManager, enabled) == 0x10, "MusicManager::enabled");
static_assert(offsetof(MusicManager, volume) == 0x14, "MusicManager::volume");
static_assert(offsetof(MusicManager, fading) == 0x18, "MusicManager::fading");
static_assert(offsetof(MusicManager, fadeAccum) == 0x1c, "MusicManager::fadeAccum");
static_assert(offsetof(MusicManager, pendingName) == 0x20, "MusicManager::pendingName");
static_assert(offsetof(MusicManager, lastStartTick) == 0x120, "MusicManager::lastStartTick");
static_assert(offsetof(MusicManager, mciDevice) == 0x124, "MusicManager::mciDevice");
static_assert(offsetof(MusicManager, musicQuality) == 0x128, "MusicManager::musicQuality");

// The music manager object. WinMain (0x00480e80) calls PlayTrack with ECX = 0x0069fa98.
// (An earlier revision of this header said 0x0068db30; that address is the sound manager.)
#define g_musicManager GLOBAL_AS(MusicManager, g_musicManager_storage)
