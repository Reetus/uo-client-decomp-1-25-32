// Other views of the client's global data (game/globals.h).
//
// globals.h declares each global once, with the type most of the ported code
// uses for it. Where a file reads the same bytes as something else (a
// `void *` global as the class it points to, an object's storage as the
// class one file knows, a table through a pointer of another element type)
// it says so with one of these. Each use is a type question to settle in the
// headers, like the PORT_* forms in ported.h; none depends on where the
// global lives.
#pragma once

#include <cstddef>
#include <cstdint>

// (C casts, because a view may add `const` below the top level, which reinterpret_cast refuses.)

// The global `obj` read as a `type`:  GLOBAL_AS(CApp, g_app_storage).Run()
#define GLOBAL_AS(type, obj) (*(type *)(&(obj)))

// A `type *` to the global `obj`:  GLOBAL_PTR(void, g_mouse_storage)
#define GLOBAL_PTR(type, obj) ((type *)(&(obj)))

// The address of a global as a number, where the original has it as an
// immediate: an argument that is an `int`, the base of a computed address.
#define GADDR(obj) (reinterpret_cast<uintptr_t>(&(obj)))

// The address just past a global: the bound of a loop that walks it.
#define GEND(obj) (reinterpret_cast<uintptr_t>(&(obj)) + sizeof(obj))

// A `type` at a computed address:  AT_ADDR(int32_t, GADDR(g_pingTable) + i * 4)
#define AT_ADDR(type, addr) (*reinterpret_cast<type *>(static_cast<uintptr_t>(addr)))

// A `type *` from a computed address, or from a code address a port passes as an object.
#define PTR_AT(type, addr) (reinterpret_cast<type *>(static_cast<uintptr_t>(addr)))
