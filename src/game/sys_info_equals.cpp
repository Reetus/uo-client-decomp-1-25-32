// 0x0045f500  size 0x182
// CSysInfo::Equals: compares a registry-cached system record with a freshly built 0xA4 packet record.
// Used by the server-list handler (0x004603e0) to decide whether the record has to be re-sent.
#include <cstdlib>
#include <cstring>

#include "../runtime/ported.h"
#include "sys_info.h"

// 1 when this record and `other` describe the same machine, else 0.
int __thiscall CSysInfo::Equals(const CSysInfo *other)
{
    int a = (clockMHz >> 8) & 0xff;
    int b = (other->clockMHz >> 8) & 0xff;
    int smaller = a;
    if (!(a < b))
        smaller = b;
    int diff = abs(a - b);
    // fild + fmul by 0.25 (the constant at 0x0049fc18) + __ftol; the product is exact in double.
    int limit = static_cast<int>(static_cast<double>(smaller) * 0.25);
    int close = diff <= limit;

    if (cpuClass != other->cpuClass || !close)
        return 0;
    if (totalRamMB != other->totalRamMB)
        return 0;
    if (largestDiskMB != other->largestDiskMB)
        return 0;
    if (timeZoneBias != other->timeZoneBias)
        return 0;
    if (strcmp(video1, other->video1) != 0)
        return 0;
    if (strcmp(video2, other->video2) != 0)
        return 0;
    if (strcmp(modemMfg, other->modemMfg) == 0 && strcmp(modemModel, other->modemModel) == 0)
        return 1;
    return 0;
}
HOOK_METHOD(0x0045f500, CSysInfo, Equals);
