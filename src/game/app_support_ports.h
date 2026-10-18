// Prototypes of the ported procedures in app_support.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

struct MacroAction;

char * __fastcall App_ReadLineFromFile(FILE *f);   // 0x0044ad10
void __fastcall App_ObfuscateString(char *text);   // 0x0044ad90
void __fastcall App_DeobfuscateString(char *text);   // 0x0044adc0
char * __fastcall App_GetInstCDPath(char *out, int outSize);   // 0x0044b130
int __cdecl App_OutOfMemoryHandler(size_t);   // 0x0044b200
void __cdecl App_SetCursor13(void);   // 0x0044b240
void __cdecl App_SetCursor11(void);   // 0x0044b260
int __fastcall App_RandomMod(int n);   // 0x0044b650
int __fastcall App_IsInList_0069e8b0(int value);   // 0x0044c000
void __stdcall App_GameTick(int flag);   // 0x0044c180
void __cdecl App_SendAction43(void);   // 0x0044c300
void __stdcall App_HandleKey(int key);   // 0x0044c4a0
void * __fastcall Gump_FindByKind(int kind);   // 0x0044c640
void __fastcall App_RunSimpleMacro(int command);   // 0x0044c760
void __fastcall Macro_ParseAction(char *line, MacroAction *out);   // 0x0044d140
