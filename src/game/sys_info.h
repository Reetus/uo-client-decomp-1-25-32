// CSysInfo: the "sysinfo block" filled in at start-up (CPU, clock, time zone,
// modem and display adapter names from the registry, RAM, biggest fixed disk).
// Layout from decomp/pass2/0x0043ef00.cpp; object size and address unknown.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"

class CSysInfo {
public:
    // 0x0043ef00: collect the info; returns 0 when the time zone query fails, else 1.
    int __thiscall Collect();
    // 0x0043f240: read the block back from the HWProfile registry key under Origin Worlds Online; 1 when
    // every value was read, else 0. The key is not closed.
    int __thiscall ReadRegistry();
    // 0x0043f440: write the block to the same key (created when missing); 1 on success, else 0. The key is not closed.
    int __thiscall WriteRegistry();
    // 0x0045f500: RET 4. Same configuration as `other` (cpu class, clock high byte within a quarter of the smaller,
    // RAM, disk, time zone and the four description strings)? 1 when equal, else 0.
    int __thiscall Equals(const CSysInfo *other);

    uint8_t pad_00;
    uint8_t cpuClass;          // +0x01  1 = 0x182 (386), 2 = 0x1e6 (486), 3 = 0x24a (Pentium), 4 = other
    uint16_t clockMHz;         // +0x02  0 when above 0x2ee
    uint8_t processors;        // +0x04
    char video1[0x20];         // +0x05  display "0000" DriverDesc
    char video2[0x20];         // +0x25  display "0001" DriverDesc
    char modemMfg[0x20];       // +0x45
    char modemModel[0x20];     // +0x65
    uint8_t pad_85;
    uint16_t totalRamMB;       // +0x86
    uint16_t largestDiskMB;    // +0x88
    uint8_t pad_8a[2];
    int32_t timeZoneBias;      // +0x8c  minutes
};

static_assert(offsetof(CSysInfo, cpuClass) == 0x01, "CSysInfo::cpuClass");
static_assert(offsetof(CSysInfo, clockMHz) == 0x02, "CSysInfo::clockMHz");
static_assert(offsetof(CSysInfo, processors) == 0x04, "CSysInfo::processors");
static_assert(offsetof(CSysInfo, video1) == 0x05, "CSysInfo::video1");
static_assert(offsetof(CSysInfo, video2) == 0x25, "CSysInfo::video2");
static_assert(offsetof(CSysInfo, modemMfg) == 0x45, "CSysInfo::modemMfg");
static_assert(offsetof(CSysInfo, modemModel) == 0x65, "CSysInfo::modemModel");
static_assert(offsetof(CSysInfo, totalRamMB) == 0x86, "CSysInfo::totalRamMB");
static_assert(offsetof(CSysInfo, largestDiskMB) == 0x88, "CSysInfo::largestDiskMB");
static_assert(offsetof(CSysInfo, timeZoneBias) == 0x8c, "CSysInfo::timeZoneBias");
