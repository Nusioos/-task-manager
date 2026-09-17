#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 

#pragma comment(lib, "psapi.lib")

#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <psapi.h>
#include <shellapi.h>

#include <iostream>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <map>
#include <cmath>
#include <numeric>
#include <string>

using namespace std;

struct ProcInfo {
    wstring name;
    double mb = 0.0;
    double cpu_usage = 0.0;
    wstring exe_path;
    DWORD PID;
};

void layout_rect(int x, int y, int w, int h, int start_index, int count, const std::vector<ProcInfo>& processes, double total_ram_in_slice, std::vector<std::vector<int>>& matrix);
BOOL Module_snap(DWORD& dwPID, double& size_mb);
double RAM_USAGE_WHOLE_WINDOWS();
void debug(const std::vector<std::vector<int>>& matrix);
void output_apps(std::vector<ProcInfo>& processes, double whole_size, double RAM_SIZE);
void Create_listing_with_boudneries(std::vector<ProcInfo>& processes, std::vector<std::vector<int>>& matrix, double RAM_SIZE, int beg, int limit);
void Create_listing_2(std::vector<ProcInfo>& processes, std::vector<std::vector<int>>& matrix, double RAM_SIZE, int limit);
void Create_listing(std::vector<ProcInfo>& processes, std::vector<std::vector<int>>& matrix, double RAM_SIZE);
void draw(const std::vector<std::vector<int>>& matrix);
bool is_ok_to_place(int beg_row, int beg_col, std::vector<std::vector<int>>& matrix, int limit);
void placer(int beg_row, int beg_col, std::vector<std::vector<int>>& matrix, int identifier, int limit);
bool EnableDebugPrivilege();
