#include "Collect_processes.hpp"
#include <chrono>
#include <thread>
#include <algorithm>

using namespace std;

vector<ProcInfo> collect_processes(DOUBLE& whole_size)
{
    whole_size = 0;
    FILETIME sysTimeBefore;
    GetSystemTimeAsFileTime(&sysTimeBefore);

    HANDLE snap_shot_process_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap_shot_process_handle == INVALID_HANDLE_VALUE || snap_shot_process_handle == NULL)
    {
        cerr << "error process_handle" << endl;
        return {};
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(snap_shot_process_handle, &pe32))
    {
        CloseHandle(snap_shot_process_handle);
        return {};
    }

    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    DOUBLE totalPhysMB = memInfo.ullTotalPhys / (1024.0 * 1024.0);
    DOUBLE usedPhysMB = totalPhysMB - (memInfo.ullAvailPhys / (1024.0 * 1024.0));

    struct Temp { CpuSample samp = {}; bool have_sample = false; wstring process_name = L""; FILETIME time_stamp = {}; };
    static unordered_map<DWORD, Temp> prev_handles;
    static unordered_map<DWORD, Temp> handles;
    unordered_map<wstring, ProcInfo> merged;

    do
    {
        HANDLE hProcess_curr = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if (hProcess_curr == NULL)
        {
            cerr << "Process NULL" << endl;
            continue;
        }

        CpuSample samp{};
        bool first_info = Get_process_CPU_info(hProcess_curr, samp);
        auto prev_it = prev_handles.find(pe32.th32ProcessID);
        DOUBLE cpu_usage = 0.0;
        if (prev_it != prev_handles.end() && prev_it->second.have_sample && first_info)
        {
            cpu_usage = Compute_Cpu_Usage(prev_it->second.samp, samp, prev_it->second.time_stamp, sysTimeBefore);
        }

        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hProcess_curr, &pmc, sizeof(pmc)))
        {
            DOUBLE size_mb = static_cast<DOUBLE>((pmc.WorkingSetSize) / (1024.0 * 1024.0));
            wstring name = pe32.szExeFile;
            handles[pe32.th32ProcessID] = Temp{ samp, first_info, name, sysTimeBefore };

            wchar_t exePathBuf[MAX_PATH] = {};
            DWORD pathLen = MAX_PATH;
            wstring exePath = L"";
            if (QueryFullProcessImageNameW(hProcess_curr, 0, exePathBuf, &pathLen))
                exePath.assign(exePathBuf, pathLen);

            auto it = merged.find(name);
            if (it == merged.end())
            {
                //w
                ProcInfo p{};
                p.name = name;
                p.mb = size_mb;
                p.cpu_usage = 0.0;
                p.exe_path = exePath;
                p.PID = pe32.th32ProcessID;

                merged[name] = move(p);
            }
            else
            {
                it->second.mb += size_mb;
            }
            whole_size += size_mb;
            merged[name].cpu_usage += cpu_usage;
        }

        CloseHandle(hProcess_curr);

    } while (Process32Next(snap_shot_process_handle, &pe32));

    prev_handles = move(handles);
    handles.clear();

    vector<ProcInfo> processes;
    processes.reserve(merged.size() + 1);
    for (auto& el : merged)
        processes.push_back(move(el.second));

    CloseHandle(snap_shot_process_handle);
    return processes;
}


void collect_processes_time()
{
    while (g_threadrunning)
    {
        double whole = 0;
        vector<ProcInfo> local = collect_processes(whole);
        sort(local.begin(), local.end(), [](const ProcInfo& a, const ProcInfo& b) {
            return a.mb > b.mb;
            });

   
        vector<vector<int>> local_matrix(100, vector<int>(100, -1));
        vector<vector<int>> local_matrix_under(15, vector<int>(88, -1));
        vector<vector<int>> local_matrix_under_small(15, vector<int>(12, -1));

        Create_listing_2(local, local_matrix, totalPhysMB_g, 20);
        Create_listing_with_boudneries(local, local_matrix_under, totalPhysMB_g, 21, 26);
        int cap_max = min(60, (int)local.size());
        Create_listing_with_boudneries(local, local_matrix_under_small, totalPhysMB_g, 27, cap_max);


        {
            lock_guard<mutex> lock(g_process_mutex);
            g_processes_shared = std::move(local);
            g_whole_size = whole;
            g_matrix = std::move(local_matrix);
            g_matrix_under = std::move(local_matrix_under);
            g_matrix_under_small = std::move(local_matrix_under_small);
        }

        this_thread::sleep_for(chrono::seconds(1));
    }
}
