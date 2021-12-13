#include "OrderIndependentTransparencySample.hpp"
#include "imgui.h"
#include "MapHelper.hpp"

namespace Diligent
{

struct ListNode
{
    Uint32 Next;
    Uint32 Color;
    Uint32 Depth;
    Uint32 Coverage;
};


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

    std::vector<std::string> DRSNFiles{"RenderStates/RenderStatesLibrary.drsn"};

    RefCntAutoPtr<IShaderSourceInputStreamFactory> pStreamShaderFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory("Shaders", &pStreamShaderFactory);
    VERIFY_EXPR(pStreamShaderFactory != nullptr);

    RefCntAutoPtr<IShaderSourceInputStreamFactory> pStreamRenderStatesFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory("RenderStates", &pStreamRenderStatesFactory);
    VERIFY_EXPR(pStreamRenderStatesFactory != nullptr);

    m_pPSOArchive = std::make_unique<RenderStateArchive>(m_pDevice, pStreamShaderFactory, pStreamRenderStatesFactory, DRSNFiles);
    m_pSRBArchive = std::make_unique<ShaderResourceBindingArchive>();

    m_pSRBArchive->CreateShaderResourceBinding("GeometryOpaque", m_pPSOArchive->GetPipelineState("GeometryOpaque"));
    m_pSRBArchive->CreateShaderResourceBinding("GeometryTransparent", m_pPSOArchive->GetPipelineState("GeometryTransparent"));
    m_pSRBArchive->CreateShaderResourceBinding("GeometryResolve", m_pPSOArchive->GetPipelineState("GeometryResolve"));
    m_pSRBArchive->CreateShaderResourceBinding("BlitTexture", m_pPSOArchive->GetPipelineState("BlitTexture"));
    m_pSRBArchive->CreateShaderResourceBinding("ClearUnorderedAccessViewUint", m_pPSOArchive->GetPipelineState("ClearUnorderedAccessViewUint"));
    m_pSRBArchive->CreateShaderResourceBinding("ClearBufferCounter", m_pPSOArchive->GetPipelineState("ClearBufferCounter"));
}

void OrderIndependentTransparencySample::Render()
{
    auto CmdClearUnorderedAccessViewUint = [&](IDeviceContext* pDeviceContext, ITextureView* pUAV) {
        VERIFY_EXPR(pDeviceContext != nullptr);
        VERIFY_EXPR(pUAV != nullptr);

        auto const ThreadGroupsX = static_cast<Uint32>(std::ceil(pUAV->GetTexture()->GetDesc().Width / 8.0f));
        auto const ThreadGroupsY = static_cast<Uint32>(std::ceil(pUAV->GetTexture()->GetDesc().Height / 8.0f));

        auto* pPSO = m_pPSOArchive->GetPipelineState("ClearUnorderedAccessViewUint");
        auto* pSRB = m_pSRBArchive->GetShaderResourceBinding("ClearUnorderedAccessViewUint");

        pDeviceContext->SetPipelineState(pPSO);

        pSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "TextureUAV")->Set(pUAV);

        pDeviceContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pDeviceContext->DispatchCompute(DispatchComputeAttribs{ThreadGroupsX, ThreadGroupsY});
    };

    auto CmdClearBufferCounter = [&](IDeviceContext* pDeviceContext, IBufferView* pUAV) {
        VERIFY_EXPR(pDeviceContext != nullptr);
        VERIFY_EXPR(pUAV != nullptr);

        auto* pPSO = m_pPSOArchive->GetPipelineState("ClearBufferCounter");
        auto* pSRB = m_pSRBArchive->GetShaderResourceBinding("ClearBufferCounter");

        pDeviceContext->SetPipelineState(pPSO);

        pSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "BufferUAV")->Set(pUAV);

        pDeviceContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pDeviceContext->DispatchCompute(DispatchComputeAttribs{1, 1});
    };

    auto CmdBlitTexture = [&](IDeviceContext* pDeviceContext, ITextureView* pTextureSrc, ITextureView* pTextureDst) {
        VERIFY_EXPR(pDeviceContext != nullptr);
        VERIFY_EXPR(pTextureSrc != nullptr);
        VERIFY_EXPR(pTextureDst != nullptr);

        auto* pPSO = m_pPSOArchive->GetPipelineState("BlitTexture");
        auto* pSRB = m_pSRBArchive->GetShaderResourceBinding("BlitTexture");

        pDeviceContext->SetRenderTargets(1, &pTextureDst, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pDeviceContext->SetPipelineState(pPSO);

        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "TextureSRV")->Set(pTextureSrc);

        pDeviceContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pDeviceContext->Draw(DrawAttribs{3, DRAW_FLAG_VERIFY_ALL});
    };

    {
        const float ClearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};

        auto* pRTV = m_pTextureColor_MS->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
        auto* pDSV = m_pTextureDepth_MS->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
        m_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        CmdClearUnorderedAccessViewUint(m_pImmediateContext, m_pTextureHead->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS));
        CmdClearBufferCounter(m_pImmediateContext, m_pBufferLinkedListCounter->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS));
    }

    {
        auto* pPSO = m_pPSOArchive->GetPipelineState("GeometryOpaque");
        auto* pRTV = m_pTextureColor_MS->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
        auto* pDSV = m_pTextureDepth_MS->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
        m_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->SetPipelineState(pPSO);
        m_pImmediateContext->Draw(DrawAttribs{3, DRAW_FLAG_VERIFY_ALL, 5});
    }

    {
        auto* pPSO = m_pPSOArchive->GetPipelineState("GeometryTransparent");
        auto* pSRB = m_pSRBArchive->GetShaderResourceBinding("GeometryTransparent");
        auto* pDSV = m_pTextureDepth_MS->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);

        m_pImmediateContext->SetRenderTargets(0, nullptr, pDSV, RESOURCE_STATE_TRANSITION_MODE_NONE);
        m_pImmediateContext->SetPipelineState(pPSO);

        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "TextureHead")->Set(m_pTextureHead->GetDefaultView(TEXTURE_VIEW_UNORDERED_ACCESS));
        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "BufferLinkedList")->Set(m_pBufferLinkedList->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS));
        pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "BufferLinkedListCounter")->Set(m_pBufferLinkedListCounter->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS));

        m_pImmediateContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->Draw(DrawAttribs{3, DRAW_FLAG_NONE, 5});
    }

    {
        ResolveTextureSubresourceAttribs ResolveAttribs;
        ResolveAttribs.SrcTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        ResolveAttribs.DstTextureTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        m_pImmediateContext->ResolveTextureSubresource(m_pTextureColor_MS, m_pTextureColor, ResolveAttribs);

        auto const ThreadGroupsX = static_cast<Uint32>(std::ceil(m_pTextureColor_MS->GetDesc().Width / 8.0f));
        auto const ThreadGroupsY = static_cast<Uint32>(std::ceil(m_pTextureColor_MS->GetDesc().Height / 8.0f));

        auto* pPSO = m_pPSOArchive->GetPipelineState("GeometryResolve");
        auto* pSRB = m_pSRBArchive->GetShaderResourceBinding("GeometryResolve");

        m_pImmediateContext->SetPipelineState(pPSO);

        pSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "TextureColor")->Set(m_pTextureColorUAV_Linear);
        pSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "TextureHead")->Set(m_pTextureHead->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
        pSRB->GetVariableByName(SHADER_TYPE_COMPUTE, "BufferLinkedList")->Set(m_pBufferLinkedList->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));

        m_pImmediateContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->DispatchCompute(DispatchComputeAttribs{ThreadGroupsX, ThreadGroupsY});
    }

    CmdBlitTexture(m_pImmediateContext, m_pTextureColorSRV_Gamma, m_pSwapChain->GetCurrentBackBufferRTV());
}

void OrderIndependentTransparencySample::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);

    UpdateUI();
}

void OrderIndependentTransparencySample::WindowResize(Uint32 Width, Uint32 Height)
{
    SampleBase::WindowResize(Width, Height);

    m_pTextureColorSRV_Gamma.Release();
    m_pTextureColorUAV_Linear.Release();

    m_pTextureColor.Release();
    m_pTextureColor_MS.Release();
    m_pTextureDepth_MS.Release();
    m_pTextureHead.Release();
    m_pBufferLinkedList.Release();
    m_pBufferLinkedListCounter.Release();

    {
        TextureDesc TextureCI{"TextureColor", RESOURCE_DIM_TEX_2D, Width, Height, 1, TEX_FORMAT_RGBA8_TYPELESS};
        TextureCI.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
        m_pDevice->CreateTexture(TextureCI, nullptr, &m_pTextureColor);

        m_pTextureColor->CreateView(TextureViewDesc{"TextureColor-SRV", TEXTURE_VIEW_SHADER_RESOURCE, RESOURCE_DIM_TEX_2D, TEX_FORMAT_RGBA8_UNORM_SRGB}, &m_pTextureColorSRV_Gamma);
        m_pTextureColor->CreateView(TextureViewDesc{"TextureColor-UAV", TEXTURE_VIEW_UNORDERED_ACCESS, RESOURCE_DIM_TEX_2D, TEX_FORMAT_RGBA8_UNORM}, &m_pTextureColorUAV_Linear);
    }

    {
        TextureDesc TextureCI{"TextureColor-MS", RESOURCE_DIM_TEX_2D, Width, Height, 1, m_pSwapChain->GetCurrentBackBufferRTV()->GetDesc().Format};
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
        BufferDesc BufferCI{"BufferLinkedList", sizeof(ListNode) * Width * Height * m_LayerCount, BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS};
        BufferCI.Mode              = BUFFER_MODE_STRUCTURED;
        BufferCI.ElementByteStride = sizeof(ListNode);
        m_pDevice->CreateBuffer(BufferCI, nullptr, &m_pBufferLinkedList);
    }

    {
        BufferDesc BufferCI{"BufferLinkedListCounter", sizeof(uint32_t), BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS};
        BufferCI.Mode              = BUFFER_MODE_STRUCTURED;
        BufferCI.ElementByteStride = sizeof(uint32_t);
        m_pDevice->CreateBuffer(BufferCI, nullptr, &m_pBufferLinkedListCounter);
    }
}

void OrderIndependentTransparencySample::UpdateUI()
{

    ImGui::Begin("Settings");
    ImGui::SliderInt("Layer Count", &m_LayerCount, 1, 32);

    const Char* ComboItems[] = {"x1", "x2", "x4", "x8"};

    const Char* CurrentItem = ComboItems[static_cast<Uint32>(log2<Uint32>(static_cast<Uint32>(m_SampleCount)))];

    if (ImGui::BeginCombo("##SampleCount", CurrentItem))
    {
        for (Int32 ElementID = 0; ElementID < IM_ARRAYSIZE(ComboItems); ElementID++)
        {
            bool IsSelected = (CurrentItem == ComboItems[ElementID]);
            if (ImGui::Selectable(ComboItems[ElementID], IsSelected))
                m_SampleCount = static_cast<SAMPLE_COUNT>(ElementID);

            if (IsSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::End();
}

} // namespace Diligent
