#pragma comment(lib, "psapi.lib")
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <psapi.h>
#include <stdio.h>
#include<vector>
#include<algorithm>
#include<map>
#include <shellapi.h>
#include <cmath>
#include <algorithm>
#include <numeric>


using namespace std;
struct CpuSample {
    FILETIME idle;
    FILETIME kernel;
    FILETIME user;
};
double Compute_System_Cpu_Usage(const CpuSample& s1, const CpuSample& s2);
double Compute_Cpu_Usage(const CpuSample& s1, const CpuSample& s2, const FILETIME& sysBefore, const FILETIME& sysAfter);
bool Get_process_CPU_info(HANDLE curr_process, CpuSample& sample);
bool Get_system_CPU_info(CpuSample& sample);
ULONGLONG FileTimeToQuad(const FILETIME& ft);
