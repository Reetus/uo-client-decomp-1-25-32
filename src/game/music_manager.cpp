#include "music_manager.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <windows.h>
#include <mmsystem.h>

#include "../runtime/ported.h"
#include "app.h"
#include "globals.h"
#include "crt.h"
#include "sound_ops_ports.h"

#pragma comment(lib, "winmm.lib")

#define g_mainWindowHandle GLOBAL_AS(HWND, g_mainWindow)
// Original rand() (0x0048c6c0) keeps its own state.
// Sound lock bracket (critical section 0x00758288, busy flag 0x00758a14).
#define SoundLockAcquire() (LockSoundBuffers())
#define SoundLockRelease() (UnlockSoundBuffers())

// mciSendCommand(MCI_STATUS, MCI_STATUS_ITEM) with MCI_STATUS_MODE: true when the device is in MCI_MODE_PLAY (0x20e).
static inline int MciIsPlaying(UINT device)
{
    MCI_STATUS_PARMS st;
    st.dwItem = 4;   // MCI_STATUS_MODE
    if (mciSendCommandA(device, 0x814, 0x100, (DWORD_PTR)&st) != 0)
        return 0;
    return st.dwReturn == 0x20e ? 1 : 0;
}

// midiOutGetDevCapsA(-1, ...); if MIDICAPS_VOLUME: midiOutSetVolume(-1, v in both channels' high byte).
static inline void ApplyMidiVolume(int v)
{
    MIDIOUTCAPSA caps;
    midiOutGetDevCapsA((UINT_PTR)-1, &caps, 0x34);
    if (caps.dwSupport & 1)
        midiOutSetVolume((HMIDIOUT)(UINT_PTR)-1, (((DWORD)(v & 0xff) << 16) | (DWORD)(v & 0xff)) << 8);
}

// 0x0047f0d0  size 0x133
void __thiscall MusicManager::BuildMusicPath(const char *name, char *out)
{
    char subdir[260];
    subdir[0] = '\0';
    switch (musicQuality) {
    case 0:
        break;
    case 1:
        strcpy(subdir, "512K\\");
        break;
    case 2:
        strcpy(subdir, "4MB\\");
        break;
    default:
        break;
    }
    sprintf(out, "music\\%s%s", subdir, name);

    FILE *fp = fopen(g_app.ResolveDataPath(out), g_fopenReadMode);
    if (fp != nullptr) {
        fclose(fp);
        strcpy(out, g_app.ResolveDataPath(out));
        return;
    }
    fp = fopen(g_app.ResolveDataPath2(out), g_fopenReadMode);
    if (fp != nullptr) {
        fclose(fp);
        strcpy(out, g_app.ResolveDataPath2(out));
        return;
    }
    out[0] = '\0';
}
HOOK_METHOD(0x0047f0d0, MusicManager, BuildMusicPath);

// 0x0047ed00  size 0x253
// Per-frame: every 0x24 ticks start a random track when idle; fade out the current track
// when a change is pending, then replay the queued name.
void __thiscall MusicManager::Tick()
{
    int playing;
    int inRange;
    int track;
    int v;

    tickCountdown = tickCountdown - 1;
    if (tickCountdown <= 0) {
        tickCountdown = 0x24;
        if (enabled != 0) {
            playing = MciIsPlaying(mciDevice);
            if (playing != 0 && curTrack >= 0x26 && curTrack < 0x29)
                inRange = 1;
            else
                inRange = 0;

            if (g_player != nullptr) {
                const int flag = *(int *)((char *)g_player + 0x150);   // ? meaning of the player field at +0x150 unknown
                if (flag != 0 && inRange == 0) {
                    track = rand() % 3 + 0x26;
                    PlayTrack((uint16_t)track, 1, 1);
                } else if (flag == 0) {
                    bool skipPlay = false;
                    if (inRange == 0) {
                        playing = MciIsPlaying(mciDevice);
                        if (playing != 0)
                            skipPlay = true;   // jumps to the fade code
                    }
                    if (!skipPlay) {
                        track = rand() % 9;
                        PlayTrack((uint16_t)track, inRange, inRange);
                    }
                }
                // else: flag != 0 and inRange != 0 -> nothing
            }
        }
    }

    if (fading != 0) {
        fadeAccum = fadeAccum + 10;
        if (volume - fadeAccum >= 0) {
            playing = MciIsPlaying(mciDevice);
            if (playing != 0) {
                // ? the original does MOV AL,[this+0x14]; MOV [this+0x14],EAX, so the upper bits of EAX
                // ? at that point would land in the field; pass2 reads it as a plain byte truncation
                volume = (uint8_t)volume;
                v = volume - fadeAccum;
                if (v < 0)
                    v = 0;
                if (v > 0xff)
                    v = 0xff;
                ApplyMidiVolume(v);
                return;
            }
        }
        // finish the fade
        v = (uint8_t)volume;
        fadeAccum = 0;
        volume = v;
        if (v < 0)
            v = 0;   // dead in practice (v is a byte)
        if (v > 0xff)
            v = 0xff;
        ApplyMidiVolume(v);
        fading = 0;
        if (mciDevice != 0) {
            mciSendCommandA(mciDevice, 0x804, 0, 0);   // MCI_CLOSE
            mciDevice = 0;
        }
        PlayMidi(pendingName, 1);
        pendingName[0] = 0;
    }
}
HOOK_METHOD(0x0047ed00, MusicManager, Tick);

// 0x0047ef60  size 0x169
// Play MIDI track `track` by index (0..0x30); refuses within 10 s of the last start unless forced.
int __thiscall MusicManager::PlayTrack(uint16_t track, int arg2, int force)
{
    DWORD now;
    int playing;
    int result;
    int v;
    uint8_t vol;

    now = GetTickCount();
    playing = MciIsPlaying(mciDevice);
    if (playing != 0 && force == 0 && now < lastStartTick + 0x2710)
        return 1;

    if (track >= 0x31)
        return 0;
    if (enabled == 0)
        return 1;
    if (track == curTrack)
        return 1;

    SoundLockAcquire();
    result = PlayMidi(g_trackNames[track], arg2);
    arg2 = result;
    SoundLockRelease();

    if (result != 0) {
        if (volumeInitialised == 0) {
            vol = g_musicDefaultVolume;
            volume = vol;
            v = vol - fadeAccum;
            if (v < 0)
                v = 0;
            if (v > 0xff)
                v = 0xff;
            ApplyMidiVolume(v);
            volumeInitialised = 1;
        }
        curTrack = track;
        lastStartTick = GetTickCount();
    }
    return result;
}
HOOK_METHOD(0x0047ef60, MusicManager, PlayTrack);

// 0x0047f210  size 0x1a0
// Open and play a MIDI file through MCI "sequencer". While something plays or fades, the
// requested name is queued in pendingName and the fade is started.
int __thiscall MusicManager::PlayMidi(const char *name, int forceClose)
{
    MCI_OPEN_PARMSA open;
    MCI_STATUS_PARMS st2;
    MCI_PLAY_PARMS play;
    char path[260];
    int playing;

    if (enabled == 0)
        return 1;

    playing = MciIsPlaying(mciDevice);

    if (playing != 0 || fading != 0) {
        fading = 1;
        strcpy(pendingName, name);
        return 1;
    }

    if (mciDevice != 0) {
        if (forceClose == 0)
            return -1;
        mciSendCommandA(mciDevice, 0x804, 0, 0);   // MCI_CLOSE
        mciDevice = 0;
    }

    open.lpstrDeviceType = g_sequencerType;
    BuildMusicPath(name, path);
    open.lpstrElementName = path;
    if (mciSendCommandA(0, 0x803, 0x2200, (DWORD_PTR)&open) != 0) {   // MCI_OPEN, MCI_OPEN_TYPE|MCI_OPEN_ELEMENT
        mciDevice = 0;
        return 0;
    }
    mciDevice = open.wDeviceID;

    st2.dwItem = 0x4003;   // MCI_SEQ_STATUS_PORT
    if (mciSendCommandA(mciDevice, 0x814, 0x100, (DWORD_PTR)&st2) != 0) {
        mciSendCommandA(mciDevice, 0x804, 0, 0);   // MCI_CLOSE
        return 0;
    }

    play.dwCallback = (DWORD_PTR)g_mainWindowHandle;
    if (mciSendCommandA(mciDevice, 0x806, 1, (DWORD_PTR)&play) != 0) {   // MCI_PLAY, MCI_NOTIFY
        mciSendCommandA(mciDevice, 0x804, 0, 0);   // MCI_CLOSE
        return 0;
    }
    return 1;
}
HOOK_METHOD(0x0047f210, MusicManager, PlayMidi);
