// CPU speed helpers around 0x00478740: the registry lookup (0x00478670) and the QueryPerformanceCounter
// estimate (0x00478bc0). (Porting pass 2, batch 29.)
#include <windows.h>

#include <intrin.h>

#include <cstdint>

#include "../runtime/ported.h"
#include "cpu_speed_ports.h"

// 0x00478740 (ported in cpu_speed.cpp): ECX = requested MHz, stack = 16-byte result buffer (also returned), RET 4.
#define Cpu_Measure(a1, a2, a3) (CpuSpeedMeasure(a1, a2, a3))

// 0x00478670  size 0xc9  ECX = requested MHz (forwarded to 0x00478740). Reads "~MHz" (then "~Mhz", "~mhz") from
// HKLM\HARDWARE\DESCRIPTION\System\CentralProcessor\0 and returns it; if the key or all three values are missing
// returns the fourth word of the measurement 0x00478740.
uint32_t __fastcall GetCpuSpeedMHz(int requestedMHz)
{
    HKEY key;
    DWORD value;
    DWORD cb;
    LONG rc;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, 0x20019, &key) == 0) {
        value = 0;
        cb = 4;
        rc = RegQueryValueExA(key, "~MHz", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &cb);
        if (rc != 0) {
            rc = RegQueryValueExA(key, "~Mhz", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &cb);
            if (rc != 0)
                rc = RegQueryValueExA(key, "~mhz", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &cb);
        }
        RegCloseKey(key);
        if (rc == 0)
            return value;
    }
    struct CpuResult {
        uint32_t w[4];
    };
    CpuResult out;
    // The original copies the four result words into a second local that is never read.
    CpuResult res = *reinterpret_cast<CpuResult *>(
        PORT_THISCALL1(&CpuSpeedMeasure, uint32_t *, reinterpret_cast<void *>(requestedMHz), uint32_t *)(out.w));   // 0x00478740
    return res.w[3];
}
HOOK_FUNC(0x00478670, GetCpuSpeedMHz);

// 0x00478bc0  size 0xe5  QueryPerformanceCounter estimate. ECX = a speed guess (the original 0x00478740 passes its
// table value * 4000 or requested MHz * 4000 there), stack = 16-byte result buffer, RET 4, returns the buffer.
// out[0] = ECX, out[1] = scaled best delta, out[2] = ECX / out[1], out[3] = the same rounded.
// Zeroed when the performance counter is missing. The smallest of ten delays of 4000 BSF instructions is used.
uint32_t *__fastcall MeasureCpuSpeedQPC(uint32_t guess, int /*edx*/, uint32_t *out)
{
    LARGE_INTEGER freq;
    LARGE_INTEGER t0;
    LARGE_INTEGER t1;
    uint32_t best = 0xffffffffu;

    if (QueryPerformanceFrequency(&freq) == 0) {
        out[0] = 0;
        out[1] = 0;
        out[2] = 0;
        out[3] = 0;
        return out;
    }
    for (int round = 10; round != 0; round--) {
        QueryPerformanceCounter(&t0);
        // 4000 times "bsf ecx, 0x80000000"; a dependent chain so the loop is not removed
        volatile unsigned long idx = 0;
        for (unsigned n = 0xfa0; n != 0; n--) {
            unsigned long i;
            _BitScanForward(&i, 0x80000000u);
            idx = i;
        }
        QueryPerformanceCounter(&t1);
        uint32_t delta = t1.LowPart - t0.LowPart;
        if (delta < best)
            best = delta;
    }
    uint32_t f = freq.LowPart;
    uint32_t tenth = static_cast<uint32_t>((static_cast<uint64_t>(f) * 0xcccccccdui64) >> 32) >> 3;
    uint32_t scaled = (best * 3125u) << 5;
    uint32_t q = scaled / tenth;
    uint32_t r = q % f;
    uint32_t speed = q;
    if (r > (f >> 1))
        speed++;
    uint32_t mhz = guess / speed;
    uint32_t rem = guess % speed;
    uint32_t mhzRounded = mhz;
    if (rem > (speed >> 1))
        mhzRounded++;
    out[0] = guess;
    out[1] = speed;
    out[2] = mhz;
    out[3] = mhzRounded;
    return out;
}
HOOK_FUNC(0x00478bc0, MeasureCpuSpeedQPC);
