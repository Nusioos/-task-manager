#pragma once 
#include <d3d11.h>
#include <vector>
#include <wincodec.h>
#include <wrl/client.h>
#include <vector>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
using Microsoft::WRL::ComPtr;
using namespace std;
bool LoadTextureWIC(const wchar_t* filename, ID3D11Device* device,
    ID3D11ShaderResourceView** out_srv,
    int* out_width, int* out_height);

