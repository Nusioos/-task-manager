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
#include <shellapi.h>
#include <cmath>
#include <algorithm>
#include <numeric>
#define WIN32_LEAN_AND_MEAN
using namespace std;

struct ProcInfo {
    wstring name;
    DOUBLE mb = 0.0;
    DOUBLE cpu_usage = 0.0;
    wstring exe_path;  
};

void layout_rect(int x, int y, int w, int h,int start_index, int count,const std::vector<ProcInfo>& processes,double total_ram_in_slice,std::vector<std::vector<int>>& matrix);
BOOL Module_snap(DWORD& dwPID, DOUBLE& size_mb);
DOUBLE RAM_USAGE_WHOLE_WINDOWS();
void debug(const vector<vector<int>>& matrix);
void output_apps(vector<ProcInfo>& processes, DOUBLE whole_size, DOUBLE RAM_SIZE);
void Create_listing_with_boudneries(vector<ProcInfo>& processes, vector<vector<int>>& matrix, DOUBLE RAM_SIZE, int beg, int limit);
void Create_listing_2(vector<ProcInfo>& processes, vector<vector<int>>& matrix, DOUBLE RAM_SIZE, int limit);
void Create_listing(vector<ProcInfo>& processes, vector<vector<int>>& matrix, DOUBLE RAM_SIZE);
void draw(const vector<vector<int>>& matrix);
bool is_ok_to_place(int beg_row, int beg_col, vector<vector<int>>& matrix, int limit);
void placer(int beg_row, int beg_col, vector<vector<int>>& matrix, int identifier, int limit);
bool EnableDebugPrivilege();
