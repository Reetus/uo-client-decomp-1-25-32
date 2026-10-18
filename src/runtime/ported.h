// The record of which original procedure each function replaces, and calls between ported procedures
// where the caller's idea of the callee's type is not the type the port was written with.
//
// HOOK_FUNC / HOOK_METHOD (HOOK_CTOR / HOOK_DTOR for a real constructor / destructor) stand next to every port and give the address of the procedure of the original
// client (orig/CLIENT.EXE, 1.25.32) it was ported from. They compile to nothing. The scripts read them as
// the list of what is ported (port_status.py, check_port.py, check_refs.py, gen_vtables.py). The name
// comes from the time the ports ran inside a host that carried the original image and each port was
// hooked into its original entry.
//
// PORT_*: while procedures were ported one batch at a time, a caller reached every other procedure
// through its original address with a function type written at the call site. Now the call names the
// port. Where the site's types agree with the port it is an ordinary call. Where they do not (a `void *`
// for a class pointer, an `int` for a `uint16_t`, a free function with the object in ECX standing in for
// a method) the PORT_* forms keep the exact call the site made before: the port's code address converted
// to the function type the site declares. Each use is a type disagreement to settle in the headers.
#pragma once

#include <cstddef>
#include <cstdint>

// The measurement build (OUO_MATCH_TABLE, set by `build_vc5.py --match`; scripts/match.py) makes each
// annotation also define `decomp_pairF_<addr>` (a function: constant-initialised data) or
// `decomp_pairM_<addr>` (a member: set by a dynamic initialiser, the one conversion a pointer to member
// allows) holding the code address of the port. The tool finds the symbols in the link map. Without the
// define these expand to nothing and the annotations are exactly what they were.
#ifdef OUO_MATCH_TABLE
#define HOOK_PAIR_F_(addr, fn) ; extern const void *const decomp_pairF_##addr = (const void *)&fn
#define HOOK_PAIR_M_(addr, cls, method) ; extern void *const decomp_pairM_##addr = ::decomp::code_ptr(&cls::method)
// A constructor or destructor has no address a program can take, so the pair is made by name: the variable's
// type spells the class (and, for a constructor, the parameter types), the compiler gives the variable a
// decorated name that holds them, and scripts/match.py reads the signature back from it and looks for the
// constructor / destructor symbol of the link map with that class and those parameters. The variable itself is null.
#define HOOK_PAIR_C_(addr, cls, args) ; extern void (*const decomp_pairC_##addr)(cls *, void (*) args) = 0
#define HOOK_PAIR_D_(addr, cls) ; extern cls *const decomp_pairD_##addr = 0
// The same by-name pair for the table and the deleting destructors the compiler writes for a class with a virtual function:
// V names the class whose vftable `??_7cls@@6B@` stands for the original vtable at addr, S / E the class whose `??_G` / `??_E` is the
// original procedure at addr. Null variables; match.py reads the class from the decorated name.
#define HOOK_PAIR_V_(addr, cls) ; extern cls *const decomp_pairV_##addr = 0
#define HOOK_PAIR_S_(addr, cls) ; extern cls *const decomp_pairS_##addr = 0
// A virtual member: &cls::method is the compiler's anonymous call thunk, which only names a slot. T keeps that pointer (set by a
// dynamic initialiser, like M), W names the class (null, like V); match.py reads the slot from the thunk's body and takes the
// built function from that slot of the class's vftable.
#define HOOK_PAIR_T_(addr, cls, method) ; extern void *const decomp_pairT_##addr = ::decomp::code_ptr(&cls::method); extern cls *const decomp_pairW_##addr = 0
#define HOOK_PAIR_E_(addr, cls) ; extern cls *const decomp_pairE_##addr = 0
#else
#define HOOK_PAIR_F_(addr, fn)
#define HOOK_PAIR_M_(addr, cls, method)
#define HOOK_PAIR_C_(addr, cls, args)
#define HOOK_PAIR_D_(addr, cls)
#define HOOK_PAIR_V_(addr, cls)
#define HOOK_PAIR_S_(addr, cls)
#define HOOK_PAIR_T_(addr, cls, method)
#define HOOK_PAIR_E_(addr, cls)
#endif

#define HOOK_FUNC(addr, fn) static_assert((addr) != 0, #fn) HOOK_PAIR_F_(addr, fn)
#define HOOK_METHOD(addr, cls, method) static_assert((addr) != 0, #cls "::" #method) HOOK_PAIR_M_(addr, cls, method)
// A real out-of-line constructor / destructor of cls (not a method that has `Ctor` in its name). `args` is the
// constructor's parameter type list in parentheses, `()` for none: HOOK_CTOR(0x00454c30, CPlayerNew, (int, int, uint8_t));
// The signature is part of the annotation so that overloads are told apart (docs/matching.md, "Pairing").
// A virtual member function of cls: the original procedure at addr is what the compiler's vftable of cls holds in method's slot
// (cls needs HOOK_VTABLE). The thunk behind &cls::method names the slot; match.py checks it against the original's table.
#define HOOK_VMETHOD(addr, cls, method) static_assert((addr) != 0, #cls "::" #method) HOOK_PAIR_T_(addr, cls, method)
#define HOOK_CTOR(addr, cls, args) static_assert((addr) != 0, #cls) HOOK_PAIR_C_(addr, cls, args)
#define HOOK_DTOR(addr, cls) static_assert((addr) != 0, #cls) HOOK_PAIR_D_(addr, cls)
// A class with a real virtual function (the compiler writes the vptr and the vftable): HOOK_VTABLE(addr, cls) says which vtable of
// the original (its address in .rdata) the compiler's `??_7cls@@6B@` is; HOOK_SDD(addr, cls) / HOOK_VDD(addr, cls) the original
// procedure that is the compiler's scalar / vector deleting destructor (`??_G` / `??_E`). Each once per class, in a .cpp, after
// the class. They generate no code (docs/matching.md, "Virtual tables of the compiler").
#define HOOK_VTABLE(addr, cls) static_assert((addr) != 0, #cls) HOOK_PAIR_V_(addr, cls)
#define HOOK_SDD(addr, cls) static_assert((addr) != 0, #cls) HOOK_PAIR_S_(addr, cls)
#define HOOK_VDD(addr, cls) static_assert((addr) != 0, #cls) HOOK_PAIR_E_(addr, cls)

// A ported procedure that never had an entry of its own to replace: hand-written assembly with a
// register convention no C++ function can take, whose callers call the C++ directly. The scripts read
// this line as a port of `addr` (they match the HOOK_FUNC( inside the name).
#define PORT_NOHOOK_FUNC(addr, fn) static_assert((addr) != 0, #fn) HOOK_PAIR_F_(addr, fn)

namespace decomp {

// The code address of a function or of a non-virtual member function.
template <class T>
inline void *code_ptr(T target)
{
    static_assert(sizeof(T) == sizeof(void *),
                  "target must be a plain code pointer: a function, or a member of a fully defined single-inheritance class");
    union {
        T t;
        void *v;
    } u;
    u.t = target;
    return u.v;
}

} // namespace decomp

// The call forms. The macros are fixed-arity families, one macro per number of parameter types, because Visual C++
// 5.0 has no variadic macros; the same definitions serve every compiler. `n` is the number of parameter types
// after the object, so the argument count of the call is n (n + 1 before the object moved into the macro).
//
// PORT_FUNCn(target, ret, conv, T1..Tn): the port's code address as a pointer to a free function of that type:
//   PORT_FUNC2(&DrawGumpArt, void, __fastcall, void *, int)(art, id)
//
// PORT_THISCALLn(target, ret, obj, T1..Tn): a __thiscall call of the port on `obj`, ECX = obj, the arguments
// on the stack. The call goes through a pointer to a member of an empty class (decomp::Any), which is the one
// way a compiler without the `__thiscall` keyword (VC5) is made to load ECX and call a code pointer; it
// compiles to the same instructions as a virtual call. The object is evaluated once:
//   PORT_THISCALL2(&CGump::LinkInto, void, control, void *, int)(this, 1)
//
// PORT_METHODn(cls, ret, target, T1..Tn): the pointer to member of `cls` whose code is the port, for a member
// of another class called on a `cls` object. Its call convention is the compiler's default for members:
//   (obj->*PORT_METHOD1(CApp, char *, &CFileManager::Resolve, char *))(name)
//
// PORT_VSLOTn(obj, off, ret, T1..Tn): a virtual call. The slot at byte offset `off` of the table the object
// points to at +0 (game/vtables.h), called with `this` = obj. The object is named once by the caller and
// appears twice in the expansion (once to find the table, once as `this`), so it must be free of side effects:
//   PORT_VSLOT2(gump, 0x18, void, int, int)(mx, my)
//
// PORT_FUNC and the others without the number do not exist any more. A site that passes a type with a comma
// outside parentheses (a template with two arguments) needs a typedef.

namespace decomp {

// A complete class with no bases and no virtual functions: a pointer to one of its members is a plain code
// pointer (4 bytes) in Visual C++ 5.0.
class Any {};

// A code address, read as a pointer to member. The union does the conversion that no cast can, and the
// class template carries the type, so that no explicit template argument is needed.
template <class PMF>
struct pmf_cast {
    PMF p;
    explicit pmf_cast(void *code)
    {
        static_assert(sizeof(PMF) == sizeof(void *),
                      "pointer-to-member must be a plain code pointer: define the class with "
                      "single inheritance before taking members through PORT_METHOD");
        union {
            void *v;
            PMF m;
        } u;
        u.v = code;
        p = u.m;
    }
};

// The same for the slot at `byte_offset` of the table `object` points to.
template <class PMF>
struct slot_pmf_cast {
    PMF p;
    slot_pmf_cast(const void *object, unsigned byte_offset)
    {
        static_assert(sizeof(PMF) == sizeof(void *), "pointer-to-member must be a plain code pointer");
        union {
            void *v;
            PMF m;
        } u;
        u.v = (*reinterpret_cast<void *const *const *>(object))[byte_offset / 4];
        p = u.m;
    }
};

} // namespace decomp

// PORT_FUNCn(target, ret, conv, T1..Tn)
#define PORT_FUNC0(target, ret, conv) (reinterpret_cast<ret(conv *)()>(::decomp::code_ptr(target)))
#define PORT_FUNC1(target, ret, conv, T1) (reinterpret_cast<ret(conv *)(T1)>(::decomp::code_ptr(target)))
#define PORT_FUNC2(target, ret, conv, T1, T2) (reinterpret_cast<ret(conv *)(T1, T2)>(::decomp::code_ptr(target)))
#define PORT_FUNC3(target, ret, conv, T1, T2, T3) (reinterpret_cast<ret(conv *)(T1, T2, T3)>(::decomp::code_ptr(target)))
#define PORT_FUNC4(target, ret, conv, T1, T2, T3, T4) (reinterpret_cast<ret(conv *)(T1, T2, T3, T4)>(::decomp::code_ptr(target)))
#define PORT_FUNC5(target, ret, conv, T1, T2, T3, T4, T5) (reinterpret_cast<ret(conv *)(T1, T2, T3, T4, T5)>(::decomp::code_ptr(target)))
#define PORT_FUNC6(target, ret, conv, T1, T2, T3, T4, T5, T6) (reinterpret_cast<ret(conv *)(T1, T2, T3, T4, T5, T6)>(::decomp::code_ptr(target)))
#define PORT_FUNC7(target, ret, conv, T1, T2, T3, T4, T5, T6, T7) (reinterpret_cast<ret(conv *)(T1, T2, T3, T4, T5, T6, T7)>(::decomp::code_ptr(target)))
#define PORT_FUNC8(target, ret, conv, T1, T2, T3, T4, T5, T6, T7, T8) (reinterpret_cast<ret(conv *)(T1, T2, T3, T4, T5, T6, T7, T8)>(::decomp::code_ptr(target)))
#define PORT_FUNC9(target, ret, conv, T1, T2, T3, T4, T5, T6, T7, T8, T9) (reinterpret_cast<ret(conv *)(T1, T2, T3, T4, T5, T6, T7, T8, T9)>(::decomp::code_ptr(target)))
#define PORT_FUNC10(target, ret, conv, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10) (reinterpret_cast<ret(conv *)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10)>(::decomp::code_ptr(target)))

// PORT_THISCALLn(target, ret, obj, T1..Tn)
#define PORT_THISCALL0(target, ret, obj) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)()>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL1(target, ret, obj, T1) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL2(target, ret, obj, T1, T2) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL3(target, ret, obj, T1, T2, T3) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2, T3)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL4(target, ret, obj, T1, T2, T3, T4) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2, T3, T4)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL5(target, ret, obj, T1, T2, T3, T4, T5) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2, T3, T4, T5)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL6(target, ret, obj, T1, T2, T3, T4, T5, T6) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2, T3, T4, T5, T6)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL7(target, ret, obj, T1, T2, T3, T4, T5, T6, T7) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2, T3, T4, T5, T6, T7)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL8(target, ret, obj, T1, T2, T3, T4, T5, T6, T7, T8) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2, T3, T4, T5, T6, T7, T8)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL9(target, ret, obj, T1, T2, T3, T4, T5, T6, T7, T8, T9) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2, T3, T4, T5, T6, T7, T8, T9)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL10(target, ret, obj, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL11(target, ret, obj, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL12(target, ret, obj, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL13(target, ret, obj, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL14(target, ret, obj, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL15(target, ret, obj, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL16(target, ret, obj, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL17(target, ret, obj, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL18(target, ret, obj, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL19(target, ret, obj, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL20(target, ret, obj, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20)>(::decomp::code_ptr(target)).p))
#define PORT_THISCALL21(target, ret, obj, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21) \
    (((::decomp::Any *)(obj))->*(::decomp::pmf_cast<ret(::decomp::Any::*)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21)>(::decomp::code_ptr(target)).p))

// PORT_METHODn(cls, ret, target, T1..Tn)
#define PORT_METHOD0(cls, ret, target) \
    (::decomp::pmf_cast<ret(cls::*)()>(::decomp::code_ptr(target)).p)
#define PORT_METHOD1(cls, ret, target, T1) \
    (::decomp::pmf_cast<ret(cls::*)(T1)>(::decomp::code_ptr(target)).p)
#define PORT_METHOD2(cls, ret, target, T1, T2) \
    (::decomp::pmf_cast<ret(cls::*)(T1, T2)>(::decomp::code_ptr(target)).p)
#define PORT_METHOD3(cls, ret, target, T1, T2, T3) \
    (::decomp::pmf_cast<ret(cls::*)(T1, T2, T3)>(::decomp::code_ptr(target)).p)

// PORT_VSLOTn(obj, off, ret, T1..Tn)
#define PORT_VSLOT0(obj, off, ret) \
    (((::decomp::Any *)(obj))->*(::decomp::slot_pmf_cast<ret(::decomp::Any::*)()>((obj), off).p))
#define PORT_VSLOT1(obj, off, ret, T1) \
    (((::decomp::Any *)(obj))->*(::decomp::slot_pmf_cast<ret(::decomp::Any::*)(T1)>((obj), off).p))
#define PORT_VSLOT2(obj, off, ret, T1, T2) \
    (((::decomp::Any *)(obj))->*(::decomp::slot_pmf_cast<ret(::decomp::Any::*)(T1, T2)>((obj), off).p))
#define PORT_VSLOT3(obj, off, ret, T1, T2, T3) \
    (((::decomp::Any *)(obj))->*(::decomp::slot_pmf_cast<ret(::decomp::Any::*)(T1, T2, T3)>((obj), off).p))

// The code address of a port where the original has the procedure's address as a number: a callback stored in a
// button, a handler in a dispatch table, an atexit registration. An `int`, as the literal was.
#define PORT_ADDR(target) (static_cast<int>(reinterpret_cast<intptr_t>(::decomp::code_ptr(target))))

// The return type of a call whose argument types are deduced (game/vcall.h, game/com_call.h): a null `T *`. The
// function takes it as its first parameter and deduces the return type from it; Visual C++ 5.0 accepts no explicit
// function-template argument. A type with a comma outside parentheses needs a typedef.
#define PORT_RET(T) (static_cast<T *>(0))
