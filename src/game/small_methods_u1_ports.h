// Prototypes of the ported procedures in small_methods_u1.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

int __fastcall U1_Return0(void *);   // 0x0040c7c0
int __fastcall U1_Return1(void *);   // 0x0040c7d0
void * __fastcall U1_VecCtorNop(void *self);   // 0x0040cf20
void * __fastcall U1_OpenGumpType7(void *);   // 0x0040d4d0
void * __fastcall U1_OpenGumpType1388(void *);   // 0x0040d680
void __fastcall U1_HelpButton(void *);   // 0x0040d710
int __fastcall U1_CallSlot1c(void *self);   // 0x0040f820
void __fastcall U1_ExitButton(void *);   // 0x00410db0
void __fastcall U1_DocumentationButton(void *);   // 0x00411090
int __fastcall U1_RegistrationButton(void *);   // 0x004110d0
void __fastcall U1_LoginCompleteCallback(void *);   // 0x00412200
void __fastcall U1_TutorialFinish(void *self);   // 0x00412730
void __fastcall U1_LowerNotorietyYes(void *self);   // 0x00412890
