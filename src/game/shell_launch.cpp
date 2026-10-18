// Launching other programs: CreateProcessA wrapper and "open this URL with the
// registered .html handler".
#include <cstring>

#include <windows.h>

#include "../runtime/ported.h"
#include "globals.h"


// 0x0047df10  size 0x10f
// Launch a process (CreateProcessA); returns the process handle or -1.
// __fastcall: ECX unused, EDX = command line, one stack arg = working directory (RET 4).
static HANDLE __fastcall LaunchProcess(int unused, char *cmdLine, char *workDir)
{
    (void)unused;
    char cmdCopy[260];
    char dirCopy[260];
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    char *dirArg;

    strcpy(cmdCopy, cmdLine);

    si.cb = 0x44;
    strcpy(dirCopy, workDir);

    si.lpReserved = 0;
    si.lpDesktop = 0;
    si.lpTitle = 0;
    si.dwX = 0;
    si.dwY = 0;
    si.dwXSize = 0;
    si.dwYSize = 0;
    si.dwXCountChars = 0;
    si.dwYCountChars = 0;
    si.dwFillAttribute = 0;
    si.dwFlags = 0;
    si.wShowWindow = 0;
    si.cbReserved2 = 0;
    si.lpReserved2 = 0;
    si.hStdInput = 0;
    si.hStdOutput = 0;
    si.hStdError = 0;

    // the working directory is passed as NULL when the string is empty
    dirArg = (dirCopy[0] != 0) ? dirCopy : nullptr;

    if (CreateProcessA(nullptr, cmdCopy, nullptr, nullptr, FALSE, 0, nullptr, dirArg, &si, &pi) != 0) {
        CloseHandle(pi.hThread);
        return pi.hProcess;
    }
    g_launchDirError = (GetLastError() == 0x10b) ? 1 : 0;
    return (HANDLE)0xffffffff;
}
HOOK_FUNC(0x0047df10, LaunchProcess);

static inline int IsZero(LONG status)
{
    return status == 0;
}

// 0x0047e020  size 0x17f
// Open a URL via the registered .html handler (HKCR\.html -> <class>\shell\open\command), then launch it
// through LaunchProcess. __fastcall: ECX is passed through unchanged, EDX = URL, one stack arg = working
// directory (RET 4). The registry keys are never closed (as in the original).
HANDLE __fastcall OpenUrlWithBrowser(int ecxArg, char *url, char *workDir)
{
    HKEY hKey;
    DWORD size2;
    DWORD size1;
    BYTE valueA[1024];
    BYTE valueB[260];
    int ok;
    char *found;

    size2 = 0x104;
    if (RegOpenKeyExA(HKEY_CLASSES_ROOT, ".html", 0, KEY_QUERY_VALUE, &hKey) != 0) {
        ok = 0;
    } else {
        size1 = 0x400;
        if (RegQueryValueExA(hKey, g_emptyString, nullptr, nullptr, valueA, &size1) != 0) {
            ok = 0;
        } else {
            strcat((char *)valueA, "\\shell\\open\\command");

            if (RegOpenKeyExA(HKEY_CLASSES_ROOT, (LPCSTR)valueA, 0, KEY_QUERY_VALUE, &hKey) != 0)
                ok = 0;
            else
                ok = IsZero(RegQueryValueExA(hKey, g_emptyString, nullptr, nullptr, valueB, &size2));
        }
    }
    if (ok == 0)
        return nullptr;   // EAX = 0

    found = strstr((char *)valueB, " \"%1\"");
    if (found != nullptr)
        *found = 0;

    strcat((char *)valueB, " ");
    strcat((char *)valueB, url);

    return LaunchProcess(ecxArg, (char *)valueB, workDir);   // the caller compares the result with -1
}
HOOK_FUNC(0x0047e020, OpenUrlWithBrowser);
