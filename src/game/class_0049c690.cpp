#include "class_0049c690.h"

#include "../runtime/ported.h"
#include "raw_field.h"
#include "world_obj_ops.h"
#include "world_obj.h"
#include "vtables.h"

// 0x00406690  size 0x85
// Destructor of the class with vtable 0x0049c690: release the resource at +0x1c, unlink from the
// sibling list (moving the owner's head back to `prev` when this was the head), run the base dtor.
CObj0049c690::~CObj0049c690()
{
    if (inWorld != 0)
        this->CWorldObj::RemoveFromWorld();   // release resource held at +0x1c
    if (prev != nullptr)
        prev->next = next;
    if (next != nullptr) {
        next->prev = prev;
    } else {
        if (RAWF(CObj0049c690 *, owner, 0xb0) == this)
            RAWF(CObj0049c690 *, owner, 0xb0) = prev;   // list head moves back to prev
    }
}
HOOK_DTOR(0x00406690, CObj0049c690);
HOOK_VTABLE(0x0049c690, CObj0049c690);
HOOK_SDD(0x00406660, CObj0049c690);
