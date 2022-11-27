#pragma once

#include "SampleBase.hpp"
#include "BasicMath.hpp"
#include "RenderStateNotationLoader.h"
#include <memory>

namespace Diligent
{

	class RSNLoaderPipelineCache final
	{
	public:
		RSNLoaderPipelineCache(IRenderDevice* pRenderDevice, std::string_view RenderStatesPath, std::string_view ShadersPath, const std::vector<std::string_view>& RSNFileNames);

		IPipelineState* GetPipelineState(std::string_view Name, PIPELINE_TYPE Type = PIPELINE_TYPE_GRAPHICS);

	private:
		RefCntAutoPtr<IRenderStateNotationLoader> m_pRenderStateNotationLoader;

		struct PSOHasher
		{
			size_t operator()(const std::tuple<HashMapStringKey, PIPELINE_TYPE>& Key) const
			{
				return ComputeHash(std::get<0>(Key), std::get<1>(Key));
			}
		};

		using TNamedPSOHashMap = std::unordered_map<std::tuple<HashMapStringKey, PIPELINE_TYPE>, RefCntAutoPtr<IPipelineState>, PSOHasher>;

		TNamedPSOHashMap m_PSOHashMap;
	};

	class RSNLoaderSRBCache final
	{

	public:
		RSNLoaderSRBCache();

		IShaderResourceBinding* GetShaderResourceBinding(std::string_view Name, IPipelineState* pPSO);

	private:
		struct SRBHasher
		{
			size_t operator()(const std::tuple<HashMapStringKey, HashMapStringKey, PIPELINE_TYPE>& Key) const
			{
				return ComputeHash(std::get<0>(Key), std::get<1>(Key), std::get<2>(Key));
			}
		};

		using TNamedSRBHashMap = std::unordered_map<std::tuple<HashMapStringKey, HashMapStringKey, PIPELINE_TYPE>, RefCntAutoPtr<IShaderResourceBinding>, SRBHasher>;
		TNamedSRBHashMap m_SRBHashMap;
	};


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
		Int32        m_LayerCount = 4;
		SAMPLE_COUNT m_SampleCount = SAMPLE_COUNT_4;

		RefCntAutoPtr<ITexture> m_pTextureColor;
		RefCntAutoPtr<ITexture> m_pTextureColor_MS;
		RefCntAutoPtr<ITexture> m_pTextureDepth_MS;
		RefCntAutoPtr<ITexture> m_pTextureHead;
		RefCntAutoPtr<IBuffer>  m_pBufferLinkedList;
		RefCntAutoPtr<IBuffer>  m_pBufferLinkedListCounter;

		RefCntAutoPtr<ITextureView> m_pTextureColorUAV_Linear;
		RefCntAutoPtr<ITextureView> m_pTextureColorSRV_Gamma;

		std::unique_ptr<RSNLoaderPipelineCache> m_pPSOLoader;
		std::unique_ptr<RSNLoaderSRBCache>      m_pSRBLoader;

	};

} // namespace Diligent
