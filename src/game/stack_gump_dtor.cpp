// CStackGump scalar deleting destructor. (Porting pass 2, batch 25.)
#include "stack_gump.h"

#include "../runtime/ported.h"
#include "crt.h"

HOOK_VTABLE(0x0049fe58, CStackGump);
HOOK_SDD(0x00469ae0, CStackGump);
