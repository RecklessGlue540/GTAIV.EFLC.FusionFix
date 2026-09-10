module;

#include <common.hxx>
#include <wrl/client.h>

export module consolegamma;

import common;
import comvars;
import d3dx9_43;
import settings;

using Microsoft::WRL::ComPtr;

#define IDR_VS_BlitXenonGamma_Dither 134
#define IDR_PS_BlitXenonGamma_Dither 135

#define IDR_VS_BlitCellGamma_Dither 136
#define IDR_PS_BlitCellGamma_Dither 137

class ConsoleGamma
{
private:
    static inline ComPtr<IDirect3DVertexShader9> VS_BlitXenonGamma_Dither, VS_BlitCellGamma_Dither;
    static inline ComPtr<IDirect3DPixelShader9> PS_BlitXenonGamma_Dither, PS_BlitCellGamma_Dither;
    static inline ComPtr<IDirect3DVertexShader9> g_vertexShader;
    static inline ComPtr<IDirect3DPixelShader9> g_pixelShader;

    static inline rage::grcRenderTargetPC* pSceneRT = nullptr;
    static inline ComPtr<IDirect3DSurface9> pSceneSurf;

    static inline UINT g_width = 0, g_height = 0;
    static inline bool g_initialized = false;

    static ComPtr<IDirect3DSurface9> GetRealBackBuffer(IDirect3DDevice9* device)
    {
        ComPtr<IDirect3DSurface9> backBuffer;
        if (!device)
            return backBuffer;

        ComPtr<IDirect3DSwapChain9> swapChain;
        if (SUCCEEDED(device->GetSwapChain(0, &swapChain)) && swapChain)
            swapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);

        return backBuffer;
    }

    static const DWORD* LoadCompiledShaderResource(HMODULE hModule, int resourceId)
    {
        HRSRC hRes = FindResourceW(hModule, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
        if (!hRes)
            return nullptr;

        HGLOBAL hGlob = LoadResource(hModule, hRes);
        if (!hGlob)
            return nullptr;

        return reinterpret_cast<const DWORD*>(LockResource(hGlob));
    }

    static void SelectShaders(int ConsoleGamma)
    {
        if (ConsoleGamma == 1)
        {
            g_vertexShader = VS_BlitXenonGamma_Dither;
            g_pixelShader = PS_BlitXenonGamma_Dither;
        }
        else if (ConsoleGamma == 2)
        {
            g_vertexShader = VS_BlitCellGamma_Dither;
            g_pixelShader = PS_BlitCellGamma_Dither;
        }
        else
        {
            g_vertexShader = nullptr;
            g_pixelShader = nullptr;
        }
    }

    static void __fastcall OnDeviceLost()
    {
        pSceneSurf.Reset();

        if (pSceneRT)
        {
            pSceneRT->Destroy();

            pSceneRT = nullptr;
        }
    }

    static void __fastcall OnDeviceReset()
    {
        auto* device = rage::grcDevice::GetD3DDevice();
        if (!device)
            return;

        auto backBuffer = GetRealBackBuffer(device);
        if (!backBuffer)
            return;

        D3DSURFACE_DESC backBufferDesc{};
        backBuffer->GetDesc(&backBufferDesc);

        g_width = backBufferDesc.Width;
        g_height = backBufferDesc.Height;

        pSceneSurf.Reset();
        if (pSceneRT)
        {
            pSceneRT->Destroy();

            pSceneRT = nullptr;
        }

        rage::grcRenderTargetDesc renderTargetDesc{};
        renderTargetDesc.mMultisampleCount = 0;
        renderTargetDesc.field_0 = 1;
        renderTargetDesc.field_12 = 1;
        renderTargetDesc.mDepthRT = nullptr;
        renderTargetDesc.field_8 = 1;
        renderTargetDesc.field_10 = 1;
        renderTargetDesc.field_11 = 1;
        renderTargetDesc.field_24 = false;
        renderTargetDesc.mFormat = rage::getEngineTextureFormat(backBufferDesc.Format);

        auto* renderTarget = rage::grcTextureFactory::GetInstance()->CreateRenderTarget("ConsoleGammaScene", 3, g_width, g_height, 32, &renderTargetDesc);

        rage::grcDevice::grcResolveFlags resolveFlags{};
        rage::grcTextureFactoryPC::GetInstance()->LockRenderTarget(0, renderTarget, nullptr);
        rage::grcTextureFactoryPC::GetInstance()->UnlockRenderTarget(0, &resolveFlags);

        pSceneRT = renderTarget;
        if (pSceneRT && pSceneRT->mD3DTexture)
            pSceneRT->mD3DTexture->GetSurfaceLevel(0, &pSceneSurf);
    }

    static bool Initialize(IDirect3DDevice9* device)
    {
        if (g_initialized || !device)
            return g_initialized;

        static bool deviceCallbacksRegistered = false;
        if (!deviceCallbacksRegistered)
        {
            auto onDeviceLostCB = rage::grcDevice::Functor0(nullptr, OnDeviceLost, nullptr, 0);
            auto onDeviceResetCB = rage::grcDevice::Functor0(nullptr, OnDeviceReset, nullptr, 0);

            rage::grcDevice::RegisterDeviceCallbacks(onDeviceLostCB, onDeviceResetCB);

            deviceCallbacksRegistered = true;
        }

        static auto ConsoleGamma = FusionFixSettings.GetRef("PREF_CONSOLE_GAMMA");
        if (ConsoleGamma->get() != 1 && ConsoleGamma->get() != 2)
            return false;

        HMODULE hModule = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&Initialize, &hModule);

        auto LoadVertexShader = [&](int id, ComPtr<IDirect3DVertexShader9>& shader) -> bool
        {
            if (shader)
                return true;

            const DWORD* vsData = LoadCompiledShaderResource(hModule, id);

            return vsData && SUCCEEDED(device->CreateVertexShader(vsData, &shader));
        };

        auto LoadPixelShader = [&](int id, ComPtr<IDirect3DPixelShader9>& shader) -> bool
        {
            if (shader)
                return true;

            const DWORD* psData = LoadCompiledShaderResource(hModule, id);

            return psData && SUCCEEDED(device->CreatePixelShader(psData, &shader));
        };

        if (!LoadVertexShader(IDR_VS_BlitXenonGamma_Dither, VS_BlitXenonGamma_Dither) || !LoadPixelShader(IDR_PS_BlitXenonGamma_Dither, PS_BlitXenonGamma_Dither)
            || !LoadVertexShader(IDR_VS_BlitCellGamma_Dither, VS_BlitCellGamma_Dither) || !LoadPixelShader(IDR_PS_BlitCellGamma_Dither, PS_BlitCellGamma_Dither))
            return false;

        SelectShaders(ConsoleGamma->get());

        OnDeviceReset();

        g_initialized = true;
        return true;
    }

    static void ReloadShaders()
    {
        g_initialized = false;

        g_vertexShader.Reset();
        g_pixelShader.Reset();
    }

    static void Render(IDirect3DDevice9* device)
    {
        static auto ConsoleGamma = FusionFixSettings.GetRef("PREF_CONSOLE_GAMMA");
        if (!ConsoleGamma->get() || !device)
            return;

        if (!g_initialized && !Initialize(device))
            return;

        if (!pSceneRT || !pSceneRT->mD3DTexture || !pSceneSurf || !g_vertexShader || !g_pixelShader)
            return;

        auto backBuffer = GetRealBackBuffer(device);
        if (!backBuffer)
            return;

        ComPtr<IDirect3DSurface9> currentRenderTarget;
        ComPtr<IDirect3DSurface9> oldDepthStencil;

        ComPtr<IDirect3DVertexBuffer9> oldVertexBuffer;
        ComPtr<IDirect3DVertexDeclaration9> oldVertexDecl;

        UINT oldOffset = 0, oldStride = 0;
        DWORD oldFVF = 0;

        if (FAILED(device->GetRenderTarget(0, &currentRenderTarget)) || !currentRenderTarget)
            return;

        if (FAILED(device->StretchRect(currentRenderTarget.Get(), nullptr, pSceneSurf.Get(), nullptr, D3DTEXF_POINT)))
            return;

        device->GetDepthStencilSurface(&oldDepthStencil);

        device->GetStreamSource(0, &oldVertexBuffer, &oldOffset, &oldStride);
        device->GetVertexDeclaration(&oldVertexDecl);
        device->GetFVF(&oldFVF);

        device->SetRenderState(D3DRS_ZENABLE, FALSE);
        device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

        device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

        device->SetRenderTarget(0, backBuffer.Get());
        device->SetDepthStencilSurface(nullptr);

        device->SetStreamSource(0, nullptr, 0, 0);
        device->SetVertexDeclaration(nullptr);
        device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

        device->SetTexture(0, pSceneRT->mD3DTexture);
        device->SetVertexShader(g_vertexShader.Get());
        device->SetPixelShader(g_pixelShader.Get());

        struct ScreenVertex { float x, y, z, rhw, u, v; };
        ScreenVertex vertices[4] =
        {
            { -0.5f,                 -0.5f,                  0.0f, 1.0f, 0.0f, 0.0f },
            { -0.5f,                 (float)g_height - 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
            { (float)g_width - 0.5f, -0.5f,                  0.0f, 1.0f, 1.0f, 0.0f },
            { (float)g_width - 0.5f, (float)g_height - 0.5f, 0.0f, 1.0f, 1.0f, 1.0f },
        };

        device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(ScreenVertex));

        device->SetTexture(0, nullptr);
        device->SetVertexShader(nullptr);
        device->SetPixelShader(nullptr);

        device->SetRenderTarget(0, currentRenderTarget.Get());
        device->SetDepthStencilSurface(oldDepthStencil.Get());

        device->SetStreamSource(0, oldVertexBuffer.Get(), oldOffset, oldStride);
        device->SetVertexDeclaration(oldVertexDecl.Get());
        device->SetFVF(oldFVF);
    }

public:
    ConsoleGamma()
    {
        FusionFix::onInitEventAsync() += []()
        {
            FusionFixSettings.SetCallback("PREF_CONSOLE_GAMMA", [](int32_t)
            {
                ReloadShaders();
            });

            if (GetD3DX9_43DLL())
            {
                FusionFix::onEndScene() += []()
                {
                    ConsoleGamma::Render(rage::grcDevice::GetD3DDevice());
                };
            }
        };
    }
} ConsoleGamma;