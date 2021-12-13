#pragma once

#include <vector>
#include "SampleBase.hpp"
#include "BasicMath.hpp"
#include "RenderStateArchive.hpp"

namespace Diligent
{

class OrderIndependentTransparencySample final : public SampleBase
{
public:
    ~OrderIndependentTransparencySample();

    virtual void Initialize(const SampleInitInfo& InitInfo) override final;

    virtual void Render() override final;

    virtual void Update(double CurrTime, double ElapsedTime) override final;

    virtual void WindowResize(Uint32 Width, Uint32 Height) override final;

    virtual const Char* GetSampleName() const override final { return "Order Independent Transparency"; }

private:
    void UpdateUI();

private:
    Int32 m_FragmentCount = 32;
    Int32 m_LayerCount    = 8;
    Int32 m_SampleCount   = 4;

    std::unique_ptr<RenderStateArchive> m_pArchive;

    RefCntAutoPtr<ITexture> m_pTextureColor;
    RefCntAutoPtr<ITexture> m_pTextureColor_MS;
    RefCntAutoPtr<ITexture> m_pTextureDepth_MS;
    RefCntAutoPtr<ITexture> m_pTextureHead;
    RefCntAutoPtr<IBuffer>  m_pBufferLinkedList;
    RefCntAutoPtr<IBuffer>  m_pBufferLinkedListCounter;
    
    RefCntAutoPtr<IShaderResourceBinding> m_pSRBGeometryOpaque;
    RefCntAutoPtr<IShaderResourceBinding> m_pSRBGeometryTransparent;
    RefCntAutoPtr<IShaderResourceBinding> m_pSRBGeometryResolve;

    RefCntAutoPtr<IPipelineState> m_pPSOGeometryOpaque;
    RefCntAutoPtr<IPipelineState> m_pPSOGeometryTransparent;
    RefCntAutoPtr<IPipelineState> m_pPSOGeometryResolve;
};

} // namespace Diligent