#pragma once

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include "App_icons.hpp"
#include <shellapi.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>
#include "Windows_api_RAM.hpp"
#include "Windows_api_CPU.hpp"
#include <codecvt>
#include <thread>
#include<mutex>
#include <memory>

extern atomic<bool> g_threadrunning;
extern mutex g_process_mutex;
extern vector<ProcInfo> g_processes_shared;
extern vector<vector<int>> g_matrix;
extern vector<vector<int>> g_matrix_under;
extern vector<vector<int>> g_matrix_under_small;
extern DOUBLE g_whole_size;
extern DOUBLE totalPhysMB_g;

vector<ProcInfo> collect_processes(DOUBLE& whole_size);
void collect_processes_time();
