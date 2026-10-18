// CPU identification helpers (family probe, CPUID feature flags).
#include <intrin.h>

#include <cstdint>
#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"

#define g_vendorInit(i) (g_cpuVendorInit[(i)])
#define g_vendorExpect(i) (g_cpuVendorExpect[(i)])

// 16-bit PUSHF / POPF equivalents on the 32-bit EFLAGS (a 16-bit POPF leaves the upper half unchanged).
static inline uint16_t read_flags16()
{
    return static_cast<uint16_t>(__readeflags());
}
static inline void write_flags16(uint16_t v)
{
    __writeeflags((__readeflags() & 0xffff0000u) | v);
}

// 0x00429190  size 0x1dc
// CPU family probe: 0 = 8086, 2 = 286, 3 = 386, 4 = 486, the CPUID family otherwise;
// |0x8000 when the vendor flag is set.
unsigned int __cdecl CpuFamilyProbe()
{
    char vendor[13];
    char expectedVendor[13];
    uint32_t famWord;
    unsigned char stepping;
    unsigned char model;
    int i;
    unsigned int flags, saved;
    unsigned int eax;
    int regs[4];

    famWord = 1;
    flags = __readeflags();
    __writeeflags(flags ^ 0x200000);
    if ((__readeflags() ^ flags) == 0)          // EFLAGS.ID cannot be toggled
        famWord = 0;

    if (static_cast<uint16_t>(famWord) != 0) {
        *reinterpret_cast<uint32_t *>(&vendor[0]) = g_vendorInit(0);
        *reinterpret_cast<uint32_t *>(&vendor[4]) = g_vendorInit(1);
        *reinterpret_cast<uint32_t *>(&vendor[8]) = g_vendorInit(2);
        vendor[12] = g_vendorInitByte;
        *reinterpret_cast<uint32_t *>(&expectedVendor[0]) = g_vendorExpect(0);
        *reinterpret_cast<uint32_t *>(&expectedVendor[4]) = g_vendorExpect(1);
        *reinterpret_cast<uint32_t *>(&expectedVendor[8]) = g_vendorExpect(2);
        expectedVendor[12] = g_vendorExpectByte;
        famWord = 0xffff;
        stepping = 0;
        __cpuid(regs, 0);                       // 0x00429215
        *reinterpret_cast<uint32_t *>(&vendor[0]) = static_cast<uint32_t>(regs[1]);   // EBX
        *reinterpret_cast<uint32_t *>(&vendor[4]) = static_cast<uint32_t>(regs[3]);   // EDX
        *reinterpret_cast<uint32_t *>(&vendor[8]) = static_cast<uint32_t>(regs[2]);   // ECX
        for (i = 0; i < 12; i++) {
            if (vendor[i] != expectedVendor[i])
                g_cpuVendorFlag = 1;
        }
        if (i >= 1) {                           // always true (i == 12)
            __cpuid(regs, 1);                   // 0x00429242
            eax = static_cast<unsigned int>(regs[0]);
            stepping = static_cast<unsigned char>(eax & 0xf);
            model = static_cast<unsigned char>((eax & 0xf0) >> 4);
            famWord = static_cast<uint16_t>(((eax & 0xf00) >> 8) & 0xf);
        }
        (void)stepping;
        (void)model;
        eax = famWord;
    } else {
        uint16_t ax, cx, bx;
        famWord = 0;
        // NEC V20/V30 style DIV flag test (AX = 0x5555, DX = 0, CX = 2, DIV CX), modelled as in
        // pass2: ZF after DIV is architecturally undefined.  // ?
        {
            uint16_t divAx = 0x5555;
            uint16_t divRem = static_cast<uint16_t>(divAx % 2);
            int cf = (divRem != 0);
            ax = static_cast<uint16_t>((cf & 1) ^ 1);
            famWord = ax;
            eax = (famWord & 0xff) & 1;
        }
        famWord = 0xffff;
        g_cpuVendorFlag = eax;                  // 0x0042929e
        // PUSHF ; POP AX ; MOV CX,AX ; AND AX,0xfff ; PUSH AX ; POPF ; PUSHF ; POP AX ; AND AX,0xf000
        cx = read_flags16();
        write_flags16(static_cast<uint16_t>(cx & 0xfff));
        ax = static_cast<uint16_t>(read_flags16() & 0xf000);
        famWord = (famWord & 0xffff0000u) | 0;
        if (ax != 0xf000)
            famWord = (famWord & 0xffff0000u) | 0xffff;
        write_flags16(cx);
        if (static_cast<uint16_t>(famWord) == 0) {      // 8086
            eax = famWord;
            goto done;
        }
        famWord = 0xffff;
        bx = read_flags16();
        cx = static_cast<uint16_t>(bx | 0xf000);
        write_flags16(cx);
        ax = static_cast<uint16_t>(read_flags16() & 0xf000);
        famWord = (famWord & 0xffff0000u) | 2;
        if (ax != 0)
            famWord = (famWord & 0xffff0000u) | 0xffff;
        write_flags16(bx);
        eax = famWord;
        if (static_cast<uint16_t>(famWord) == 2)        // 286
            goto done;
        famWord = 0xffff;
        // AND SP,0xfffc ; PUSHFD ; ... XOR EAX,0x40000 (EFLAGS.AC) ; POPFD ; PUSHFD ; XOR EAX,ECX
        saved = __readeflags();
        __writeeflags(saved ^ 0x40000);
        famWord = (famWord & 0xffff0000u) | 3;
        if ((__readeflags() ^ saved) != 0)              // AC toggled -> JZ not taken
            famWord = (famWord & 0xffff0000u) | 0xffff;
        __writeeflags(saved);
        eax = famWord & 0xffff;
        eax = famWord;
        if (static_cast<uint16_t>(famWord) != 3)        // 486
            eax = 4;
    }
done:
    if (g_cpuVendorFlag != 0)
        eax = eax | 0x8000;
    return eax;
}
HOOK_FUNC(0x00429190, CpuFamilyProbe);

// 0x00429370  size 0xb9
// CPUID: compare the vendor string, return CPUID(1).EDX feature flags (0 when CPUID is not available).
unsigned int __cdecl CpuFeatureFlags()
{
    char expectedVendor[13];
    char vendor[13];
    int hasCpuid;
    unsigned int features;
    int i;
    int regs[4];

    *reinterpret_cast<uint32_t *>(&vendor[0]) = g_vendorInit(0);
    *reinterpret_cast<uint32_t *>(&vendor[4]) = g_vendorInit(1);
    *reinterpret_cast<uint32_t *>(&vendor[8]) = g_vendorInit(2);
    vendor[12] = g_vendorInitByte;
    *reinterpret_cast<uint32_t *>(&expectedVendor[0]) = g_vendorExpect(0);
    *reinterpret_cast<uint32_t *>(&expectedVendor[4]) = g_vendorExpect(1);
    *reinterpret_cast<uint32_t *>(&expectedVendor[8]) = g_vendorExpect(2);
    expectedVendor[12] = g_vendorExpectByte;

    features = 0;
    hasCpuid = 1;

    unsigned int flags = __readeflags();
    __writeeflags(flags ^ 0x200000);
    if ((__readeflags() ^ flags) == 0)          // EFLAGS.ID cannot be toggled -> no CPUID
        hasCpuid = 0;

    if (static_cast<short>(hasCpuid) != 0) {
        __cpuid(regs, 0);                       // CPUID leaf 0
        *reinterpret_cast<uint32_t *>(&vendor[0]) = static_cast<uint32_t>(regs[1]);
        *reinterpret_cast<uint32_t *>(&vendor[4]) = static_cast<uint32_t>(regs[3]);
        *reinterpret_cast<uint32_t *>(&vendor[8]) = static_cast<uint32_t>(regs[2]);

        for (i = 0; i < 12; i++) {
            if (vendor[i] != expectedVendor[i])
                g_cpuVendorFlag = static_cast<uint32_t>(regs[3]);   // EDX = vendor dword 2 (loop-invariant)
        }

        if (i >= 1) {                           // always true here (i == 12)
            __cpuid(regs, 1);                   // CPUID leaf 1
            features = static_cast<unsigned int>(regs[3]);
        }
    }
    return features;
}
HOOK_FUNC(0x00429370, CpuFeatureFlags);
