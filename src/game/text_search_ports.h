// Prototypes of the ported procedures in text_search.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

int __fastcall StrFindNoCase(const char *needle, const char *haystack);   // 0x004067b0
