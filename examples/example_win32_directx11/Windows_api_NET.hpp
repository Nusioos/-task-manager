#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
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
inline bool Get_network_info(Network_sample& sample)
{
    PMIB_IF_TABLE2 ifTable = nullptr; //ULONG NumEntries;    MIB_IF_ROW2 Table[ANYSIZE_ARRAY];

    if (GetIfTable2(&ifTable) != 0)
    {
        return false;
    }
    sample.in_octet = 0;
    sample.out_octet = 0;

    for (ULONG i = 0; i < ifTable->NumEntries ;i++)
    {
        MIB_IF_ROW2& row = ifTable->Table[i];

        //Skip all software itnerefaces
        if (row.InterfaceAndOperStatusFlags.FilterInterface)
            continue;
        if (row.OperStatus != IfOperStatusUp)
            continue;
        if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK)
            continue;

        sample.in_octet += row.InOctets;
        sample.out_octet += row.OutOctets;
    }
    FreeMibTable(ifTable);
    return true;
}
inline void Computer_Network_usage(const Network_sample& s1, const Network_sample& s2, DOUBLE time, DOUBLE& out_download_KBs, DOUBLE& out_upload_KBs)
{
    if (time <= 0)
    {
        out_download_KBs = 0;
        out_upload_KBs = 0;
        return;
    }
    //underflow protection
    ULONG64 delta_in = max(s2.in_octet, s1.in_octet) - s1.in_octet;
    ULONG64 delta_out = max(s2.out_octet, s1.out_octet) - s1.out_octet;

    out_download_KBs = ((DOUBLE)delta_in / 1024) / time;
    out_upload_KBs = ((DOUBLE)delta_out / 1024) / time;
}

