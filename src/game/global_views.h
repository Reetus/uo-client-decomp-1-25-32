// Views of global objects that more than one ported file shares. Included at
// the end of globals.h (which declares the storage); not generated.
//
// Each of these objects has no single class in the headers yet: the files
// that use it pass it as `this` to methods of whatever class they know it
// by. A file with its own view of the same storage defines that next to its
// use.
#pragma once

#define g_gumpManager GLOBAL_PTR(void, g_gumpMgr_storage)           // this for gump-list calls
#define g_objectManager GLOBAL_PTR(void, g_objectManager_storage)
#define g_mapObject GLOBAL_PTR(void, g_map_storage)                 // world map / tile-block view
