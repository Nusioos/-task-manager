#include"App_icons.hpp"

using namespace std;



wstring GetProcessPath(HANDLE hProcess)
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
 HICON GetIconForFile(const wstring& path)
{
    if (path.empty()) return nullptr;
    SHFILEINFOW sfi = {};
    if (SHGetFileInfoW(path.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON))
        return sfi.hIcon;
    return nullptr;
}
 HICON Get_icon(HANDLE hProcess)
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
 void proces_icon_test(int index, vector<ProcInfo>& processes, ImVec2 icon_place, int w, int h, ID3D11Device* g_pd3dDevice)
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
