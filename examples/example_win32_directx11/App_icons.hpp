#pragma once
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <shellapi.h>

using namespace std;

 inline wstring GetProcessPath(HANDLE hProcess)
{
    wstring path;
    if (hProcess)
    {
        wchar_t buffer[MAX_PATH];
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProcess, 0, buffer, &size)) //get name of the image 
        {
            path = buffer;
        }
        //CloseHandle(hProcess);
    }
    //wcout << path << "<-- path\n";
    return path;
}
 inline HICON GetIconForFile(const wstring& path)
 {
     if (path.empty()) return nullptr;
     SHFILEINFOW sfi = {};
     if (SHGetFileInfoW(path.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON))
         return sfi.hIcon;
     return nullptr;
 }
 inline HICON Get_icon(HANDLE hProcess)
{
    wstring exePath = GetProcessPath(hProcess);
    if (!exePath.empty())
    {
        HICON hIcon = GetIconForFile(exePath);
        if (hIcon)
        {
            return hIcon;
        }
    }
   // CloseHandle(hProcess);
    return nullptr;
}

