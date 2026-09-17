#define IMGUI_DEFINE_MATH_OPERATORS
#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <windows.h>
#include"imgui.h"
#include"Graphs.hpp"
#include"Colors.hpp"

using namespace std;

ImVec2 Map_pixel_to_screen(DOUBLE x, DOUBLE y, DOUBLE x_min, DOUBLE x_max, DOUBLE y_min, DOUBLE y_max, ImVec2 placing, ImVec2 size)
{
    double procent_x = (x - x_min) / (x_max - x_min);
    double offset_x = 10;
    double pixel_x = placing.x + procent_x * size.x - offset_x;

    double procent_y = (y - y_min) / (y_max - y_min);
    double usable_height = size.y * 0.75;
    double offset_y = (size.y - usable_height) / 2;
    double pixel_y = placing.y + size.y - (offset_y)-(procent_y * usable_height);

    return ImVec2((float)pixel_x, (float)pixel_y);

}
void Draw_graph_pixels(ImDrawList* draw_list, const Graph_buffer& buffer, ImVec2 placing, ImVec2 size, DOUBLE time_now, DOUBLE time_period, int id)
{
    int count = buffer.Data.size();
    if (count < 2) return;
    DOUBLE time_min = time_now - time_period;
    DOUBLE time_max = time_now;

    float y_min, y_max;
    FindMinMax(buffer, time_min, time_max, y_min, y_max);

    for (int i = 0;i < count - 1;i++)
    {
        int idx_A = (buffer.Offset + i) % count;
        int idx_B = (buffer.Offset + i + 1) % count;

        ImVec2 point_A = buffer.Data[idx_A];
        ImVec2 point_B = buffer.Data[idx_B];

        ImVec2 pixel_A = Map_pixel_to_screen(point_A.x, point_A.y, time_min, time_max, y_min, y_max, placing, size);
        ImVec2 pixel_B = Map_pixel_to_screen(point_B.x, point_B.y, time_min, time_max, y_min, y_max, placing, size);

        draw_list->AddLine(pixel_A, pixel_B, GetColorById(id), 2.0f);

    }
}
void FindMinMax(const Graph_buffer& buffer, DOUBLE time_min, DOUBLE time_max,
    float& out_min, float& out_max)
{
    int count = buffer.Data.size();
    out_min = FLT_MAX;
    out_max = -FLT_MAX;

    for (int i = 0; i < count; i++)
    {
        int idx = (buffer.Offset + i) % count;
        ImVec2 point = buffer.Data[idx];

        if (point.x < time_min) continue;
        if (point.y < out_min) out_min = point.y;
        if (point.y > out_max) out_max = point.y;
    }


    // if same value
    if (out_max - out_min < 1.0f)
    {
        out_min -= 0.5f;
        out_max += 0.5f;
    }
}
void Draw_graph(ImDrawList* draw_list, const ImVec2& origin, DOUBLE size_x, DOUBLE size_y, Pos place, int time_period, const Graph_buffer& buffer, DOUBLE time_now, int id)
{
    ImVec2 placing = ImVec2(origin.x + place.pos.x, origin.y + place.pos.y);
    ImVec2 size = ImVec2((float)(size_x * 2), (float)size_y);
    ImVec2 res = ImVec2(placing.x + size.x, placing.y + size.y);
    draw_list->AddRectFilled(placing, res, GetColorById(13), 2.0f);
    int time_min = 0;
    int time_max = time_period;
    Draw_graph_pixels(draw_list, buffer, placing, size, time_now, (DOUBLE)time_period, id);

}

void UpdateAndDrawUI_RAM(ImDrawList* draw_list, ImVec2 origin, ImFont* fontmed)
{
    //checks if user moved window and time was frozen
    static float ram_percent = 0;
    float dt = ImGui::GetIO().DeltaTime;
    if (dt > Refresh_time) dt = Refresh_time;
    current_time_RAM += dt;

    if (current_time_RAM - last_sample_time_RAM >= Refresh_time)
    {
        ram_percent = RAM_USAGE_WHOLE_WINDOWS();
        ram_buffer.add_point(current_time_RAM, ram_percent);
        last_sample_time_RAM = current_time_RAM;
    }
    Pos RAM_pos;
    RAM_pos.pos.x = 0;
    RAM_pos.pos.y = 1;

    ImVec2 window_size = ImGui::GetWindowSize();
    Draw_graph(draw_list, origin + ImVec2(0, 11), 70, 70, RAM_pos, 10, ram_buffer, current_time_RAM, 7);

    stringstream info;
    if (ram_percent > 0.01)
    {


        info << fixed << setprecision(2) << "RAM\n" << ram_percent << "%";
        string string_UI_info = info.str();
        //cout << string_UI_info << endl;
        const char* const_UI = string_UI_info.c_str();
        int font_size = 12;
        ImVec2 Ram_text = ImVec2(50 + origin.x, 14 + origin.y);
        draw_list->AddText(fontmed, font_size, Ram_text, GetColorById(14), const_UI);
    }


    //ADDING IMAGE RAM 
    //Add image test
    ID3D11ShaderResourceView* srv = nullptr;
    if (g_srvRAM)
    {
        ImVec2 offset_1 = ImVec2(origin.x + 4, origin.y + 4);
        ImVec2 offset_2 = ImVec2(44 + origin.x, origin.y + 50);
        draw_list->AddImage((ImTextureID)g_srvRAM, offset_1, offset_2);
    }
}
void UpdateAndDrawUI_CPU(ImDrawList* draw_list, ImVec2 origin, ImFont* fontmed)
{
    //checks if user moved window and time was frozen
    float dt = ImGui::GetIO().DeltaTime;
    if (dt > Refresh_time) dt = Refresh_time;
    current_time_CPU += dt;
    static DOUBLE cpu_percent = 0;
    CpuSample curr;
    if (current_time_CPU - last_sample_time_CPU >= Refresh_time)
    {
        if (Get_system_CPU_info(curr))
        {
            if (g_haveSample)
            {
                cpu_percent = Compute_System_Cpu_Usage(g_prevSample, curr);
                cpu_buffer.add_point(current_time_CPU, cpu_percent);
            }
            g_prevSample = curr;
            g_haveSample = true;
        }
        last_sample_time_CPU = current_time_CPU;
    }
    Pos CPU_pos;
    CPU_pos.pos.x = 100;
    CPU_pos.pos.y = 1;
    ImVec2 window_size = ImGui::GetWindowSize();
    Draw_graph(draw_list, origin + ImVec2(45, 11), 75, 70, CPU_pos, 10, cpu_buffer, current_time_CPU, 4);
    stringstream info;
    if (cpu_percent > 0.01)
    {


        info << fixed << setprecision(2) << "CPU\n" << cpu_percent << "%";
        string string_UI_info = info.str();
        //cout << string_UI_info << endl;
        const char* const_UI = string_UI_info.c_str();
        int font_size = 12;
        ImVec2 CPU_TEXT = ImVec2(180 + origin.x, 14 + origin.y);
        draw_list->AddText(fontmed, font_size, CPU_TEXT, GetColorById(14), const_UI);
    }
    //ADDING IMAGE CPU 
    ID3D11ShaderResourceView* srv = nullptr;
    //Add image test

    if (g_srvCPU)
    {
        ImVec2 offset_1 = ImVec2(origin.x + 145, origin.y + 15);
        ImVec2 offset_2 = ImVec2(175 + origin.x, origin.y + 40);
        draw_list->AddImage((ImTextureID)g_srvCPU, offset_1, offset_2);
    }


}
void UpdateAndDrawUI_NET(ImDrawList* draw_list, ImVec2 origin, ImFont* fontmed)
{
    //checks if user moved window and time was frozen
    float dt = ImGui::GetIO().DeltaTime;
    if (dt > Refresh_time) dt = Refresh_time;
    current_time_NET += dt;
    static DOUBLE download_KBs = 0, upload_KBs = 0;
    Network_sample curr_sample_net = {};
    if (current_time_NET - last_sample_time_NET >= Refresh_time)
    {
        if (Get_network_info(curr_sample_net))
        {
            if (g_haveNetSample)
            {
                download_KBs = 0, upload_KBs = 0;
                DOUBLE time = current_time_NET - last_sample_time_NET;
                Computer_Network_usage(g_prevNetSample, curr_sample_net, time, download_KBs, upload_KBs);

                net_buffer_dow.add_point(current_time_NET, (float)download_KBs);
                net_buffer_upl.add_point(current_time_NET, (float)upload_KBs);
            }
            g_haveNetSample = true;
            g_prevNetSample = curr_sample_net;
        }
        last_sample_time_NET = current_time_NET;
    }
    Pos NET_pos_dow;
    NET_pos_dow.pos.x = 200;
    NET_pos_dow.pos.y = 1;
    Pos NET_pos_upl;
    NET_pos_upl.pos.x = 300;
    NET_pos_upl.pos.y = 1;
    ImVec2 window_size = ImGui::GetWindowSize();
    Draw_graph(draw_list, origin + ImVec2(100, 11), 75, 70, NET_pos_dow, 10, net_buffer_dow, current_time_NET, 6);
    Draw_graph(draw_list, origin + ImVec2(155, 11), 72, 70, NET_pos_upl, 10, net_buffer_upl, current_time_NET, 2);

    int font_size = 12;
    if (download_KBs > 0)
    {
        stringstream info;

        info << fixed << setprecision(2) << "Download\n" << download_KBs << " KBs";
        string string_UI_info = info.str();
        //  cout << string_UI_info << endl;
        const char* const_UI = string_UI_info.c_str();

        ImVec2 DOW_text = ImVec2(335 + origin.x, origin.y + 14);
        draw_list->AddText(fontmed, font_size, DOW_text, GetColorById(14), const_UI);
    }
    else
    {
        ImVec2 pozdro = ImVec2(335 + origin.x, origin.y + 14);
        draw_list->AddText(fontmed, font_size, pozdro, GetColorById(14), "Downlaod\n 0.0 KBs");
    }

    if (upload_KBs > 0)
    {
        stringstream info;

        info << fixed << setprecision(2) << "Upload\n" << upload_KBs << " KBs";
        string string_UI_info = info.str();
        // cout << string_UI_info << endl;
        const char* const_UI = string_UI_info.c_str();
        ImVec2 pozdro = ImVec2(483 + origin.x, origin.y + 14);
        draw_list->AddText(fontmed, font_size, pozdro, GetColorById(14), const_UI);
    }
    else
    {
        ImVec2 pozdro = ImVec2(483 + origin.x, origin.y + 14);
        draw_list->AddText(fontmed, font_size, pozdro, GetColorById(14), "Upload\n 0.0 KBs");
    }

    //ADDING IMAGE NET 

    //Add image test
    if (g_srvDOW)
    {
        ImVec2 offset_1 = ImVec2(origin.x + 300, origin.y + 12);
        ImVec2 offset_2 = ImVec2(330 + origin.x, origin.y + 42);
        draw_list->AddImage((ImTextureID)g_srvDOW, offset_1, offset_2);
    }

    //Add image test

    if (g_srvUPL)
    {
        ImVec2 offset_11 = ImVec2(origin.x + 455, origin.y + 13);
        ImVec2 offset_22 = ImVec2(480 + origin.x, origin.y + 41);
        draw_list->AddImage((ImTextureID)g_srvUPL, offset_11, offset_22);
    }

}
void UpdateAndDrawUI_RAM_process(ImDrawList* draw_list, ImVec2 origin, const ProcInfo& process)
{
    //checks if user moved window and time was frozen
    static ProcInfo proc = process;
    static float ram_data_PROCESS = 0;
    float dt = ImGui::GetIO().DeltaTime;
    if (dt > Refresh_time) dt = Refresh_time;
    current_time_RAM_PROCESS += dt;

    if (process.name != proc.name)
    {
        ram_buffer_process.Data.clear();
        ram_buffer_process.Erase();
    }


    if (current_time_RAM_PROCESS - last_sample_time_RAM_process >= Refresh_time)
    {
        ram_data_PROCESS = process.mb;
        ram_buffer_process.add_point(current_time_RAM_PROCESS, ram_data_PROCESS);
        last_sample_time_RAM_process = current_time_RAM_PROCESS;

        proc = process;
    }
    Pos RAM_pos;
    RAM_pos.pos.x = 5;
    RAM_pos.pos.y = 100;

    ImVec2 window_size = ImGui::GetWindowSize();
    Draw_graph(draw_list, origin, 80, 30, RAM_pos, 10, ram_buffer_process, current_time_RAM_PROCESS, 12);
}
void UpdateAndDrawUI_CPU_process(ImDrawList* draw_list, ImVec2 origin, const ProcInfo& process)
{
    //checks if user moved window and time was frozen
    static ProcInfo proc = process;
    static float cpu_data_PROCESS = 0;
    float dt = ImGui::GetIO().DeltaTime;
    if (dt > Refresh_time) dt = Refresh_time;
    current_time_CPU_PROCESS += dt;

    if (process.name != proc.name)
    {
        cpu_buffer_process.Data.clear();
        cpu_buffer_process.Erase();
    }

    if (current_time_CPU_PROCESS - last_sample_time_CPU_process >= Refresh_time)
    {
        cpu_data_PROCESS = process.cpu_usage;
        cpu_buffer_process.add_point(current_time_CPU_PROCESS, cpu_data_PROCESS);
        last_sample_time_CPU_process = current_time_CPU_PROCESS;

        proc = process;
    }
    Pos RAM_pos;
    RAM_pos.pos.x = 5;
    RAM_pos.pos.y = 160;

    ImVec2 window_size = ImGui::GetWindowSize();
    Draw_graph(draw_list, origin, 80, 30, RAM_pos, 10, cpu_buffer_process, current_time_CPU_PROCESS, 4);
}

