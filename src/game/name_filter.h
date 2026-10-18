// Character name validity / reserved-word filter. decomp/pass2/0x0044adf0.cpp.
#pragma once

// 0x0044adf0  size 0x334
// Returns 1 = rejected, 0 = accepted; the name is in ECX.
int __fastcall NameIsRejected(char *name);
