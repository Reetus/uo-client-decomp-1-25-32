// client.exe: the seams of src/runtime/test_seams.h always say no.
#include "../runtime/test_seams.h"

namespace decomp {

bool test_switch(const char *)
{
    return false;
}

bool test_value(const char *, char *, size_t)
{
    return false;
}

} // namespace decomp
