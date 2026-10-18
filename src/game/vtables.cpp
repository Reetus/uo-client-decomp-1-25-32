// The client's vtables as tables of ports. Written by scripts/gen_vtables.py; do not edit.
//
// One struct per table with a field per slot, each field of its own type: a pointer to member function for a
// method (the type is in symbols/CLIENT.EXE.vtable-slot-types.tsv), `void *` for a free function. Every
// initialiser is an address, so the tables are constant-initialised read-only data, complete before any
// constructor runs; each has external linkage, declared in vtables.h. A pointer to a member of a
// single-inheritance class is one plain code pointer, which the static_asserts check.
#include "vtables.h"

#include <cstdlib>

#include "app.h"
#include "client_socket.h"
#include "gump_manager.h"
#include "mouse_manager.h"
#include "palette_manager.h"
#include "text_list.h"

// 0x0049f72c
struct T_0049f72c {
    void *(CApp::*s0)(int);
};
extern const T_0049f72c t_0049f72c = {
    &CApp::ScalarDeletingDtor,   // +0x00  0x0044b620
};
static_assert(sizeof(T_0049f72c) == 0x4, "every slot must be a plain code pointer");
