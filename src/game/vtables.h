// The client's vtables. Written by scripts/gen_vtables.py from the original's tables; do not edit.
//
// Each `vtbl_<Class>` is the address of a table of ports in the original's slot order. A constructor stores
// it at +0 of the object (`vtable = vtbl_CBarkGump;`) and a virtual call reads the slot by its byte offset
// (PORT_VSLOT in runtime/ported.h). The comment gives the original table's address and slot count.
//
// The table is declared here as an object of an incomplete type (defined in vtables.cpp) and `vtbl_<Class>`
// is a macro for its address, so the compiler stores an immediate (`mov [obj], offset table`) as the
// original does, not the value of a pointer variable. Take no `vtbl_<Class>` as an lvalue.
#pragma once

#include <cstdint>

struct T_0049f72c; extern const T_0049f72c t_0049f72c;
#define vtbl_CApp ((void *)&t_0049f72c)   // 0x0049f72c, 1 slots
