#pragma once

#include <vector>
#include "SampleBase.hpp"
#include "BasicMath.hpp"
#include "RenderStateArchive.hpp"
#include "ShaderResourceBindingArchive.hpp"

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
    Int32        m_LayerCount    = 4;
    SAMPLE_COUNT m_SampleCount   = SAMPLE_COUNT_4;

    RefCntAutoPtr<ITexture> m_pTextureColor;
    RefCntAutoPtr<ITexture> m_pTextureColor_MS;
    RefCntAutoPtr<ITexture> m_pTextureDepth_MS;
    RefCntAutoPtr<ITexture> m_pTextureHead;
    RefCntAutoPtr<IBuffer>  m_pBufferLinkedList;
    RefCntAutoPtr<IBuffer>  m_pBufferLinkedListCounter;

    RefCntAutoPtr<ITextureView> m_pTextureColorUAV_Linear;
    RefCntAutoPtr<ITextureView> m_pTextureColorSRV_Gamma;

    std::unique_ptr<RenderStateArchive>           m_pPSOArchive;
    std::unique_ptr<ShaderResourceBindingArchive> m_pSRBArchive;
};

} // namespace Diligent