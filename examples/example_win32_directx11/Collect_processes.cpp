
#include"Collect_processes.hpp"
#include <chrono>
#include <thread>
#include <algorithm>


vector<ProcInfo> collect_processes(DOUBLE& whole_size)
{
    whole_size = 0;
    FILETIME sysTimeBefore;
    GetSystemTimeAsFileTime(&sysTimeBefore);
    HANDLE snap_shot_process_handle;
    HANDLE hProcess_curr;


    snap_shot_process_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap_shot_process_handle == 0)
    {
        cerr << "error process_handle" << endl;
        return{};
    }
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(snap_shot_process_handle, &pe32))
    {
        CloseHandle(snap_shot_process_handle);
        return{};
    }

    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    DOUBLE totalPhysMB = memInfo.ullTotalPhys / (1024.0 * 1024.0);
    DOUBLE usedPhysMB = totalPhysMB - (memInfo.ullAvailPhys / (1024.0 * 1024.0));

    struct Temp { CpuSample samp = {}; bool have_sample = false; wstring process_name = L"";FILETIME time_stamp = {}; };
    static unordered_map<DWORD, Temp> prev_handles;
    static unordered_map<DWORD, Temp> handles;
    unordered_map<wstring, ProcInfo> merged;
    do
    {
        hProcess_curr = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if (hProcess_curr == NULL)
        {
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
            handles[pe32.th32ProcessID] = Temp{ samp, first_info, name,sysTimeBefore };

            wchar_t exePathBuf[MAX_PATH] = {};
            DWORD pathLen = MAX_PATH;
            wstring exePath;
            if (QueryFullProcessImageNameW(hProcess_curr, 0, exePathBuf, &pathLen))
                exePath.assign(exePathBuf, pathLen);

            auto it = merged.find(name);
            if (it == merged.end())
            {
                merged[name] = ProcInfo{ name, size_mb, 0.0, exePath };
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

    DOUBLE protected_files = max(0.0, usedPhysMB - whole_size);
    // processes.push_back({ (wstring)(L"NIEDOSTEPNE"),protected_files,nullptr }); //Switch on if needed tracks apps that non admin user dont have access to
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

        {
           lock_guard<mutex> lock(g_process_mutex);
            g_processes_shared = move(local);
            g_whole_size = whole;
            Create_listing_2(g_processes_shared, g_matrix, totalPhysMB_g, 20);
            Create_listing_with_boudneries(g_processes_shared, g_matrix_under, totalPhysMB_g, 21, 26);
            int cap_max = min(60, (int)g_processes_shared.size());
            Create_listing_with_boudneries(g_processes_shared, g_matrix_under_small, totalPhysMB_g, 27, cap_max);
        }
         this_thread::sleep_for(chrono::seconds(1));
    }
}
