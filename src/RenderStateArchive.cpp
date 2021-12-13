#include "RenderStateArchive.hpp"
#include "RenderStateNotationParser.h"
#include "RefCntAutoPtr.hpp"
#include "FileSystem.hpp"
#include "DynamicLinearAllocator.hpp"
#include "DefaultRawMemoryAllocator.hpp"

namespace Diligent
{

RenderStateArchive::RenderStateArchive(
    RefCntAutoPtr<IRenderDevice>                   pRenderDevice,
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pStreamShaderFactory,
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pStreamRenderStatesFactory,
    const std::vector<std::string>&                DRSNFiles)
{

    std::vector<RefCntAutoPtr<IRenderStateNotationParser>> NotationParsers;
    for (auto const& Path : DRSNFiles)
    {
        RefCntAutoPtr<IRenderStateNotationParser> pParser;

        RenderStateNotationParserCreateInfo ParserCI{};
        ParserCI.FilePath       = Path.c_str();
        ParserCI.pStreamFactory = pStreamRenderStatesFactory;
        CreateRenderStateNotationParser(ParserCI, &pParser);
        DEV_CHECK_ERR(pParser != nullptr, "Failed to parse file '", Path, "'.");
        NotationParsers.push_back(pParser);
    }

    std::unordered_map<std::string, RefCntAutoPtr<IShader>>                    Shaders;
    std::unordered_map<std::string, RefCntAutoPtr<IRenderPass>>                RenderPasses;
    std::unordered_map<std::string, RefCntAutoPtr<IPipelineResourceSignature>> ResourceSignatures;

    DynamicLinearAllocator Allocator{DefaultRawMemoryAllocator::GetAllocator()};
    for (auto const& pNotationParser : NotationParsers)
    {
        auto const& ParserInfo = pNotationParser->GetInfo();

        for (Uint32 ShaderID = 0; ShaderID < ParserInfo.ShaderCount; ShaderID++)
        {
            ShaderCreateInfo ShaderCI           = *pNotationParser->GetShaderByIndex(ShaderID);
            ShaderCI.pShaderSourceStreamFactory = pStreamShaderFactory;

            RefCntAutoPtr<IShader> pShader;
            pRenderDevice->CreateShader(ShaderCI, &pShader);
            DEV_CHECK_ERR(pShader != nullptr, "Failed to create shader from file '", ShaderCI.FilePath, "'.");
            Shaders.emplace(ShaderCI.Desc.Name, pShader);
        }

        for (Uint32 RenderPassID = 0; RenderPassID < ParserInfo.RenderPassCount; RenderPassID++)
        {
            RenderPassDesc             RenderPassDesc = *pNotationParser->GetRenderPassByIndex(RenderPassID);
            RefCntAutoPtr<IRenderPass> pRenderPass;
            pRenderDevice->CreateRenderPass(RenderPassDesc, &pRenderPass);
            DEV_CHECK_ERR(pRenderPass != nullptr, "Failed to create render pass '", RenderPassDesc.Name, "'.");
            RenderPasses.emplace(RenderPassDesc.Name, pRenderPass);
        }

        for (Uint32 SignatureID = 0; SignatureID < ParserInfo.ResourceSignatureCount; SignatureID++)
        {
            PipelineResourceSignatureDesc ResourceSignatureDesc = *pNotationParser->GetResourceSignatureByIndex(SignatureID);

            RefCntAutoPtr<IPipelineResourceSignature> pSignature;
            pRenderDevice->CreatePipelineResourceSignature(ResourceSignatureDesc, &pSignature);
            DEV_CHECK_ERR(pSignature != nullptr, "Failed to create resource signature '", ResourceSignatureDesc.Name, "'.");
            ResourceSignatures.emplace(ResourceSignatureDesc.Name, pSignature);
        }

        auto FindShader = [&](const char* Name) -> IShader* {
            if (Name == nullptr)
                return nullptr;

            auto Iter = Shaders.find(Name);
            if (Iter == Shaders.end())
                DEV_ERROR("Unable to find shader '", Name, "'.");
            return Iter->second;
        };

        auto FindRenderPass = [&](const char* Name) -> IRenderPass* {
            if (Name == nullptr)
                return nullptr;

            auto Iter = RenderPasses.find(Name);
            if (Iter == RenderPasses.end())
                DEV_ERROR("Unable to find render pass '", Name, "'.");
            return Iter->second;
        };

        auto FindResourceSignature = [&](const char* Name) -> IPipelineResourceSignature* {
            if (Name == nullptr)
                return nullptr;

            auto Iter = ResourceSignatures.find(Name);
            if (Iter == ResourceSignatures.end())
                DEV_ERROR("Unable to find resource signature '", Name, "'.");
            return Iter->second;
        };

        auto UnpackPipelineStateCreateInfo = [&](PipelineStateNotation const& ResourceDescRSN, PipelineStateCreateInfo& ResourceDesc) {
            ResourceDesc.PSODesc                 = ResourceDescRSN.PSODesc;
            ResourceDesc.Flags                   = ResourceDescRSN.Flags;
            ResourceDesc.ResourceSignaturesCount = ResourceDescRSN.ResourceSignaturesNameCount;
            ResourceDesc.ppResourceSignatures    = Allocator.ConstructArray<IPipelineResourceSignature*>(ResourceDescRSN.ResourceSignaturesNameCount);
            for (Uint32 SignatureID = 0; SignatureID < ResourceDesc.ResourceSignaturesCount; SignatureID++)
                ResourceDesc.ppResourceSignatures[SignatureID] = FindResourceSignature(ResourceDescRSN.ppResourceSignatureNames[SignatureID]);
        };

        for (Uint32 PipelineID = 0; PipelineID < ParserInfo.GraphicsPipelineStateCount; PipelineID++)
        {
            auto pPipelineDescRSN = pNotationParser->GetGraphicsPipelineStateByIndex(PipelineID);

            GraphicsPipelineStateCreateInfo PipelineCI = {};
            UnpackPipelineStateCreateInfo(*pPipelineDescRSN, PipelineCI);
            PipelineCI.GraphicsPipeline             = static_cast<GraphicsPipelineDesc>(pPipelineDescRSN->Desc);
            PipelineCI.GraphicsPipeline.pRenderPass = FindRenderPass(pPipelineDescRSN->pRenderPassName);

            PipelineCI.pVS = FindShader(pPipelineDescRSN->pVSName);
            PipelineCI.pPS = FindShader(pPipelineDescRSN->pPSName);
            PipelineCI.pDS = FindShader(pPipelineDescRSN->pDSName);
            PipelineCI.pHS = FindShader(pPipelineDescRSN->pHSName);
            PipelineCI.pGS = FindShader(pPipelineDescRSN->pGSName);
            PipelineCI.pAS = FindShader(pPipelineDescRSN->pASName);
            PipelineCI.pMS = FindShader(pPipelineDescRSN->pMSName);

            RefCntAutoPtr<IPipelineState> pPipeline;
            pRenderDevice->CreateGraphicsPipelineState(PipelineCI, &pPipeline);
            if (!pPipeline)
                DEV_ERROR("Failed to create graphics pipeline '", PipelineCI.PSODesc.Name, "'.");

            m_pPipelines.emplace(PipelineCI.PSODesc.Name, pPipeline);
        }

        for (Uint32 PipelineID = 0; PipelineID < ParserInfo.ComputePipelineStateCount; PipelineID++)
        {
            auto pPipelineDescRSN = pNotationParser->GetComputePipelineStateByIndex(PipelineID);

            ComputePipelineStateCreateInfo PipelineCI{};
            UnpackPipelineStateCreateInfo(*pPipelineDescRSN, PipelineCI);
            PipelineCI.pCS = FindShader(pPipelineDescRSN->pCSName);

            RefCntAutoPtr<IPipelineState> pPipeline;
            pRenderDevice->CreateComputePipelineState(PipelineCI, &pPipeline);
            if (!pPipeline)
                DEV_ERROR("Failed to create compute pipeline '", PipelineCI.PSODesc.Name, "'.");

            m_pPipelines.emplace(PipelineCI.PSODesc.Name, pPipeline);
        }
    }
}

IPipelineState* RenderStateArchive::GetPipelineState(const std::string& Name)
{
    auto Iter = m_pPipelines.find(Name);
    if (Iter == m_pPipelines.end())
        DEV_ERROR("Unable to find pipeline state '", Name, "'.");
    return Iter->second;
}

} // namespace Diligent
