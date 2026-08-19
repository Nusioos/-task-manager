#pragma once
#define WIN32_LEAN_AND_MEAN
#define IMGUI_DEFINE_MATH_OPERATORS
#include<iostream>
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include "App_icons.hpp"
#include <shellapi.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <unordered_map>
#include <algorithm>s
#include <string>
#include <sstream>
#include <iomanip>
#include "Windows_api_RAM.hpp"
#include "Windows_api_CPU.hpp"
#include"Image_load.hpp"
#include"Collect_processes.hpp"
#include "Colors.hpp"
#include "Graphs.hpp"
using namespace std;

static int selected_process = -1;


void DrawMergedMatrix(ImDrawList* draw_list, const ImVec2& origin, const vector<vector<int>>& matrix, vector<ProcInfo>& processes, float totalPhysMB, int which_matrix, bool show_elemetns, ID3D11Device* g_pd3dDevice, ImFont* fontmed);
void DrawSidePanel(vector<ProcInfo>& processes, float totalPhysMB, ImVec2 origin, ImDrawList* draw_list, ID3D11Device* g_pd3dDevice);
string WStringToString(const wstring& wstr);

