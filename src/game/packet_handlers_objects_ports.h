// Prototypes of the ported procedures in packet_handlers_objects.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __stdcall Packet_Handle_004577d0(uint8_t *packet);   // 0x004577d0
void __stdcall Packet_GlobalLight(uint8_t *packet);   // 0x004577f0
void __stdcall Packet_AsciiSpeech(uint8_t *packet);   // 0x00457850
void __stdcall Packet_NewSubserver(uint8_t *packet);   // 0x00458080
void __stdcall Packet_DeleteObject(uint8_t *packet);   // 0x00458220
void __stdcall Packet_DrawPlayer(uint8_t *packet);   // 0x004582b0
void __stdcall Packet_MoveAccept(uint8_t *packet);   // 0x00458580
void __stdcall Packet_MobileMoving(uint8_t *packet);   // 0x004585f0
void __stdcall Packet_CorpseEquip(uint8_t *packet);   // 0x00458850
void __stdcall Packet_WorldItem(uint8_t *packet);   // 0x00458cc0
void __stdcall Packet_Group(uint8_t *packet);   // 0x00458f50
void __stdcall Packet_MoveReject(uint8_t *packet);   // 0x00459400
void __stdcall Packet_AddToContainer(uint8_t *packet);   // 0x00459820
void __stdcall Packet_ContainerContents(uint8_t *packet);   // 0x004599a0
void __stdcall Packet_DropClear_0x28(uint8_t *packet);   // 0x00459f00
void __stdcall Packet_DropClear_0x29(uint8_t *packet);   // 0x00459f70
void __stdcall Packet_FightOccurring(uint8_t *packet);   // 0x00459f90
void __stdcall Packet_ClientCode(uint8_t *packet);   // 0x00459ff0
void __stdcall Packet_MobileAttributes(uint8_t *packet);   // 0x0045a0f0
void __stdcall Packet_UpdateHealthBar(uint8_t *packet);   // 0x0045a3b0
void __stdcall Packet_UpdateManaBar(uint8_t *packet);   // 0x0045a500
void __stdcall Packet_UpdateStaminaBar(uint8_t *packet);   // 0x0045a650
void __stdcall Packet_EquipItem(uint8_t *packet);   // 0x0045a7a0
