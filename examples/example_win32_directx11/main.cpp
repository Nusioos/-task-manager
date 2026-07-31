
// - Documentation https://dearimgui.com/docs (same as your local docs/ folder).
#include<iostream>
#define WIN32_LEAN_AND_MEAN
#define IMGUI_DEFINE_MATH_OPERATORS
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
#include "Windows_api_NET.hpp"
#include <codecvt>
#include <thread>
#include<mutex>
#include <memory>
#include"Image_load.hpp";
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
struct Pos
{
    ImVec2 pos;
    Pos() {
        pos.x = 0;
        pos.y = 0;
    }
};

//TEMPORARY
void DrawMergedMatrix_2(ImDrawList* draw_list, const ImVec2& origin, const vector<vector<int>>& matrix, vector<ProcInfo>& processes, float totalPhysMB, int which_matrix);
//END TEMP
ImU32 GetColorById(int id);
void collect_processes_time();
string WStringToString(const wstring& wstr);
void UpdateAndDrawUI_CPU_process(ImDrawList* draw_list, ImVec2 origin, ProcInfo process);
void UpdateAndDrawUI_RAM_process(ImDrawList* draw_list, ImVec2 origin, ProcInfo process);
void DrawSidePanel(vector<ProcInfo>& processes, float totalPhysMB, ImVec2 origin, ImDrawList* draw_list);
void UpdateAndDrawUI_NET(ImDrawList* draw_list, ImVec2 origin);
void UpdateAndDrawUI_CPU(ImDrawList* draw_list, ImVec2 origin);
void proces_icon_test(int index, vector<ProcInfo>& processes, ImVec2 icon_place, int w, int h);
void DrawMergedMatrix(ImDrawList* draw_list, const ImVec2& origin, const vector<vector<int>>& matrix, vector<ProcInfo>& processes, float totalPhysMB, int which_matrix);
void Draw_graph(ImDrawList* draw_list, const ImVec2& origin, DOUBLE size_x, DOUBLE size_y,
Pos place, int time_period, const Graph_buffer& buffer, DOUBLE time_now, int id);
void Draw_graph_pixels(ImDrawList* draw_list, const Graph_buffer& buffer, ImVec2 placing, ImVec2 size, DOUBLE time_now, DOUBLE time_period, int id);
ImVec2 Map_pixel_to_screen(DOUBLE x, DOUBLE y, DOUBLE x_min, DOUBLE x_max, DOUBLE y_min, DOUBLE y_max, ImVec2 placing, ImVec2 size);
void UpdateAndDrawUI_RAM(ImDrawList* draw_list, ImVec2 origin);
void FindMinMax(const Graph_buffer& buffer, DOUBLE time_min, DOUBLE time_max,
    float& out_min, float& out_max);
vector<ProcInfo> collect_processes(DOUBLE& whole_size);
bool LoadIconAsTexture(HICON hIcon, ID3D11Device* device, ID3D11ShaderResourceView** outSRV, int* outWidth, int* outHeight);
static Graph_buffer ram_buffer;
static Graph_buffer cpu_buffer;
static Graph_buffer net_buffer_dow;
static Graph_buffer net_buffer_upl;
static Graph_buffer ram_buffer_process;
static Graph_buffer cpu_buffer_process;

static DOUBLE current_time_NET = 0.0;
static DOUBLE current_time_RAM = 0.0;
static DOUBLE current_time_CPU = 0.0;
static DOUBLE current_time_RAM_PROCESS= 0.0;
static DOUBLE current_time_CPU_PROCESS = 0.0;

static DOUBLE last_sample_time_NET = 0.0;
static DOUBLE last_sample_time_RAM = 0.0;
static DOUBLE last_sample_time_CPU = 0.0;
static DOUBLE last_sample_time_RAM_process = 0.0;
static DOUBLE last_sample_time_CPU_process = 0.0;

static bool g_Dragging = false;
static POINT g_DragStartCursor;
static POINT g_DragStartWindow;

static CpuSample g_prevSample;
static bool g_haveSample = false;

static Network_sample g_prevNetSample;
static bool g_haveNetSample = false;

static int selected_process = -1;

static atomic<bool> g_threadrunning = true;
static thread g_collect;
static DOUBLE g_whole_size=0;
static mutex g_process_mutex;
static vector<ProcInfo> g_processes_shared;
static vector<vector<int>> g_matrix(100, vector<int>(100, -1));
static vector<vector<int>> g_matrix_under(15, vector<int>(88, -1));
static vector<vector<int>> g_matrix_under_small(15, vector<int>(12, -1));
static DOUBLE totalPhysMB_g = 0;

static mutex g_icon_mutex;
static unordered_map<wstring, ID3D11ShaderResourceView*> g_iconCache;
static unordered_map<wstring, ImVec2> g_iconSizeCache;

static ID3D11ShaderResourceView* g_srvDOW = nullptr;
static ID3D11ShaderResourceView* g_srvUPL = nullptr;
static ID3D11ShaderResourceView* g_srvRAM = nullptr;
static ID3D11ShaderResourceView* g_srvCPU = nullptr;

static float Refresh_time = 0.1;
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
    // - If fonts are not explicitly loaded, Dear ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    ImFont* fontSmall = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 12.0f);
    ImFont* fontmed = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 12.0f);
    //ImFont* fontMedium = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 24.0f);
    //ImFont* fontLarge = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 48.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
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

        DrawMergedMatrix(draw_list, Matrix_pos, matrix, processes, totalPhysMB,1);
        DrawMergedMatrix(draw_list, Matrix_pos_under, matrix_under_main, processes, totalPhysMB,2);
        DrawMergedMatrix_2(draw_list, Matrix_pos_under_small, matrix_under_main_small, processes, totalPhysMB, 3);
       //DRAW UI RAM
       UpdateAndDrawUI_RAM(draw_list, origin);
       //DRAW UI CPU
       UpdateAndDrawUI_CPU(draw_list, origin);
       //DRAW UI NET
       UpdateAndDrawUI_NET(draw_list, origin);
       //Side UI PANEL
        DrawSidePanel(processes, totalPhysMB,origin,draw_list);
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
ImU32 GetColorById(int id) {
    switch (id) {
    case 1:  return IM_COL32(30, 130, 30, 255);    // Medium-Dark Green
    case 2:  return IM_COL32(30, 80, 160, 255);    // Medium-Dark Blue
    case 3:  return IM_COL32(160, 40, 40, 255);    // Medium-Dark Red
    case 4:  return IM_COL32(150, 140, 30, 255);   // Dark Yellow / Gold
    case 5:  return IM_COL32(130, 40, 130, 255);   // Medium-Dark Magenta
    case 6:  return IM_COL32(170, 90, 20, 255);    // Dark Orange / Burnt Orange
    case 7:  return IM_COL32(90, 30, 120, 255);    // Medium-Dark Purple
    case 8:  return IM_COL32(160, 70, 100, 255);   // Dark Pink / Rose
    case 9:  return IM_COL32(110, 150, 30, 255);   // Muted Lime
    case 10: return IM_COL32(110, 60, 20, 255);    // Warm Brown
    case 11: return IM_COL32(160, 160, 160, 255);  // Soft White / Off-White
    case 12: return IM_COL32(90, 90, 90, 255);     // Mid Gray
    case 13: return IM_COL32(28, 28, 28, 255);     // Dark Charcoal
    case 14: return IM_COL32(255, 255, 255, 255);
    default: return IM_COL32(45, 45, 45, 255);     // Dark Gray
    }
}
void DrawMergedMatrix(ImDrawList* draw_list, const ImVec2& origin, const vector<vector<int>>& matrix,vector<ProcInfo>& processes, float totalPhysMB,int which_matrix)
{
    int rows = matrix.size();
    if (rows == 0) return;
    int cols = matrix[0].size();

    ImVec2 grid_max = ImVec2(origin.x + (cols * 6), origin.y + (rows * 5));
    draw_list->AddRectFilled(origin, grid_max, GetColorById(13));
    vector<vector<bool>> visited(rows, vector<bool>(cols, false));
    for (int j = 0; j < rows; j++) 
    {
        for (int i = 0; i < cols; i++)
        {
            if (visited[j][i]) continue;
            int color_idx = matrix[j][i];
            if (color_idx == 13 || color_idx == -1) {
                visited[j][i] = true;
                continue;
            }
            if (color_idx - 1 < 0 || color_idx - 1 >= (int)processes.size()) {
                visited[j][i] = true;
                continue;
            }
            int width = 1;
            int height = 1;
            while (i + width < cols && matrix[j][i + width] == color_idx) {
                width++;
            }
            while (j + height < rows) {
                bool full_row_matches = true;
                for (int k = 0; k < width; ++k) {
                    if (matrix[j + height][i + k] != color_idx) {
                        full_row_matches = false;
                        break;
                    }
                }
                if (!full_row_matches) {
                    break;
                }

                height++;
            }
            for (int dy = 0; dy < height; dy++) {
                for (int dx = 0; dx < width; dx++) {
                    visited[j + dy][i + dx] = true;
                }
            }

            ImVec2 square_min = ImVec2(origin.x + (i * 6) + 4.0f, origin.y + (j * 5) + 4.0f); //top-left
            ImVec2 square_max = ImVec2(origin.x + ((i + width) * 6), origin.y + ((j + height) * 5)); //down-right
            ImVec2 square_min_icons = ImVec2(origin.x + (i), origin.y + (j));
            float size_x_box = square_max.x - square_min.x;
            float size_y_box = square_max.y - square_min.y;
            ImU32 color = GetColorById(color_idx%13);
            draw_list->AddRectFilled(square_min, square_max, color,2);

                ImVec2 icon_place = ImVec2(square_min.x+ 0.15* size_x_box, square_min.y + 0.1 * size_y_box);
                int pick_size = min((int)size_x_box, (int)size_y_box);
                pick_size = max( pick_size/3, 32); //divide by 3 looks the best
                pick_size = max(1, pick_size); // watch out for 0
                ImVec2 text_place = ImVec2(square_min.x + 0.15 * size_x_box, square_min.y + (0.18 * size_y_box+ pick_size/3)+1);
                ImVec2 text_place_ram = ImVec2(square_min.x + 0.15 * size_x_box, square_min.y + (0.26 * size_y_box + pick_size / 3)+5);
                ImVec2 text_place_cpu = ImVec2(square_min.x + 0.15 * size_x_box, square_min.y + (0.34 * size_y_box + pick_size / 3) + 6);
               proces_icon_test(matrix[j][i]-1,processes, icon_place,pick_size/2, pick_size / 2); //matrix[j][i]-1 due to indexing starting from 0
               

               int font_size = pick_size / 3; // looks the best 
               int how_much_to_cut = size_x_box / (font_size/1.5); // dividing by 1.5 due to choosen font

               string app_name_string(processes[matrix[j][i] - 1].name.begin(), processes[matrix[j][i] - 1].name.end());
               app_name_string = app_name_string.substr(0, min((size_t)how_much_to_cut, app_name_string.length()));


               stringstream stream_RAM;
               stream_RAM << fixed << setprecision(2) << ((double)processes[matrix[j][i] - 1].mb / totalPhysMB) * 100.0  << " % RAM";
               string app_size_RAM_string = stream_RAM.str();
               app_size_RAM_string = app_size_RAM_string.substr(0, min((size_t)how_much_to_cut, app_size_RAM_string.length()));

               stringstream stream_CPU;
               stream_CPU << fixed << setprecision(2) << ((double)processes[matrix[j][i] - 1].cpu_usage) << " % CPU";
               string app_size_CPU_string = stream_CPU.str();
               app_size_CPU_string = app_size_CPU_string.substr(0, min((size_t)how_much_to_cut, app_size_CPU_string.length()));

               const char* app_name_charptr_CPU = app_size_CPU_string.c_str();
               const char* app_name_charptr_RAM = app_size_RAM_string.c_str();
               const char* app_name_charptr = app_name_string.c_str();
               

               draw_list->AddText(fontSmall, font_size, text_place, GetColorById(14), app_name_charptr);
               if (size_y_box > 40)
               draw_list->AddText(fontmed, font_size, text_place_ram, GetColorById(14), app_name_charptr_RAM);
               if(size_y_box> 80)
               draw_list->AddText(fontmed, font_size, text_place_cpu, GetColorById(14), app_name_charptr_CPU);

               ImGui::SetCursorScreenPos(square_min);
               ImGui::PushID(i * 10000 + j+ which_matrix);

               bool clicked = ImGui::InvisibleButton("##treemap_cell", ImVec2(size_x_box, size_y_box));
               bool hovered = ImGui::IsItemHovered();
               ImGui::PopID();

               if (clicked)
               {
                   selected_process = matrix[j][i] - 1;
                   cout << "Epsein land " << i * 10000 + j << endl;
               }
               if (hovered)
               {
                   ImGui::SetTooltip("%s", WStringToString(processes[matrix[j][i] - 1].name).c_str());
                   draw_list->AddRect(square_min, square_max, GetColorById(14), 2, 0, 4.0f);
               }
        }
    }
}
void DrawMergedMatrix_2(ImDrawList* draw_list, const ImVec2& origin, const vector<vector<int>>& matrix, vector<ProcInfo>& processes, float totalPhysMB, int which_matrix)
{
    int rows = matrix.size();
    if (rows == 0) return;
    int cols = matrix[0].size();

    ImVec2 grid_max = ImVec2(origin.x + (cols * 6), origin.y + (rows * 5));
    draw_list->AddRectFilled(origin, grid_max, GetColorById(13));
    vector<vector<bool>> visited(rows, vector<bool>(cols, false));
    for (int j = 0; j < rows; j++)
    {
        for (int i = 0; i < cols; i++)
        {
            if (visited[j][i]) continue;
            int color_idx = matrix[j][i];
            if (color_idx == 13 || color_idx == -1) {
                visited[j][i] = true;
                continue;
            }
            if (color_idx - 1 < 0 || color_idx - 1 >= (int)processes.size()) {
                visited[j][i] = true;
                continue;
            }
            int width = 1;
            int height = 1;
            while (i + width < cols && matrix[j][i + width] == color_idx) {
                width++;
            }
            while (j + height < rows) {
                bool full_row_matches = true;
                for (int k = 0; k < width; ++k) {
                    if (matrix[j + height][i + k] != color_idx) {
                        full_row_matches = false;
                        break;
                    }
                }
                if (!full_row_matches) {
                    break;
                }

                height++;
            }
            for (int dy = 0; dy < height; dy++) {
                for (int dx = 0; dx < width; dx++) {
                    visited[j + dy][i + dx] = true;
                }
            }

            ImVec2 square_min = ImVec2(origin.x + (i * 6) + 4.0f, origin.y + (j * 5) + 4.0f); //top-left
            ImVec2 square_max = ImVec2(origin.x + ((i + width) * 6), origin.y + ((j + height) * 5)); //down-right
            ImVec2 square_min_icons = ImVec2(origin.x + (i), origin.y + (j));
            float size_x_box = square_max.x - square_min.x;
            float size_y_box = square_max.y - square_min.y;
            ImU32 color = GetColorById(color_idx % 13);
            draw_list->AddRectFilled(square_min, square_max, color);


        }
    }
}
void Draw_graph(ImDrawList* draw_list, const ImVec2& origin, DOUBLE size_x,DOUBLE size_y, Pos place,int time_period, const Graph_buffer& buffer, DOUBLE time_now,int id)
{
    ImVec2 placing = ImVec2(origin.x + place.pos.x, origin.y + place.pos.y);
    ImVec2 size = ImVec2((float)(size_x * 2), (float)size_y);
    ImVec2 res = ImVec2(placing.x + size.x, placing.y + size.y);
    draw_list->AddRectFilled(placing, res, GetColorById(13),2.0f);
    int time_min = 0;
    int time_max = time_period;
    Draw_graph_pixels(draw_list, buffer, placing, size, time_now, (DOUBLE)time_period,id);

}
ImVec2 Map_pixel_to_screen(DOUBLE x, DOUBLE y, DOUBLE x_min, DOUBLE x_max, DOUBLE y_min, DOUBLE y_max, ImVec2 placing, ImVec2 size)
{
    double procent_x = (x - x_min) / (x_max - x_min);
    double offset_x = 10;
    double pixel_x = placing.x + procent_x * size.x - offset_x;

    double procent_y = (y - y_min) / (y_max - y_min);
    double usable_height = size.y * 0.75;
    double offset_y = (size.y - usable_height)/2;
    double pixel_y = placing.y + size.y - (offset_y) - (procent_y * usable_height);

    return ImVec2((float)pixel_x, (float)pixel_y);

}
void Draw_graph_pixels(ImDrawList* draw_list, const Graph_buffer& buffer,ImVec2 placing, ImVec2 size,DOUBLE time_now, DOUBLE time_period, int id)
{
    int count = buffer.Data.size();
    if (count < 2) return;
    DOUBLE time_min = time_now - time_period;
    DOUBLE time_max = time_now;

    float y_min, y_max;
    FindMinMax(buffer, time_min, time_max, y_min, y_max);

    for (int i = 0;i < count-1;i++)
    {
        int idx_A = (buffer.Offset + i) % count;
        int idx_B = (buffer.Offset + i + 1) % count;

        ImVec2 point_A = buffer.Data[idx_A];
        ImVec2 point_B = buffer.Data[idx_B];

        ImVec2 pixel_A = Map_pixel_to_screen(point_A.x, point_A.y,time_min, time_max, y_min, y_max,placing, size);
        ImVec2 pixel_B = Map_pixel_to_screen(point_B.x, point_B.y,time_min, time_max, y_min, y_max,placing, size);

        draw_list->AddLine(pixel_A, pixel_B, GetColorById(id), 2.0f);

    }
}
void UpdateAndDrawUI_RAM(ImDrawList* draw_list, ImVec2 origin)
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
    Draw_graph(draw_list, origin+ImVec2(0,11), 70, 70, RAM_pos, 10, ram_buffer, current_time_RAM, 7);

    stringstream info;
    if (ram_percent > 0.01)
    {


        info << fixed << setprecision(2) << "RAM\n" << ram_percent << "%";
        string string_UI_info = info.str();
        //cout << string_UI_info << endl;
        const char* const_UI = string_UI_info.c_str();
        int font_size = 12;
        ImVec2 Ram_text = ImVec2(50+origin.x, 14+origin.y);
        draw_list->AddText(fontmed, font_size, Ram_text, GetColorById(14), const_UI);
    }


    //ADDING IMAGE RAM 
    //Add image test
    ID3D11ShaderResourceView* srv = nullptr;
    if (g_srvRAM)
    {
        ImVec2 offset_1 = ImVec2(origin.x+4, origin.y+4);
        ImVec2 offset_2 = ImVec2(44 + origin.x, origin.y + 50);
        draw_list->AddImage((ImTextureID)g_srvRAM, offset_1, offset_2);
    }
}
void UpdateAndDrawUI_CPU(ImDrawList* draw_list, ImVec2 origin)
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
    Draw_graph(draw_list, origin+ImVec2(45, 11), 75, 70, CPU_pos, 10, cpu_buffer, current_time_CPU, 4);
    stringstream info;
    if (cpu_percent > 0.01)
    {


        info << fixed << setprecision(2) << "CPU\n" << cpu_percent << "%";
        string string_UI_info = info.str();
        //cout << string_UI_info << endl;
        const char* const_UI = string_UI_info.c_str();
        int font_size = 12;
        ImVec2 CPU_TEXT = ImVec2(180+origin.x, 14+origin.y);
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
void UpdateAndDrawUI_NET(ImDrawList* draw_list, ImVec2 origin)
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
    Draw_graph(draw_list, origin+ImVec2(100,11), 75, 70, NET_pos_dow, 10, net_buffer_dow, current_time_NET, 6);
    Draw_graph(draw_list, origin + ImVec2(155, 11), 72, 70, NET_pos_upl, 10, net_buffer_upl, current_time_NET, 2);

    int font_size = 12;
    if (download_KBs > 0)
    {
        stringstream info;

        info << fixed << setprecision(2) << "Download\n" << download_KBs << " KBs";
        string string_UI_info = info.str();
      //  cout << string_UI_info << endl;
        const char* const_UI = string_UI_info.c_str();
       
        ImVec2 DOW_text = ImVec2(335+ origin.x, origin.y+14);
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
            handles[pe32.th32ProcessID] = Temp{  samp, first_info, name,sysTimeBefore };

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
bool LoadIconAsTexture(HICON hIcon, ID3D11Device* device, ID3D11ShaderResourceView** outSRV, int* outWidth, int* outHeight)
{
    //icons from bitmap
    ICONINFO iconInfo;
    if (!GetIconInfo(hIcon, &iconInfo))
        return false;

    BITMAP bmpColor;
    GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmpColor);
    int width = bmpColor.bmWidth;
    int height = bmpColor.bmHeight;

    // raw RGBA
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    vector<unsigned char> pixels(width * height * 4);

    HDC hdc = GetDC(nullptr);
    GetDIBits(hdc, iconInfo.hbmColor, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);
    ReleaseDC(nullptr, hdc);

    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);

    // BRGA->RGBA
    for (size_t i = 0; i < pixels.size(); i += 4)
        swap(pixels[i], pixels[i + 2]);

    // D3D11
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = pixels.data();
    subResource.SysMemPitch = width * 4;

    ID3D11Texture2D* texture = nullptr;
    if (FAILED(device->CreateTexture2D(&desc, &subResource, &texture)))
        return false;

    // SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    HRESULT hr = device->CreateShaderResourceView(texture, &srvDesc, outSRV);
    texture->Release();

    if (FAILED(hr))
        return false;

    outWidth = &width;
    outHeight = &height;
    return true;
}



void proces_icon_test(int index, vector<ProcInfo>& processes, ImVec2 icon_place, int w, int h)
{
    if (index < 0 || index >= (int)processes.size())
        return;
    ProcInfo& proc = processes[index];

    ID3D11ShaderResourceView* srv = nullptr;
    ImVec2 iconSize(55, 55);


    auto it = g_iconCache.find(proc.name);
    if (it != g_iconCache.end())
    {
        srv = it->second;
        iconSize = g_iconSizeCache[proc.name];
 
    }
    else if (!proc.exe_path.empty())
    {
        HICON hIcon = GetIconForFile(proc.exe_path);
        if (hIcon)
        {
            ID3D11ShaderResourceView* newSRV = nullptr;
            int outW = w, outH = h;
            if (LoadIconAsTexture(hIcon, g_pd3dDevice, &newSRV, &outW, &outH))
            {
                g_iconCache[proc.name] = newSRV;
                g_iconSizeCache[proc.name] = ImVec2((float)outW, (float)outH);
                srv = newSRV;
                iconSize = ImVec2((float)outW, (float)outH);
            }
            DestroyIcon(hIcon);
        }
    }

    if (srv)
    {
        ImVec2 p2 = ImVec2(icon_place.x + w, icon_place.y + h);
        ImGui::GetWindowDrawList()->AddImage((ImTextureID)srv, icon_place, p2);
    }
}




void UpdateAndDrawUI_RAM_process(ImDrawList* draw_list, ImVec2 origin,ProcInfo process)
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
    RAM_pos.pos.y = 80;

    ImVec2 window_size = ImGui::GetWindowSize();
    Draw_graph(draw_list, origin, 80, 30, RAM_pos, 10, ram_buffer_process, current_time_RAM_PROCESS, 12);
}

void UpdateAndDrawUI_CPU_process(ImDrawList* draw_list, ImVec2 origin, ProcInfo process)
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
    RAM_pos.pos.y = 140;

    ImVec2 window_size = ImGui::GetWindowSize();
    Draw_graph(draw_list, origin, 80, 30, RAM_pos, 10, cpu_buffer_process, current_time_CPU_PROCESS, 4);
}


void DrawSidePanel(vector<ProcInfo>& processes, float totalPhysMB,ImVec2 origin, ImDrawList* draw_list)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
    float panel_width = 250.0f;
    ImVec2 widget_pos = ImVec2(origin.x+610, origin.y+12);
    ImGui::SetNextWindowPos(widget_pos);
    ImGui::SetNextWindowSize(ImVec2(180, 648));

    ImGui::BeginChild("##side_panel", ImVec2(250, 0), true);

    if (selected_process >= 0 && selected_process < (int)processes.size())
    {
       
        ProcInfo& p = processes[selected_process];
        int index = (int)selected_process;
        ImVec2 icon_pos = ImVec2(widget_pos.x + 5, widget_pos.y );
        proces_icon_test(index, processes, icon_pos, 35, 35);
        ImGui::Text("\n");
        ImGui::Text("\n");
        ImGui::Text("Proces: %s", WStringToString(p.name).c_str());
        ImGui::Separator();
        ImGui::Text("RAM: %.2f %%", ((double)p.mb / totalPhysMB) * 100.0);
        ImGui::Text("\n");
        ImGui::Text("\n");
        ImGui::Text("\n");
        ImGui::Text("CPU: %.2f %%", p.cpu_usage);
        UpdateAndDrawUI_RAM_process(draw_list, widget_pos, p);
        UpdateAndDrawUI_CPU_process(draw_list, widget_pos, p);
    }
    else
    {
        ImGui::TextDisabled("Click to see detatils");
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}
string WStringToString(const wstring& wstr) {
    using convert_type = codecvt_utf8<wchar_t>;
    wstring_convert<convert_type, wchar_t> converter;
    return converter.to_bytes(wstr);
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
            int cap_max = min(60, g_processes_shared.size());
            Create_listing_with_boudneries(g_processes_shared, g_matrix_under_small, totalPhysMB_g, 27, cap_max);
        }
        this_thread::sleep_for(chrono::seconds(1));
    }
}
