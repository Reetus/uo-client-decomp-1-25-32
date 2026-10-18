// The static initialisers of the original, in the order of its initialiser table (0x004a6004 to 0x004a60a0,
// which the CRT start-up walks before WinMain). Each constructs one global object and, where the object has a
// destructor, registers it with atexit.
#pragma once

void __cdecl CircleTrans_Init_004242c0();   // 0x004242c0
void __cdecl WorldBuildVisibleCellTable();   // 0x0042a350
void __cdecl InitSinCosTables();   // 0x00430df0
void __cdecl StaticInit_ColorWhite_00431d80();   // 0x00431d80
void __cdecl StaticInit_ColorCyan_00431da0();   // 0x00431da0
void __cdecl StaticInit_ColorMagenta_00431dc0();   // 0x00431dc0
void __cdecl StaticInit_ColorGray_00431de0();   // 0x00431de0
void __cdecl StaticInit_Obj005285e0_00443fe0();   // 0x00443fe0
void __cdecl StaticInit_App();   // 0x0044a880
void __cdecl StaticInit_ColourTables();   // 0x0044a8b0
void __cdecl StaticInit_ArtRecord_64cdac();   // 0x0044a8c0
void __cdecl StaticInit_FileManager();   // 0x0044a8f0
void __cdecl StaticInit_Obj_528698();   // 0x0044a920
void __cdecl StaticInit_MouseManager();   // 0x0044a950
void __cdecl StaticInit_ArtRecord_68d2d8();   // 0x0044a980
void __cdecl StaticInit_WorldRenderer();   // 0x0044a9b0
void __cdecl StaticInit_CellGrid();   // 0x0044a9e0
void __cdecl StaticInit_ObjectManager();   // 0x0044aa10
void __cdecl StaticInit_PaletteManager();   // 0x0044aa40
void __cdecl StaticInit_Obj_69e0e0();   // 0x0044aa70
void __cdecl StaticInit_Map();   // 0x0044aaa0
void __cdecl StaticInit_WalkManager();   // 0x0044aad0
void __cdecl StaticInit_TextList();   // 0x0044ab00
void __cdecl StaticInit_MusicManager();   // 0x0044ab30
void __cdecl StaticInit_SoundManager();   // 0x0044ab60
void __cdecl StaticInit_HueTable();   // 0x0044ab90
void __cdecl StaticInit_DDrawDisplay();   // 0x0044abc0
void __cdecl StaticInit_FontManager();   // 0x0044abf0
void __cdecl StaticInit_GumpManager();   // 0x0044ac20
void __cdecl StaticInit_AStarSearch();   // 0x0044ac50
void __cdecl StaticInit_ObjList();   // 0x0044ac80
void __cdecl StaticInit_LightTables();   // 0x0044acb0
void __cdecl StaticInit_AnimData();   // 0x0044acc0
void __cdecl StaticInit_RegionManager();   // 0x0044acd0
void __cdecl StaticInit_Weather();   // 0x0044ace0
void __cdecl StaticInit_0044e910();   // 0x0044e910
void __cdecl StaticInit_LightCircleTables();   // 0x0044ef20
void __cdecl StaticInit_AnimOverlayBuffer();   // 0x00470510
void __cdecl StaticInit_MainWindow();   // 0x00480b60

extern void(__cdecl *const g_staticInitialisers[])();
extern const int g_staticInitialiserCount;
