// CPU speed measurement (0x00478740).
#include <windows.h>

#include <intrin.h>

#include <cstdint>
#include <cstdlib>

#include "../runtime/ported.h"
#include "cpu_probe_ports.h"
#include "cpu_speed_qpc_ports.h"
#include "globals.h"

// 0x00429190: CPU feature word (low 16 bits used; bit 0x8000 = no timestamp counter support path).
#define Cpu_GetFeatures() ((uint32_t)CpuFamilyProbe())
// 0x00429370: CPU flag word (bit 0x10 = rdtsc available).
#define Cpu_GetFlags() ((uint32_t)CpuFeatureFlags())
// 0x00478bc0: fallback estimate, ECX = speed guess, one stack arg (16-byte result buffer), callee pops, returns the buffer.
#define Cpu_Estimate(a1, a2, a3) (MeasureCpuSpeedQPC(a1, (int /*edx*/)(a2), a3))

// 0x00478740  size 0x47d  CPU speed measurement (rdtsc against QueryPerformanceCounter, or against the CMOS RTC seconds register)
// ECX = requested MHz (<= 0: measure with QueryPerformanceCounter; 1..150: trust a fixed speed guess; > 150: RTC path),
// the single stack argument (RET 4) is a hidden 16-byte result pointer, also returned in EAX.
// out[0] = tsc delta, out[1] = time delta, out[2] = MHz (truncated), out[3] = MHz (rounded).
uint32_t *__fastcall CpuSpeedMeasure(int requestedMHz, int /*edx unused*/, uint32_t *out)
{
    uint16_t cpuFeat;
    uint32_t cpuFlags;
    int fixedSpeed = 0;

    cpuFeat = static_cast<uint16_t>(Cpu_GetFeatures());
    cpuFlags = Cpu_GetFlags();

    if (cpuFeat & 0x8000) {
        out[0] = 0;
        out[1] = 0;
        out[2] = 0;
        out[3] = 0;
        return out;
    }

    // Speed guess handed to the fallback estimate (0x00478bc0) in ECX: the per-family table value or the
    // requested MHz, times 4000. Above 150 MHz the original leaves the result pointer in ECX.
    uint32_t guess;
    if (requestedMHz <= 0) {
        guess = g_004af788[cpuFeat] * 4000u;
    } else if (requestedMHz <= 0x96) {
        guess = static_cast<uint32_t>(requestedMHz) * 4000u;
        fixedSpeed = 1;
    } else {
        guess = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(out));
    }

    if ((cpuFlags & 0x10) == 0 || fixedSpeed != 0) {
        if (cpuFeat < 3) {
            out[0] = 0;
            out[1] = 0;
            out[2] = 0;
            out[3] = 0;
            return out;
        }
        uint32_t buf[4];
        uint32_t *r = Cpu_Estimate(guess, 0, buf);
        out[0] = r[0];
        out[1] = r[1];
        out[2] = r[2];
        out[3] = r[3];
        return out;
    }

    if (requestedMHz == 0) {
        // ---- QueryPerformanceCounter path ----
        uint32_t slot8 = 0;     // previous sample
        int iter = 0;
        uint32_t sumDT = 0;
        uint32_t sumQ = 0;
        uint32_t slot14 = 0;    // sample before previous
        uint32_t best = 0;
        uint32_t q = 0, dt;
        uint32_t tscLo0 = 0, tscLo1 = 0;
        LARGE_INTEGER freq;
        LARGE_INTEGER t0;
        LARGE_INTEGER t1;
        uint32_t res0 = 0, res1 = 0;
        uint32_t mhzFloor = 0, mhzRound = 0;
        uint32_t tenths;
        HANDLE hThread = GetCurrentThread();
        int prio;

        if (QueryPerformanceFrequency(&freq) == 0) {
            out[0] = res0;
            out[1] = res1;
            out[2] = 0;
            out[3] = 0;
            return out;
        }

        for (;;) {
            iter++;
            slot14 = slot8;
            slot8 = best;
            QueryPerformanceCounter(&t0);
            t1.LowPart = t0.LowPart;
            t1.HighPart = t0.HighPart;
            prio = GetThreadPriority(hThread);
            if (prio != 0x7fffffff)
                SetThreadPriority(hThread, 0xf);

            while (static_cast<uint32_t>(t1.LowPart - t0.LowPart) < 0x32) {
                QueryPerformanceCounter(&t1);
                tscLo0 = static_cast<uint32_t>(__rdtsc());
            }
            t0.LowPart = t1.LowPart;
            t0.HighPart = t1.HighPart;
            do {
                QueryPerformanceCounter(&t1);
                tscLo1 = static_cast<uint32_t>(__rdtsc());
            } while (static_cast<uint32_t>(t1.LowPart - t0.LowPart) < 0x3e8);

            if (prio != 0x7fffffff)
                SetThreadPriority(hThread, prio);

            dt = tscLo1 - tscLo0;
            q = ((t1.LowPart - t0.LowPart) * 100000) / (static_cast<uint32_t>(freq.LowPart) / 10);
            sumQ += q;
            sumDT += dt;
            if ((q % freq.LowPart) > (freq.LowPart >> 1))
                q++;
            best = dt / q;
            if ((dt % q) > (q >> 1))
                best++;

            uint32_t sum = slot14 + slot8 + best;
            if (iter < 3)
                continue;
            if (iter >= 0x14)
                break;
            if (abs(static_cast<int>(best * 3 - sum)) > 3)
                continue;
            if (abs(static_cast<int>(slot8 * 3 - sum)) > 3)
                continue;
            if (abs(static_cast<int>(slot14 * 3 - sum)) > 3)
                continue;
            break;
        }

        tenths = (sumDT * 10) / sumQ;
        if (((sumDT * 100) % sumQ) - tenths * 10 >= 6)
            tenths++;
        mhzFloor = sumDT / sumQ;
        mhzRound = mhzFloor;
        if (tenths - mhzFloor * 10 >= 6)
            mhzRound = mhzFloor + 1;
        out[0] = sumDT;
        out[1] = sumQ;
        out[2] = mhzFloor;
        out[3] = mhzRound;
        return out;
    }

    // ---- CMOS RTC seconds path (requestedMHz != 0) ----
    {
        HANDLE hThread = GetCurrentThread();
        int prio = GetThreadPriority(hThread);
        int sec0, sec1, sec2, d;
        uint32_t t0lo, t1lo;
        uint32_t diffLo, mhz, mhzR;

        if (prio != 0x7fffffff)
            SetThreadPriority(hThread, prio + 1);

        __outbyte(0x70, 0);
        sec0 = __inbyte(0x71);
        do {
            __outbyte(0x70, 0);
            sec1 = __inbyte(0x71);
            if (sec1 < sec0)
                d = sec1 - sec0 + 10;
            else
                d = (sec1 - sec0) > 0;
        } while (d == 0);

        t0lo = static_cast<uint32_t>(__rdtsc());
        do {
            __outbyte(0x70, 0);
            sec2 = __inbyte(0x71);
            if (sec2 < sec1)
                d = sec2 - sec1 + 10;
            else
                d = (sec2 - sec1) > 0;
        } while (d == 0);
        t1lo = static_cast<uint32_t>(__rdtsc());
        if (prio != 0x7fffffff)
            SetThreadPriority(hThread, prio);

        diffLo = t1lo - t0lo;   // ? the original also keeps the high word difference, which is never used
        mhz = diffLo / 1000000;
        mhzR = mhz;
        if ((diffLo / 100000) - mhz * 10 >= 6)
            mhzR = mhz + 1;
        out[0] = diffLo;
        out[1] = static_cast<uint32_t>(sec2 * 1000000 - sec1 * 1000000);
        out[2] = mhz;
        out[3] = mhzR;
        return out;
    }
}
HOOK_FUNC(0x00478740, CpuSpeedMeasure);
