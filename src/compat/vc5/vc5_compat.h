// Force-included (-FIvc5_compat.h) into every translation unit of the Visual C++ 5.0 build.
// It only papers over differences of spelling between the C++ the sources are written in
// and the C++ Visual C++ 5.0 accepts. It does not emulate variadic macros, variadic templates,
// lambdas, auto, or any other construct that needs real work in the sources.
#ifndef VC5_COMPAT_H
#define VC5_COMPAT_H

// std::memcpy etc.: VC5's <cstring>, <cmath> put these names in the global namespace only.
// Pull in the five the sources use. (std::nothrow already exists in VC5's <new>.)
#include <string.h>
#include <math.h>
#include <new>
namespace std {
    using ::memcpy;
    using ::memset;
    using ::sqrt;
    using ::sin;
    using ::cos;
}

// nullptr -> 0: VC5 has no nullptr. Cannot tell a null pointer from an integer 0 in overload
// resolution or template deduction; the build will show where that matters.
#define nullptr 0

// static_assert(cond, msg) -> negative-size typedef array (C2118 when cond is false). Works at
// namespace, class and block scope. The name is made unique by __LINE__. The one-argument
// form of C++17 is not supported; the macro takes exactly two arguments.
#define VC5_PASTE2_(a, b) a##b
#define VC5_PASTE_(a, b)  VC5_PASTE2_(a, b)
#define static_assert(cond, msg) typedef char VC5_PASTE_(vc5_static_assert_, __LINE__)[(cond) ? 1 : -1]

// __thiscall -> nothing: VC5 has no such keyword; member functions use that convention by
// default, so the keyword on a member declaration or definition is right as nothing.
// Guaranteed: no function-pointer type in src/ spells the keyword (`R (__thiscall *)(...)` would be
// a __cdecl pointer here: no ECX, caller-cleaned stack). scripts/build_vc5.py refuses to compile when
// any source outside src/compat contains `__thiscall` followed by `*`, with file:line. A thiscall
// through a code address is made with PORT_THISCALLn / PORT_VSLOTn / PORT_METHODn (runtime/ported.h),
// i.e. a pointer to a member of the empty class decomp::Any, which Visual C++ 5.0 compiles to
// `mov ecx, obj` + `call` with the callee cleaning the stack. A non-member function declared
// __thiscall still loses the convention under VC5; none exists in src/.
#define __thiscall

// The Win32 SDK that ships with VC5 has no pointer-sized integer types (they came with the Win64 SDK).
// On 32-bit x86 they are plain unsigned int / unsigned long, which is what the newer SDK typedefs them to.
typedef unsigned int  UINT_PTR;
typedef unsigned long DWORD_PTR;

// WINVER: the 1997 SDK declares the Windows 95 forms of the multimedia calls (midiOutGetVolume and
// midiOutSetVolume take an HMIDIOUT, as in the newer SDK) only for WINVER >= 0x0400, and defaults to the
// older UINT device-id form. The client targets Windows 95/98 and the original binary was built with
// 0x0400, so ask for it. (The value pushed for the handle is the same 32-bit -1 either way.)
#ifndef WINVER
#define WINVER 0x0400
#endif

// _time32(): the newer CRT spells the 32-bit time() that way; in VC5 time_t is already 32 bits.
#include <time.h>
#define _time32 time

// for-scope: VC5 leaks the variable declared in a for-init into the enclosing scope, so two
// "for (int i..." in one block conflict (C2374). This is the standard repair: the else branch
// owns a fresh scope. Hazard: it changes the structure of "if (x) for (...) ...; else ...".
#define for if (0) ; else for

#endif
