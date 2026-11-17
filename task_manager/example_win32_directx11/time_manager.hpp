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


struct Clock
{
    int hours;
    int minutes;
    int seconds;
};
double creation_time_hprocess(DWORD PID);
Clock second_to_Clock(double time_in_seconds);
