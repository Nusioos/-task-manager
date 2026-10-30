#pragma once
#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <windows.h>
#include"imgui.h"
#include <sstream>
#include <iomanip>
#include <d3d11.h>
#include "Windows_api_RAM.hpp"
#include "Windows_api_CPU.hpp"
#include "Windows_api_NET.hpp"
using namespace std;

struct Pos
{
    ImVec2 pos;
    Pos() {
        pos.x = 0;
        pos.y = 0;
    }
};
struct Graph_buffer
{
    int Max_size = 80; //chnges length of the graph
    int Offset = 0;
    ImVector<ImVec2> Data;

    Graph_buffer() { Data.reserve(Max_size); }

    void add_point(DOUBLE x, DOUBLE y)
    {
        if (Data.size() < Max_size)
            Data.push_back(ImVec2(x, y));
        else
        {
            Data[Offset] = ImVec2(x, y);
            Offset = (Offset + 1) % Max_size;
        }
    }
    void Erase()
    {
        Data.clear();
        Offset = 0;
    }
};

static Graph_buffer ram_buffer;
static Graph_buffer cpu_buffer;
static Graph_buffer net_buffer_dow;
static Graph_buffer net_buffer_upl;
static Graph_buffer ram_buffer_process;
static Graph_buffer cpu_buffer_process;

static DOUBLE current_time_NET = 0.0;
static DOUBLE current_time_RAM = 0.0;
static DOUBLE current_time_CPU = 0.0;
static DOUBLE current_time_RAM_PROCESS = 0.0;
static DOUBLE current_time_CPU_PROCESS = 0.0;

static DOUBLE last_sample_time_NET = 0.0;
static DOUBLE last_sample_time_RAM = 0.0;
static DOUBLE last_sample_time_CPU = 0.0;
static DOUBLE last_sample_time_RAM_process = 0.0;
static DOUBLE last_sample_time_CPU_process = 0.0;

extern ID3D11ShaderResourceView* g_srvDOW;
extern ID3D11ShaderResourceView* g_srvUPL;
extern ID3D11ShaderResourceView* g_srvRAM;
extern ID3D11ShaderResourceView* g_srvCPU;

static CpuSample g_prevSample;
static bool g_haveSample = false;

static Network_sample g_prevNetSample;
static bool g_haveNetSample = false;

static float Refresh_time = 0.1;


ImVec2 Map_pixel_to_screen(DOUBLE x, DOUBLE y, DOUBLE x_min, DOUBLE x_max, DOUBLE y_min, DOUBLE y_max, ImVec2 placing, ImVec2 size);

void FindMinMax(const Graph_buffer& buffer, DOUBLE time_min, DOUBLE time_max, float& out_min, float& out_max);
void Draw_graph_pixels(ImDrawList* draw_list, const Graph_buffer& buffer, ImVec2 placing, ImVec2 size, DOUBLE time_now, DOUBLE time_period, int id);
void Draw_graph(ImDrawList* draw_list, const ImVec2& origin, DOUBLE size_x, DOUBLE size_y, Pos place, int time_period, const Graph_buffer& buffer, DOUBLE time_now, int id);
void UpdateAndDrawUI_RAM(ImDrawList* draw_list, ImVec2 origin, ImFont* fontmed);
void UpdateAndDrawUI_CPU(ImDrawList* draw_list, ImVec2 origin, ImFont* fontmed);
void UpdateAndDrawUI_NET(ImDrawList* draw_list, ImVec2 origin, ImFont* fontmed);
void UpdateAndDrawUI_RAM_process(ImDrawList* draw_list, ImVec2 origin, const ProcInfo& process);
void UpdateAndDrawUI_CPU_process(ImDrawList* draw_list, ImVec2 origin, const ProcInfo& process);
