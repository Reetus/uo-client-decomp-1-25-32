#include "app.h"

#include <cstring>

#include "../runtime/ported.h"
#include "globals.h"

// Path buffers the original code shares for resolved file names.

// The "\\" separator string the original code appends (PTR_DAT_004a8f50).

static void append_path(char *out, const char *base, const char *name)
{
    strcpy(out, base);
    size_t len = strlen(out);
    if (len != 0 && out[len - 1] != '\\')
        strcat(out, g_pathSeparator);
    strcat(out, name);
}

// 0x0044c340  size 0xb0
char *__thiscall CApp::ResolveDataPath(char *name)
{
    append_path(g_resolvedPath, dataPath, name);
    return g_resolvedPath;
}
HOOK_METHOD(0x0044c340, CApp, ResolveDataPath);

// 0x0044c3f0  size 0xb0
char *__thiscall CApp::ResolveDataPath2(char *name)
{
    append_path(g_resolvedPath2, dataPath2, name);
    return g_resolvedPath2;
}
HOOK_METHOD(0x0044c3f0, CApp, ResolveDataPath2);
