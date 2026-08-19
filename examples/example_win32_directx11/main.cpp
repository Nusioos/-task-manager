
// - Documentation https://dearimgui.com/docs (same as your local docs/ folder).
#include<iostream>
#define WIN32_LEAN_AND_MEAN
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "Colors.hpp"
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
#include "Windows_api_NET.hpp"
#include "Graphs.hpp"
#include <codecvt>
#include <thread>
#include<mutex>
#include <memory>
#include"Image_load.hpp"
#include"Collect_processes.hpp"
#include"Drawing_Panels_and_Matrix.hpp"
using namespace std;
// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
static ImFont* fontSmall = nullptr;
static ImFont* fontmed = nullptr;
// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code

//string WStringToString(const wstring& wstr);
//void DrawSidePanel(vector<ProcInfo>& processes, float totalPhysMB, ImVec2 origin, ImDrawList* draw_list);
//void DrawMergedMatrix(ImDrawList* draw_list, const ImVec2& origin, const vector<vector<int>>& matrix, vector<ProcInfo>& processes, float totalPhysMB, int which_matrix,bool show_elemetns);

static bool g_Dragging = false;
static POINT g_DragStartCursor;
static POINT g_DragStartWindow;

//static int selected_process = -1;

 vector<ProcInfo> g_processes_shared;
 vector<vector<int>> g_matrix(100, vector<int>(100, -1));
 vector<vector<int>> g_matrix_under(15, vector<int>(88, -1));
 vector<vector<int>> g_matrix_under_small(15, vector<int>(12, -1));
 DOUBLE totalPhysMB_g = 0;

 mutex g_icon_mutex;
 atomic<bool> g_threadrunning = true;
 thread g_collect;
 DOUBLE g_whole_size = 0;
 mutex g_process_mutex;

//static unordered_map<wstring, ID3D11ShaderResourceView*> g_iconCache;
//static unordered_map<wstring, ImVec2> g_iconSizeCache;


int main(int, char**)
{
    // Make process DPI aware and obtain main monitor scale
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Create application window
    int screen_w = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);//to work on both screens
    int screen_h = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int screen_x = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
    int screen_y = ::GetSystemMetrics(SM_YVIRTUALSCREEN);

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST,  
        wc.lpszClassName, L"Dear ImGui DirectX11 Example", WS_POPUP,
        0, 0, screen_w, screen_h, nullptr, nullptr, wc.hInstance, nullptr);
    ::SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_COLORKEY);
    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    float color_num = 16.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(color_num / 255.0f, color_num / 255.0f, color_num / 255.0f, 1.0f);
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load Fonts
    ImFont* fontSmall = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 12.0f);
    ImFont* fontmed = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 12.0f);
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);


    if (!EnableDebugPrivilege())
    {
        cerr << "Warning: ADMIN MODE IS OFF " << endl;
    }
   DOUBLE whole_size = 0;
    

    //Windnows API work start
   vector<ProcInfo> processes= collect_processes(whole_size);
    //Windnows API work end
    sort(processes.begin(), processes.end(), [](const ProcInfo& a, const ProcInfo& b)
        {
            return a.mb > b.mb;
        });
    vector<vector<int>> matrix(100, vector<int>(100, -1));
    vector<vector<int>> matrix_under_main(15, vector<int>(88, -1));
    vector<vector<int>> matrix_under_main_small(15, vector<int>(12, -1));

    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);

    DOUBLE totalPhysMB = memInfo.ullTotalPhys / (1024.0 * 1024.0);
    totalPhysMB_g = memInfo.ullTotalPhys / (1024.0 * 1024.0);
    DOUBLE usedPhysMB = totalPhysMB - (memInfo.ullAvailPhys / (1024.0 * 1024.0));

    Create_listing_2(processes, matrix, totalPhysMB,20);
    Create_listing_with_boudneries(processes, matrix_under_main, totalPhysMB,21,26);
    int cap_max = min(60, processes.size());
    Create_listing_with_boudneries(processes, matrix_under_main_small, totalPhysMB, 27, cap_max);


    output_apps(processes, usedPhysMB, totalPhysMB);

    int x_image = 0;
    int* x = &x_image;
    int y_image = 0;
    int* y = &y_image;
    
    LoadTextureWIC(L"DOW_comp.png", g_pd3dDevice, &g_srvDOW, x, y);
    LoadTextureWIC(L"RAM_comp.png", g_pd3dDevice, &g_srvRAM, x, y);
    LoadTextureWIC(L"CPU_comp.png", g_pd3dDevice, &g_srvCPU, x, y);
    LoadTextureWIC(L"UPL_comp.png", g_pd3dDevice, &g_srvUPL, x, y);
    
    debug(matrix);
    // Main loop
    bool done = false;
    bool app_open = true;
    g_collect = thread(collect_processes_time);
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_NoScrollbar;
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowSize(ImVec2(30.0f, 30.0f), ImGuiCond_FirstUseEver);

//START----------------------------------------------------------------------------------------------------------------------
        ImGui::Begin("MY_APP", &app_open, window_flags);
   
        ImGuiIO& io = ImGui::GetIO();

        // Moving window
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.WantCaptureMouse)
        {
            g_Dragging = true;
            ::GetCursorPos(&g_DragStartCursor);
            RECT rect;
            ::GetWindowRect(hwnd, &rect);
            g_DragStartWindow.x = rect.left;
            g_DragStartWindow.y = rect.top;
        }
        if (g_Dragging)
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                POINT cursor;
                ::GetCursorPos(&cursor);
                int dx = cursor.x - g_DragStartCursor.x;
                int dy = cursor.y - g_DragStartCursor.y;
                ::SetWindowPos(hwnd, nullptr,
                    g_DragStartWindow.x + dx, g_DragStartWindow.y + dy,
                    0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
            else
            {
                g_Dragging = false;
            }
        }

        {
            lock_guard<mutex> lock(g_process_mutex);
            processes = g_processes_shared;
            whole_size = g_whole_size;
            matrix = g_matrix;
            matrix_under_main = g_matrix_under;

        }
       // draw constructor
       ImDrawList* draw_list = ImGui::GetWindowDrawList();
       //screen pos
       ImVec2 origin = ImGui::GetCursorScreenPos();
       ImVec2 Matrix_pos = ImVec2(origin.x, origin.y+85);
       ImVec2 Matrix_pos_under = ImVec2(origin.x, origin.y + 585);
       ImVec2 Matrix_pos_under_small = ImVec2(origin.x+527, origin.y + 585);

        DrawMergedMatrix(draw_list, Matrix_pos, matrix, processes, totalPhysMB,1,true, g_pd3dDevice,fontmed);
        DrawMergedMatrix(draw_list, Matrix_pos_under, matrix_under_main, processes, totalPhysMB,2,true, g_pd3dDevice, fontmed);
        DrawMergedMatrix(draw_list, Matrix_pos_under_small, matrix_under_main_small, processes, totalPhysMB, 3, false, g_pd3dDevice, fontmed);
  
       //DRAW UI RAM
       UpdateAndDrawUI_RAM(draw_list, origin,fontmed);
       //DRAW UI CPU
       UpdateAndDrawUI_CPU(draw_list, origin, fontmed);
       //DRAW UI NET
       UpdateAndDrawUI_NET(draw_list, origin, fontmed);
       //Side UI PANEL
        DrawSidePanel(processes, totalPhysMB,origin,draw_list, g_pd3dDevice);
       //Prccess amount TEXT
        int fonst_size_text = 15;
        ImVec2 process_amount_text_pos = ImVec2(origin.x+5, origin.y + 660);

        stringstream stream_process_amount_text;
        stream_process_amount_text << fixed << setprecision(0) <<"Processes: " << processes.size();
        string process_amount_text_string = stream_process_amount_text.str();
        const char* process_amount_text_charptr = process_amount_text_string.c_str();

        draw_list->AddText(fontSmall, fonst_size_text, process_amount_text_pos, GetColorById(14), process_amount_text_charptr);
        //Refresh time Text
        ImVec2 refresh_time_text_pos = ImVec2(origin.x + 110, origin.y + 660);

        stringstream stream_refresh_time_text;
        stream_refresh_time_text << fixed << setprecision(1) << "Refersh time: " << Refresh_time << "s";
        string refresh_time_text_string = stream_refresh_time_text.str();
        const char* refresh_time_text_charptr = refresh_time_text_string.c_str();

        draw_list->AddText(fontSmall, fonst_size_text, refresh_time_text_pos, GetColorById(14), refresh_time_text_charptr);
      // if (g_iconSRV) g_iconSRV->Release();
       if (ImGui::IsKeyPressed(ImGuiKey_Escape))
           done = true;
        ImGui::End();
 //END--------------------------------------------------------------------------------------------------------------------------
      if (!app_open)
        done = true;
        // Rendering
        ImGui::Render();
       
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }
    if (g_collect.joinable())
    {
        g_threadrunning = false;
        g_collect.join();
    }
    for (auto& el : g_iconCache)
        if (el.second) el.second->Release();
    g_iconCache.clear();
    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    // This is a basic setup. Optimally could use e.g. DXGI_SWAP_EFFECT_FLIP_DISCARD and handle fullscreen mode differently. See #8979 for suggestions.
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}
void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}
void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}
void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
