// Prototypes of the ported procedures in path_search_b25.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

class CPathSearch;
struct PathNode;

CPathSearch * __fastcall AStarSearch_Ctor(CPathSearch *self);   // 0x004660b0
void __fastcall AStarSearch_FreeStack(CPathSearch *self);   // 0x00466100
void __fastcall AStarSearch_ExpandNode(CPathSearch *self, int /*edx*/, PathNode *node, int gx, int gy, int gz);   // 0x00466390
