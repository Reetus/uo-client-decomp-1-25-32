// Prototypes of the ported procedures in packet_builders_b25.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __stdcall Pkt_Build_0x66_BookPageRequest(uint8_t *buf, uint32_t serial, uint32_t page);   // 0x004641e0
void __stdcall Pkt_Build_0x66_BookPageSend(uint8_t *buf, void *book, uint32_t page);   // 0x00464360
void __stdcall Pkt_Build_0x6F_SecureTrade(uint8_t *buf, uint8_t action, uint32_t id1, uint32_t id2, uint32_t id3);   // 0x00464900
void __stdcall Pkt_Build_0x71_BBoardSimple(uint8_t *buf, uint8_t type, uint32_t boardSerial, uint32_t msgSerial);   // 0x00464ac0
void __stdcall Pkt_Build_0x72_WarMode(uint8_t *buf, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);   // 0x00464be0
void __stdcall Pkt_Build_0x73_Ping(uint8_t *buf, uint8_t seq);   // 0x00465030
void __stdcall Pkt_Build_0x75_RenameCharacter(uint8_t *buf, uint32_t serial, const void *name30);   // 0x004650b0
void __stdcall Pkt_Build_0x7D_MenuResponse(uint8_t *buf, uint32_t dialogId, uint32_t w1, uint32_t w2, uint32_t w3, uint32_t w4);   // 0x00465190
void __stdcall Pkt_Build_0x95_DyeWindowReply(uint8_t *buf, uint32_t serial, uint32_t w1, uint32_t w2);   // 0x00465360
void __stdcall Pkt_Build_0x80_AccountLogin(uint8_t *buf, const void *account30, const void *password30, uint8_t key);   // 0x00465490
void __stdcall Pkt_Build_0x83_DeleteCharacter(uint8_t *buf, uint32_t slot, const void *password30);   // 0x004655c0
void __stdcall Pkt_Build_0x84(uint8_t *buf, uint32_t slot, const void *blobA30, const void *blobB30);   // 0x00465700
void __stdcall Pkt_Build_0x98_AllNamesQuery(uint8_t *buf, uint32_t serial);   // 0x00465890
void __stdcall Pkt_Build_0x9A_PromptReply(uint8_t *buf, uint32_t s1, uint32_t s2, uint32_t s3, const char *text);   // 0x00465910
void __stdcall Pkt_Build_0xA0_SelectServer(uint8_t *buf, uint32_t index);   // 0x00465ab0
void __stdcall Walk_ForcedFromPacket(const uint8_t *packet);   // 0x00465b30
void __stdcall Pkt_Build_0xA7_RequestTip(uint8_t *buf, uint32_t tipIndex, uint8_t type);   // 0x00465de0
void __stdcall Pkt_Build_0xAC_GumpTextEntryReply(uint8_t *buf, uint32_t serial, uint32_t gumpType, uint8_t button, const char *text);   // 0x00465eb0
