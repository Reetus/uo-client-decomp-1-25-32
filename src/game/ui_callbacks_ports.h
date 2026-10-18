// Prototypes of the ported procedures in ui_callbacks.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

class CBillGump;
class CGump;
class CShopGump;
class CSkillListGump;

void __fastcall UiCb_ToggleChildSend56(CGump *self);   // 0x0040cf90
void __fastcall UiCb_OpenStatusGump(CGump *self);   // 0x0040cfe0
void __cdecl UiCb_RequestPlayerStatus(void);   // 0x0040d4f0
void __cdecl UiCb_ToggleFightingGump(void);   // 0x0040d530
void __cdecl OpenQuitDialog(void);   // 0x0040d6a0
void __fastcall UiCb_TextScrollDown(CGump *self);   // 0x0040d720
void __fastcall UiCb_TextScrollUp(CGump *self);   // 0x0040d7d0
void __fastcall UiCb_TipNext(CGump *self);   // 0x0040d870
void __fastcall UiCb_TipPrev(CGump *self);   // 0x0040d8b0
void __fastcall UiCb_TextScrollToEnd(CGump *self);   // 0x0040d8f0
void __fastcall UiCb_PickUpOwner(CGump *self);   // 0x0040d920
void __fastcall UiCb_ShopNext(CShopGump *self);   // 0x0040d960
void __fastcall UiCb_ShopPrev(CShopGump *self);   // 0x0040da90
void __fastcall UiCb_BillNext(CBillGump *self);   // 0x0040dbc0
void __fastcall UiCb_BillPrev(CBillGump *self);   // 0x0040dc30
void __fastcall UiCb_SkillListDown(CSkillListGump *self);   // 0x0040dcc0
void __fastcall UiCb_SkillListUp(CSkillListGump *self);   // 0x0040de20
