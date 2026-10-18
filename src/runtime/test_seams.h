// Seams, not part of the client's behavior.
//
// test_switch: is the switch `name` set? test_value: its text, when it is set. Three places in the
// ported code ask (winmain.cpp, app_startup.cpp, config_file.cpp). client.exe links versions that always
// say no (standalone/seams_off.cpp), so the client behaves as the original.
#pragma once

#include <cstddef>

namespace decomp {

bool test_switch(const char *name);
bool test_value(const char *name, char *out, size_t size);

} // namespace decomp
