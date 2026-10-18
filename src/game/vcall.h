// Virtual calls by slot offset, and a __thiscall at an address. The argument types are DEDUCED from the call's
// arguments (pass exactly the width the original pushes: int, uint32_t, pointers; a literal is an int); only the
// return type is named, because Visual C++ 5.0 accepts no explicit function-template argument. It is named through
// PORT_RET(R) (runtime/ported.h), a null `R *` that the first parameter deduces R from. A function returning void
// has its own name (..._void): VC5 cannot return a void expression from a template. The call is made through a
// pointer to a member of the empty class decomp::Any, the way PORT_THISCALLn and PORT_VSLOTn are made (ECX = obj,
// arguments on the stack).
//
//   decomp::vcall_void(obj, 0x14);                       // obj->vtable[0x14/4](obj), a table of vtables.h
//   decomp::vcall(PORT_RET(int), obj, 0x24);              // returns int
//   decomp::vcall_void(obj, 0x18, mx, my);                // two arguments, their types those of mx and my
//   decomp::thiscall_at_void(0x0043c3d0, gump);           // original __thiscall at an address
//   decomp::thiscall_at(PORT_RET(void *), 0x00401000, obj, id);
//
// Overloads exist for 0 to 3 arguments after the object (vcall) and 0 to 6 (thiscall_at), the most the sources use.
#pragma once

#include <cstdint>

#include "../runtime/ported.h"

namespace decomp {

template <class R>
inline R vcall(R *, void *obj, unsigned byte_offset)
{
    return (((Any *)obj)->*slot_pmf_cast<R (Any::*)()>(obj, byte_offset).p)();
}

inline void vcall_void(void *obj, unsigned byte_offset)
{
    (((Any *)obj)->*slot_pmf_cast<void (Any::*)()>(obj, byte_offset).p)();
}

template <class R, class A1>
inline R vcall(R *, void *obj, unsigned byte_offset, A1 a1)
{
    return (((Any *)obj)->*slot_pmf_cast<R (Any::*)(A1)>(obj, byte_offset).p)(a1);
}
template <class A1>
inline void vcall_void(void *obj, unsigned byte_offset, A1 a1)
{
    (((Any *)obj)->*slot_pmf_cast<void (Any::*)(A1)>(obj, byte_offset).p)(a1);
}

template <class R, class A1, class A2>
inline R vcall(R *, void *obj, unsigned byte_offset, A1 a1, A2 a2)
{
    return (((Any *)obj)->*slot_pmf_cast<R (Any::*)(A1, A2)>(obj, byte_offset).p)(a1, a2);
}
template <class A1, class A2>
inline void vcall_void(void *obj, unsigned byte_offset, A1 a1, A2 a2)
{
    (((Any *)obj)->*slot_pmf_cast<void (Any::*)(A1, A2)>(obj, byte_offset).p)(a1, a2);
}

template <class R, class A1, class A2, class A3>
inline R vcall(R *, void *obj, unsigned byte_offset, A1 a1, A2 a2, A3 a3)
{
    return (((Any *)obj)->*slot_pmf_cast<R (Any::*)(A1, A2, A3)>(obj, byte_offset).p)(a1, a2, a3);
}
template <class A1, class A2, class A3>
inline void vcall_void(void *obj, unsigned byte_offset, A1 a1, A2 a2, A3 a3)
{
    (((Any *)obj)->*slot_pmf_cast<void (Any::*)(A1, A2, A3)>(obj, byte_offset).p)(a1, a2, a3);
}

template <class R>
inline R thiscall_at(R *, uintptr_t address, void *obj)
{
    return (((Any *)obj)->*pmf_cast<R (Any::*)()>((void *)(address)).p)();
}

inline void thiscall_at_void(uintptr_t address, void *obj)
{
    (((Any *)obj)->*pmf_cast<void (Any::*)()>((void *)(address)).p)();
}

template <class R, class A1>
inline R thiscall_at(R *, uintptr_t address, void *obj, A1 a1)
{
    return (((Any *)obj)->*pmf_cast<R (Any::*)(A1)>((void *)(address)).p)(a1);
}
template <class A1>
inline void thiscall_at_void(uintptr_t address, void *obj, A1 a1)
{
    (((Any *)obj)->*pmf_cast<void (Any::*)(A1)>((void *)(address)).p)(a1);
}

template <class R, class A1, class A2>
inline R thiscall_at(R *, uintptr_t address, void *obj, A1 a1, A2 a2)
{
    return (((Any *)obj)->*pmf_cast<R (Any::*)(A1, A2)>((void *)(address)).p)(a1, a2);
}
template <class A1, class A2>
inline void thiscall_at_void(uintptr_t address, void *obj, A1 a1, A2 a2)
{
    (((Any *)obj)->*pmf_cast<void (Any::*)(A1, A2)>((void *)(address)).p)(a1, a2);
}

template <class R, class A1, class A2, class A3>
inline R thiscall_at(R *, uintptr_t address, void *obj, A1 a1, A2 a2, A3 a3)
{
    return (((Any *)obj)->*pmf_cast<R (Any::*)(A1, A2, A3)>((void *)(address)).p)(a1, a2, a3);
}
template <class A1, class A2, class A3>
inline void thiscall_at_void(uintptr_t address, void *obj, A1 a1, A2 a2, A3 a3)
{
    (((Any *)obj)->*pmf_cast<void (Any::*)(A1, A2, A3)>((void *)(address)).p)(a1, a2, a3);
}

template <class R, class A1, class A2, class A3, class A4>
inline R thiscall_at(R *, uintptr_t address, void *obj, A1 a1, A2 a2, A3 a3, A4 a4)
{
    return (((Any *)obj)->*pmf_cast<R (Any::*)(A1, A2, A3, A4)>((void *)(address)).p)(a1, a2, a3, a4);
}
template <class A1, class A2, class A3, class A4>
inline void thiscall_at_void(uintptr_t address, void *obj, A1 a1, A2 a2, A3 a3, A4 a4)
{
    (((Any *)obj)->*pmf_cast<void (Any::*)(A1, A2, A3, A4)>((void *)(address)).p)(a1, a2, a3, a4);
}

template <class R, class A1, class A2, class A3, class A4, class A5>
inline R thiscall_at(R *, uintptr_t address, void *obj, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    return (((Any *)obj)->*pmf_cast<R (Any::*)(A1, A2, A3, A4, A5)>((void *)(address)).p)(a1, a2, a3, a4, a5);
}
template <class A1, class A2, class A3, class A4, class A5>
inline void thiscall_at_void(uintptr_t address, void *obj, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    (((Any *)obj)->*pmf_cast<void (Any::*)(A1, A2, A3, A4, A5)>((void *)(address)).p)(a1, a2, a3, a4, a5);
}

template <class R, class A1, class A2, class A3, class A4, class A5, class A6>
inline R thiscall_at(R *, uintptr_t address, void *obj, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    return (((Any *)obj)->*pmf_cast<R (Any::*)(A1, A2, A3, A4, A5, A6)>((void *)(address)).p)(a1, a2, a3, a4, a5, a6);
}
template <class A1, class A2, class A3, class A4, class A5, class A6>
inline void thiscall_at_void(uintptr_t address, void *obj, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    (((Any *)obj)->*pmf_cast<void (Any::*)(A1, A2, A3, A4, A5, A6)>((void *)(address)).p)(a1, a2, a3, a4, a5, a6);
}

// Scalar deleting destructor, vtable slot 0, flags = 1 (delete the memory).
inline void vdelete(void *obj)
{
    vcall_void(obj, 0x00, 1);
}

} // namespace decomp
