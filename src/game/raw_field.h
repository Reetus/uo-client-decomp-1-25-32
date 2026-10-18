// Typed access to a field of an object whose class is not defined in this
// tree yet (CObj, a gump class not ported, ...). Byte offsets are the
// original ones. Replace uses with real members once the class exists.
//
//     RAWF(int32_t, obj, 0x1c) = 5;           // an lvalue of type T at obj + 0x1c
//     CObj *owner = RAWF(CObj *, gump, 0x30);
//
// A macro because Visual C++ 5.0 accepts no explicit function-template argument. The object is
// evaluated once; the result is an lvalue of type T, writable whether or not the pointer is const.
// A type with a comma outside parentheses needs a typedef.
#pragma once

#include <cstdint>

#define RAWF(T, object, byte_offset) (*(T *)((char *)(object) + (byte_offset)))
