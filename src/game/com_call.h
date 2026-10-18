// Calls through a COM-style vtable of an object that only exists in the original image (IDirectSound,
// IDirectSoundBuffer, IDirectDraw, ...). COM methods are __stdcall with the interface pointer pushed first (as the
// last-pushed argument, no ECX), unlike vcall.h which models __thiscall. `offset` is the byte offset of the slot.
//
// The argument types are deduced from the call's arguments; only the return type is named, through PORT_RET(R)
// (runtime/ported.h), because Visual C++ 5.0 accepts no explicit function-template argument. A function returning
// void is com_call_void (VC5 cannot return a void expression from a template).
//
//     com_call(PORT_RET(long), buffer, 0x3c, volume);   // IDirectSoundBuffer::SetVolume
//     com_call_void(buffer, 0x08);
//
// Overloads exist for 0 to 7 arguments after the object, the most the sources use.
#pragma once

#include <cstdint>
#include "../runtime/ported.h"

template <class R>
inline R com_call(R *, void *object, unsigned offset)
{
    typedef R(__stdcall * Fn)(void *);
    Fn fn = reinterpret_cast<Fn>((*reinterpret_cast<void *const *const *>(object))[offset / 4]);
    return fn(object);
}
inline void com_call_void(void *object, unsigned offset)
{
    typedef void(__stdcall * Fn)(void *);
    Fn fn = reinterpret_cast<Fn>((*reinterpret_cast<void *const *const *>(object))[offset / 4]);
    fn(object);
}

template <class R, class A1>
inline R com_call(R *, void *object, unsigned offset, A1 a1)
{
    typedef R(__stdcall * Fn)(void *, A1);
    Fn fn = reinterpret_cast<Fn>((*reinterpret_cast<void *const *const *>(object))[offset / 4]);
    return fn(object, a1);
}
template <class A1>
inline void com_call_void(void *object, unsigned offset, A1 a1)
{
    typedef void(__stdcall * Fn)(void *, A1);
    Fn fn = reinterpret_cast<Fn>((*reinterpret_cast<void *const *const *>(object))[offset / 4]);
    fn(object, a1);
}

template <class R, class A1, class A2>
inline R com_call(R *, void *object, unsigned offset, A1 a1, A2 a2)
{
    typedef R(__stdcall * Fn)(void *, A1, A2);
    Fn fn = reinterpret_cast<Fn>((*reinterpret_cast<void *const *const *>(object))[offset / 4]);
    return fn(object, a1, a2);
}
template <class A1, class A2>
inline void com_call_void(void *object, unsigned offset, A1 a1, A2 a2)
{
    typedef void(__stdcall * Fn)(void *, A1, A2);
    Fn fn = reinterpret_cast<Fn>((*reinterpret_cast<void *const *const *>(object))[offset / 4]);
    fn(object, a1, a2);
}

template <class R, class A1, class A2, class A3>
inline R com_call(R *, void *object, unsigned offset, A1 a1, A2 a2, A3 a3)
{
    typedef R(__stdcall * Fn)(void *, A1, A2, A3);
    Fn fn = reinterpret_cast<Fn>((*reinterpret_cast<void *const *const *>(object))[offset / 4]);
    return fn(object, a1, a2, a3);
}
template <class A1, class A2, class A3>
inline void com_call_void(void *object, unsigned offset, A1 a1, A2 a2, A3 a3)
{
    typedef void(__stdcall * Fn)(void *, A1, A2, A3);
    Fn fn = reinterpret_cast<Fn>((*reinterpret_cast<void *const *const *>(object))[offset / 4]);
    fn(object, a1, a2, a3);
}

template <class R, class A1, class A2, class A3, class A4>
inline R com_call(R *, void *object, unsigned offset, A1 a1, A2 a2, A3 a3, A4 a4)
{
    typedef R(__stdcall * Fn)(void *, A1, A2, A3, A4);
    Fn fn = reinterpret_cast<Fn>((*reinterpret_cast<void *const *const *>(object))[offset / 4]);
    return fn(object, a1, a2, a3, a4);
}
template <class A1, class A2, class A3, class A4>
inline void com_call_void(void *object, unsigned offset, A1 a1, A2 a2, A3 a3, A4 a4)
{
    typedef void(__stdcall * Fn)(void *, A1, A2, A3, A4);
    Fn fn = reinterpret_cast<Fn>((*reinterpret_cast<void *const *const *>(object))[offset / 4]);
    fn(object, a1, a2, a3, a4);
}

template <class R, class A1, class A2, class A3, class A4, class A5>
inline R com_call(R *, void *object, unsigned offset, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    typedef R(__stdcall * Fn)(void *, A1, A2, A3, A4, A5);
    Fn fn = reinterpret_cast<Fn>((*reinterpret_cast<void *const *const *>(object))[offset / 4]);
    return fn(object, a1, a2, a3, a4, a5);
}
template <class A1, class A2, class A3, class A4, class A5>
inline void com_call_void(void *object, unsigned offset, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    typedef void(__stdcall * Fn)(void *, A1, A2, A3, A4, A5);
    Fn fn = reinterpret_cast<Fn>((*reinterpret_cast<void *const *const *>(object))[offset / 4]);
    fn(object, a1, a2, a3, a4, a5);
}

template <class R, class A1, class A2, class A3, class A4, class A5, class A6>
inline R com_call(R *, void *object, unsigned offset, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    typedef R(__stdcall * Fn)(void *, A1, A2, A3, A4, A5, A6);
    Fn fn = reinterpret_cast<Fn>((*reinterpret_cast<void *const *const *>(object))[offset / 4]);
    return fn(object, a1, a2, a3, a4, a5, a6);
}
template <class A1, class A2, class A3, class A4, class A5, class A6>
inline void com_call_void(void *object, unsigned offset, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    typedef void(__stdcall * Fn)(void *, A1, A2, A3, A4, A5, A6);
    Fn fn = reinterpret_cast<Fn>((*reinterpret_cast<void *const *const *>(object))[offset / 4]);
    fn(object, a1, a2, a3, a4, a5, a6);
}

template <class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
inline R com_call(R *, void *object, unsigned offset, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
{
    typedef R(__stdcall * Fn)(void *, A1, A2, A3, A4, A5, A6, A7);
    Fn fn = reinterpret_cast<Fn>((*reinterpret_cast<void *const *const *>(object))[offset / 4]);
    return fn(object, a1, a2, a3, a4, a5, a6, a7);
}
template <class A1, class A2, class A3, class A4, class A5, class A6, class A7>
inline void com_call_void(void *object, unsigned offset, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
{
    typedef void(__stdcall * Fn)(void *, A1, A2, A3, A4, A5, A6, A7);
    Fn fn = reinterpret_cast<Fn>((*reinterpret_cast<void *const *const *>(object))[offset / 4]);
    fn(object, a1, a2, a3, a4, a5, a6, a7);
}
