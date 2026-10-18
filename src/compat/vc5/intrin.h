// Stand-in for <intrin.h>, which Visual C++ 5.0 does not ship. Only the five intrinsics the
// sources use, written as inline assembly (VC5 has no compiler intrinsics for them).
#ifndef VC5_COMPAT_INTRIN_H
#define VC5_COMPAT_INTRIN_H

// The assembly bodies return through eax/edx without a return statement.
#pragma warning(disable: 4035)

// __cpuid(regs, leaf): regs = { eax, ebx, ecx, edx } after CPUID.
static __inline void __cpuid(int regs[4], int leaf)
{
    __asm {
        mov   edi, regs
        mov   eax, leaf
        _emit 0x0F              // cpuid (VC5's assembler does not know the mnemonic)
        _emit 0xA2
        mov   dword ptr [edi],      eax
        mov   dword ptr [edi + 4],  ebx
        mov   dword ptr [edi + 8],  ecx
        mov   dword ptr [edi + 12], edx
    }
}

// __rdtsc(): 64-bit time-stamp counter.
static __inline unsigned __int64 __rdtsc(void)
{
    __asm {
        _emit 0x0F              // rdtsc, result in edx:eax = the __int64 return value
        _emit 0x31
    }
}

// __readeflags() / __writeeflags(v): whole EFLAGS register.
static __inline unsigned int __readeflags(void)
{
    __asm {
        pushfd
        pop   eax
    }
}

static __inline void __writeeflags(unsigned int v)
{
    __asm {
        push  v
        popfd
    }
}

// _BitScanForward(&index, mask): index of the lowest set bit; returns 0 when mask == 0.
static __inline unsigned char _BitScanForward(unsigned long *index, unsigned long mask)
{
    __asm {
        mov   edx, index
        mov   ecx, mask
        xor   eax, eax
        bsf   ecx, ecx
        jz    done
        mov   dword ptr [edx], ecx
        mov   al, 1
    done:
    }
}

// Port I/O and interrupt flag: VC5 has these as intrinsics under other names (<conio.h>, <stdlib.h>).
// Each expands to a single in/out/cli/sti instruction (checked in the object code).
#include <conio.h>
extern "C" void __cdecl _disable(void);
extern "C" void __cdecl _enable(void);
#pragma intrinsic(_inp, _outp, _disable, _enable)
#define __outbyte(port, data) ((void)_outp((unsigned short)(port), (int)(unsigned char)(data)))
#define __inbyte(port)        ((unsigned char)_inp((unsigned short)(port)))

#pragma warning(default: 4035)

#endif
