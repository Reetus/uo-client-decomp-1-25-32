// CSysInfo::ReadRegistry / WriteRegistry: the HWProfile registry key. (Porting pass 2, batch 15.)
#include "sys_info.h"

#include <windows.h>

#include "../runtime/ported.h"

#define HWPROFILE_KEY "SOFTWARE\\Origin Worlds Online\\Ultima Online\\1.0\\HWProfile"

// 0x0043f240  size 0x1f2
// Every value is read into its field; the first failure returns 0. The key handle is not closed.
int __thiscall CSysInfo::ReadRegistry()
{
    HKEY key;
    DWORD type;
    DWORD size;
    DWORD r;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, HWPROFILE_KEY, 0, KEY_QUERY_VALUE, &key) != 0)
        return 0;

    size = 1;
    if (RegQueryValueExA(key, "processorType", nullptr, &type, &cpuClass, &size) != 0)
        return 0;
    size = 2;
    if (RegQueryValueExA(key, "processorClock", nullptr, &type, reinterpret_cast<LPBYTE>(&clockMHz), &size) != 0)
        return 0;
    size = 1;
    if (RegQueryValueExA(key, "nProcessors", nullptr, &type, &processors, &size) != 0)
        return 0;
    size = 0x20;
    if (RegQueryValueExA(key, "szVideoCardDescrip1", nullptr, &type, reinterpret_cast<LPBYTE>(video1), &size) != 0)
        return 0;
    size = 0x20;
    if (RegQueryValueExA(key, "szVideoCardDescrip2", nullptr, &type, reinterpret_cast<LPBYTE>(video2), &size) != 0)
        return 0;
    size = 0x20;
    if (RegQueryValueExA(key, "szModemManufacturer", nullptr, &type, reinterpret_cast<LPBYTE>(modemMfg), &size) != 0)
        return 0;
    size = 0x20;
    if (RegQueryValueExA(key, "szModemDescrip", nullptr, &type, reinterpret_cast<LPBYTE>(modemModel), &size) != 0)
        return 0;
    size = 2;
    if (RegQueryValueExA(key, "totalRAMInMB", nullptr, &type, reinterpret_cast<LPBYTE>(&totalRamMB), &size) != 0)
        return 0;
    size = 2;
    if (RegQueryValueExA(key, "largestPartitionInMB", nullptr, &type, reinterpret_cast<LPBYTE>(&largestDiskMB), &size) != 0)
        return 0;
    size = 4;
    r = RegQueryValueExA(key, "timeZoneBias", nullptr, &type, reinterpret_cast<LPBYTE>(&timeZoneBias), &size);
    return r == 0;
}
HOOK_METHOD(0x0043f240, CSysInfo, ReadRegistry);

// 0x0043f440  size 0x19a
// Creates the key (KEY_ALL_ACCESS) and stores every field: REG_BINARY for the numbers, REG_SZ for the strings.
int __thiscall CSysInfo::WriteRegistry()
{
    HKEY key;
    DWORD disposition;
    DWORD r;

    if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, HWPROFILE_KEY, 0, const_cast<LPSTR>(""), 0, KEY_ALL_ACCESS, nullptr, &key,
                        &disposition) != 0)
        return 0;

    if (RegSetValueExA(key, "processorType", 0, REG_BINARY, &cpuClass, 1) != 0)
        return 0;
    if (RegSetValueExA(key, "processorClock", 0, REG_BINARY, reinterpret_cast<const BYTE *>(&clockMHz), 2) != 0)
        return 0;
    if (RegSetValueExA(key, "nProcessors", 0, REG_BINARY, &processors, 1) != 0)
        return 0;
    if (RegSetValueExA(key, "szVideoCardDescrip1", 0, REG_SZ, reinterpret_cast<const BYTE *>(video1), 0x20) != 0)
        return 0;
    if (RegSetValueExA(key, "szVideoCardDescrip2", 0, REG_SZ, reinterpret_cast<const BYTE *>(video2), 0x20) != 0)
        return 0;
    if (RegSetValueExA(key, "szModemManufacturer", 0, REG_SZ, reinterpret_cast<const BYTE *>(modemMfg), 0x20) != 0)
        return 0;
    if (RegSetValueExA(key, "szModemDescrip", 0, REG_SZ, reinterpret_cast<const BYTE *>(modemModel), 0x20) != 0)
        return 0;
    if (RegSetValueExA(key, "totalRAMInMB", 0, REG_BINARY, reinterpret_cast<const BYTE *>(&totalRamMB), 2) != 0)
        return 0;
    if (RegSetValueExA(key, "largestPartitionInMB", 0, REG_BINARY, reinterpret_cast<const BYTE *>(&largestDiskMB), 2) != 0)
        return 0;
    r = RegSetValueExA(key, "timeZoneBias", 0, REG_BINARY, reinterpret_cast<const BYTE *>(&timeZoneBias), 4);
    return r == 0;
}
HOOK_METHOD(0x0043f440, CSysInfo, WriteRegistry);
