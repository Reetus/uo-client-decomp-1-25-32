// Prototypes of the ported procedures in missile_gump_new.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

class CMissileGump;
class CObj;
struct Pos;

CMissileGump * __stdcall NewMissileGump_PosToPos(uint16_t art, Pos start, Pos end, uint8_t speed, uint8_t noRotate, uint8_t flag);   // 0x00437a80
CMissileGump * __stdcall NewMissileGump_PosToObj(uint16_t art, Pos start, CObj *target, uint8_t speed, uint8_t noRotate, uint8_t flag);   // 0x00437b20
CMissileGump * __stdcall NewMissileGump_ObjToPos(uint16_t art, CObj *src, Pos end, uint8_t speed, uint8_t noRotate, uint8_t flag);   // 0x00437bb0
CMissileGump * __stdcall NewMissileGump_ObjToObj(uint16_t art, CObj *src, CObj *dst, uint8_t speed, uint8_t noRotate, uint8_t flag);   // 0x00437c40
