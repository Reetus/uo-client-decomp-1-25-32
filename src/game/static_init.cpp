// Third porting pass, batch u3: compiler static initialisers (entries of the CRT init table at 0x004a6000) and the
// atexit thunks they register. A static initialiser constructs one global object and registers a thunk with the CRT's
// atexit (0x0048c1e0); the thunk calls the object's destructor. All are __cdecl with no arguments. Constructors and
// destructors are called at their original addresses (ported or not).
#include <cstdint>
#include <cstring>

#include <windows.h>
#include <mmsystem.h>

#include "../runtime/ported.h"
#include "crt.h"
#include "raw_field.h"
#include "app.h"
#include "light_tables_ports.h"
#include "art_record_ops_ports.h"
#include "class_0049c690.h"
#include "file_manager.h"
#include "vtable_stubs_u2_ports.h"
#include "mouse_manager.h"
#include "world_renderer.h"
#include "object_manager.h"
#include "palette_manager.h"
#include "obj_extra_ports.h"
#include "map_world.h"
#include "walk.h"
#include "text_list.h"
#include "music_manager.h"
#include "sound_manager.h"
#include "hue_table.h"
#include "ddraw_display.h"
#include "font_manager.h"
#include "gump_manager.h"
#include "path_search_b25_ports.h"
#include "circle_trans.h"
#include "light.h"
#include "anim_data.h"
#include "region_manager.h"
#include "weather.h"
#include "mobile_anim.h"
#include "cell_grid.h"
#include "client_helpers_31.h"
#include "obj_00457780.h"
#include "static_init_ports.h"
#include "globals.h"

#pragma comment(lib, "winmm.lib")



// 0x0044a880  size 0x18
// Static initialiser of the object at 0x0069e0e8: constructor 0x0044b280 (CApp::Ctor), then atexit(0x0044a8a0).
void __cdecl StaticInit_App()
{
    reinterpret_cast<CApp *>(GLOBAL_PTR(void, g_app_storage))->Ctor();   // CApp::Ctor
    atexit(AtExit_App);
}
HOOK_FUNC(0x0044a880, StaticInit_App);

// 0x0044a8a0  size 0xa
// atexit thunk of the object at 0x0069e0e8: ECX = object, tail jump to the destructor 0x0044c030 (CApp::Dtor).
void __cdecl AtExit_App()
{
    reinterpret_cast<CApp *>(GLOBAL_PTR(void, g_app_storage))->Dtor();   // CApp::Dtor
}
HOOK_FUNC(0x0044a8a0, AtExit_App);

// 0x0044a8b0  size 0xa
// Static initialiser of the object at 0x0064cde0: ECX = object, tail jump to 0x0044f050 (BuildColourTables). No atexit.
void __cdecl StaticInit_ColourTables()
{
    PORT_THISCALL0(&BuildColourTables, void, GLOBAL_PTR(void, g_0064cde0_storage))();   // BuildColourTables
}
HOOK_FUNC(0x0044a8b0, StaticInit_ColourTables);

// 0x0044a8c0  size 0x18
// Static initialiser of the object at 0x0064cdac: constructor 0x00476120 (ArtRecord_ReturnThis), then atexit(0x0044a8e0).
void __cdecl StaticInit_ArtRecord_64cdac()
{
    PORT_THISCALL0(&ArtRecord_ReturnThis, void, GLOBAL_PTR(void, g_frameDataAllocator_storage))();   // ArtRecord_ReturnThis
    atexit(AtExit_ArtRecord_64cdac);
}
HOOK_FUNC(0x0044a8c0, StaticInit_ArtRecord_64cdac);

// 0x0044a8e0  size 0xa
// atexit thunk of the object at 0x0064cdac: ECX = object, tail jump to the destructor 0x00406720 (CWorldObj::ToVoid).
void __cdecl AtExit_ArtRecord_64cdac()
{
    reinterpret_cast<CWorldObj *>(GLOBAL_PTR(void, g_frameDataAllocator_storage))->CWorldObj::ToVoid();   // CWorldObj::ToVoid
}
HOOK_FUNC(0x0044a8e0, AtExit_ArtRecord_64cdac);

// 0x0044a8f0  size 0x18
// Static initialiser of the object at 0x0068d980: constructor 0x0043f9e0 (CFileManager::ClearTables), then atexit(0x0044a910).
void __cdecl StaticInit_FileManager()
{
    reinterpret_cast<CFileManager *>(GLOBAL_PTR(void, g_fileManager_storage))->ClearTables();   // CFileManager::ClearTables
    atexit(AtExit_FileManager);
}
HOOK_FUNC(0x0044a8f0, StaticInit_FileManager);

// 0x0044a910  size 0xa
// atexit thunk of the object at 0x0068d980: ECX = object, tail jump to the destructor 0x0043fa40 (file manager destructor).
void __cdecl AtExit_FileManager()
{
    PORT_THISCALL0(&FileManager_Dtor_0043fa40, void, GLOBAL_PTR(void, g_fileManager_storage))();   // file manager destructor
}
HOOK_FUNC(0x0044a910, AtExit_FileManager);

// 0x0044a920  size 0x18
// Static initialiser of the object at 0x00528698: constructor 0x0047d490 (CObj0047d490::InitSizes), then atexit(0x0044a940).
void __cdecl StaticInit_Obj_528698()
{
    reinterpret_cast<CObj0047d490 *>(GLOBAL_PTR(void, g_regionMusic_storage))->InitSizes();   // CObj0047d490::InitSizes
    atexit(AtExit_Obj_528698);
}
HOOK_FUNC(0x0044a920, StaticInit_Obj_528698);

// 0x0044a940  size 0xa
// atexit thunk of the object at 0x00528698: ECX = object, tail jump to the destructor 0x0047d560 (CObj0047d490::NopTwice).
void __cdecl AtExit_Obj_528698()
{
    reinterpret_cast<CObj0047d490 *>(GLOBAL_PTR(void, g_regionMusic_storage))->NopTwice();   // CObj0047d490::NopTwice
}
HOOK_FUNC(0x0044a940, AtExit_Obj_528698);

// 0x0044a950  size 0x18
// Static initialiser of the object at 0x0068d2e0: constructor 0x00446330 (CMouseManager::Ctor), then atexit(0x0044a970).
void __cdecl StaticInit_MouseManager()
{
    reinterpret_cast<CMouseManager *>(GLOBAL_PTR(void, g_mouse_storage))->CMouseManager::CMouseManager();   // CMouseManager::CMouseManager
    atexit(AtExit_MouseManager);
}
HOOK_FUNC(0x0044a950, StaticInit_MouseManager);

// 0x0044a970  size 0xa
// atexit thunk of the object at 0x0068d2e0: ECX = object, tail jump to the destructor 0x004467b0 (CMouseManager::Dtor).
void __cdecl AtExit_MouseManager()
{
    PORT_THISCALL0(&MouseManager_Dtor_004467b0, void, GLOBAL_PTR(void, g_mouse_storage))();   // mouse manager destructor
}
HOOK_FUNC(0x0044a970, AtExit_MouseManager);

// 0x0044a980  size 0x18
// Static initialiser of the object at 0x0068d2d8: constructor 0x00476120 (ArtRecord_ReturnThis), then atexit(0x0044a9a0).
void __cdecl StaticInit_ArtRecord_68d2d8()
{
    PORT_THISCALL0(&ArtRecord_ReturnThis, void, GLOBAL_PTR(void, g_artCache_storage))();   // ArtRecord_ReturnThis
    atexit(AtExit_ArtRecord_68d2d8);
}
HOOK_FUNC(0x0044a980, StaticInit_ArtRecord_68d2d8);

// 0x0044a9a0  size 0xa
// atexit thunk of the object at 0x0068d2d8: ECX = object, tail jump to the destructor 0x00406720 (CWorldObj::ToVoid).
void __cdecl AtExit_ArtRecord_68d2d8()
{
    reinterpret_cast<CWorldObj *>(GLOBAL_PTR(void, g_artCache_storage))->CWorldObj::ToVoid();   // CWorldObj::ToVoid
}
HOOK_FUNC(0x0044a9a0, AtExit_ArtRecord_68d2d8);

// 0x0044a9b0  size 0x18
// Static initialiser of the object at 0x0069e0c0: constructor 0x0042a490 (CWorldRenderer::Reset), then atexit(0x0044a9d0).
void __cdecl StaticInit_WorldRenderer()
{
    reinterpret_cast<CWorldRenderer *>(GLOBAL_PTR(void, g_worldPicker_storage))->Reset();   // CWorldRenderer::Reset
    atexit(AtExit_WorldRenderer);
}
HOOK_FUNC(0x0044a9b0, StaticInit_WorldRenderer);

// 0x0044a9d0  size 0xa
// atexit thunk of the object at 0x0069e0c0: ECX = object, tail jump to the destructor 0x00406720 (CWorldObj::ToVoid).
void __cdecl AtExit_WorldRenderer()
{
    reinterpret_cast<CWorldObj *>(GLOBAL_PTR(void, g_worldPicker_storage))->CWorldObj::ToVoid();   // CWorldObj::ToVoid
}
HOOK_FUNC(0x0044a9d0, AtExit_WorldRenderer);

// 0x0044aa10  size 0x18
// Static initialiser of the object at 0x0068ce58: constructor 0x00454f10 (CObjectManager::Ctor), then atexit(0x0044aa30).
void __cdecl StaticInit_ObjectManager()
{
    reinterpret_cast<CObjectManager *>(GLOBAL_PTR(void, g_objMgr_storage))->Ctor();   // CObjectManager::Ctor
    atexit(AtExit_ObjectManager);
}
HOOK_FUNC(0x0044aa10, StaticInit_ObjectManager);

// 0x0044aa30  size 0xa
// atexit thunk of the object at 0x0068ce58: ECX = object, tail jump to the destructor 0x00454fc0 (CObjectManager::Dtor).
void __cdecl AtExit_ObjectManager()
{
    reinterpret_cast<CObjectManager *>(GLOBAL_PTR(void, g_objMgr_storage))->Dtor();   // CObjectManager::Dtor
}
HOOK_FUNC(0x0044aa30, AtExit_ObjectManager);

// 0x0044aa40  size 0x18
// Static initialiser of the object at 0x0068dd98: constructor 0x004368f0 (CPaletteManager::Ctor), then atexit(0x0044aa60).
void __cdecl StaticInit_PaletteManager()
{
    reinterpret_cast<CPaletteManager *>(GLOBAL_PTR(void, g_clientState_storage))->CPaletteManager::CPaletteManager();   // CPaletteManager::CPaletteManager
    atexit(AtExit_PaletteManager);
}
HOOK_FUNC(0x0044aa40, StaticInit_PaletteManager);

// 0x0044aa60  size 0xa
// atexit thunk of the object at 0x0068dd98: ECX = object, tail jump to the destructor 0x00436b20 (CPaletteManager::Dtor).
void __cdecl AtExit_PaletteManager()
{
    PORT_THISCALL0(&PaletteManager_Dtor_00436b20, void, GLOBAL_PTR(void, g_clientState_storage))();   // palette manager destructor
}
HOOK_FUNC(0x0044aa60, AtExit_PaletteManager);

// 0x0044aa70  size 0x18
// Static initialiser of the object at 0x0069e0e0: constructor 0x00457780 (CObj_00457780::Ctor), then atexit(0x0044aa90).
void __cdecl StaticInit_Obj_69e0e0()
{
    reinterpret_cast<CObj_00457780 *>(GLOBAL_PTR(void, g_packetBuilder_storage))->CObj_00457780::CObj_00457780();   // CObj_00457780::CObj_00457780
    atexit(AtExit_Obj_69e0e0);
}
HOOK_FUNC(0x0044aa70, StaticInit_Obj_69e0e0);

// 0x0044aa90  size 0xa
// atexit thunk of the object at 0x0069e0e0: ECX = object, tail jump to the destructor 0x004577c0 (sets the vtable pointer, 0x004577c0).
void __cdecl AtExit_Obj_69e0e0()
{
    PORT_THISCALL0(&Obj457780_Dtor, void, GLOBAL_PTR(void, g_packetBuilder_storage))();   // sets the vtable pointer, 0x004577c0
}
HOOK_FUNC(0x0044aa90, AtExit_Obj_69e0e0);

// 0x0044aaa0  size 0x18
// Static initialiser of the object at 0x0068dbb8: constructor 0x00445370 (CMap::Ctor), then atexit(0x0044aac0).
void __cdecl StaticInit_Map()
{
    reinterpret_cast<CMap *>(GLOBAL_PTR(void, g_map_storage))->CMap::CMap();   // CMap::CMap
    atexit(AtExit_Map);
}
HOOK_FUNC(0x0044aaa0, StaticInit_Map);

// 0x0044aac0  size 0xa
// atexit thunk of the object at 0x0068dbb8: ECX = object, tail jump to the destructor 0x00445410 (map destructor).
void __cdecl AtExit_Map()
{
    PORT_THISCALL0(&MapData_SetVtable_00445410, void, GLOBAL_PTR(void, g_map_storage))();   // map destructor
}
HOOK_FUNC(0x0044aac0, AtExit_Map);

// 0x0044aad0  size 0x18
// Static initialiser of the object at 0x00648848: constructor 0x00448580 (CWalkManager::Ctor), then atexit(0x0044aaf0).
void __cdecl StaticInit_WalkManager()
{
    reinterpret_cast<CWalkManager *>(GLOBAL_PTR(void, g_walk_storage))->CWalkManager::CWalkManager();   // CWalkManager::CWalkManager
    atexit(AtExit_WalkManager);
}
HOOK_FUNC(0x0044aad0, StaticInit_WalkManager);

// 0x0044aaf0  size 0xa
// atexit thunk of the object at 0x00648848: ECX = object, tail jump to the destructor 0x004487d0 (walk manager destructor).
void __cdecl AtExit_WalkManager()
{
    PORT_THISCALL0(&WalkManager_SetVtable_004487d0, void, GLOBAL_PTR(void, g_walk_storage))();   // walk manager destructor
}
HOOK_FUNC(0x0044aaf0, AtExit_WalkManager);

// 0x0044ab00  size 0x18
// Static initialiser of the object at 0x006486b0: constructor 0x00478e80 (CTextList::Ctor), then atexit(0x0044ab20).
void __cdecl StaticInit_TextList()
{
    reinterpret_cast<CTextList *>(GLOBAL_PTR(void, g_msgTarget_storage))->CTextList::CTextList();   // CTextList::CTextList
    atexit(AtExit_TextList);
}
HOOK_FUNC(0x0044ab00, StaticInit_TextList);

// 0x0044ab20  size 0xa
// atexit thunk of the object at 0x006486b0: ECX = object, tail jump to the destructor 0x00478ef0 (CTextList::Dtor, 0x00478ef0).
void __cdecl AtExit_TextList()
{
    PORT_THISCALL0(&TextList_Dtor_00478ef0, void, GLOBAL_PTR(void, g_msgTarget_storage))();   // CTextList destructor, 0x00478ef0
}
HOOK_FUNC(0x0044ab20, AtExit_TextList);

// 0x0044ab30  size 0x18
// Static initialiser of the object at 0x0069fa98: constructor 0x0047ec20 (MusicManager::Ctor), then atexit(0x0044ab50).
void __cdecl StaticInit_MusicManager()
{
    reinterpret_cast<MusicManager *>(GLOBAL_PTR(void, g_musicManager_storage))->Ctor();   // MusicManager::Ctor
    atexit(AtExit_MusicManager);
}
HOOK_FUNC(0x0044ab30, StaticInit_MusicManager);

// 0x0044ab50  size 0xa
// atexit thunk of the object at 0x0069fa98: ECX = object, tail jump to the destructor 0x0047eca0 (MusicManager::Dtor).
void __cdecl AtExit_MusicManager()
{
    reinterpret_cast<MusicManager *>(GLOBAL_PTR(void, g_musicManager_storage))->Dtor();   // MusicManager::Dtor
}
HOOK_FUNC(0x0044ab50, AtExit_MusicManager);

// 0x0044ab60  size 0x18
// Static initialiser of the object at 0x0068db30: constructor 0x0047f4b0 (CSoundManager::Ctor), then atexit(0x0044ab80).
void __cdecl StaticInit_SoundManager()
{
    reinterpret_cast<CSoundManager *>(GLOBAL_PTR(void, g_soundManager_storage))->Ctor();   // CSoundManager::Ctor
    atexit(AtExit_SoundManager);
}
HOOK_FUNC(0x0044ab60, StaticInit_SoundManager);

// 0x0044ab80  size 0xa
// atexit thunk of the object at 0x0068db30: ECX = object, tail jump to the destructor 0x0047f680 (CSoundManager::Dtor).
void __cdecl AtExit_SoundManager()
{
    reinterpret_cast<CSoundManager *>(GLOBAL_PTR(void, g_soundManager_storage))->Dtor();   // CSoundManager::Dtor
}
HOOK_FUNC(0x0044ab80, AtExit_SoundManager);

// 0x0044ab90  size 0x18
// Static initialiser of the object at 0x006a3ff8: constructor 0x00441370 (CHueTable::LoadHues), then atexit(0x0044abb0).
void __cdecl StaticInit_HueTable()
{
    reinterpret_cast<CHueTable *>(GLOBAL_PTR(void, g_hueTable_storage))->LoadHues();   // CHueTable::LoadHues
    atexit(AtExit_HueTable);
}
HOOK_FUNC(0x0044ab90, StaticInit_HueTable);

// 0x0044abb0  size 0xa
// atexit thunk of the object at 0x006a3ff8: ECX = object, tail jump to the destructor 0x00406720 (CWorldObj::ToVoid).
void __cdecl AtExit_HueTable()
{
    reinterpret_cast<CWorldObj *>(GLOBAL_PTR(void, g_hueTable_storage))->CWorldObj::ToVoid();   // CWorldObj::ToVoid
}
HOOK_FUNC(0x0044abb0, AtExit_HueTable);

// 0x0044abc0  size 0x18
// Static initialiser of the object at 0x00648820: constructor 0x0047e4e0 (DDrawDisplay::ResetReady), then atexit(0x0044abe0).
void __cdecl StaticInit_DDrawDisplay()
{
    reinterpret_cast<DDrawDisplay *>(GLOBAL_PTR(void, g_ddraw_storage))->ResetReady();   // DDrawDisplay::ResetReady
    atexit(AtExit_DDrawDisplay);
}
HOOK_FUNC(0x0044abc0, StaticInit_DDrawDisplay);

// 0x0044abe0  size 0xa
// atexit thunk of the object at 0x00648820: ECX = object, tail jump to the destructor 0x0047e500 (DDrawDisplay::Release).
void __cdecl AtExit_DDrawDisplay()
{
    reinterpret_cast<DDrawDisplay *>(GLOBAL_PTR(void, g_ddraw_storage))->Release();   // DDrawDisplay::Release
}
HOOK_FUNC(0x0044abe0, AtExit_DDrawDisplay);

// 0x0044abf0  size 0x18
// Static initialiser of the object at 0x0064cde8: constructor 0x00431e00 (CFontManager::Ctor), then atexit(0x0044ac10).
void __cdecl StaticInit_FontManager()
{
    reinterpret_cast<CFontManager *>(GLOBAL_PTR(void, g_fontMgr_storage))->Ctor();   // CFontManager::Ctor
    atexit(AtExit_FontManager);
}
HOOK_FUNC(0x0044abf0, StaticInit_FontManager);

// 0x0044ac10  size 0xa
// atexit thunk of the object at 0x0064cde8: ECX = object, tail jump to the destructor 0x00431e60 (CFontManager::Dtor).
void __cdecl AtExit_FontManager()
{
    reinterpret_cast<CFontManager *>(GLOBAL_PTR(void, g_fontMgr_storage))->Dtor();   // CFontManager::Dtor
}
HOOK_FUNC(0x0044ac10, AtExit_FontManager);

// 0x0044ac20  size 0x18
// Static initialiser of the object at 0x006488c0: constructor 0x00437000 (CGumpManager::Ctor), then atexit(0x0044ac40).
void __cdecl StaticInit_GumpManager()
{
    reinterpret_cast<CGumpManager *>(GLOBAL_PTR(void, g_gumpMgr_storage))->CGumpManager::CGumpManager();   // CGumpManager::CGumpManager
    atexit(AtExit_GumpManager);
}
HOOK_FUNC(0x0044ac20, StaticInit_GumpManager);

// 0x0044ac40  size 0xa
// atexit thunk of the object at 0x006488c0: ECX = object, tail jump to the destructor 0x00437140 (CGumpManager::Dtor).
void __cdecl AtExit_GumpManager()
{
    PORT_THISCALL0(&GumpManager_Dtor_00437140, void, GLOBAL_PTR(void, g_gumpMgr_storage))();   // gump manager destructor
}
HOOK_FUNC(0x0044ac40, AtExit_GumpManager);

// 0x0044ac50  size 0x18
// Static initialiser of the object at 0x0064cdb0: constructor 0x004660b0 (AStarSearch_Ctor), then atexit(0x0044ac70).
void __cdecl StaticInit_AStarSearch()
{
    PORT_THISCALL0(&AStarSearch_Ctor, void, GLOBAL_PTR(void, g_pathSearch_storage))();   // AStarSearch_Ctor
    atexit(AtExit_AStarSearch);
}
HOOK_FUNC(0x0044ac50, StaticInit_AStarSearch);

// 0x0044ac70  size 0xa
// atexit thunk of the object at 0x0064cdb0: ECX = object, tail jump to the destructor 0x00466100 (AStarSearch_FreeStack).
void __cdecl AtExit_AStarSearch()
{
    PORT_THISCALL0(&AStarSearch_FreeStack, void, GLOBAL_PTR(void, g_pathSearch_storage))();   // AStarSearch_FreeStack
}
HOOK_FUNC(0x0044ac70, AtExit_AStarSearch);

// 0x0044ac80  size 0x18
// Static initialiser of the object at 0x006f4740: constructor 0x004248b0 (CObjList::Init), then atexit(0x0044aca0).
void __cdecl StaticInit_ObjList()
{
    PORT_THISCALL0(&CObjList::Init, void, GLOBAL_PTR(void, g_tickIndexHead_storage))();   // CObjList::Init
    atexit(AtExit_ObjList);
}
HOOK_FUNC(0x0044ac80, StaticInit_ObjList);

// 0x0044aca0  size 0xa
// atexit thunk of the object at 0x006f4740: ECX = object, tail jump to the destructor 0x00406720 (CWorldObj::ToVoid).
void __cdecl AtExit_ObjList()
{
    reinterpret_cast<CWorldObj *>(GLOBAL_PTR(void, g_tickIndexHead_storage))->CWorldObj::ToVoid();   // CWorldObj::ToVoid
}
HOOK_FUNC(0x0044aca0, AtExit_ObjList);

// 0x0044acb0  size 0xa
// Static initialiser of the object at 0x0064cda8: ECX = object, tail jump to 0x0044f2e0 (LoadLightTables). No atexit.
void __cdecl StaticInit_LightTables()
{
    PORT_THISCALL0(&LoadLightTables, void, GLOBAL_PTR(void, g_0064cda8_storage))();   // LoadLightTables
}
HOOK_FUNC(0x0044acb0, StaticInit_LightTables);

// 0x0044acc0  size 0xa
// Static initialiser of the object at 0x005286b0: ECX = object, tail jump to 0x00401000 (CAnimData::Ctor). No atexit.
void __cdecl StaticInit_AnimData()
{
    reinterpret_cast<CAnimData *>(GLOBAL_PTR(void, g_animData_storage))->Ctor();   // CAnimData::Ctor
}
HOOK_FUNC(0x0044acc0, StaticInit_AnimData);

// 0x0044acd0  size 0xa
// Static initialiser of the object at 0x006e4738: ECX = object, tail jump to 0x0046b160 (CRegionManager::Ctor, 0x0046b160). No atexit.
void __cdecl StaticInit_RegionManager()
{
    reinterpret_cast<CRegionManager *>(GLOBAL_PTR(void, g_006e4738_storage))->Ctor();   // CRegionManager::Ctor, 0x0046b160
}
HOOK_FUNC(0x0044acd0, StaticInit_RegionManager);

// 0x0044ace0  size 0x18
// Static initialiser of the object at 0x006486e0: constructor 0x0047d590 (CWeather::Ctor), then atexit(0x0044ad00).
void __cdecl StaticInit_Weather()
{
    PORT_THISCALL0(&CWeather::Ctor, void, GLOBAL_PTR(void, g_weatherObj_storage))();   // CWeather::Ctor
    atexit(AtExit_Weather);
}
HOOK_FUNC(0x0044ace0, StaticInit_Weather);

// 0x0044ad00  size 0xa
// atexit thunk of the object at 0x006486e0: ECX = object, tail jump to the destructor 0x0047d5d0 (CWeather::DestroyParticles).
void __cdecl AtExit_Weather()
{
    PORT_THISCALL0(&CWeather::DestroyParticles, void, GLOBAL_PTR(void, g_weatherObj_storage))();   // CWeather::DestroyParticles
}
HOOK_FUNC(0x0044ad00, AtExit_Weather);

// 0x0044ef20  size 0xa
// Static initialiser of the object at 0x0071c6a8: ECX = object, tail jump to 0x0044ef30 (BuildLightCircleTables). No atexit.
void __cdecl StaticInit_LightCircleTables()
{
    PORT_THISCALL0(&BuildLightCircleTables, void, GLOBAL_PTR(void, g_0071c6a8_storage))();   // BuildLightCircleTables
}
HOOK_FUNC(0x0044ef20, StaticInit_LightCircleTables);

// 0x00470510  size 0xa
// Static initialiser of the object at 0x007226a0: ECX = object, tail jump to 0x00470520 (CAnimOverlayBuffer::BuildIndexMaps). No atexit.
void __cdecl StaticInit_AnimOverlayBuffer()
{
    reinterpret_cast<CAnimOverlayBuffer *>(GLOBAL_PTR(void, g_animOverlayBuffer_storage))->BuildIndexMaps();   // CAnimOverlayBuffer::BuildIndexMaps
}
HOOK_FUNC(0x00470510, StaticInit_AnimOverlayBuffer);

// 0x0044a9e0  size 0x18
// Static initialiser of the cell grid at 0x0069ffe8: constructor 0x0042fa60 (CCellGrid::Ctor), then atexit(0x0044aa00).
void __cdecl StaticInit_CellGrid()
{
    reinterpret_cast<CCellGrid *>(GLOBAL_PTR(void, g_cellGrid_storage))->Ctor();   // CCellGrid::Ctor
    atexit(AtExit_CellGrid);
}
HOOK_FUNC(0x0044a9e0, StaticInit_CellGrid);

// 0x0044aa00  size 0x1
// atexit thunk of the cell grid: a bare ret (the grid has no destructor). Followed by padding.
void __cdecl AtExit_CellGrid()
{
}
HOOK_FUNC(0x0044aa00, AtExit_CellGrid);

// 0x0044e910  size 0xe
// Static initialiser with no object: only atexit(0x0044e920).
void __cdecl StaticInit_0044e910()
{
    atexit(AtExit_0044e910);
}
HOOK_FUNC(0x0044e910, StaticInit_0044e910);

// 0x0044e920  size 0x1
// atexit thunk that does nothing: a bare ret. Followed by padding.
void __cdecl AtExit_0044e910()
{
}
HOOK_FUNC(0x0044e920, AtExit_0044e910);

// 0x00480b60  size 0x48
// Static initialiser of the main window object at 0x00758368 (CMainWindow): zero its 0x400 bytes, set the scalar
// globals around it (0x007588b0 timer id, 0x0075876c, 0x007588a8, 0x007588a4 = 1, 0x007582a4, 0x007589f8), then
// atexit(0x00480bb0). The timer id store comes before the clear in the original; the address is outside the cleared
// range, so the order is not observable.
void __cdecl StaticInit_MainWindow()
{
    g_timerId = 0;
    memset(GLOBAL_PTR(void, g_mainWindowObject_storage), 0, 0x400);
    g_0075876c = 0;
    g_007588a8 = 0;
    wp_activeFlag = 1;
    GLOBAL_AS(uint32_t, wh_fullscreenFlag) = 0;
    wp_frameBusy = 0;
    atexit(AtExit_MainWindow);
}
HOOK_FUNC(0x00480b60, StaticInit_MainWindow);

// 0x00480bb0  size 0x1
// atexit thunk of the main window object: a bare ret. Followed by padding.
void __cdecl AtExit_MainWindow()
{
}
HOOK_FUNC(0x00480bb0, AtExit_MainWindow);

// 0x00480e40  size 0xd
// atexit handler registered by WinMain (0x00480f80): CloseHandle on the handle stored at 0x004b0264.
void __cdecl AtExit_CloseHandle_004b0264()
{
    CloseHandle(g_singleInstanceMapping);
}
HOOK_FUNC(0x00480e40, AtExit_CloseHandle_004b0264);

// 0x00480e50  size 0x27
// atexit handler registered by WinMain (0x00480f8f): under the present critical section (0x00528640) kill the
// multimedia timer whose id is at 0x007588b0, when there is one.
void __cdecl AtExit_KillPresentTimer()
{
    CRITICAL_SECTION *cs = GLOBAL_PTR(CRITICAL_SECTION, g_presentLock_storage);
    EnterCriticalSection(cs);
    UINT timer = g_timerId;
    if (timer != 0)
        timeKillEvent(timer);
    LeaveCriticalSection(cs);
}
HOOK_FUNC(0x00480e50, AtExit_KillPresentTimer);
