#include "OrderIndependentTransparencySample.hpp"
#include "imgui.h"

namespace Diligent
{

SampleBase* CreateSample()
{
    return new OrderIndependentTransparencySample();
}

OrderIndependentTransparencySample::~OrderIndependentTransparencySample()
{
}

void OrderIndependentTransparencySample::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    std::vector<std::string> DRSNFiles{
        "RenderStates/GeometryOpaque.drsn",
        "RenderStates/GeometryTransparent.drsn",
        "RenderStates/GeometryResolve.drsn",
        "RenderStates/GraphicsPrimitives.drsn"};

    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory("Shaders", &pShaderSourceFactory);
    m_pArchive = std::make_unique<RenderStateArchive>(m_pDevice, pShaderSourceFactory, DRSNFiles);

    m_pPSOGeometryOpaque      = m_pArchive->GetPipelineState("GeometryOpaque");
    m_pPSOGeometryTransparent = m_pArchive->GetPipelineState("GeometryTransparent");
    m_pPSOGeometryResolve     = m_pArchive->GetPipelineState("GeometryResolve");
}

void OrderIndependentTransparencySample::Render()
{
    auto CmdClearUnorderedAccessViewUint = [&](ITextureView* pUAV, Uint32 ClearColor) {
        pUAV->GetTexture()->GetDesc().Width;
        pUAV->GetTexture()->GetDesc().Height;

        auto* pPSO = m_pArchive->GetPipelineState("ClearUnorderedAccessViewUint");
        m_pImmediateContext->SetPipelineState(pPSO);
        m_pImmediateContext->CommitShaderResources(nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->DispatchCompute(DispatchComputeAttribs{1, 1});
    };

    auto CmdClearBufferCounter = [&](IBufferView* pTexture, Uint32 Value) {     
        auto* pPSO = m_pArchive->GetPipelineState("ClearBufferCounter");
        m_pImmediateContext->SetPipelineState(pPSO);
        m_pImmediateContext->CommitShaderResources(nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->DispatchCompute(DispatchComputeAttribs{1, 1});
    };

    auto CmdBlitTexture = [&](ITexture* pTextureSrc, ITexture* pTextureDst) {
       // auto* pSRV = pTextureSrc->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        auto* pRTV = pTextureDst->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
        auto* pPSO = m_pArchive->GetPipelineState("BlitTexture");
        m_pImmediateContext->SetRenderTargets(1, &pRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->SetPipelineState(pPSO);
        m_pImmediateContext->CommitShaderResources(nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->Draw(DrawAttribs{3, DRAW_FLAG_VERIFY_ALL});
    };

    auto const ThreadGroupsX = static_cast<Uint32>(std::ceil(m_pTextureColor_MS->GetDesc().Width / 8.0f));
    auto const ThreadGroupsY = static_cast<Uint32>(std::ceil(m_pTextureColor_MS->GetDesc().Height / 8.0f));

    {
        const float ClearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};

        auto* pRTV = m_pTextureColor_MS->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
        auto* pDSV = m_pTextureDepth_MS->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
        m_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    {
        auto* pRTV = m_pTextureColor_MS->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
        auto* pDSV = m_pTextureDepth_MS->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
        m_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->SetPipelineState(m_pPSOGeometryOpaque);
        m_pImmediateContext->Draw(DrawAttribs{3, DRAW_FLAG_VERIFY_ALL, 5});
    }

    {
        auto* pDSV = m_pTextureDepth_MS->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
        m_pImmediateContext->SetRenderTargets(0, nullptr, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->SetPipelineState(m_pPSOGeometryTransparent);

        CmdClearUnorderedAccessViewUint(m_pTextureHead->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS), 0xFFFFFFF);
        CmdClearBufferCounter(m_pBufferLinkedList->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS), 0);

        m_pSRBGeometryTransparent->GetVariableByName(SHADER_TYPE_PIXEL, "TextureHead")->Set(m_pTextureHead->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS));
        m_pSRBGeometryTransparent->GetVariableByName(SHADER_TYPE_PIXEL, "BufferLinkedList")->Set(m_pBufferLinkedList->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS));
        m_pSRBGeometryTransparent->GetVariableByName(SHADER_TYPE_PIXEL, "BufferLinkedListCounter")->Set(m_pBufferLinkedListCounter->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS));

        m_pImmediateContext->CommitShaderResources(m_pSRBGeometryTransparent, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->Draw(DrawAttribs{3, DRAW_FLAG_NONE, 5});
    }

    {
        ResolveTextureSubresourceAttribs ResolveAttribs;
        ResolveAttribs.SrcTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        ResolveAttribs.DstTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        m_pImmediateContext->ResolveTextureSubresource(m_pTextureColor_MS, m_pTextureColor, ResolveAttribs);

        m_pSRBGeometryResolve->GetVariableByName(SHADER_TYPE_COMPUTE, "TextureColor")->Set(m_pTextureColor->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS));
        m_pSRBGeometryResolve->GetVariableByName(SHADER_TYPE_COMPUTE, "TextureHead")->Set(m_pTextureHead->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        m_pSRBGeometryResolve->GetVariableByName(SHADER_TYPE_COMPUTE, "BufferLinkedList")->Set(m_pBufferLinkedList->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));

        m_pImmediateContext->SetPipelineState(m_pPSOGeometryResolve);
        m_pImmediateContext->CommitShaderResources(m_pSRBGeometryResolve, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->DispatchCompute(DispatchComputeAttribs{ThreadGroupsX, ThreadGroupsY});
    }

    CmdBlitTexture(m_pTextureColor, m_pSwapChain->GetCurrentBackBufferRTV()->GetTexture());
}

void OrderIndependentTransparencySample::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);

    UpdateUI();
}

void OrderIndependentTransparencySample::WindowResize(Uint32 Width, Uint32 Height)
{

    SampleBase::WindowResize(Width, Height);

    m_pTextureColor.Release();
    m_pTextureColor_MS.Release();
    m_pTextureDepth_MS.Release();
    m_pTextureHead.Release();
    m_pBufferLinkedList.Release();
    m_pBufferLinkedListCounter.Release();

    {
        auto pCurrentBackBuffer = m_pSwapChain->GetCurrentBackBufferRTV()->GetTexture();

        TextureDesc TextureCI{"TextureColor", RESOURCE_DIM_TEX_2D, Width, Height, 1, pCurrentBackBuffer->GetDesc().Format};
        TextureCI.BindFlags = BIND_UNORDERED_ACCESS | BIND_RENDER_TARGET;
        m_pDevice->CreateTexture(TextureCI, nullptr, &m_pTextureColor);
    }

    {
        auto pCurrentBackBuffer = m_pSwapChain->GetCurrentBackBufferRTV()->GetTexture();

        TextureDesc TextureCI{"TextureColor-MS", RESOURCE_DIM_TEX_2D, Width, Height, 1, pCurrentBackBuffer->GetDesc().Format};
        TextureCI.BindFlags   = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
        TextureCI.SampleCount = m_SampleCount;
        m_pDevice->CreateTexture(TextureCI, nullptr, &m_pTextureColor_MS);
    }

    {
        TextureDesc TextureCI{"TextureDepth-MS", RESOURCE_DIM_TEX_2D, Width, Height, 1, TEX_FORMAT_D32_FLOAT};
        TextureCI.BindFlags   = BIND_DEPTH_STENCIL;
        TextureCI.SampleCount = m_SampleCount;
        m_pDevice->CreateTexture(TextureCI, nullptr, &m_pTextureDepth_MS);
    }

    {
        TextureDesc TextureCI{"TextureHead", RESOURCE_DIM_TEX_2D, Width, Height, 1, TEX_FORMAT_R32_UINT};
        TextureCI.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
        m_pDevice->CreateTexture(TextureCI, nullptr, &m_pTextureHead);
    }

    {
        struct ListNode
        {
            uint32_t Next;
            uint32_t Color;
            uint32_t Depth;
            uint32_t Coverage;
        };

        BufferDesc BufferCI{"BufferLinkedList", sizeof(ListNode) * Width * Height * m_LayerCount, BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS};
        BufferCI.Mode              = BUFFER_MODE_STRUCTURED;
        BufferCI.ElementByteStride = sizeof(ListNode);
        m_pDevice->CreateBuffer(BufferCI, nullptr, &m_pBufferLinkedList);
    }

    {
        BufferDesc BufferCI{"BufferLinkedListCounter", sizeof(uint32_t), BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS};
        BufferCI.Mode = BUFFER_MODE_STRUCTURED;
        BufferCI.ElementByteStride = sizeof(uint32_t);
        m_pDevice->CreateBuffer(BufferCI, nullptr, &m_pBufferLinkedListCounter);
    }


    //m_pSRBGeometryOpaque.Release();
    //m_pPSOGeometryOpaque->CreateShaderResourceBinding(&m_pSRBGeometryOpaque);

    m_pSRBGeometryTransparent.Release();
    m_pPSOGeometryTransparent->CreateShaderResourceBinding(&m_pSRBGeometryTransparent);

    m_pSRBGeometryResolve.Release();
    m_pPSOGeometryResolve->CreateShaderResourceBinding(&m_pSRBGeometryResolve);
}

void OrderIndependentTransparencySample::UpdateUI()
{
    {
        ImGui::Begin("Settings");
        ImGui::SliderInt("Fragment Count", &m_FragmentCount, 1, 128);
        ImGui::SliderInt("Layer Count", &m_LayerCount, 1, 32);
        ImGui::SliderInt("MSAA Samples", &m_SampleCount, 1, 8);
        ImGui::End();
    }
}

} // namespace Diligent
