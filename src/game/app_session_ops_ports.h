// Prototypes of the ported procedures in app_session_ops.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

class CObj;

void __cdecl RedrawInputLine(void);   // 0x0044dd70
void __fastcall SetWarMode(int mode);   // 0x0044dda0
void __fastcall ApplyWarMode(int mode);   // 0x0044de60
void __fastcall SendTextPacket(int type, const char *text);   // 0x0044df70
void __fastcall SendTargetCommand(CObj *target);   // 0x0044dfd0
void __fastcall BeginTargetCommand(const char *prefix, int kind, int number);   // 0x0044e1a0
void __cdecl ResetWorldState(void);   // 0x0044e200
void __fastcall SelectLoginAddress(int kind);   // 0x0044e250
void __fastcall ShowEnteringBritannia(int a, int b, int c);   // 0x0044e360
void __cdecl SendCreateCharacter(void);   // 0x0044e430
void __fastcall SendAccountLogin(int a, int b, int c);   // 0x0044e570
void __cdecl SingleClickVisible(void);   // 0x0044e640
int __cdecl ConnectLoginServer(void);   // 0x0044e750
