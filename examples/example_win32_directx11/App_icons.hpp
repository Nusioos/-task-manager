#pragma once
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <shellapi.h>
#include"Windows_api_RAM.hpp"
#include"imgui.h"
#include <d3d11.h>
#include<unordered_map>
using namespace std;

static unordered_map<wstring, ID3D11ShaderResourceView*> g_iconCache;
static unordered_map<wstring, ImVec2> g_iconSizeCache;

wstring GetProcessPath(HANDLE hProcess);
HICON GetIconForFile(const wstring& path);
HICON Get_icon(HANDLE hProcess);
void proces_icon_test(int index, vector<ProcInfo>& processes, ImVec2 icon_place, int w, int h, ID3D11Device* g_pd3dDevice);
bool LoadIconAsTexture(HICON hIcon, ID3D11Device* device, ID3D11ShaderResourceView** outSRV, int* outWidth, int* outHeight);
