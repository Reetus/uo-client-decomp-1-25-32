// A position by value: three 16-bit coordinates (6 bytes, passed in 8 bytes of stack). The original's copy has an
// empty user-declared destructor (the lone `ret` at 0x0040caf0), which gives every function that holds one -- as a
// local or a by-value parameter -- its exception frame.
#pragma once

#include <cstdint>

struct Pos3 {
    int16_t x;
    int16_t y;
    int16_t z;
    ~Pos3() {}
};
static_assert(sizeof(Pos3) == 6, "Pos3 size");
