#pragma once

#define WIN32_LEAN_AND_MEAN
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>

#include <iostream>
#include <algorithm>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
using namespace std;

struct Network_sample
{
    ULONG64 in_octet = 0;
    ULONG64 out_octet = 0;
};
bool Get_network_info(Network_sample& sample);
void Computer_Network_usage(const Network_sample& s1, const Network_sample& s2, double time, double& out_download_KBs, double& out_upload_KBs);

