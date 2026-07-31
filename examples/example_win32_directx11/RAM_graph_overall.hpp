#pragma once
#pragma comment(lib, "psapi.lib")
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <psapi.h>
#include <stdio.h>
#include<vector>
#include<algorithm>
#include<map>

using namespace std;

inline DOUBLE RAM_USAGE_WHOLE_WINDOWS()
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    DOUBLE totalPhysMB = memInfo.ullTotalPhys / (1024.0 * 1024.0);
    DOUBLE usedPhysMB = totalPhysMB - (memInfo.ullAvailPhys / (1024.0 * 1024.0));
    DOUBLE usedPhysMB_percent = (usedPhysMB / totalPhysMB) * 100;
    return usedPhysMB_percent;
}
