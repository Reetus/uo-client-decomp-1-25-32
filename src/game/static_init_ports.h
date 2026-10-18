// Prototypes of the ported procedures in static_init.cpp that other files call.
// Written by scripts/unbridge.py; a procedure moves to a class header when its class is settled.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <windows.h>

void __cdecl AtExit_App(void);   // 0x0044a8a0
void __cdecl AtExit_ArtRecord_64cdac(void);   // 0x0044a8e0
void __cdecl AtExit_FileManager(void);   // 0x0044a910
void __cdecl AtExit_Obj_528698(void);   // 0x0044a940
void __cdecl AtExit_MouseManager(void);   // 0x0044a970
void __cdecl AtExit_ArtRecord_68d2d8(void);   // 0x0044a9a0
void __cdecl AtExit_WorldRenderer(void);   // 0x0044a9d0
void __cdecl AtExit_CellGrid(void);   // 0x0044aa00
void __cdecl AtExit_ObjectManager(void);   // 0x0044aa30
void __cdecl AtExit_PaletteManager(void);   // 0x0044aa60
void __cdecl AtExit_Obj_69e0e0(void);   // 0x0044aa90
void __cdecl AtExit_Map(void);   // 0x0044aac0
void __cdecl AtExit_WalkManager(void);   // 0x0044aaf0
void __cdecl AtExit_TextList(void);   // 0x0044ab20
void __cdecl AtExit_MusicManager(void);   // 0x0044ab50
void __cdecl AtExit_SoundManager(void);   // 0x0044ab80
void __cdecl AtExit_HueTable(void);   // 0x0044abb0
void __cdecl AtExit_DDrawDisplay(void);   // 0x0044abe0
void __cdecl AtExit_FontManager(void);   // 0x0044ac10
void __cdecl AtExit_GumpManager(void);   // 0x0044ac40
void __cdecl AtExit_AStarSearch(void);   // 0x0044ac70
void __cdecl AtExit_ObjList(void);   // 0x0044aca0
void __cdecl AtExit_Weather(void);   // 0x0044ad00
void __cdecl AtExit_0044e910(void);   // 0x0044e920
void __cdecl AtExit_MainWindow(void);   // 0x00480bb0
void __cdecl AtExit_CloseHandle_004b0264(void);   // 0x00480e40
void __cdecl AtExit_KillPresentTimer(void);   // 0x00480e50
