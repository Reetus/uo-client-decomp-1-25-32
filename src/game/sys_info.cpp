// CSysInfo::Collect. 0x0043ef00.
#include "sys_info.h"

#include <windows.h>

#include <cstdio>
#include <cstring>

#include "../runtime/ported.h"
#include "crt.h"
#include "cpu_speed_qpc_ports.h"
#include "globals.h"


// 0x0043ef00  size 0x33f
// The Modem key walk runs once: the loop test is `idx <= 0` after one increment.
// The two Display sub-keys are read independently. Registry handles are not
// closed, as in the original.
int __thiscall CSysInfo::Collect()
{
    SYSTEM_INFO si;
    TIME_ZONE_INFORMATION tz;
    MEMORYSTATUS ms;
    HKEY hClass;
    HKEY hSub;
    HKEY hLeaf;
    DWORD cb;
    int idx;
    char name[0x10];
    char root[0x100];

    GetSystemInfo(&si);
    switch (si.dwProcessorType) {
    case 0x182:
        cpuClass = 1;
        break;
    case 0x1e6:
        cpuClass = 2;
        break;
    case 0x24a:
        cpuClass = 3;
        break;
    default:
        cpuClass = 4;
        break;
    }

    clockMHz = (uint16_t)GetCpuSpeedMHz(-1);   // CPU clock probe
    if (clockMHz > 0x2ee)
        clockMHz = 0;
    processors = (uint8_t)si.dwNumberOfProcessors;

    DWORD tzResult = GetTimeZoneInformation(&tz);
    if (tzResult == 0xffffffffu || tzResult == 0)
        return 0;
    timeZoneBias = tz.Bias;
    g_localTimezone = tz.Bias / 60;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\Class\\", 0, KEY_QUERY_VALUE, &hClass) == 0) {
        modemMfg[0] = 0;
        modemModel[0] = 0;
        if (RegOpenKeyExA(hClass, "Modem", 0, KEY_QUERY_VALUE, &hSub) == 0) {
            idx = 0;
            do {
                sprintf(name, "%04d", idx);
                if (RegOpenKeyExA(hSub, name, 0, KEY_QUERY_VALUE, &hLeaf) != 0)
                    break;
                cb = 0x20;
                if (RegQueryValueExA(hLeaf, "Manufacturer", nullptr, nullptr, reinterpret_cast<LPBYTE>(modemMfg), &cb) != 0)
                    break;
                cb = 0x20;
                if (RegQueryValueExA(hLeaf, "Model", nullptr, nullptr, reinterpret_cast<LPBYTE>(modemModel), &cb) != 0)
                    break;
                idx = idx + 1;
            } while (idx <= 0);
        }

        video1[0] = 0;
        video2[0] = 0;
        if (RegOpenKeyExA(hClass, "Display", 0, KEY_QUERY_VALUE, &hSub) == 0) {
            if (RegOpenKeyExA(hSub, "0000", 0, KEY_QUERY_VALUE, &hLeaf) == 0) {
                cb = 0x20;
                RegQueryValueExA(hLeaf, "DriverDesc", nullptr, nullptr, reinterpret_cast<LPBYTE>(video1), &cb);
            }
            if (RegOpenKeyExA(hSub, "0001", 0, KEY_QUERY_VALUE, &hLeaf) == 0) {
                cb = 0x20;
                RegQueryValueExA(hLeaf, "DriverDesc", nullptr, nullptr, reinterpret_cast<LPBYTE>(video2), &cb);
            }
        }
    }

    GlobalMemoryStatus(&ms);
    totalRamMB = (uint16_t)(ms.dwTotalPhys >> 20);
    largestDiskMB = 0;

    DWORD len = GetLogicalDriveStringsA(0, nullptr);
    char *buf = static_cast<char *>(::operator new(len + 1));
    GetLogicalDriveStringsA(len, buf);
    {
        char *p = buf;
        if (*p != 0) {
            do {
                if (GetDriveTypeA(p) == DRIVE_FIXED) {
                    DWORD spc, bps, freeClusters, totalClusters;
                    unsigned int mb;
                    root[0] = *p;
                    root[1] = 0;
                    strcat(root, ":\\");
                    if (GetDiskFreeSpaceA(root, &spc, &bps, &freeClusters, &totalClusters) == 0) {
                        mb = 0;
                    } else {
                        // 2^-20 is the float constant at 0x0049f2cc
                        mb = (unsigned int)(int)((double)totalClusters * ((double)spc * ((double)bps * 9.5367431640625e-07f)));
                    }
                    if (mb >= largestDiskMB)
                        largestDiskMB = (uint16_t)mb;
                }
                while (*p != 0)
                    p = p + 1;
                p = p + 1;
            } while (*p != 0);
        }
    }
    operator delete(buf);
    return 1;
}
HOOK_METHOD(0x0043ef00, CSysInfo, Collect);
