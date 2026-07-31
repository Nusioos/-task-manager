#pragma comment(lib, "psapi.lib")
#pragma once
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
//CHAANGE PALCE AFTER

inline void layout_rect(
    int x, int y, int w, int h,
    int start_index, int count,
    const std::vector<ProcInfo>& processes,
    double total_ram_in_slice,
    std::vector<std::vector<int>>& matrix);
BOOL Module_snap(DWORD& dwPID, DOUBLE& size_mb);
inline bool EnableDebugPrivilege()
{
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return false;

    LUID luid;//local unqiue identifier
    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
    {
        CloseHandle(hToken);
        return false;
    }

    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    bool ok = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
    CloseHandle(hToken);
    return ok && GetLastError() == ERROR_SUCCESS;
}
inline void placer(int beg_row, int beg_col, vector<vector<int>>& matrix, int identifier, int limit)
{

    for (int i = beg_row;i < beg_row + limit;i++)
    {
        for (int j = beg_col;j < beg_col + limit;j++)
        {
            matrix[i][j] = identifier;
        }
    }
}
inline bool is_ok_to_place(int beg_row, int beg_col, vector<vector<int>>& matrix, int limit)
{
    if (beg_row + limit > matrix.size() || beg_col + limit > matrix[0].size())
        return false;
    for (int i = beg_row;i < beg_row + limit;i++)
    {
        for (int j = beg_col;j < beg_col + limit;j++)
        {
            if (matrix[i][j] != -1)
                return false;
        }
    }
    return true;
}
inline void draw(const vector<vector<int>>& matrix)
{
    for (int i = 0;i < matrix.size();i++)
    {
        for (int j = 0;j < matrix[0].size();j++)
        {
            switch (matrix[i][j])
            {
            case 0:  cout << "\033[40;90m0 \033[0m"; break; // Czarne t這, szary tekst
            case 1:  cout << "\033[41;97m* \033[0m"; break; // Czerwone t這, bia造 tekst
            case 2:  cout << "\033[42;97m# \033[0m"; break; // Zielone t這, bia造 tekst
            case 3:  cout << "\033[43;30m@ \033[0m"; break; // 草速e t這, czarny tekst
            case 4:  cout << "\033[44;97m+ \033[0m"; break; // Niebieskie t這, bia造 tekst
            case 5:  cout << "\033[45;97m% \033[0m"; break; // Fioletowe t這, bia造 tekst
            case 6:  cout << "\033[46;30m& \033[0m"; break; // Cyjanowe t這, czarny tekst
            case 7:  cout << "\033[101;97mX \033[0m"; break; // Jasnoczerwone t這
            case 8:  cout << "\033[102;30mO \033[0m"; break; // Jasnozielone t這
            case 9:  cout << "\033[103;30m4 \033[0m"; break; // Jasno鄴速e t這
            case 10: cout << "\033[104;97m5 \033[0m"; break; // Jasnoniebieskie t這
            case 11: cout << "\033[105;97m2 \033[0m"; break; // Jasnofioletowe t這
            case 12: cout << "\033[106;30m1 \033[0m"; break; // Jasnocyjanowe t這
            case 13: cout << "\033[107;30m0 \033[0m"; break; // Bia貫 t這, czarny tekst
            case 14: cout << "\033[41;97m2 \033[0m"; break; // Powr鏒 do czerwonego
            case 15: cout << "\033[42;97m2 \033[0m"; break; // Powr鏒 do zielonego
            default:
                cout << "\033[37m? \033[0m";
                break;
            }

        }
        cout << "\033[0m" << string(30, ' ') << endl;
    }
}
inline void Create_listing(vector<ProcInfo>& processes, vector<vector<int>>& matrix,DOUBLE RAM_SIZE)
{
    for (int element = 0;element < processes.size();element++)
    {
        double box = ((processes[element].mb) / RAM_SIZE) * 100; // RAM size
        double square = sqrt(box * 100);
        int limit = static_cast<int>(round(square));
        if (limit < 1 && box > 1)
        {
            limit = 1;
        }
        limit = min(limit, (int)matrix.size());
        bool placed = false;

        for (int i = 0;i < matrix.size();i++)
        {
            for (int j = 0;j < matrix[0].size();j++)
            {
                if (matrix[i][j] == -1)
                {
                    bool place_manager = is_ok_to_place(i, j, matrix, limit);
                    if (place_manager)
                    {
                        placer(i, j, matrix, element+1, limit); //element+1 due to indexing starting from 0
                        placed = true;
                        break;
                    }

                }
            }
            if (placed) break;

        }
    }
}

inline void Create_listing_2(vector<ProcInfo>& processes, vector<vector<int>>& matrix, DOUBLE RAM_SIZE,int limit)
{
    int rows = matrix.size();
    int cols = matrix[0].size();
    double total_processes_ram = 0.0;
    limit = min(limit, (int)processes.size());  
    if (limit <= 0) return;
    for (int i = 0; i < limit; i++) {
        total_processes_ram += processes[i].mb;
    }
    layout_rect(0, 0, cols, rows, 0, limit, processes, total_processes_ram, matrix);
}
inline void Create_listing_with_boudneries(vector<ProcInfo>& processes, vector<vector<int>>& matrix, DOUBLE RAM_SIZE,int beg ,int limit)
{
    int rows = matrix.size();
    int cols = matrix[0].size();
    double total_processes_ram = 0.0;
    limit = min(limit, (int)processes.size());
    if (limit <= 0) return;
    int count = 0;
    for (int i = beg; i < limit; i++) {
        total_processes_ram += processes[i].mb;
        wstring w = processes[i].name;
        count++;
    }
    layout_rect(0, 0, cols, rows, beg, count, processes, total_processes_ram, matrix);
}
inline void output_apps(vector<ProcInfo>& processes, DOUBLE whole_size,DOUBLE RAM_SIZE)
{
    for (auto& i : processes)
    {
        wcout << i.name << L" zajal :" << i.mb << L" MB" << endl;
    }
    cout << "\n       whole RAM size   = " << whole_size << " MB" << endl;
    cout << "\n       whole RAM        = " << (whole_size / RAM_SIZE) * 100 << " %" << endl;
}

inline void debug(const vector<vector<int>>& matrix)
{
    float overall_size = matrix.size() * matrix[0].size();
    float sum_not_zero = 0;
    for (int i = 0;i < matrix.size();i++)
    {
        for (int j = 0;j < matrix[0].size();j++)
        {
            if (matrix[j][i] != 0)
                sum_not_zero++;
        }
    }
    float percent = (sum_not_zero / overall_size) * 100;
    cout << percent << " % krealnego pokrycia kwadratu" << endl;
    cout << "\n\n\n\n\n";
}
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
inline void layout_rect(int x, int y, int w, int h,int start_index, int count,const vector<ProcInfo>& processes,double total_ram_in_slice,vector<vector<int>>& matrix)
{

    if (count == 1) { //base case
        int process_id = start_index + 1; 
        for (int i = y; i < y + h; ++i) {
            for (int j = x; j < x + w; ++j) {
                if (i < matrix.size() && j < matrix[0].size()) {
                    matrix[i][j] = process_id;
                }
            }
        }
        return;
    }

    int half = count / 2;
    double left_ram = 0.0;
    for (int i = 0; i < half; ++i) {
        left_ram += processes[start_index + i].mb;
    }

  
    if (w >= h) {
        int split_w = static_cast<int>(round(w * (left_ram / total_ram_in_slice)));
        split_w = max(1, min(w - 1, split_w)); 

        layout_rect(x, y, split_w, h, start_index, half, processes, left_ram, matrix);
        layout_rect(x + split_w, y, w - split_w, h, start_index + half, count - half, processes, total_ram_in_slice - left_ram, matrix);
    }
    else {
        int split_h = static_cast<int>(round(h * (left_ram / total_ram_in_slice)));
        split_h = max(1, min(h - 1, split_h)); 
        layout_rect(x, y, w, split_h, start_index, half, processes, left_ram, matrix);
        layout_rect(x, y + split_h, w, h - split_h, start_index + half, count - half, processes, total_ram_in_slice - left_ram, matrix);
    }
}

